/* app_music.c - 音乐播放模块
 * 原理（）：
 *   1) fork 出子进程，execlp 启动 mplayer -slave 播放歌曲
 *   2) 创建"有名管道(fifo)"，父进程往里写命令（pause/seek/volume/quit...）
 *      mplayer -slave 会把 fifo 当成命令输入，实现控制
 *   3) 点退出时写 "quit\n" 并 waitpid 回收子进程，避免僵尸进程
 */
#include "lvgl/lvgl.h"
#include "mysrc/inc/app_music.h"
#include "mysrc/inc/ui_home.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

/* 下面两个头文件只有板端 Linux 有（Windows 侧 gcc -fsyntax-only 自检会找不到），
 * 所以整块功能用 __linux__ 圈起来，非 Linux 平台给一个空实现。 */
#if defined(__linux__)
#include <sys/ioctl.h>
#include <linux/input.h>   /* struct input_event / EV_KEY / BTN_TOUCH / ABS_MT_TRACKING_ID */
#include <linux/fb.h>      /* 恢复 /dev/fb0 的 var info */
#endif

#define FIFO_PATH   "/tmp/smart_music.fifo"
#define MUSIC_DIR_1 "/mnt/usb"     /* 优先扫 U 盘挂载目录 */
#define MUSIC_DIR_2 "/jing/music"  /* 备用：板子 /jing 下的 music 目录 */
#define MAX_SONGS   64

/* 触摸设备节点：必须和 main.c 里 lv_evdev_create() 用的是同一个 */
#define TOUCH_DEV   "/dev/input/event6"

static char  g_songs[MAX_SONGS][512];  /* 歌曲全路径表 */
static int   g_song_cnt = 0;
static int   g_cur = -1;               /* 当前播放第几首 */

static int  g_fifo_fd = -1;            /* 有名管道 fd（写命令用） */
static pid_t g_player_pid = -1;        /* mplayer 子进程 */
static bool g_video_active = false;    /* 当前播放的是视频(mp4)？是则暂停 LVGL 刷新 */
static uint32_t g_video_start = 0;     /* 视频开始播放的 tick，供看门狗判断是否卡死 */
static int  g_touch_fd = -1;           /* 视频期间自己读触摸用的 fd（绕过 LVGL） */
static bool g_touch_armed = false;     /* 本次视频是否已完成"首次读触摸"，见 poll_touch */

/* 单个视频最长播放上限（最后一道保险）。
 * 视频播放期间主循环跳过 lv_timer_handler()，若 mplayer 死卡在 fb ioctl（D 态），
 * 界面就再也回不来。正常情况下用户"点屏幕任意位置"就能停视频（见
 * music_video_poll_touch），所以这个看门狗只兜底，给得很宽松，
 * 免得把一部两小时的长片中途掐掉。 */
#define VIDEO_WATCHDOG_MS (2UL * 60UL * 60UL * 1000UL)   /* 2 小时 */

static lv_obj_t *song_list;
static lv_obj_t *now_lbl;              /* 正在播放提示 */

/* ================= 播放器控制 ================= */
/* 往 fifo 写一条 mplayer slave 命令 */
static void mplayer_cmd(const char *cmd)
{
    if (g_fifo_fd >= 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s\n", cmd);
        if (write(g_fifo_fd, buf, strlen(buf)) < 0)
            perror("fifo write");
    }
}

/* ---------------------------------------------------------------------
 * 触摸直读：视频播放期间绕过 LVGL 自己读 /dev/input/eventX
 *
 * 背景（本项目最容易踩的设计陷阱）：
 *   lv_timer_handler() 既是"绘图"入口，也是"读触摸"入口。
 *   视频走 mplayer -vo fbdev2 独占 /dev/fb0，所以播放期间必须停掉
 *   lv_timer_handler()（否则 LVGL 重绘会把视频画面盖掉）——
 *   但一停，触摸也就读不到了：所有按钮毫无反应，视频却照播不误。
 * 解法：
 *   视频期间由 main 主循环轮询 music_video_poll_touch()，自己读触摸设备；
 *   检测到"按下"就调 music_video_stop() 停视频、恢复界面。
 * ------------------------------------------------------------------- */
#if defined(__linux__)

static void touch_open(void)
{
    if (g_touch_fd >= 0) return;
    g_touch_fd = open(TOUCH_DEV, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (g_touch_fd < 0) perror("open " TOUCH_DEV);
}

bool music_video_poll_touch(void)
{
    touch_open();
    if (g_touch_fd < 0) return false;

    struct input_event ev;
    bool pressed = false;
    /* 把缓冲区里的事件一次性读完，避免越积越多 */
    while (read(g_touch_fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
        /* 单点触摸屏两种上报方式都认：BTN_TOUCH(旧) / ABS_MT_TRACKING_ID(MT-B) */
        if (ev.type == EV_KEY && ev.code == BTN_TOUCH && ev.value == 1) pressed = true;
        if (ev.type == EV_ABS && ev.code == ABS_MT_TRACKING_ID && ev.value >= 0) pressed = true;
    }

    /* 视频刚开播后的第一次 poll 只清缓冲、不判定。
     * 因为"点开这首歌"的那一按（按下+抬起）还留在我们自己的 fd 缓冲里
     * （视频之外的时间我们从不读这个 fd），直接判定会把刚播的视频秒关掉。 */
    if (!g_touch_armed) {
        g_touch_armed = true;
        if (pressed)
            printf("[MUSIC] ignored stale touch while entering video\n"), fflush(stdout);
        return false;
    }
    return pressed;
}

/* mplayer 用 -vo fbdev2 时会改写 /dev/fb0 的 var info（yoffset / yres_virtual），
 * 并可能把屏幕 blank 掉。它正常退出会自己恢复，但**被 SIGKILL 打死就没机会了**。
 * 而 LVGL 的 fbdev 驱动只在启动时 mmap 一次、把 vinfo 缓存下来，之后一直往那块
 * 内存画 —— fb 的实际显示参数一旦和 LVGL 缓存的对不上，LVGL 画的就全落到
 * "看不见的地方"，表现就是画面冻死、按键毫无反应。
 * 所以 mplayer 退出后主动把 var info 复位 + 解除 blank。 */
static void fb_restore(void)
{
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) { perror("open /dev/fb0"); return; }

    struct fb_var_screeninfo vi;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) == 0) {
        vi.xoffset  = 0;
        vi.yoffset  = 0;
        vi.activate = FB_ACTIVATE_NOW;
        if (ioctl(fd, FBIOPUT_VSCREENINFO, &vi) != 0)
            perror("ioctl(FBIOPUT_VSCREENINFO)");
    } else {
        perror("ioctl(FBIOGET_VSCREENINFO)");
    }
    if (ioctl(fd, FBIOBLANK, FB_BLANK_UNBLANK) != 0)
        perror("ioctl(FBIOBLANK)");
    close(fd);
}

#else  /* 非 Linux（Windows 侧语法自检）：给空实现，保证能编过 */

bool music_video_poll_touch(void) { return false; }
static void fb_restore(void) { }

#endif

/* 停掉当前 mplayer：写 quit + 回收；若它不响应则强制 SIGKILL。
 * 注意：绝不能用 waitpid(pid, NULL, 0) 无限阻塞——mplayer 若不响应 fifo 的
 * quit（打不开 fbdev2 / 卡在启动等），会让整个 LVGL 主线程冻结，
 * 表现就是"所有按钮都点不动，只能 Ctrl+C"。 */
static void mplayer_stop(void)
{
    if (g_player_pid > 0) {
        mplayer_cmd("quit");                 /* 先礼：请它自己退 */
        int st = 0, reaped = 0;
        /* 视频给的时间长一些（1.5s）：它得跑完 vo 的 uninit 才能把 fb 恢复回去，
         * 剪得太急就只能 SIGKILL，那 fb 就留在 mplayer 的状态里了。 */
        int iters = g_video_active ? 150 : 30;
        for (int i = 0; i < iters; i++) {    /* 音频 ~300ms / 视频 ~1.5s */
            if (waitpid(g_player_pid, &st, WNOHANG) == g_player_pid) { reaped = 1; break; }
            usleep(10000);
        }
        if (!reaped) {                       /* 后兵：强杀，保证 UI 不卡死 */
            printf("[MUSIC] mplayer 未响应 quit，SIGKILL pid=%d\n", (int)g_player_pid);
            fflush(stdout);
            kill(g_player_pid, SIGKILL);
            /* 有界回收：最多再等 ~1s。D 态（卡在 fb ioctl）进程连 SIGKILL 都杀不掉，
             * 此时宁可留个僵尸，也绝不让主线程永久阻塞（那会让整个 UI 冻死）。 */
            for (int i = 0; i < 100; i++) {
                if (waitpid(g_player_pid, &st, WNOHANG) == g_player_pid) break;
                usleep(10000);
            }
        }
        g_player_pid = -1;
    }
    if (g_fifo_fd >= 0) {
        close(g_fifo_fd);
        g_fifo_fd = -1;
    }
    g_video_active = false;
    g_video_start = 0;
    /* 顺序很重要：先把 framebuffer 的参数复位（mplayer 被强杀时它自己没恢复），
     * 再让 LVGL 重绘 —— 否则重绘会被画到"看不见的地方"，屏幕看着还是死的。 */
    fb_restore();
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(NULL);
}

/* 视频播放期间被"点屏幕"触发：停掉 mplayer、恢复界面，回到音乐列表页。 */
void music_video_stop(void)
{
    if (!g_video_active && g_player_pid <= 0) return;
    printf("[MUSIC] tap on screen -> stop video\n");
    fflush(stdout);
    mplayer_stop();                    /* 内含 fb_restore + 重绘音乐页 */
    /* 注意：不在这里"清触摸缓冲"。evdev 是**每个打开的 fd 各拿一份事件**，
     * 我们读掉自己那份，LVGL 那份还在，照样会被当成一次真点击（会误触列表里的歌）。
     * 挡这一下是 main.c 的活：停视频前先 lv_indev_wait_release(indev)，
     * 由 LVGL 自己忽略输入直到手指抬起。 */
}

/* 供 main.c 主循环查询：
 *   播放视频(mp4) 期间返回 true —— 此时 mplayer 独占 framebuffer，
 *   LVGL 必须停止刷新，否则会不断重绘界面、把视频画面盖掉。
 *   顺便在此非阻塞回收已结束的 mplayer，并在视频播完时恢复界面。 */
bool music_video_active(void)
{
    if (!g_video_active) return false;
    if (g_player_pid <= 0) { g_video_active = false; return false; }

    int status = 0;
    pid_t r = waitpid(g_player_pid, &status, WNOHANG);
    if (r == g_player_pid) {             /* 视频播完，mplayer 已自行退出 */
        g_player_pid = -1;
        g_video_active = false;
        g_video_start = 0;
        /* 收尾：关 fifo、复位当前曲目。否则之后点 play/pause 会往无读端的
         * fifo 写命令，静默失效 */
        if (g_fifo_fd >= 0) { close(g_fifo_fd); g_fifo_fd = -1; }
        g_cur = -1;
        fb_restore();                 /* mplayer 自己退了，但 fb 参数仍可能被它改过 */
        lv_obj_invalidate(lv_screen_active());
        lv_refr_now(NULL);
        return false;
    }
    /* 看门狗（最后一道保险）：极端情况下 mplayer 卡死，主循环会一直跳过
     * lv_timer_handler()。正常情况下用户"点屏幕"就能停视频，所以这里给得很宽松。 */
    if (g_video_start && lv_tick_elaps(g_video_start) > VIDEO_WATCHDOG_MS) {
        printf("[MUSIC] video watchdog timeout, force stop\n"); fflush(stdout);
        mplayer_stop();
        return false;
    }
    return true;
}

/* 播放第 i 首：fork + execlp 启动 mplayer */
static void mplayer_play(int idx)
{
    if (idx < 0 || idx >= g_song_cnt) return;
    mplayer_stop();

    /* 判断是视频(.mp4) 还是纯音频：视频要独占屏幕，播放期间暂停 LVGL 刷新 */
    const char *dot = strrchr(g_songs[idx], '.');
    g_video_active = (dot != NULL && strcasecmp(dot, ".mp4") == 0);
    printf("[MUSIC] play #%d %s  (mode=%s)\n", idx, g_songs[idx],
           g_video_active ? "VIDEO-fbdev2" : "AUDIO");
    fflush(stdout);

    if (g_video_active) {
        /* 重新装填触摸判定：本次视频的第一轮 poll 只清缓冲不判定，
         * 免得把"点这首歌"那一按的残留事件当成"点屏幕停视频"（见 poll_touch）。 */
        g_touch_armed = false;

        /* 视频一开播就占满整块屏（mplayer 独占 fb0），界面上什么都看不见了。
         * 所以先在音乐页上盖一条提示，停 1.2s 让用户看清"怎么退出视频"——
         * 否则用户会以为板子死机了（这正是之前的真实反馈）。 */
        lv_obj_t *hint = lv_label_create(lv_screen_active());
        lv_label_set_text(hint, LV_SYMBOL_PLAY " Video playing\n\n"
                                               "Tap anywhere on screen to STOP");
        lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(hint, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(hint, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_color(hint, lv_color_hex(0x101418), 0);
        lv_obj_set_style_bg_opa(hint, LV_OPA_90, 0);
        lv_obj_set_style_radius(hint, 12, 0);
        lv_obj_set_style_pad_all(hint, 28, 0);
        lv_obj_set_style_border_width(hint, 0, 0);
        lv_obj_center(hint);
        lv_obj_move_foreground(hint);
        lv_refr_now(NULL);
        usleep(1200 * 1000);
        lv_obj_del(hint);
        lv_refr_now(NULL);
    }

    /* fifo 已存在就不重建（EEXIST 是正常情况） */
    int ret = mkfifo(FIFO_PATH, 0777);
    if (ret == -1 && errno != EEXIST) { perror("mkfifo"); return; }

    pid_t pid = fork();
    if (pid == 0) {
        /* ---- 子进程 ----
         * stdin / stdout 都接到 /dev/null（注意：不是管道！）。
         *   stdout：原写法用了一个"父进程不读"的管道，mplayer 一往 stdout 写就会
         *          因为没有读端而收到 SIGPIPE 被打死 —— 视频模式下尤其容易踩到。
         *   stdin ：mplayer 默认还会去抢终端做按键输入，会把控制台的 termios
         *          改乱（前台程序的按键、Ctrl+C 都会变怪）。接 /dev/null 断掉。
         * stderr 保留，便于看到 mplayer 的报错。 */
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, 0);
            dup2(devnull, 1);
            close(devnull);
        }
        char fifoname[128];
        snprintf(fifoname, sizeof(fifoname), "file=%s", FIFO_PATH);
        /* 公共参数：-slave 接受 fifo 命令；-quiet 少打印；
         *          -input file= 把 slave 命令源指向我们的有名管道。 */
        if (g_video_active) {
            /* 视频：-vo fbdev2 直接把画面怼到开发板 framebuffer；
             * -zoom 允许缩放到屏幕大小；-framedrop 掉帧优先保音画同步（板子算力有限）。 */
            execlp("mplayer", "mplayer", "-slave", "-quiet",
                   "-vo", "fbdev2", "-zoom", "-framedrop",
                   "-input", fifoname, g_songs[idx], (char *)NULL);
        } else {
            /* 纯音频：绝不能走 -vo fbdev2！
             * 那会让 mplayer 去 open("/dev/fb0")、改它的 var info 甚至清屏，
             * 而 LVGL 是常驻独占 fb0 的 —— 两个进程抢同一块屏，结果是
             * mplayer 一退出一片黑、LVGL 画的全都落到看不见的地方。
             * -vo null + -novideo：完全不碰 framebuffer
             * （-novideo 顺带挡掉 mp3 内嵌封面被当视频渲染的情况）。 */
            execlp("mplayer", "mplayer", "-slave", "-quiet",
                   "-vo", "null", "-novideo",
                   "-input", fifoname, g_songs[idx], (char *)NULL);
        }
        _exit(127);                       /* exec 失败才会走到这 */
    }
    /* ---- 父进程：不读 mplayer 输出 ---- */
    g_player_pid = pid;

    /* O_RDWR 打开 fifo：即使 mplayer 还没 open 也不会阻塞 */
    g_fifo_fd = open(FIFO_PATH, O_RDWR);
    g_cur = idx;
    g_video_start = g_video_active ? lv_tick_get() : 0;   /* 视频开始计时，供看门狗 */

    char buf[128];
    const char *base = strrchr(g_songs[idx], '/');
    base = base ? base + 1 : g_songs[idx];
    snprintf(buf, sizeof(buf), "Now: %s", base);
    lv_label_set_text(now_lbl, buf);

    /* 高亮列表当前项 */
    for (int i = 0; i < g_song_cnt && i < lv_obj_get_child_count(song_list); i++) {
        lv_obj_t *btn = lv_obj_get_child(song_list, i);
        lv_obj_set_style_bg_color(btn, i == idx ? lv_color_hex(0x2F6E4F) : lv_color_hex(0x24303B), 0);
    }
}

/* ================= 扫描歌曲目录 ================= */
static void scan_songs(void)
{
    g_song_cnt = 0;
    const char *dirs[2] = { MUSIC_DIR_1, MUSIC_DIR_2 };
    for (int d = 0; d < 2 && g_song_cnt < MAX_SONGS; d++) {
        DIR *dp = opendir(dirs[d]);
        if (!dp) continue;                /* 目录不存在就跳过 */
        struct dirent *ent;
        while ((ent = readdir(dp)) && g_song_cnt < MAX_SONGS) {
            const char *n = ent->d_name;
            if (n[0] == '.') continue;
            const char *ext = strrchr(n, '.');
            if (!ext) continue;
            if (strcasecmp(ext, ".mp3") && strcasecmp(ext, ".wav") &&
                strcasecmp(ext, ".flac") && strcasecmp(ext, ".mp4"))
                continue;                 /* 只要音频/视频文件 */
            snprintf(g_songs[g_song_cnt], 512, "%s/%s", dirs[d], n);
            g_song_cnt++;
        }
        closedir(dp);
        if (g_song_cnt) break;            /* 第一个有歌的目录生效 */
    }
}

/* ================= 事件回调 ================= */
static void song_click_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);
    printf("[MUSIC] song clicked idx=%d\n", idx); fflush(stdout);
    mplayer_play(idx);
}

static void ctrl_click_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    const char *txt = lv_label_get_text(lbl);
    printf("[MUSIC] ctrl clicked: %s\n", txt); fflush(stdout);

    if      (!strcmp(txt, "prev"))  mplayer_play(g_cur <= 0 ? g_song_cnt - 1 : g_cur - 1);
    else if (!strcmp(txt, "play"))  { if (g_cur < 0) mplayer_play(0); else mplayer_cmd("pause"); }  /* pause 命令在暂停时再发一次=继续 */
    else if (!strcmp(txt, "next"))  mplayer_play(g_cur < 0 ? 0 : (g_cur + 1) % g_song_cnt);
    else if (!strcmp(txt, "rew"))   mplayer_cmd("seek -10");
    else if (!strcmp(txt, "ff"))    mplayer_cmd("seek +10");
    else if (!strcmp(txt, "vol+"))  mplayer_cmd("volume +2");
    else if (!strcmp(txt, "vol-"))  mplayer_cmd("volume -2");
    else if (!strcmp(txt, "mute"))  mplayer_cmd("mute");
    else if (!strcmp(txt, "stop"))  mplayer_stop();
}

static void back_cb(lv_event_t *e)   /* 返回主界面 */
{
    (void)e;
    printf("[MUSIC] BACK clicked -> ui_home_show\n"); fflush(stdout);
    music_app_close();
}

/* ================= 构建界面 ================= */
void music_app_open(void)
{
    lv_obj_clean(lv_screen_active());
    lv_obj_clear_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);   /* 禁止整屏滚动，防手势被吞 */
    lv_refr_now(NULL);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x1B2631), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    /* 顶部栏 */
    lv_obj_t *top = lv_obj_create(lv_screen_active());
    lv_obj_set_size(top, 1024, 56);
    lv_obj_set_pos(top, 0, 0);
    lv_obj_set_style_bg_color(top, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_radius(top, 0, 0);
    lv_obj_set_style_pad_all(top, 0, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);                 /* 容器不滚动，避免吞点击 */

    lv_obj_t *title = lv_label_create(top);
    lv_label_set_text(title, "Music Player (mplayer)");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(title);

    lv_obj_t *back = lv_btn_create(top);
    lv_obj_set_size(back, 90, 38);
    lv_obj_align(back, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "BACK");
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    /* 左侧歌曲列表 */
    song_list = lv_list_create(lv_screen_active());
    lv_obj_set_size(song_list, 330, 380);
    lv_obj_set_pos(song_list, 10, 66);
    lv_obj_set_style_bg_color(song_list, lv_color_hex(0x24303B), 0);

    /* 右侧"正在播放" */
    now_lbl = lv_label_create(lv_screen_active());
    lv_label_set_text(now_lbl, "Now: none");
    lv_obj_set_style_text_color(now_lbl, lv_color_hex(0xF0C040), 0);
    lv_obj_set_pos(now_lbl, 360, 90);

    /* 底部控制按钮行 */
    lv_obj_t *ctrl_bar = lv_obj_create(lv_screen_active());
    lv_obj_set_size(ctrl_bar, 1024, 90);
    lv_obj_set_pos(ctrl_bar, 0, 600 - 90);
    lv_obj_set_style_bg_opa(ctrl_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctrl_bar, 0, 0);
    lv_obj_set_style_pad_all(ctrl_bar, 0, 0);
    lv_obj_clear_flag(ctrl_bar, LV_OBJ_FLAG_SCROLLABLE);            /* 容器不滚动，避免吞点击 */
    lv_obj_set_flex_flow(ctrl_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrl_bar, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char *btn_text[] = { "prev", "play", "next", "rew", "ff",
                               "vol+", "vol-", "mute", "stop" };
    for (int i = 0; i < (int)(sizeof(btn_text) / sizeof(btn_text[0])); i++) {
        lv_obj_t *btn = lv_btn_create(ctrl_bar);
        lv_obj_set_size(btn, 86, 46);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_text[i]);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, ctrl_click_cb, LV_EVENT_CLICKED, NULL);
    }

    /* 扫描歌曲并填充列表 */
    scan_songs();
    if (g_song_cnt == 0) {
        lv_obj_t *btn = lv_list_add_btn(song_list, LV_SYMBOL_WARNING, "no mp3 found");
        lv_obj_set_user_data(btn, (void *)(intptr_t)-1);
    }
    for (int i = 0; i < g_song_cnt; i++) {
        const char *base = strrchr(g_songs[i], '/');
        base = base ? base + 1 : g_songs[i];
        lv_obj_t *btn = lv_list_add_btn(song_list, LV_SYMBOL_AUDIO, base);
        lv_obj_set_user_data(btn, (void *)(intptr_t)i);
        lv_obj_add_event_cb(btn, song_click_cb, LV_EVENT_CLICKED, NULL);
    }
}

void music_app_close(void)
{
    mplayer_stop();          /* 停 mplayer、回收子进程 */
    unlink(FIFO_PATH);
    if (g_touch_fd >= 0) {   /* 视频期间自开的触摸 fd，离开时收掉 */
        close(g_touch_fd);
        g_touch_fd = -1;
    }
    ui_home_show();
}
