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

#define FIFO_PATH   "/tmp/smart_music.fifo"
#define MUSIC_DIR_1 "/mnt/usb"     /* 优先扫 U 盘挂载目录 */
#define MUSIC_DIR_2 "/jing/music"  /* 备用：板子 /jing 下的 music 目录 */
#define MAX_SONGS   64

static char  g_songs[MAX_SONGS][512];  /* 歌曲全路径表 */
static int   g_song_cnt = 0;
static int   g_cur = -1;               /* 当前播放第几首 */

static int  g_fifo_fd = -1;            /* 有名管道 fd（写命令用） */
static pid_t g_player_pid = -1;        /* mplayer 子进程 */
static bool g_video_active = false;    /* 当前播放的是视频(mp4)？是则暂停 LVGL 刷新 */

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

/* 停掉当前 mplayer：写 quit + waitpid 回收 */
static void mplayer_stop(void)
{
    if (g_player_pid > 0) {
        mplayer_cmd("quit");
        waitpid(g_player_pid, NULL, 0);   /* 回收，避免僵尸进程 */
        g_player_pid = -1;
    }
    if (g_fifo_fd >= 0) {
        close(g_fifo_fd);
        g_fifo_fd = -1;
    }
    g_video_active = false;
    /* mplayer 用 fbdev 输出视频后会占住屏幕，退出时强制 LVGL 重绘，恢复界面 */
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(NULL);
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
        lv_obj_invalidate(lv_screen_active());
        lv_refr_now(NULL);
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

    /* fifo 已存在就不重建（EEXIST 是正常情况） */
    int ret = mkfifo(FIFO_PATH, 0777);
    if (ret == -1 && errno != EEXIST) { perror("mkfifo"); return; }

    pid_t pid = fork();
    if (pid == 0) {
        /* ---- 子进程 ----
         * 把 stdout 重定向到 /dev/null（注意：不是管道！）。
         * 原写法用了一个"父进程不读"的管道，mplayer 一往 stdout 写就会因为
         * 没有读端而收到 SIGPIPE 被杀死 —— 视频模式下尤其容易踩到。
         * stderr 保留，便于看到 mplayer 的报错。 */
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, 1);
            close(devnull);
        }
        char fifoname[128];
        snprintf(fifoname, sizeof(fifoname), "file=%s", FIFO_PATH);
        /* -slave：接受命令；-quiet：少打印；-input file=：命令走 fifo；
         * -vo fbdev2：视频直接输出到开发板 framebuffer（这样 mp4 有画面）；
         * -zoom：允许视频缩放到屏幕大小 */
        execlp("mplayer", "mplayer", "-slave", "-quiet",
               "-vo", "fbdev2", "-zoom",
               "-input", fifoname, g_songs[idx], (char *)NULL);
        _exit(127);                       /* exec 失败才会走到这 */
    }
    /* ---- 父进程：不读 mplayer 输出 ---- */
    g_player_pid = pid;

    /* O_RDWR 打开 fifo：即使 mplayer 还没 open 也不会阻塞 */
    g_fifo_fd = open(FIFO_PATH, O_RDWR);
    g_cur = idx;

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
    mplayer_play(idx);
}

static void ctrl_click_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    const char *txt = lv_label_get_text(lbl);

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
    music_app_close();
}

/* ================= 构建界面 ================= */
void music_app_open(void)
{
    lv_obj_clean(lv_screen_active());
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
    ui_home_show();
}
