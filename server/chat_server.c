/* main.c - 聊天服务器入口
 * 智能终端项目 服务器端（跑在 PC / Ubuntu 上，开发板作为客户端连过来）
 * 编译：gcc chat_server.c cJSON.c -o chat_server -lpthread
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "cJSON.h"

#define SERVER_PORT  8888
#define MAX_PKT      65536

/* ============ 数据结构 ============ */
typedef struct user {
    int id;
    char name[32];
    char password[32];
    struct user *next;
} user_t;

typedef struct client {
    int sockfd;
    int id;                    /* 登录成功后才有 id，-1 表示未登录 */
    char name[32];
    struct client *next;
} client_t;

static user_t   *g_users   = NULL;   /* 注册用户表 */
static client_t *g_clients = NULL;   /* 在线客户端链表 */
static int       g_next_id = 1;      /* 下一个分配的用户 id */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/* ============ 封包收发 ============ */
static int send_packet(int fd, const char *buf, int len)
{
    uint32_t net_len = htonl(len);
    if (send(fd, &net_len, 4, 0) != 4) return -1;
    return send(fd, buf, len, 0) == len ? 0 : -1;
}

static int recv_packet(int fd, char *buf, int max)
{
    uint32_t net_len;
    if (recv(fd, &net_len, 4, MSG_WAITALL) != 4) return -1;
    int len = ntohl(net_len);
    if (len <= 0 || len > max) return -1;
    return recv(fd, buf, len, MSG_WAITALL);
}

/* 发送一个 status/message/data 组成的 JSON 响应 */
static void send_json(int fd, const char *status, const char *msg, cJSON *data)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", status);
    cJSON_AddStringToObject(root, "message", msg);
    if (data) cJSON_AddItemToObject(root, "data", data);
    char *s = cJSON_PrintUnformatted(root);
    send_packet(fd, s, strlen(s));
    free(s);
    cJSON_Delete(root);
}

/* ============ 业务处理 ============ */
static user_t *find_user(const char *name)
{
    for (user_t *u = g_users; u; u = u->next)
        if (strcmp(u->name, name) == 0) return u;
    return NULL;
}

/* 在线客户端里按 id 找人 */
static client_t *find_online(int id)
{
    for (client_t *c = g_clients; c; c = c->next)
        if (c->id == id) return c;
    return NULL;
}

/* 组装在线用户列表 JSON: [{id,name}...] */
static cJSON *build_online_list(void)
{
    cJSON *arr = cJSON_CreateArray();
    for (client_t *c = g_clients; c; c = c->next) {
        if (c->id < 0) continue;
        cJSON *o = cJSON_CreateObject();
        cJSON_AddNumberToObject(o, "id", c->id);
        cJSON_AddStringToObject(o, "name", c->name);
        cJSON_AddItemToArray(arr, o);
    }
    return arr;
}

/* 处理注册：data{name, password} */
static void handle_register(client_t *c, cJSON *data)
{
    const char *name = cJSON_GetObjectItem(data, "name") ? cJSON_GetObjectItem(data, "name")->valuestring : NULL;
    const char *pass = cJSON_GetObjectItem(data, "password") ? cJSON_GetObjectItem(data, "password")->valuestring : "";

    if (!name || !strlen(name)) { send_json(c->sockfd, "error", "name_required", NULL); return; }
    if (strlen(name) >= sizeof(((user_t*)0)->name)) { send_json(c->sockfd, "error", "name_too_long", NULL); return; }

    pthread_mutex_lock(&g_lock);
    if (find_user(name)) {
        pthread_mutex_unlock(&g_lock);
        send_json(c->sockfd, "error", "name_exists", NULL);
        return;
    }
    user_t *u = calloc(1, sizeof(user_t));
    u->id = g_next_id++;
    snprintf(u->name, sizeof(u->name), "%s", name);
    snprintf(u->password, sizeof(u->password), "%s", pass ? pass : "");
    u->next = g_users;
    g_users = u;
    pthread_mutex_unlock(&g_lock);
    send_json(c->sockfd, "success", "registered", NULL);
}

/* 处理登录：data{name, password} */
static void handle_login(client_t *c, cJSON *data)
{
    const char *name = cJSON_GetObjectItem(data, "name") ? cJSON_GetObjectItem(data, "name")->valuestring : NULL;
    const char *pass = cJSON_GetObjectItem(data, "password") ? cJSON_GetObjectItem(data, "password")->valuestring : "";

    if (!name || !strlen(name)) { send_json(c->sockfd, "error", "name_required", NULL); return; }

    pthread_mutex_lock(&g_lock);
    user_t *u = find_user(name);
    if (!u) { pthread_mutex_unlock(&g_lock); send_json(c->sockfd, "error", "no_such_user", NULL); return; }
    if (strcmp(u->password, pass ? pass : "") != 0) {
        pthread_mutex_unlock(&g_lock);
        send_json(c->sockfd, "error", "wrong_password", NULL);
        return;
    }
    /* 同名用户不允许重复登录 */
    for (client_t *k = g_clients; k; k = k->next) {
        if (k->id == u->id) {
            pthread_mutex_unlock(&g_lock);
            send_json(c->sockfd, "error", "already_online", NULL);
            return;
        }
    }
    c->id = u->id;
    snprintf(c->name, sizeof(c->name), "%s", u->name);
    pthread_mutex_unlock(&g_lock);

    cJSON *d = cJSON_CreateObject();
    cJSON_AddNumberToObject(d, "id", c->id);
    cJSON_AddStringToObject(d, "name", c->name);
    send_json(c->sockfd, "success", "welcome", d);
    printf("[LOGIN] %s (id=%d) 上线\n", c->name, c->id);

    /* 群发上线通知 */
    cJSON *note = cJSON_CreateObject();
    cJSON_AddStringToObject(note, "status", "info");
    cJSON_AddStringToObject(note, "message", "user_online");
    cJSON *nd = cJSON_CreateObject();
    cJSON_AddNumberToObject(nd, "id", c->id);
    cJSON_AddStringToObject(nd, "name", c->name);
    cJSON_AddItemToObject(note, "data", nd);
    char *s = cJSON_PrintUnformatted(note);
    pthread_mutex_lock(&g_lock);
    for (client_t *k = g_clients; k; k = k->next)
        if (k != c && k->id >= 0) send_packet(k->sockfd, s, strlen(s));
    pthread_mutex_unlock(&g_lock);
    free(s);
    cJSON_Delete(note);
}

/* 处理私聊：data{receiver_id, message} */
static void handle_pm(client_t *c, cJSON *data)
{
    cJSON *jid = cJSON_GetObjectItem(data, "receiver_id");
    cJSON *jmsg = cJSON_GetObjectItem(data, "message");
    if (!jid || !jmsg) { send_json(c->sockfd, "error", "bad_request", NULL); return; }

    pthread_mutex_lock(&g_lock);
    client_t *to = find_online((int)jid->valuedouble);
    if (!to) {
        pthread_mutex_unlock(&g_lock);
        send_json(c->sockfd, "error", "user_offline", NULL);
        return;
    }
    /* 给接收方发通知 */
    cJSON *note = cJSON_CreateObject();
    cJSON_AddStringToObject(note, "status", "message");
    cJSON_AddStringToObject(note, "message", "private_message");
    cJSON *nd = cJSON_CreateObject();
    cJSON_AddNumberToObject(nd, "sender_id", c->id);
    cJSON_AddStringToObject(nd, "sender_name", c->name);
    cJSON_AddStringToObject(nd, "text", jmsg->valuestring);
    cJSON_AddItemToObject(note, "data", nd);
    char *s = cJSON_PrintUnformatted(note);
    send_packet(to->sockfd, s, strlen(s));
    pthread_mutex_unlock(&g_lock);
    free(s);
    cJSON_Delete(note);
    send_json(c->sockfd, "success", "sent", NULL);
}

/* 处理群聊（广播）：data{message} */
static void handle_gmsg(client_t *c, cJSON *data)
{
    cJSON *jmsg = cJSON_GetObjectItem(data, "message");
    if (!jmsg) { send_json(c->sockfd, "error", "bad_request", NULL); return; }

    cJSON *note = cJSON_CreateObject();
    cJSON_AddStringToObject(note, "status", "message");
    cJSON_AddStringToObject(note, "message", "group_message");
    cJSON *nd = cJSON_CreateObject();
    cJSON_AddNumberToObject(nd, "sender_id", c->id);
    cJSON_AddStringToObject(nd, "sender_name", c->name);
    cJSON_AddStringToObject(nd, "text", jmsg->valuestring);
    cJSON_AddItemToObject(note, "data", nd);
    char *s = cJSON_PrintUnformatted(note);

    pthread_mutex_lock(&g_lock);
    for (client_t *k = g_clients; k; k = k->next)
        if (k->id >= 0)                       /* 广播给所有人（包括自己，方便回显） */
            send_packet(k->sockfd, s, strlen(s));
    pthread_mutex_unlock(&g_lock);
    free(s);
    cJSON_Delete(note);
}

/* 处理在线列表 */
static void handle_list(client_t *c)
{
    pthread_mutex_lock(&g_lock);
    cJSON *arr = build_online_list();
    pthread_mutex_unlock(&g_lock);
    send_json(c->sockfd, "success", "online_users", arr);
}

/* 每个客户端一个线程 */
static void *client_thread(void *arg)
{
    client_t *c = (client_t *)arg;
    char buf[MAX_PKT];

    while (1) {
        int n = recv_packet(c->sockfd, buf, sizeof(buf) - 1);
        if (n <= 0) break;                       /* 对端断开 */
        buf[n] = '\0';

        cJSON *root = cJSON_Parse(buf);
        if (!root) continue;
        cJSON *jcmd = cJSON_GetObjectItem(root, "command");
        cJSON *jdata = cJSON_GetObjectItem(root, "data");
        const char *cmd = jcmd ? jcmd->valuestring : "";

        if      (!strcmp(cmd, "register")) handle_register(c, jdata);
        else if (!strcmp(cmd, "login"))    handle_login(c, jdata);
        else if (!strcmp(cmd, "list"))     handle_list(c);
        else if (!strcmp(cmd, "pm"))       handle_pm(c, jdata);
        else if (!strcmp(cmd, "gmsg"))     handle_gmsg(c, jdata);
        else if (!strcmp(cmd, "quit"))     { cJSON_Delete(root); break; }
        else    send_json(c->sockfd, "error", "unknown_command", NULL);

        cJSON_Delete(root);
    }

    /* 清理：从在线链表摘掉 */
    pthread_mutex_lock(&g_lock);
    client_t **pp = &g_clients;
    while (*pp && *pp != c) pp = &(*pp)->next;
    if (*pp) *pp = c->next;
    pthread_mutex_unlock(&g_lock);
    printf("[QUIT] %s (id=%d) 下线\n", c->name, c->id);
    close(c->sockfd);
    free(c);
    return NULL;
}

int main(void)
{
    /* 预置一个测试账号，方便演示 */
    user_t *u = calloc(1, sizeof(user_t));
    u->id = g_next_id++;
    strcpy(u->name, "admin");
    strcpy(u->password, "1234");
    u->next = g_users;
    g_users = u;

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(lfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(lfd, 16) < 0) { perror("listen"); return 1; }

    printf("聊天服务器已启动, 端口 %d\n预置账号: admin / 1234\n", SERVER_PORT);

    while (1) {
        struct sockaddr_in cli = {0};
        socklen_t clilen = sizeof(cli);
        int fd = accept(lfd, (struct sockaddr *)&cli, &clilen);
        if (fd < 0) { perror("accept"); continue; }
        printf("[CONNECT] %s:%d\n", inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));

        client_t *c = calloc(1, sizeof(client_t));
        c->sockfd = fd;
        c->id = -1;
        pthread_mutex_lock(&g_lock);
        c->next = g_clients;
        g_clients = c;
        pthread_mutex_unlock(&g_lock);

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, c);
        pthread_detach(tid);                    /* 结束后自动回收 */
    }
    return 0;
}
