/* ui_kb.c - 触摸键盘统一布局
 *
 * 把 LVGL 自带键盘换成「PC 式 QWERTY + 常驻数字行」，登录页和聊天页共用。
 * 只在 LVGL 的键盘表上做替换，不碰 lvgl/ 里的任何源码。
 *
 * 关键点（跟 lv_keyboard.c 的内部约定对齐）：
 *   1) 键盘按"模式"存表：kb_map[LV_KEYBOARD_MODE_*]。所以只要覆盖
 *      TEXT_LOWER / TEXT_UPPER 两个槽位即可，符号页(SPECIAL)沿用自带那页。
 *   2) 模式切换是按**按键文字**硬编码匹配的（见 lv_keyboard.c 的
 *      lv_keyboard_def_event_cb）：
 *          "ABC" -> 大写页      "abc" -> 小写页      "1#" -> 符号页
 *      所以下面两张表里必须分别放 "ABC" / "abc" 才能来回切。
 *   3) ctrl 表长度必须 >= 按键总数，且**每行的宽度单位之和要相等**，
 *      否则各行的键宽会对不齐。下面每行都按 12 个单位算。
 */
#include "lvgl/lvgl.h"
#include "mysrc/inc/ui_kb.h"
#include <stdio.h>      /* 表长度校验失败时打印原因 */

/* 与 lv_keyboard.c 内部一致：宽度单位 + popover 标志。
 * （keyboard->popovers 默认为 0，LVGL 后续会把 POPOVER 位屏蔽掉，写上无害） */
#define KB_BTN(w) (LV_BUTTONMATRIX_CTRL_POPOVER | (w))

/* ---------------- 小写页（默认页，也是开机看到的那页） ----------------
 * 每行 12 单位：行1 = 10×1 + 2；行2 = 10×1 + 2；行3 = 12×1；
 *              行4 = 2 + 7×1 + 3×1；行5 = 2 + 2 + 4 + 2 + 2               */
static const char * kb_map_lc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", LV_SYMBOL_NEW_LINE,  "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", ".", "-", ":",             "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", "_", ",", "'",                "\n",
    "1#", LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
};

/* ---------------- 大写页（结构与上面完全一致，只是换大小写） ---------------- */
static const char * kb_map_uc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", LV_SYMBOL_NEW_LINE,  "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", ".", "-", ":",             "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", "_", ",", "'",                "\n",
    "1#", LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
};

/* 控制表：两张 map 结构相同，共用这一份。顺序必须和上面 map 一一对应。 */
static const lv_buttonmatrix_ctrl_t kb_ctrl[] = {
    /* 行1：10 个数字(各1) + 退格(2) —— 退格要能长按连删，所以带 CHECKED 不带 NO_REPEAT */
    KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1),
    KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1),
    LV_BUTTONMATRIX_CTRL_CHECKED | 2,

    /* 行2：10 个字母(各1) + 回车(2) */
    KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1),
    KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1),
    LV_BUTTONMATRIX_CTRL_CHECKED | 2,

    /* 行3：12 个键各 1 */
    KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1),
    KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1),

    /* 行4：ABC/abc(2) + 7 个字母(各1) + _ , ' (各1) */
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1),
    KB_BTN(1), KB_BTN(1), KB_BTN(1),

    /* 行5：1#(2) + 左移(2) + 空格(4) + 右移(2) + OK(2) */
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    LV_BUTTONMATRIX_CTRL_CHECKED | 2,
    4,
    LV_BUTTONMATRIX_CTRL_CHECKED | 2,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2
};

/* 数一张 map 里到底有几个键。
 * 只用来做防御性检查——见下面 smart_kb_apply() 的注释。 */
static int map_btn_count(const char * const map[])
{
    int n = 0;
    for (int i = 0; map[i] != NULL; i++) {
        if (map[i][0] == '\0') break;               /* 结尾的 "" 是结束标记，不是键 */
        if (map[i][0] == '\n' && map[i][1] == '\0') continue;  /* "\n" 是换行符，不是键 */
        n++;
    }
    return n;
}

void smart_kb_apply(lv_obj_t *kb)
{
    if (kb == NULL) return;

    /* 防呆：LVGL 内部是按 btn_cnt 从 ctrl 表里拷数据的，
     * ctrl 表一旦比 map 里的按键少，读的就是数组外的内存 —— 不会报错，
     * 只会表现为"某些键宽度是乱数/点下去崩"，极难查。
     * 这里先数一遍，对不上就拒绝装这张表并打印原因。 */
    int n_btn  = map_btn_count(kb_map_lc);
    int n_ctrl = (int)(sizeof(kb_ctrl) / sizeof(kb_ctrl[0]));
    if (n_btn != n_ctrl) {
        printf("[KB] layout table mismatch: map=%d buttons, ctrl=%d entries -- rejected\n",
               n_btn, n_ctrl);
        fflush(stdout);
        return;
    }

    /* 覆盖两个文字页；SPECIAL(符号页) 与 NUMBER(数字页) 保留 LVGL 自带的 */
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, kb_map_lc, kb_ctrl);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_UPPER, kb_map_uc, kb_ctrl);

    /* 默认落在小写页（带数字行那页） */
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
}
