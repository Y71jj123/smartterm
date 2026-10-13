/* net_chat.c - 网络聊天模块
 * TCP + cJSON + pthread：
 *   1) 封包格式与服务器一致：4 字节长度(网络字节序) + JSON
 *   2) 收包放在独立线程里，收到消息后用 lv_async_call 把"刷 UI"丢回
 *      LVGL 主线程执行（LVGL 对象不允许跨线程直接操作）
 */
#include "lvgl/lvgl.h"
#include "mysrc/inc/net_chat.h"
#include "mysrc/inc/ui_home.h"
#include "mysrc/inc/ui_kb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define CHAT_PORT     8888
#define MSG_MAX       2048
#define NAME_MAX_LEN  31

static void user_click_cb(lv_event_t *e);   /* 前置声明：async_add_user 里要用 */

/* ================= 会话状态 ================= */
static int  g_sock = -1;
static int  g_my_id = -1;
static char g_my_name[32] = "";
static pthread_t g_net_tid;          /* 收包线程 */
static volatile bool g_net_running = false;

int chat_sock(void)            { return g_sock; }
int chat_my_id(void)           { return g_my_id; }
const char *chat_my_name(void) { return g_my_name; }

void chat_set_session(int id, const char *name)
{
    g_my_id = id;
    snprintf(g_my_name, sizeof(g_my_name), "%s", name);
}

void chat_disconnect(void)
{
    if (g_sock >= 0) { close(g_sock); g_sock = -1; }
    g_my_id = -1;
    g_my_name[0] = '\0';
}

/* ================= 封包收发 ================= */
int chat_send_packet(int fd, const char *buf, int len)
{
    uint32_t net_len = htonl(len);
    if (send(fd, &net_len, 4, 0) != 4) return -1;
    return (send(fd, buf, len, 0) == len) ? 0 : -1;
}

int chat_recv_packet(int fd, char *buf, int max)
{
    uint32_t net_len;
    if (recv(fd, &net_len, 4, MSG_WAITALL) != 4) return -1;
    int len = ntohl(net_len);
    if (len <= 0 || len > max) return -1;
    return recv(fd, buf, len, MSG_WAITALL);
}

int chat_send_json(int fd, cJSON *root)
{
    char *s = cJSON_PrintUnformatted(root);
    int rc = chat_send_packet(fd, s, strlen(s));
    free(s);
    return rc;
}

int chat_connect(const char *ip, int port)
{
    if (g_sock >= 0) close(g_sock);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port ? port : CHAT_PORT);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    g_sock = fd;
    return fd;
}

/* ================= 聊天界面控件 ================= */
static lv_obj_t *user_list;      /* 左侧：在线用户列表 */
static lv_obj_t *msg_area;       /* 右侧：消息滚动区 */
static lv_obj_t *input_ta;       /* 底部：输入框 */
static lv_obj_t *kb;             /* 触摸键盘 */
static lv_obj_t *title_lbl;      /* 顶部标题 */
static lv_obj_t *target_lbl;     /* 当前私聊对象提示 */

static int  g_target_id = -1;    /* 当前私聊对象 id，-1 = 未选 */
static char g_target_name[32] = "";

/* 跨线程刷新用的堆消息 */
typedef struct {
    bool  is_group;
    bool  is_system;
    int   sender_id;
    char  sender_name[32];
    char  text[512];
} chat_msg_t;

/* ---------- 工具：往消息区追加一行 ---------- */
static void append_msg_line(const char *line, uint32_t color)
{
    lv_obj_t *lbl = lv_label_create(msg_area);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, lv_pct(96));
    lv_label_set_text(lbl, line);
    lv_obj_set_style_text_color(lbl, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_scroll_to_y(msg_area, LV_COORD_MAX, LV_ANIM_OFF);  /* 滚到最底 */
}

static void add_system_line(const char *s) { append_msg_line(s, 0x888888); }

/* ---------- 异步回调（LVGL 主线程执行） ---------- */
static void async_show_msg(void *p)
{
    chat_msg_t *m = (chat_msg_t *)p;
    char buf[640];
    if (m->is_group)
        snprintf(buf, sizeof(buf), "[%s]: %s", m->sender_name, m->text);
    else
        snprintf(buf, sizeof(buf), "[PM from %s]: %s", m->sender_name, m->text);
    append_msg_line(buf, m->is_group ? 0x2A6099 : 0x992A60);
    free(m);
}

static void async_add_user(void *p)      /* p = "id|name" 堆字符串 */
{
    char *info = (char *)p;
    int id = atoi(info);
    const char *name = strchr(info, '|') ? strchr(info, '|') + 1 : "?";

    lv_obj_t *btn = lv_list_add_btn(user_list, LV_SYMBOL_EYE_OPEN, name);
    lv_obj_set_user_data(btn, (void *)(intptr_t)id);
    lv_obj_add_event_cb(btn, user_click_cb, LV_EVENT_CLICKED, NULL);
    free(info);
}

static void async_refresh_users(void *p) /* p = cJSON 在线列表字符串 */
{
    char *json = (char *)p;
    lv_obj_clean(user_list);
    cJSON *arr = cJSON_Parse(json);
    if (arr && cJSON_IsArray(arr)) {
        int n = cJSON_GetArraySize(arr);
        char buf[64];
        snprintf(buf, sizeof(buf), "Online: %d", n);
        lv_obj_t *t = lv_list_add_btn(user_list, LV_SYMBOL_REFRESH, buf);
        lv_obj_set_user_data(t, (void *)(intptr_t)-1);
        lv_obj_add_event_cb(t, user_click_cb, LV_EVENT_CLICKED, NULL);  /* 点标题 = 刷新 */
        for (int i = 0; i < n; i++) {
            cJSON *o = cJSON_GetArrayItem(arr, i);
            char *info = malloc(64);
            snprintf(info, 64, "%d|%s",
                     (int)cJSON_GetObjectItem(o, "id")->valuedouble,
                     cJSON_GetObjectItem(o, "name")->valuestring);
            async_add_user(info);        /* 同线程直接调用没问题 */
        }
    }
    if (arr) cJSON_Delete(arr);
    free(json);
}

/* ---------- 收包线程 ---------- */
static void *net_thread(void *arg)
{
    (void)arg;
    char buf[MSG_MAX];
    while (g_net_running) {
        int n = chat_recv_packet(g_sock, buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';

        cJSON *root = cJSON_Parse(buf);
        if (!root) continue;
        const char *status  = cJSON_GetObjectItem(root, "status")  ? cJSON_GetObjectItem(root, "status")->valuestring : "";
        const char *msg     = cJSON_GetObjectItem(root, "message") ? cJSON_GetObjectItem(root, "message")->valuestring : "";
        cJSON *data         = cJSON_GetObjectItem(root, "data");

        if (!strcmp(msg, "private_message") && data) {
            chat_msg_t *m = calloc(1, sizeof(chat_msg_t));
            m->is_group = false;
            m->sender_id = (int)cJSON_GetObjectItem(data, "sender_id")->valuedouble;
            snprintf(m->sender_name, sizeof(m->sender_name), "%s", cJSON_GetObjectItem(data, "sender_name")->valuestring);
            snprintf(m->text, sizeof(m->text), "%s", cJSON_GetObjectItem(data, "text")->valuestring);
            lv_async_call(async_show_msg, m);
        }
        else if (!strcmp(msg, "group_message") && data) {
            chat_msg_t *m = calloc(1, sizeof(chat_msg_t));
            m->is_group = true;
            m->sender_id = (int)cJSON_GetObjectItem(data, "sender_id")->valuedouble;
            snprintf(m->sender_name, sizeof(m->sender_name), "%s", cJSON_GetObjectItem(data, "sender_name")->valuestring);
            snprintf(m->text, sizeof(m->text), "%s", cJSON_GetObjectItem(data, "text")->valuestring);
            lv_async_call(async_show_msg, m);
        }
        else if (!strcmp(msg, "online_users") && data) {
            char *s = cJSON_PrintUnformatted(data);
            lv_async_call(async_refresh_users, s);
        }
        else if (!strcmp(msg, "user_online") && data) {
            char *info = malloc(64);
            snprintf(info, 64, "%d|%s",
                     (int)cJSON_GetObjectItem(data, "id")->valuedouble,
                     cJSON_GetObjectItem(data, "name")->valuestring);
            lv_async_call(async_add_user, info);
        }
        else if (!strcmp(status, "error")) {
            chat_msg_t *m = calloc(1, sizeof(chat_msg_t));
            m->is_system = true;
            snprintf(m->text, sizeof(m->text), "server error: %s", msg);
            lv_async_call(async_show_msg, m);
        }
        cJSON_Delete(root);
    }
    return NULL;
}

/* ---------- 事件回调 ---------- */
static void user_click_cb(lv_event_t *e)   /* 点了在线用户：选为私聊对象 */
{
    lv_obj_t *btn = lv_event_get_target(e);
    int id = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (id < 0) {                            /* 点标题行 = 请求刷新在线列表 */
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "command", "list");
        chat_send_json(g_sock, root);
        cJSON_Delete(root);
        return;
    }
    g_target_id = id;
    /* 从按钮的 label 里取名字 */
    lv_obj_t *lbl = lv_obj_get_child(btn, lv_obj_get_child_count(btn) - 1);
    snprintf(g_target_name, sizeof(g_target_name), "%s", lv_label_get_text(lbl));
    char buf[80];
    snprintf(buf, sizeof(buf), "PM target: %s (#%d)", g_target_name, id);
    lv_label_set_text(target_lbl, buf);
}

static void ta_click_cb(lv_event_t *e)     /* 输入框获焦点 -> 键盘绑定它 */
{
    lv_obj_t *ta = lv_event_get_target(e);
    lv_keyboard_set_textarea(kb, ta);
}

static void send_pm_cb(lv_event_t *e)
{
    (void)e;
    const char *txt = lv_textarea_get_text(input_ta);
    if (!txt || !strlen(txt)) return;
    if (g_target_id < 0) { add_system_line("Select a user first (left list)"); return; }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "command", "pm");
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "receiver_id", g_target_id);
    cJSON_AddStringToObject(data, "message", txt);
    cJSON_AddItemToObject(root, "data", data);
    chat_send_json(g_sock, root);
    cJSON_Delete(root);

    char buf[600];
    snprintf(buf, sizeof(buf), "[me -> %s]: %s", g_target_name, txt);
    append_msg_line(buf, 0x336633);
    lv_textarea_set_text(input_ta, "");
}

static void send_group_cb(lv_event_t *e)
{
    (void)e;
    const char *txt = lv_textarea_get_text(input_ta);
    if (!txt || !strlen(txt)) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "command", "gmsg");
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "message", txt);
    cJSON_AddItemToObject(root, "data", data);
    chat_send_json(g_sock, root);
    cJSON_Delete(root);
    lv_textarea_set_text(input_ta, "");
}

static void back_cb(lv_event_t *e)         /* 返回主界面 */
{
    (void)e;
    chat_app_close();
}

/* ---------- 建聊天 UI ---------- */
void chat_app_open(void)
{
    lv_obj_clean(lv_screen_active());
    lv_obj_clear_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);   /* 禁止整屏滚动，防手势被吞 */
    lv_refr_now(NULL);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x1B2631), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    /* 顶部栏 */
    lv_obj_t *top = lv_obj_create(lv_screen_active());
    lv_obj_set_size(top, 1024, 56);
    lv_obj_set_pos(top, 0, 0);
    lv_obj_set_style_bg_color(top, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_radius(top, 0, 0);

    title_lbl = lv_label_create(top);
    lv_label_set_text_fmt(title_lbl, "Chat   [my id=%d name=%s]", g_my_id, g_my_name);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(title_lbl);

    lv_obj_t *back = lv_btn_create(top);
    lv_obj_set_size(back, 90, 38);
    lv_obj_align(back, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "BACK");
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    /* 左侧在线用户列表（上半区压缩，给底部 5 行键盘让位置） */
    user_list = lv_list_create(lv_screen_active());
    lv_obj_set_size(user_list, 240, 230);
    lv_obj_set_pos(user_list, 10, 62);

    target_lbl = lv_label_create(lv_screen_active());
    lv_label_set_text(target_lbl, "PM target: none");
    lv_obj_set_style_text_color(target_lbl, lv_color_hex(0xF0E68C), 0);
    lv_obj_set_pos(target_lbl, 10, 296);

    /* 右侧消息区（可滚动容器，内部是竖排 label） */
    msg_area = lv_obj_create(lv_screen_active());
    lv_obj_set_size(msg_area, 750, 230);
    lv_obj_set_pos(msg_area, 260, 62);
    lv_obj_set_flex_flow(msg_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(msg_area, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_color(msg_area, lv_color_hex(0x111111), 0);
    lv_obj_set_scroll_dir(msg_area, LV_DIR_VER);

    /* 底部输入行 */
    input_ta = lv_textarea_create(lv_screen_active());
    lv_obj_set_size(input_ta, 640, 50);
    lv_obj_set_pos(input_ta, 260, 316);
    /* 点击输入框 -> 键盘绑到它（CLICKED 比 FOCUSED 可靠） */
    lv_obj_add_event_cb(input_ta, ta_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *pm_btn = lv_btn_create(lv_screen_active());
    lv_obj_set_size(pm_btn, 90, 50);
    lv_obj_set_pos(pm_btn, 910, 316);
    lv_obj_t *pl = lv_label_create(pm_btn);
    lv_label_set_text(pl, "PM");
    lv_obj_center(pl);
    lv_obj_add_event_cb(pm_btn, send_pm_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *gr_btn = lv_btn_create(lv_screen_active());
    lv_obj_set_size(gr_btn, 240, 50);
    lv_obj_set_pos(gr_btn, 10, 316);
    lv_obj_set_style_bg_color(gr_btn, lv_color_hex(0x1F6F43), 0);
    lv_obj_t *gl = lv_label_create(gr_btn);
    lv_label_set_text(gl, "GROUP SEND");
    lv_obj_center(gl);
    lv_obj_add_event_cb(gr_btn, send_group_cb, LV_EVENT_CLICKED, NULL);

    /* 触摸键盘：常显（同登录页，避免板上点输入框弹不出键盘）；
     * 高度 230 = 5 行 × 46px，布局见 ui_kb.c */
    kb = lv_keyboard_create(lv_screen_active());
    lv_obj_set_size(kb, 1024, 230);
    lv_obj_set_pos(kb, 0, 370);
    smart_kb_apply(kb);                       /* 换成常驻数字行的常见布局 */
    lv_keyboard_set_textarea(kb, input_ta);   /* 默认绑到聊天输入框 */

    /* 进入即请求一次在线列表，并启动收包线程 */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "command", "list");
    chat_send_json(g_sock, root);
    cJSON_Delete(root);

    g_net_running = true;
    pthread_create(&g_net_tid, NULL, net_thread, NULL);
}

void chat_app_close(void)
{
    g_net_running = false;
    if (g_sock >= 0) shutdown(g_sock, SHUT_RDWR);   /* 让阻塞的 recv 立刻返回 */
    pthread_join(g_net_tid, NULL);
    chat_disconnect();
    ui_home_show();
}
