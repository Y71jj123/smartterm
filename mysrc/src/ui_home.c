/* ui_home.c - 主界面（门户页）
 * 登录成功后的“门户页”：四个功能入口卡片（Chat / Game / 2048 / Music）
 * + 退出登录。点击卡片进入对应模块，各模块退出时再调 ui_home_show() 回来。
 */
#include "lvgl/lvgl.h"
#include "mysrc/inc/ui_font.h"
#include "mysrc/inc/ui_home.h"
#include "mysrc/inc/ui_login.h"
#include "mysrc/inc/net_chat.h"
#include "mysrc/inc/game_pvz.h"
#include "mysrc/inc/game_2048.h"
#include "mysrc/inc/app_music.h"
#include <stdio.h>

#define HOME_SCR_W 1024
#define HOME_SCR_H 600

static void chat_btn_cb(lv_event_t *e)   { (void)e; chat_app_open(); }
static void game_btn_cb(lv_event_t *e)   { (void)e; game_pvz(); }
static void game2048_btn_cb(lv_event_t *e){ (void)e; game_2048(); }
static void music_btn_cb(lv_event_t *e)  { (void)e; music_app_open(); }
static void logout_btn_cb(lv_event_t *e) /* 退出登录：断网回登录页 */
{
    (void)e;
    chat_disconnect();
    ui_login_show();
}

/* 一个大按钮 = 底板 + 图标 + 两行文字（作为 flex 行容器的子项） */
static void make_card(lv_obj_t *parent, const char *icon, const char *name,
                      const char *desc, lv_color_t color, lv_event_cb_t cb)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 210, 180);
    lv_obj_set_style_bg_color(card, color, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 40, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_30, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);   /* 卡片不滚动，避免吞点击 */
    lv_obj_add_event_cb(card, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ic = lv_label_create(card);
    lv_label_set_text(ic, icon);
    lv_obj_set_style_text_font(ic, &ui_font_cn_32, 0);
    lv_obj_align(ic, LV_ALIGN_TOP_MID, 0, 22);

    lv_obj_t *nm = lv_label_create(card);
    lv_label_set_text(nm, name);
    lv_obj_set_style_text_font(nm, &ui_font_cn_20, 0);
    lv_obj_align(nm, LV_ALIGN_TOP_MID, 0, 78);

    lv_obj_t *ds = lv_label_create(card);
    lv_label_set_text(ds, desc);
    lv_obj_set_style_text_color(ds, lv_color_hex(0xDDDDDD), 0);
    lv_obj_align(ds, LV_ALIGN_TOP_MID, 0, 112);
}

void ui_home_show(void)
{
    lv_obj_clean(lv_screen_active());
    lv_obj_clear_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);   /* 禁止整屏滚动 */
    lv_refr_now(NULL);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x16222E), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    /* 顶部标题 + 当前用户 */
    lv_obj_t *title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "智能终端系统");
    lv_obj_set_style_text_font(title, &ui_font_cn_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 22);

    char ubuf[80];
    snprintf(ubuf, sizeof(ubuf), "你好，%s (#%d)", chat_my_name(), chat_my_id());
    lv_obj_t *user_lbl = lv_label_create(lv_screen_active());
    lv_label_set_text(user_lbl, ubuf);
    lv_obj_set_style_text_color(user_lbl, lv_color_hex(0xF0C040), 0);
    lv_obj_align(user_lbl, LV_ALIGN_TOP_LEFT, 24, 26);

    /* 退出登录按钮（右上角） */
    lv_obj_t *out = lv_btn_create(lv_screen_active());
    lv_obj_set_size(out, 120, 42);
    lv_obj_align(out, LV_ALIGN_TOP_RIGHT, -20, 16);
    lv_obj_set_style_bg_color(out, lv_color_hex(0x7A3B3B), 0);
    lv_obj_t *ol = lv_label_create(out);
    lv_label_set_text(ol, "退出登录");
    lv_obj_center(ol);
    lv_obj_add_event_cb(out, logout_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 四张卡片：用 flex 行容器横向居中排布（自适应，不重叠） */
    lv_obj_t *row = lv_obj_create(lv_screen_active());
    lv_obj_set_size(row, 980, 200);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 70);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 24, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    make_card(row, LV_SYMBOL_EDIT,    "聊天",  "网络消息", lv_color_hex(0x2C6E8F), chat_btn_cb);
    make_card(row, LV_SYMBOL_IMAGE,   "游戏",  "塔防",     lv_color_hex(0x3D7A3D), game_btn_cb);
    make_card(row, LV_SYMBOL_SHUFFLE, "2048",  "数字拼合", lv_color_hex(0xB07A2F), game2048_btn_cb);
    make_card(row, LV_SYMBOL_AUDIO,   "音乐",  "播放器",   lv_color_hex(0x8A6D2F), music_btn_cb);
}
