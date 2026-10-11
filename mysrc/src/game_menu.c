#include "lvgl/lvgl.h"
#include "mysrc/inc/game_pvz.h"
#include "mysrc/inc/game_menu.h"
#include <stdio.h>


/* 与 game_pvz.c 保持一致的屏幕尺寸宏（这里独立定义，避免引入整个 game_pvz.h 内部宏） */
#define MENU_SCR_W 1024
#define MENU_SCR_H 600

 //按钮点击回调函数：点击 START 后进入游戏
static void menu_start_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    printf("[MENU] start clicked\n"); fflush(stdout);
    /* lv_obj_clean：清空当前屏幕上的所有子对象（删掉菜单自身的所有控件） */
    lv_obj_clean(lv_screen_active());
    /* 调用 game_pvz() 进入游戏主界面（定义在 game_pvz.c 中） */
    game_pvz();
}

 //启动菜单入口函数：构建整个菜单界面
void game_menu(void)
{
    printf("[MENU] show menu\n"); fflush(stdout);

    /* ---- 第 0 步：清屏 + 强制刷新 ---- */
    /* lv_obj_clean：清掉屏幕上所有旧控件（从上局游戏回来时要清理） */
    lv_obj_clean(lv_screen_active());
    /* lv_refr_now：强制 LVGL 立即重绘一次屏幕，防止旧画面残留 */
    lv_refr_now(NULL);
    /* 给屏幕本身设置背景色（最后兜底，防止背景对象没盖住） */
    /* lv_obj_set_style_bg_color：设置对象的背景色，参数3=0 表示默认状态 */
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x4A7C2A), 0);
    /* lv_obj_set_style_bg_opa：设置背景不透明度，LV_OPA_COVER=完全不透明 */
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    /* ============================================================
     * 第 1 个控件：全屏背景（基础对象 Object）
     * 用 lv_obj_create 创建，然后移除默认样式，再挂自己的样式
     * ============================================================ */
    static lv_style_t st_bg;                    /* 样式变量（static 保证函数退出后不销毁） */
    lv_style_init(&st_bg);                      /* 初始化样式 */
    lv_style_set_bg_color(&st_bg, lv_color_hex(0x4A7C2A));      /* 主色：草绿色 */
    lv_style_set_bg_grad_color(&st_bg, lv_color_hex(0x2E4F1B)); /* 渐变色：深绿 */
    lv_style_set_bg_grad_dir(&st_bg, LV_GRAD_DIR_VER);          /* 渐变方向：从上到下(垂直) */
    lv_style_set_bg_opa(&st_bg, LV_OPA_COVER);                   /* 完全不透明 */
    lv_style_set_border_width(&st_bg, 0);                        /* 无边框 */

    /* lv_obj_create(父对象)：创建一个基础对象，父对象是活动屏幕 */
    lv_obj_t *bg = lv_obj_create(lv_screen_active());
    /* lv_obj_remove_style_all：移掉 LVGL 给基础对象的默认灰色背景和边框 */
    lv_obj_remove_style_all(bg);
    /* lv_obj_add_style(对象, &样式, 状态)：给对象挂载样式，0=默认状态 */
    lv_obj_add_style(bg, &st_bg, 0);
    lv_obj_set_size(bg, MENU_SCR_W, MENU_SCR_H);  /* 尺寸 1024x600 全屏 */
    lv_obj_set_pos(bg, 0, 0);                      /* 放在屏幕左上角(0,0) */
    /* lv_obj_move_background：把对象移到最底层，确保后面的控件都画在它上面 */
    lv_obj_move_background(bg);

    /* ============================================================
     * 第 2 个控件：顶部装饰条（基础对象 Object）
     * 仿原版 PVZ 游戏界面的棕色木条
     * ============================================================ */
    static lv_style_t st_top;
    lv_style_init(&st_top);
    lv_style_set_bg_color(&st_top, lv_color_hex(0x8B5A2B));   /* 棕色木纹 */
    lv_style_set_bg_opa(&st_top, LV_OPA_COVER);
    lv_style_set_border_width(&st_top, 0);

    lv_obj_t *top = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(top);
    lv_obj_add_style(top, &st_top, 0);
    lv_obj_set_size(top, MENU_SCR_W, 96);   /* 宽=全屏，高=96px */
    lv_obj_set_pos(top, 0, 0);               /* 顶部最左 */

    /* ============================================================
     * 第 3 个控件：大标题（标签 Label）
     * "Plants vs. Zombies" 48号字体，金黄色
     * ============================================================ */
    lv_obj_t *title = lv_label_create(lv_screen_active());
    /* lv_obj_set_style_text_color：设置文字颜色 */
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFF078), 0);
    /* lv_obj_set_style_text_font：设置字体，lv_font_montserrat_48 是 LVGL 内置 48px 字体 */
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);
    /* lv_obj_set_style_text_align：设置文字对齐方式，居中 */
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_opa(title, LV_OPA_COVER, 0);
    /* lv_label_set_text：设置标签显示的文字内容 */
    lv_label_set_text(title, "Plants vs. Zombies");
    lv_obj_set_pos(title, 0, 12);
    lv_obj_set_size(title, MENU_SCR_W, 64);

    /* ============================================================
     * 第 4 个控件：副标题（标签 Label）
     * "Classic Defense - ARM Board Demo" 20号字体，白色
     * ============================================================ */
    lv_obj_t *sub = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_color(sub, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(sub, "Classic Defense - ARM Board Demo");
    lv_obj_set_pos(sub, MENU_SCR_W / 2 - 200, 76);
    lv_obj_set_size(sub, 400, 24);

    /* ============================================================
     * 第 5 个控件：中部说明文字（标签 Label）
     * 多行文字介绍游戏规则
     * ============================================================ */
    lv_obj_t *intro = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_color(intro, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(intro, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(intro, LV_TEXT_ALIGN_CENTER, 0);
    /* 文字中 \n 是换行符，标签会自动换行显示 */
    lv_label_set_text(intro,
        "5 lanes of zombies, 9 columns of lawn\n"
        "Sunflower makes sun, Peashooter fires peas\n"
        "Snow Pea slows, Wall-nut blocks, Cherry Bomb clears");
    lv_obj_set_pos(intro, MENU_SCR_W / 2 - 360, 280);
    lv_obj_set_size(intro, 720, 100);

    /* ============================================================
     * 第 6 个控件：START 按钮（按钮 Button）
     * 绿色渐变背景 + 金色边框 + 阴影 + 圆角
     * ============================================================ */
    static lv_style_t st_btn;
    lv_style_init(&st_btn);
    lv_style_set_bg_color(&st_btn, lv_color_hex(0x60B43C));       /* 主色：亮绿 */
    lv_style_set_bg_grad_color(&st_btn, lv_color_hex(0x3A6F22)); /* 渐变：深绿 */
    lv_style_set_bg_grad_dir(&st_btn, LV_GRAD_DIR_VER);          /* 垂直渐变 */
    lv_style_set_bg_opa(&st_btn, LV_OPA_COVER);
    lv_style_set_radius(&st_btn, 16);                             /* 圆角 16px */
    lv_style_set_border_color(&st_btn, lv_color_hex(0xFFE060));  /* 金色边框 */
    lv_style_set_border_width(&st_btn, 4);
    lv_style_set_shadow_color(&st_btn, lv_color_hex(0x000000));  /* 黑色阴影 */
    lv_style_set_shadow_opa(&st_btn, LV_OPA_50);                 /* 阴影半透明 */
    lv_style_set_shadow_width(&st_btn, 12);                      /* 阴影宽度 */
    lv_style_set_shadow_offset_y(&st_btn, 6);                    /* 阴影向下偏移 6px */

    /* lv_button_create(父对象)：创建一个按钮控件，父对象是活动屏幕 */
    lv_obj_t *btn = lv_button_create(lv_screen_active());
    lv_obj_remove_style_all(btn);                /* 移除按钮默认样式 */
    lv_obj_add_style(btn, &st_btn, 0);          /* 挂载自己的按钮样式 */
    lv_obj_set_size(btn, 320, 88);               /* 按钮尺寸 320x88 */
    lv_obj_set_pos(btn, MENU_SCR_W / 2 - 160, 430);  /* 居中靠下 */
    /* lv_obj_add_event_cb(对象, 回调函数, 事件类型, 用户数据)：
       给按钮注册点击事件，点击时调用 menu_start_cb */
    lv_obj_add_event_cb(btn, menu_start_cb, LV_EVENT_CLICKED, NULL);

    /* ============================================================
     * 第 7 个控件：按钮上的文字（标签 Label）
     * 注意：这个标签的父对象是 btn（按钮），不是屏幕
     * ============================================================ */
    lv_obj_t *btn_label = lv_label_create(btn);   /* 父对象 = btn，文字画在按钮上 */
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_align(btn_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(btn_label, "START GAME");
    lv_obj_set_pos(btn_label, 0, 0);
    lv_obj_set_size(btn_label, 320, 88);
    lv_obj_set_style_text_align(btn_label, LV_TEXT_ALIGN_CENTER, 0);

    /* ============================================================
     * 第 8 个控件：底部署名（标签 Label）
     * 版本号和硬件信息，14号字体，灰色
     * ============================================================ */
    lv_obj_t *foot = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_color(foot, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_text_font(foot, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(foot, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(foot, "v1.0  |  /dev/fb0 1024x600 32bpp  |  /dev/input/event6  evdev");
    lv_obj_set_pos(foot, MENU_SCR_W / 2 - 220, MENU_SCR_H - 36);
    lv_obj_set_size(foot, 440, 22);
}
