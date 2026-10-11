#ifndef APP_MUSIC_H
#define APP_MUSIC_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 进入音乐播放界面（mplayer -slave + 有名管道控制） */
void music_app_open(void);
/* 退出音乐界面（停止 mplayer 进程），回主界面 */
void music_app_close(void);

/* 正在播放"视频"(mp4) 时返回 true：此时 mplayer 独占 framebuffer，
 * main 主循环应暂停 lv_timer_handler()，否则 LVGL 会盖掉视频画面 */
bool music_video_active(void);

#ifdef __cplusplus
}
#endif
#endif
