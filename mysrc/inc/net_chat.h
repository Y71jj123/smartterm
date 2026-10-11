#ifndef NET_CHAT_H
#define NET_CHAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"

/* ---------- 网络底层（登录页和聊天页共用） ---------- */
int  chat_connect(const char *ip, int port);          /* TCP 连接服务器，成功返回套接字 */
void chat_disconnect(void);                           /* 关闭连接并清空会话 */
int  chat_sock(void);                                 /* 当前套接字，-1 = 未连接 */
void chat_set_session(int id, const char *name);      /* 登录成功后记录自己的 id/名字 */
int  chat_my_id(void);
const char *chat_my_name(void);

/* 4 字节长度 + JSON 的封包收发（与服务器约定一致） */
int chat_send_packet(int fd, const char *buf, int len);
int chat_recv_packet(int fd, char *buf, int max);
int chat_send_json(int fd, cJSON *root);              /* 打包并发送一个 cJSON 对象 */

/* ---------- 聊天界面 ---------- */
void chat_app_open(void);                             /* 进入聊天界面（自带连接复用） */
void chat_app_close(void);                            /* 退出聊天界面，回到主界面 */

#ifdef __cplusplus
}
#endif
#endif
