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
        /* 播放视频(mp4) 时 mplayer 独占 framebuffer，暂停 LVGL 刷新，
         * 否则 LVGL 会不断重绘界面、把视频画面盖掉；视频结束/停止后自动恢复 */
        if (!music_video_active()) {
            lv_timer_handler();
        }
        usleep(5000);
    }

    return 0;
}
