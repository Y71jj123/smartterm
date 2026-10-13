#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lvgl/src/indev/lv_indev_private.h"   /* 为直接设置 indev->scroll_limit */
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include "mysrc/inc/game_pvz.h"
#include "mysrc/inc/game_menu.h"
#include "mysrc/inc/ui_login.h"
#include "mysrc/inc/app_music.h"


int main(void)
{
    lv_init();

    /*Linux frame buffer device init*/
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    lv_indev_t * indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event6");

    /* 触摸屏容错：默认 scroll_limit 只有 10px，电容屏抖动/手指微移常超过它，
     * 会把"点击"误判成"滚动"，导致按钮 CLICKED 不触发（点了没反应）。
     * 放宽到 25px：轻微抖动仍算点击，真正大距离拖动才当滚动
     * （不宜过大，否则可滚动列表/聊天页的滚动起步会变迟钝）。 */
    indev->scroll_limit = 25;

    srand(time(NULL));

    /* 启动到登录界面（登录成功 -> 主界面 -> 聊天/游戏/音乐） */
    ui_login_show();


    /*Handle LVGL tasks*/
    while(1) {
        if (music_video_active()) {
            /* 播放视频(mp4) 时 mplayer 独占 framebuffer，必须暂停 LVGL 刷新，
             * 否则 LVGL 会不断重绘界面、把视频画面盖掉。
             *
             * ⚠️ 但 lv_timer_handler() 同时也是"读触摸"的入口——完全不调它，
             *    就等于整个 UI 死掉：视频照播，可所有按钮（含 BACK）全都没反应。
             *    所以视频期间改成自己读触摸设备：点屏幕任意位置 -> 停视频回界面。 */
            if (music_video_poll_touch()) {
                /* 停视频之前先让 LVGL 忽略输入，直到收到"手指抬起"。
                 *
                 * 为什么必须这么做：这一下"点击"是我们绕过 LVGL 自己读出来的，
                 * LVGL 的 evdev 驱动**同时也收到了同一份事件**（evdev 给每个
                 * 打开的 fd 各发一份），它只是因为我们没调 lv_timer_handler()
                 * 而暂时没读。等视频停掉、LVGL 恢复运行，它就会把这次按下当成
                 * 一次真点击 —— 屏幕上正好是音乐列表，于是"刚关掉视频又自动
                 * 播了下一首"。
                 * lv_indev_wait_release() 是 LVGL 官方 API：置位后 indev_proc_press
                 * 直接返回（不再判定按下），直到 indev_proc_release 收到抬起才解除。
                 * 手指若早已抬起，下一次 release 就顺手把标志清掉，无副作用。 */
                lv_indev_wait_release(indev);
                music_video_stop();
            }
        } else {
            lv_timer_handler();
        }
        usleep(5000);
    }

    return 0;
}
