/* game_2048.c - 2048 数字拼合小游戏（LVGL 控件绘制版，零图片素材）
 *
 * 设计要点：
 *   1. 棋盘用 4x4 的 lv_obj 方块 + lv_label 数字文字绘制，
 *      数字方块按经典 2048 配色，完全不依赖外部 BMP 素材；
 *   2. 输入用“触摸滑动手势”（在棋盘上 PRESSED 记起点、RELEASED 算方向），
 *      适配开发板触摸屏，无需实体键盘；
 *   3. 含分数 / 最高分、游戏结束与胜利判定、BACK 返回主界面、RESTART 重开。
 *
 * 退出逻辑与 game_pvz.c 保持一致：lv_obj_clean 当前屏幕 + 调 ui_home_show()。
 */
#include "lvgl/lvgl.h"
#include "mysrc/inc/game_2048.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 屏幕尺寸（与项目其它界面一致） */
#define SCR_W 1024
#define SCR_H 600

/* 棋盘：4x4 网格 */
#define N 4
static int board[N][N];

/* 分数 */
static int g_score = 0;
static int g_best  = 0;

/* LVGL 对象句柄 */
static lv_obj_t *g_tiles[N][N];   /* 每个格子：底板对象 */
static lv_obj_t *g_labels[N][N];  /* 每个格子上的文字 */
static lv_obj_t *g_score_lbl;
static lv_obj_t *g_overlay;       /* 游戏结束/胜利遮罩（NULL 表示没弹出） */

/* 滑动手势用的起止点 */
static lv_point_t g_start_pt;
static bool g_pressed = false;

/* 外部函数：回到主界面（定义在 ui_home.c） */
extern void ui_home_show(void);

/* 前向声明 */
static void restart_cb(lv_event_t *e);

/* ----------------------------------------------------------------------
 * 配色：经典 2048 调色，按数值返回背景色与文字色
 * -------------------------------------------------------------------- */
static void tile_colors(int v, lv_color_t *bg, lv_color_t *fg)
{
    if (v == 0)            { *bg = lv_color_hex(0x2E3B4E); *fg = lv_color_hex(0x2E3B4E); return; }
    if (v == 2)            { *bg = lv_color_hex(0xEEE4DA); *fg = lv_color_hex(0x776E65); return; }
    if (v == 4)            { *bg = lv_color_hex(0xEDE0C8); *fg = lv_color_hex(0x776E65); return; }
    if (v == 8)            { *bg = lv_color_hex(0xF2B179); *fg = lv_color_hex(0xF9F6F2); return; }
    if (v == 16)           { *bg = lv_color_hex(0xF59563); *fg = lv_color_hex(0xF9F6F2); return; }
    if (v == 32)           { *bg = lv_color_hex(0xF67C5F); *fg = lv_color_hex(0xF9F6F2); return; }
    if (v == 64)           { *bg = lv_color_hex(0xF65E3B); *fg = lv_color_hex(0xF9F6F2); return; }
    if (v == 128)          { *bg = lv_color_hex(0xEDCF72); *fg = lv_color_hex(0xF9F6F2); return; }
    if (v == 256)          { *bg = lv_color_hex(0xEDCC61); *fg = lv_color_hex(0xF9F6F2); return; }
    if (v == 512)          { *bg = lv_color_hex(0xEDC850); *fg = lv_color_hex(0xF9F6F2); return; }
    if (v == 1024)         { *bg = lv_color_hex(0xEDC53F); *fg = lv_color_hex(0xF9F6F2); return; }
    if (v >= 2048)         { *bg = lv_color_hex(0xEDC22E); *fg = lv_color_hex(0xF9F6F2); return; }
    *bg = lv_color_hex(0x3C3A32); *fg = lv_color_hex(0xF9F6F2); /* 兜底 */
}

/* 依据数字位数选字号，避免四位数字溢出方块 */
static const lv_font_t *tile_font(int v)
{
    if (v >= 1000) return &lv_font_montserrat_28;
    if (v >= 100)  return &lv_font_montserrat_32;
    return &lv_font_montserrat_36;
}

/* ----------------------------------------------------------------------
 * 棋盘逻辑：在某一行/列上“向 0 号位合并”（左/上方向）
 * -------------------------------------------------------------------- */
static void slide_line(int *line)
{
    int tmp[4] = {0};
    int idx = 0;
    for (int i = 0; i < N; i++)
        if (line[i]) tmp[idx++] = line[i];

    for (int i = 0; i < idx - 1; i++) {
        if (tmp[i] && tmp[i] == tmp[i + 1]) {
            tmp[i] *= 2;
            g_score += tmp[i];
            tmp[i + 1] = 0;
        }
    }
    int out[4] = {0};
    int o = 0;
    for (int i = 0; i < N; i++)
        if (tmp[i]) out[o++] = tmp[i];

    memcpy(line, out, sizeof(out));
}

/* 方向枚举 */
typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } dir_t;

static void move(dir_t d)
{
    int line[4];
    if (d == DIR_LEFT) {
        for (int i = 0; i < N; i++) { memcpy(line, board[i], sizeof(line)); slide_line(line); memcpy(board[i], line, sizeof(line)); }
    } else if (d == DIR_RIGHT) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) line[j] = board[i][N - 1 - j];
            slide_line(line);
            for (int j = 0; j < N; j++) board[i][N - 1 - j] = line[j];
        }
    } else if (d == DIR_UP) {
        for (int j = 0; j < N; j++) {
            for (int i = 0; i < N; i++) line[i] = board[i][j];
            slide_line(line);
            for (int i = 0; i < N; i++) board[i][j] = line[i];
        }
    } else { /* DIR_DOWN */
        for (int j = 0; j < N; j++) {
            for (int i = 0; i < N; i++) line[i] = board[N - 1 - i][j];
            slide_line(line);
            for (int i = 0; i < N; i++) board[N - 1 - i][j] = line[i];
        }
    }
}

/* 棋盘是否还有可走步（有空格，或有相邻相等） */
static bool can_move(void)
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            if (board[i][j] == 0) return true;
            if (j + 1 < N && board[i][j] == board[i][j + 1]) return true;
            if (i + 1 < N && board[i][j] == board[i + 1][j]) return true;
        }
    return false;
}

static bool has_won(void)
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (board[i][j] >= 2048) return true;
    return false;
}

/* 在随机空格生成一个 2（90%）或 4（10%） */
static void spawn(void)
{
    int empty[16][2], cnt = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (board[i][j] == 0) { empty[cnt][0] = i; empty[cnt][1] = j; cnt++; }
    if (cnt == 0) return;
    int k = rand() % cnt;
    int i = empty[k][0], j = empty[k][1];
    board[i][j] = (rand() % 10 == 0) ? 4 : 2;
}

/* 比较两个棋盘是否相同（判断本次滑动是否产生变化） */
static bool board_equal(int a[N][N], int b[N][N])
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (a[i][j] != b[i][j]) return false;
    return true;
}

/* ----------------------------------------------------------------------
 * UI 刷新
 * -------------------------------------------------------------------- */
static void refresh_board(void)
{
    char buf[8];
    lv_color_t bg, fg;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int v = board[i][j];
            tile_colors(v, &bg, &fg);
            lv_obj_set_style_bg_color(g_tiles[i][j], bg, 0);
            if (v == 0) {
                lv_label_set_text(g_labels[i][j], "");
            } else {
                snprintf(buf, sizeof(buf), "%d", v);
                lv_label_set_text(g_labels[i][j], buf);
                lv_obj_set_style_text_font(g_labels[i][j], tile_font(v), 0);
                lv_obj_set_style_text_color(g_labels[i][j], fg, 0);
            }
        }
    }
    if (g_score > g_best) g_best = g_score;
    lv_label_set_text_fmt(g_score_lbl, "SCORE: %d    BEST: %d", g_score, g_best);
}

/* 游戏结束 / 胜利遮罩 */
static void show_overlay(const char *title)
{
    g_overlay = lv_obj_create(lv_screen_active());
    lv_obj_set_size(g_overlay, SCR_W, SCR_H);
    lv_obj_set_pos(g_overlay, 0, 0);
    lv_obj_set_style_bg_color(g_overlay, lv_color_hex(0x101418), 0);
    lv_obj_set_style_bg_opa(g_overlay, LV_OPA_80, 0);
    lv_obj_set_style_border_width(g_overlay, 0, 0);

    lv_obj_t *t = lv_label_create(g_overlay);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(t, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *btn = lv_btn_create(g_overlay);
    lv_obj_set_size(btn, 260, 80);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x3D7A3D), 0);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "RESTART");
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0);
    lv_obj_center(bl);
    lv_obj_add_event_cb(btn, restart_cb, LV_EVENT_CLICKED, NULL);
}

static void restart_cb(lv_event_t *e)
{
    (void)e;
    g_score = 0;
    if (g_overlay) { lv_obj_del(g_overlay); g_overlay = NULL; }
    memset(board, 0, sizeof(board));
    spawn();
    spawn();
    refresh_board();
}

/* ----------------------------------------------------------------------
 * 滑动手势：在棋盘容器上 PRESSED 记起点，RELEASED 判定方向并移动
 * -------------------------------------------------------------------- */
#define SWIPE_MIN 30   /* 小于该位移视为点击，不算滑动 */

static void board_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    lv_point_t pt;

    if (code == LV_EVENT_PRESSED) {
        lv_indev_get_point(indev, &pt);
        g_start_pt = pt;
        g_pressed = true;
    } else if (code == LV_EVENT_RELEASED && g_pressed) {
        lv_indev_get_point(indev, &pt);
        g_pressed = false;

        int dx = pt.x - g_start_pt.x;
        int dy = pt.y - g_start_pt.y;
        if (abs(dx) < SWIPE_MIN && abs(dy) < SWIPE_MIN) return;  /* 点击，忽略 */

        int before[N][N];
        memcpy(before, board, sizeof(before));

        if (abs(dx) > abs(dy))
            move(dx > 0 ? DIR_RIGHT : DIR_LEFT);
        else
            move(dy > 0 ? DIR_DOWN : DIR_UP);

        if (!board_equal(before, board)) {
            spawn();
            refresh_board();
            if (has_won())
                show_overlay("YOU WIN !");
            else if (!can_move())
                show_overlay("GAME OVER");
        }
    }
}

/* ----------------------------------------------------------------------
 * 返回主界面
 * -------------------------------------------------------------------- */
static void back_cb(lv_event_t *e)
{
    (void)e;
    lv_obj_clean(lv_screen_active());
    ui_home_show();
}

/* ----------------------------------------------------------------------
 * 入口：构建 2048 界面
 * -------------------------------------------------------------------- */
void game_2048(void)
{
    srand((unsigned)time(NULL));

    lv_obj_clean(lv_screen_active());
    lv_refr_now(NULL);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x0F141A), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    /* 标题 + 分数 */
    lv_obj_t *title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "2048");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xEDC22E), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 28, 14);

    g_score_lbl = lv_label_create(lv_screen_active());
    lv_label_set_text(g_score_lbl, "SCORE: 0    BEST: 0");
    lv_obj_set_style_text_font(g_score_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(g_score_lbl, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(g_score_lbl, LV_ALIGN_TOP_MID, 0, 40);

    /* 返回按钮 */
    lv_obj_t *back = lv_btn_create(lv_screen_active());
    lv_obj_set_size(back, 120, 48);
    lv_obj_align(back, LV_ALIGN_TOP_RIGHT, -24, 16);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x7A3B3B), 0);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "BACK");
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_20, 0);
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    /* 棋盘容器：4 格 * (104 方块 + 8 间隙) + 边距 = 456 */
    int cell = 104, gap = 8, pad = 8;
    int bsize = N * cell + (N - 1) * gap + pad * 2;   /* 456 */
    lv_obj_t *board_obj = lv_obj_create(lv_screen_active());
    lv_obj_set_size(board_obj, bsize, bsize);
    lv_obj_align(board_obj, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(board_obj, lv_color_hex(0x223040), 0);
    lv_obj_set_style_bg_opa(board_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(board_obj, 12, 0);
    lv_obj_set_style_border_width(board_obj, 0, 0);
    lv_obj_set_style_pad_all(board_obj, pad, 0);
    lv_obj_add_flag(board_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(board_obj, board_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(board_obj, board_event_cb, LV_EVENT_RELEASED, NULL);

    /* 格子（底板 + 文字），用绝对坐标排布 */
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int x = pad + j * (cell + gap);
            int y = pad + i * (cell + gap);
            lv_obj_t *cell_obj = lv_obj_create(board_obj);
            lv_obj_set_size(cell_obj, cell, cell);
            lv_obj_set_pos(cell_obj, x, y);
            lv_obj_set_style_radius(cell_obj, 8, 0);
            lv_obj_set_style_border_width(cell_obj, 0, 0);
            lv_obj_set_style_bg_color(cell_obj, lv_color_hex(0x2E3B4E), 0);

            lv_obj_t *lab = lv_label_create(cell_obj);
            lv_obj_set_style_text_font(lab, &lv_font_montserrat_36, 0);
            lv_obj_center(lab);

            g_tiles[i][j]  = cell_obj;
            g_labels[i][j] = lab;
        }
    }

    /* 初始化棋盘并重绘 */
    memset(board, 0, sizeof(board));
    g_score = 0;
    g_overlay = NULL;
    spawn();
    spawn();
    refresh_board();

    printf("[2048] page ready\n");
    fflush(stdout);
}
