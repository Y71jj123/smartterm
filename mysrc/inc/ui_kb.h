#ifndef UI_KB_H
#define UI_KB_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 给一个 lv_keyboard 装上「PC 式 QWERTY + 常驻数字行」布局。
 *
 * 为什么需要：LVGL 自带键盘把数字/Symbol 塞在另一页，要按 "1#" 才切过去。
 * 输服务器 IP（192.168.1.100）得在字母页和数字页之间来回切，
 * 输用户名/密码又要切回来，非常难受。这里换成常见的电脑键盘布局：
 *
 *    1 2 3 4 5 6 7 8 9 0   ⌫
 *    q w e r t y u i o p   ⏎
 *    a s d f g h j k l . - :
 *    ABC z x c v b n m _ , '
 *    1#   ◀    space    ▶  OK
 *
 *   ABC = 切大写 / 小写      1# = 切符号页（LVGL 自带那一页）
 *   ◀ ▶ = 左右移光标         OK = 确认
 *
 * 布局是 5 行，建议键盘高度给 >= 220px（每行约 44px），手指才按得准。 */
void smart_kb_apply(lv_obj_t *kb);

#ifdef __cplusplus
}
#endif
#endif
