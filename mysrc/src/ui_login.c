/* ui_login.c - 登录界面
 * 用户名 + 密码 + 服务器 IP，TCP 连上服务器后发 login/register JSON 验证。
 * 网络操作放在 pthread 里做，避免阻塞 LVGL 主线程；结果用 lv_async_call 回主线程。
 */
#include "lvgl/lvgl.h"
#include "mysrc/inc/ui_login.h"
#include "mysrc/inc/net_chat.h"
#include "mysrc/inc/ui_home.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_IP_DEFAULT  "192.168.1.100"   /* 按实际网段修改 */
#define CHAT_PORT          8888

/* ---------- 控件句柄 ---------- */
static lv_obj_t *ip_ta;          /* 服务器 IP 输入框 */
static lv_obj_t *name_ta;        /* 用户名输入框 */
static lv_obj_t *pass_ta;        /* 密码输入框（密码模式） */
static lv_obj_t *kb;             /* 触摸键盘 */
static lv_obj_t *status_lbl;     /* 提示文字（连不上/密码错等） */

static volatile bool g_busy = false;   /* 防止连点登录开多个线程 */

/* 前置声明（线程函数里要引用） */
static void *login_thread(void *arg);
void login_result_cb(void *p);
void login_success_cb(void *p);

/* ---------- 登录线程的参数与结果 ---------- */
typedef struct {
    char ip[64];
    char name[32];
    char pass[32];
    bool is_register;            /* true=注册 false=登录 */
} login_req_t;

/* 线程里跑：连接 + 发包 + 收响应（完成后自释放参数） */
static void *login_thread(void *arg)
{
    login_req_t *req = (login_req_t *)arg;

    int fd = chat_connect(req->ip, CHAT_PORT);
    if (fd < 0) {
        char *msg = strdup("connect failed (check IP/server)");
        lv_async_call(login_result_cb, msg);
        free(req);
        g_busy = false;
        return NULL;
    }

    /* 组 JSON：{"command":"login","data":{"name":..,"password":..}} */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "command", req->is_register ? "register" : "login");
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "name", req->name);
    cJSON_AddStringToObject(data, "password", req->pass);
    cJSON_AddItemToObject(root, "data", data);
    chat_send_json(fd, root);
    cJSON_Delete(root);

    char buf[1024];
    int n = chat_recv_packet(fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        char *msg = strdup("no response from server");
        lv_async_call(login_result_cb, msg);
        chat_disconnect();
        free(req);
        g_busy = false;
        return NULL;
    }
    buf[n] = '\0';

    cJSON *resp = cJSON_Parse(buf);
    const char *status = resp && cJSON_GetObjectItem(resp, "status")
                         ? cJSON_GetObjectItem(resp, "status")->valuestring : "";
    const char *msg    = resp && cJSON_GetObjectItem(resp, "message")
                         ? cJSON_GetObjectItem(resp, "message")->valuestring : "";

    if (!req->is_register && !strcmp(status, "success")) {
        cJSON *d = cJSON_GetObjectItem(resp, "data");
        int my_id = d && cJSON_GetObjectItem(d, "id") ? (int)cJSON_GetObjectItem(d, "id")->valuedouble : -1;
        chat_set_session(my_id, req->name);
        lv_async_call(login_success_cb, NULL);      /* 切到主界面 */
    } else {
        char *out = malloc(96);
        snprintf(out, 96, "%s: %s", req->is_register ? "register" : "login", msg);
        lv_async_call(login_result_cb, out);
        chat_disconnect();
    }
    if (resp) cJSON_Delete(resp);
    free(req);
    g_busy = false;
    return NULL;
}

/* ---------- 异步回调（LVGL 主线程执行） ---------- */
void login_result_cb(void *p)
{
    char *msg = (char *)p;
    if (msg) {
        lv_label_set_text(status_lbl, msg);
        free(msg);
    }
}

void login_success_cb(void *p)
{
    (void)p;
    ui_home_show();          /* 登录成功 -> 主界面 */
}

/* ---------- 事件回调 ---------- */
static void ta_click_cb(lv_event_t *e)   /* 输入框获焦点 -> 键盘绑到它 */
{
    lv_obj_t *ta = lv_event_get_target(e);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

static void start_request(bool is_register)
{
    if (g_busy) return;

    login_req_t *req = calloc(1, sizeof(login_req_t));
    snprintf(req->ip, sizeof(req->ip), "%s", lv_textarea_get_text(ip_ta));
    /* 去掉 textarea 文本末尾换行 */
    size_t l = strlen(req->ip); if (l && req->ip[l-1] == '\n') req->ip[l-1] = '\0';
    snprintf(req->name, sizeof(req->name), "%s", lv_textarea_get_text(name_ta));
    l = strlen(req->name); if (l && req->name[l-1] == '\n') req->name[l-1] = '\0';
    snprintf(req->pass, sizeof(req->pass), "%s", lv_textarea_get_text(pass_ta));
    l = strlen(req->pass); if (l && req->pass[l-1] == '\n') req->pass[l-1] = '\0';
    req->is_register = is_register;

    if (!strlen(req->name)) {
        lv_label_set_text(status_lbl, "please input username");
        free(req);
        return;
    }
    g_busy = true;
    lv_label_set_text(status_lbl, "connecting...");

    pthread_t tid;
    pthread_create(&tid, NULL, login_thread, req);
    pthread_detach(tid);
}

static void login_btn_cb(lv_event_t *e)  { (void)e; start_request(false); }
static void reg_btn_cb(lv_event_t *e)    { (void)e; start_request(true); }

/* ---------- 构建登录 UI ---------- */
void ui_login_show(void)
{
    /* 版本标记：跑起来终端会打印这一行，用来确认板子上跑的是不是最新代码 */
    printf("[UI] === login page BUILD v2 (keyboard always-on) ===\n");
    printf("[UI] screen resolution = %d x %d\n",
           (int)lv_display_get_horizontal_resolution(NULL),
           (int)lv_display_get_vertical_resolution(NULL));
    fflush(stdout);

    lv_obj_clean(lv_screen_active());
    lv_obj_clear_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);   /* 禁止整屏滚动，防止键盘被滚到视口外 */
    lv_refr_now(NULL);                                   /* 防旧画面残留 */
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x1B2A38), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    /* 标题 */
    lv_obj_t *title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Smart Terminal");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *sub = lv_label_create(lv_screen_active());
    /* 屏幕上可见的版本标记：看到这行说明跑的是新代码 */
    lv_label_set_text(sub, "Chat | Game | Music    [ BUILD v2 - KB ON ]");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x55DD88), 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 56);

    /* 三行输入框 + 右侧标签（整体上移，给底部键盘让位） */
    const struct { lv_obj_t **ta; const char *hint; bool is_pass; int y; } rows[] = {
        { &ip_ta,   "Server", false, 96  },
        { &name_ta, "User",   false, 162 },
        { &pass_ta, "Pass",   true,  228 },
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *lbl = lv_label_create(lv_screen_active());
        lv_label_set_text(lbl, rows[i].hint);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xAABBCB), 0);
        lv_obj_set_pos(lbl, 262, rows[i].y + 15);

        lv_obj_t *ta = lv_textarea_create(lv_screen_active());
        lv_obj_set_size(ta, 400, 56);
        lv_obj_set_pos(ta, 360, rows[i].y);
        lv_textarea_set_one_line(ta, true);
        lv_textarea_set_placeholder_text(ta, rows[i].hint);
        if (rows[i].is_pass) lv_textarea_set_password_mode(ta, true);
        /* 点击输入框 -> 把键盘绑到它（CLICKED 比 FOCUSED 更可靠） */
        lv_obj_add_event_cb(ta, ta_click_cb, LV_EVENT_CLICKED, NULL);
        *rows[i].ta = ta;
    }
    lv_textarea_set_text(ip_ta, SERVER_IP_DEFAULT);      /* IP 给个默认值少敲几个字 */

    /* 登录 / 注册 按钮 */
    lv_obj_t *login_btn = lv_btn_create(lv_screen_active());
    lv_obj_set_size(login_btn, 190, 54);
    lv_obj_set_pos(login_btn, 360, 296);
    lv_obj_set_style_bg_color(login_btn, lv_color_hex(0x1F6F43), 0);
    lv_obj_t *ll = lv_label_create(login_btn);
    lv_label_set_text(ll, "LOGIN");
    lv_obj_center(ll);
    lv_obj_add_event_cb(login_btn, login_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *reg_btn = lv_btn_create(lv_screen_active());
    lv_obj_set_size(reg_btn, 190, 54);
    lv_obj_set_pos(reg_btn, 570, 296);
    lv_obj_set_style_bg_color(reg_btn, lv_color_hex(0x39586E), 0);
    lv_obj_t *rl = lv_label_create(reg_btn);
    lv_label_set_text(rl, "REGISTER");
    lv_obj_center(rl);
    lv_obj_add_event_cb(reg_btn, reg_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 状态提示 */
    status_lbl = lv_label_create(lv_screen_active());
    lv_label_set_text(status_lbl, "");
    lv_obj_set_style_text_color(status_lbl, lv_color_hex(0xE67E22), 0);
    lv_obj_align(status_lbl, LV_ALIGN_TOP_MID, 0, 358);

    /* 触摸键盘：常显，贴屏幕底部 + 占满宽度（自适应分辨率） */
    kb = lv_keyboard_create(lv_screen_active());
    lv_obj_set_size(kb, lv_pct(100), 200);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, name_ta);               /* 默认绑到用户名输入框 */
    lv_obj_move_foreground(kb);                          /* 确保键盘在最上层，不被遮挡 */
    /* 关键修复：先强制算好布局，再整屏标脏+立即重绘，
     * 否则键盘首次不会被绘制，要触摸一下才出现 */
    lv_obj_update_layout(lv_screen_active());
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(NULL);
}
