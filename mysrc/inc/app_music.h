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
 * main 主循环应暂停 lv_timer_handler()，否则 LVGL 会盖掉视频画面。
 * 注意：暂停 lv_timer_handler() 等于同时暂停了"读触摸"，
 * 所以视频期间必须配合下面的 music_video_poll_touch() 使用。 */
bool music_video_active(void);

/* 视频播放期间由 main 主循环轮询：直接读触摸设备，检测到"按下"返回 true。
 * 为什么绕过 LVGL——lv_timer_handler() 既是绘图入口也是触摸入口，
 * 视频期间必须停掉它（否则 LVGL 重绘盖掉视频），但那样按钮就全死了。
 * 所以这里自己开一个 evdev fd 读触摸，用来实现"点屏幕停视频"。 */
bool music_video_poll_touch(void);

/* 视频播放期间被点屏触发：停掉 mplayer、恢复界面，回到音乐列表页 */
void music_video_stop(void);

#ifdef __cplusplus
}
#endif
#endif
