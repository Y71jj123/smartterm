# DEVLOG — SmartTerm 开发日志 & 

> 这份文件给 **VSCode 里的  / 其他 **当上下文用。
> 打开本项目的  会自动读根目录 `DEVELOPMENT.md`；需要更细的历史就来读这份。
>
> 最后更新：2026-10-13

---

## 0. 项目一句话

嵌入式 Linux 智能终端系统 **SmartTerm**：RK3568（ARM64）+ LVGL 9 + C 语言，单块 1024×600 触摸屏上跑
**登录鉴权 / 网络聊天 / 植物大战僵尸 / 2048 / 音乐视频** 五大模块，aarch64 交叉编译后部署到板子。

- 唯一正本：`C:\Users\yanji\Desktop\yan\项目`（VMware 共享 → VM 内 `/mnt/hgfs/yan/项目`）
- GitHub：`https://github.com/Y71jj123/smartterm`（main 分支）
- 构建：`cd build && make` → 产物 `../bin/main` → 传板 `/jing/main`
- 服务器：`server/chat_server.c`（板端用 `chat_server_arm`），监听 `8888`，预置账号 `admin` / `1234`

---

## 1. 当前状态（2026-10-13）

| 项 | 状态 |
| --- | --- |
| 五大模块代码 | ✅ 已全部集成、语法自检通过 |
| GitHub 仓库 | ✅ 已推送（含 README / LICENSE / DEVELOPMENT.md / ROADMAP.md / docs） |
| Obsidian 笔记 | ✅ `smartterm/` 9 篇（概览 / 技术栈 / 踩坑 / 踩坑记录） |
| **板端实测** | ⏳ **未完成 —— 见下方"待验证清单"，这是当前最优先的事** |
| 2048 滑动 / music 按钮 bug | ✅ 已定位并修复（本轮，见 §2），**待上板复验** |

> ⚠️ 目前所有改动都是"代码层改完 + 语法自检过"，**没有一块板子实测过**。
> 板子到手后第一件事就是照着 §3 的清单跑一遍。

---

## 2. 本轮调试纪要（2026-10-12 ~ 10-13）

### 2.1 用户报告的两个 bug
1. **2048**：进游戏后滑动不能移动数字，变成整个游戏/画面在平移。
2. **music**：页面上所有按钮控件点击无效，连 **BACK 都按不了**，只能 Ctrl+C 结束进程。

### 2.2 根因（已确认）

**bug 1 — LVGL 把"滑动"当成"滚动棋盘"**
- `board_obj = lv_obj_create(...)` 创建的容器在 LVGL 里**默认带 `LV_OBJ_FLAG_SCROLLABLE`**；
- 加上棋盘格子坐标多算了一重 `pad` 偏移，内容比容器内容区大了 8px → 容器真的可滚动；
- 于是手指一滑，LVGL 判定为"滚动容器"，**抢走原对象的 press（发 `LV_EVENT_PRESS_LOST`）**，
  自己的 `PRESSED/RELEASED` 手势回调收不到完整事件 → 方块不动、只有画面整体平移。

**bug 2 — 点击判定 + 主线程阻塞（两个因素叠加）**
- ① music / home / 2048 页面入口**没有清 screen 的滚动标志**（只有登录页清了），
  触摸抖动超过 `scroll_limit`（默认仅 10px）就被误判成滚动 → `CLICKED` 事件永不触发 → 按钮全失灵；
- ② `mplayer_stop()` 里 **`waitpid(pid, NULL, 0)` 是无限阻塞**：mplayer 若不响应 fifo 的 `quit`
  （打不开 fbdev2 / 卡在启动），整个 LVGL 主线程冻结 → 表现为"所有按钮都点不动，只能 Ctrl+C"。

### 2.3 修复清单（改了 4 个文件）

| 文件 | 改动 |
| --- | --- |
| `main.c` | `#include "lvgl/src/indev/lv_indev_private.h"`；`indev->scroll_limit = 25;`（默认 10 太小） |
| `mysrc/src/game_2048.c` | 入口清 screen `SCROLLABLE`；棋盘容器/格子清 `SCROLLABLE`（格子另清 `CLICKABLE`）；格子坐标去掉 pad 偏移；手势改到 **`LV_EVENT_PRESSING` 阶段判定** + 处理 `LV_EVENT_PRESS_LOST`；遮罩清 `SCROLLABLE` |
| `mysrc/src/app_music.c` | 入口 / `top` / `ctrl_bar` 清 `SCROLLABLE`；`mplayer_stop()` 改 **300ms 轮询 + SIGKILL 兜底**（SIGKILL 后也有界回收，绝不再无限阻塞）；视频**自然播完**时补关 fifo + 复位当前曲目；新增**视频看门狗** `VIDEO_MAX_MS=10min`（防 mplayer 卡死冻屏）；关键回调加 `[MUSIC] ...` 日志 |
| `mysrc/src/ui_home.c` | 入口清 screen `SCROLLABLE`；卡片清 `SCROLLABLE` |

### 2.4 独立复核结论（只读 review）
- **2048 修复判定为正确**；
- **music 原修复只覆盖一半**，据此补充了 3 点：① `mplayer_stop` 里 SIGKILL 后仍阻塞 `waitpid` → 改有界轮询；
  ② 视频自然播完未收尾 fifo / 未复位当前曲目 → 已补；③ 新增视频看门狗；
- 次要建议（已采纳）：`scroll_limit` 由 40 调为 25（过大让列表滚动迟钝）；遮罩/卡片统一清滚动标志。

### 2.5 待验证清单（板子到手后照这个点）
- [ ] 2048：上下左右滑动 → 数字能合并移动（不再整屏平移）
- [ ] 2048：BACK 键能返回主页；胜利/失败遮罩的 RESTART 能重开
- [ ] music：点歌曲能播；`play/next/prev/stop/BACK` 都有反应
- [ ] music：BACK 能立即回主页（不再卡死）
- [ ] 日志对照：终端应打印 `[2048] page ready`、`[MUSIC] ctrl clicked: xxx`、`[MUSIC] BACK clicked`
  - 点了有日志但不动作 → 逻辑问题；完全没日志 → 触摸事件仍未进来
- [ ] 视频（mp4）：有画面；播完能自动回界面
- [ ] 聊天：能登录、收发消息

---

## 3. 常用命令

```bash
# —— VM 里编译（代码改在 Windows 正本，VM 共享目录直接可见）——
cd ~/Desktop/yan/项目/build && make
ls -l ../bin/main
# HGFS 时报时钟警告无害；改了源码没重编可先 touch mysrc/src/*.c

# —— 服务器（板端跑，让客户端连 127.0.0.1）——
# 把 server/chat_server_arm 传到板子
cd /jing && ./chat_server_arm &     # 后台起，终端留出来跑 main
./main                              # GUI 客户端；Server 填 127.0.0.1 / admin / 1234

# —— 服务器（本机 Windows 侧联调，走 WSL）——
wsl.exe bash -lc "cd /mnt/c/Users/yanji/Desktop/yan/项目/server && gcc chat_server.c cJSON.c -o chat_server -lpthread && nohup ./chat_server > /tmp/chat_server.log 2>&1 & disown"
wsl.exe bash -lc "ss -ltnp | grep 8888"      # 验证监听
wsl.exe bash -lc "pkill -f chat_server"      # 停止
```

---

## 4. 环境备忘

- Windows 用户名 `yanji`；工程正本 `C:\Users\yanji\Desktop\yan\项目`。
- 交叉工具链：`aarch64-linux-gnu-gcc`（CMakeLists 里写死 `/usr/local/arm-linux/bin/`）。
- 板子：RK3568，1024×600 触摸屏；触摸节点 `/dev/input/event6`（换板要核对）。
- 出厂 demo 抢屏：`/etc/init.d/S50launch_demo`，已用 `chmod -x` 禁用（`mount -o remount,rw /` 后可改）。
- 本机侧还有个 Qt 项目 `qt-industrial-equipment-monitor`，与 SmartTerm 是两条独立线。

---

## 5. 已知坑索引

完整版见 `DEVELOPMENT.md` 与 Obsidian `smartterm/05-踩坑记录/已知坑与防坑手册.md`。高频 TOP：
1. framebuffer 抢屏（LVGL vs mplayer 共用 `/dev/fb0`）
2. SIGPIPE 杀 mplayer（stdout 重定向 `/dev/null`）
3. 触摸键盘常显（建好即显示 + `lv_refr_now`）
4. **LVGL 滚动标志吞事件**（滑动/点击被 `SCROLLABLE` 吃掉，见 §2.2）
5. **`waitpid` 无限阻塞冻结 UI**（子进程回收必须非阻塞 + 超时兜底）
6. LVGL 对象禁跨线程（一律 `lv_async_call` 回主线程）

---

## 6. 逐日工作日志（原始记录）
- `docs/worklog/2026-10-11.md` —— 集成 2048、项目定名、GitHub 化、脱敏清洗
- `docs/worklog/2026-10-12.md` —— 聊天服务器启动、2048/music 两个 bug 定位与修复

