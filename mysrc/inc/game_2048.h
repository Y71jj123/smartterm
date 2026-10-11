#ifndef __GAME_2048_H__
#define __GAME_2048_H__

/* 2048 小游戏入口：构建界面并接管屏幕，返回后由 main 的
 * lv_timer_handler 主循环驱动 LVGL 刷新。点击 BACK 回到主界面。 */
void game_2048(void);

#endif
