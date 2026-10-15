# DEVLOG — SmartTerm 开发日志 & 

> 这份文件给 **VSCode 里的  / 其他 **当上下文用。
> 打开本项目的  会自动读根目录 `DEVELOPMENT.md`；需要更细的历史就来读这份。
>
> 最后更新：2026-10-15

---

## 0. 项目一句话

嵌入式 Linux 智能终端系统 **SmartTerm**：RK3568（ARM64）+ LVGL 9 + C 语言，单块 1024×600 触摸屏上跑
**登录鉴权 / 网络聊天 / 植物大战僵尸 / 2048 / 音乐视频** 五大模块，aarch64 交叉编译后部署到板子。

- 唯一正本：`C:\Users\yanji\Desktop\yan\smartterm`（VMware 共享 → VM 内 `/mnt/hgfs/yan/smartterm`）
- GitHub：`https://github.com/Y71jj123/smartterm`（main 分支）
- 构建：`cd build && make` → 产物 `../bin/main` → 传板 `/jing/main`
- 服务器：`server/chat_server.c`（板端用 `chat_server_arm`），监听 `8888`，预置账号 `admin` / `1234`

---

## 1. 当前状态（2026-10-15）

| 项 | 状态 |
| --- | --- |
| 五大模块代码 | ✅ 已全部集成、语法自检通过 |
| GitHub 仓库 | ⏳ **本地领先远程 2 个提交待推送**（`5c0ae0d` 2048/music 修复、`41a342f` 上板首测修复+改名）；远程已有初始版 README/文档 |
| Obsidian 笔记 | ✅ `smartterm/` 9 篇（概览 / 技术栈 / 踩坑 / 踩坑记录） |
| **板端实测** | ✅ **首轮已完成**（RK3568 到手）；§3 两处修复已实现并提交 `41a342f`，**待二次上板复验** |
| 2048 滑动 / music 按钮 bug | ✅ 已修复并提交 `5c0ae0d`（见 §2） |

> 📌 下一步优先级：二次上板复验 §3.4 清单 → 复验通过后 `git push` 同步 GitHub（当前本地领先远程 2 个提交）。

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

## 3. 板上首轮实测的两个问题（2026-10-13）

> 完整纪要见 `docs/worklog/2026-10-13.md`，这里只留结论。

### 3.1 【已修】进了 Music 点一个 MP4，之后所有按钮全失灵

**现象**：点了 MP4 后视频照播，但 BACK / next / prev / stop 全部没反应，只能 `Ctrl+C`。

**根因**：`main.c` 之前为了"别让 LVGL 盖掉视频"，在视频期间**完全不调 `lv_timer_handler()`**。
但 LVGL 9 里**读触摸（`lv_indev_read_timer_cb`）和刷画面（`lv_refr_timer`）都挂在这一个 handler 上**，
不调它 = 输入也一起停了。mplayer 是 `fork` 出的独立进程，所以视频照播、UI 彻底死掉。
（原来的看门狗是 10 分钟，最坏情况 UI 僵死十分钟。）

**修法**：
- 视频期间跳过 `lv_timer_handler()`，改由 `music_video_poll_touch()` **自己读 evdev**；
  点屏幕任意位置 = `music_video_stop()` 停视频回界面。开播时先在页面上盖 1.2s 提示告知这条操作。
- **纯音频改走 `-vo null -novideo`**，压根不碰 `/dev/fb0`，就永远不需要暂停 LVGL
  → 音乐页按钮全程可用（原来音频也被 `-vo fbdev2` 拖下水）。
- 新增 `fb_restore()`：复位 mplayer 改过的 fb var info + 解除 blank。
  mplayer 正常退出会自己恢复，**被 SIGKILL 打死则不会**，此时 LVGL 仍往旧的 mmap 偏移画 → 画面"冻死"。
  必须在 `lv_refr_now()` 之前调。
- 看门狗 10 分钟 → **2 小时**（点屏已能随时退出，它只剩兜底作用）。

**过程中自审出的隐蔽 bug（两个反向坑，值得记）**：
1. evdev 事件是**按打开的 fd 各发一份**的。我们读掉自己那份，**LVGL 那份还原封不动** →
   停视频后 LVGL 把"刚才那一下"当成真点击，屏幕上正好是音乐列表 → **刚关视频就自动播下一首**。
   修法：`main.c` 在停视频**之前**调官方 API `lv_indev_wait_release(indev)`，
   让 LVGL 忽略输入直到收到抬起（手指若早抬起，下次 release 顺手清标志，无副作用）。
   > **不要**试图"自己 read 到抬起为止"——读的是自己那份 fd，等于没做。
2. 反过来：视频**开播后的第一轮 poll 必须丢弃**（`g_touch_armed` 标志）。
   因为"点开这首歌"那一按（按下+抬起）一直躺在我们的 fd 缓冲里
   （视频之外的时间我们不读它），直接判定会把刚开播的视频**秒关**。

### 3.2 【已修】键盘改成常见输入法（PC 式 QWERTY + 常驻数字行）

用户原话：「键盘也帮我改成常见输入法，不然一直切换太难受了。」
（原状：LVGL 自带键盘默认小写字母页，**打数字必须切到 `1#` 页再切回来**。）

新增 `mysrc/{inc,src}/ui_kb.{h,c}`，**不碰 `lvgl/` 任何源码**，只在键盘表上做替换：

```
1 2 3 4 5 6 7 8 9 0 ⌫          行1: 10×1 + 2
q w e r t y u i o p ⏎          行2: 10×1 + 2
a s d f g h j k l . - :        行3: 12×1
ABC z x c v b n m _ , '        行4: 2 + 7×1 + 3×1
1# ◀ space ▶ OK                行5: 2 + 2 + 4 + 2 + 2
```

摸清的 LVGL 内部约定（写进了 `ui_kb.c` 头注释）：
- 键盘按模式存表 `kb_map[LV_KEYBOARD_MODE_*]`，覆盖 `TEXT_LOWER`/`TEXT_UPPER` 两槽即可；
- **模式切换按按键文字硬编码匹配**（`"ABC"`→大写 / `"abc"`→小写 / `"1#"`→符号），
  两张表里都得放对应文字；
- **ctrl 表短于按键数 = 越界读内存**（`lv_keyboard_update_ctrl_map` 按 `btn_cnt` 拷固定条数），
  不报错不崩、只表现为键宽乱/点下去崩 → `smart_kb_apply()` 里先数按键数校验，对不上就拒绝装表并打日志。

### 3.3 本轮改动文件

| 文件 | 说明 |
| --- | --- |
| `main.c` | 视频期间自己读触摸；`lv_indev_wait_release()` 防误触 |
| `mysrc/inc/app_music.h` | 导出 `music_video_poll_touch()` / `music_video_stop()` |
| `mysrc/src/app_music.c` | 音频/视频分流、`fb_restore()`、点屏停播、`g_touch_armed`、stdin 接 `/dev/null` |
| `mysrc/inc/ui_kb.h` `mysrc/src/ui_kb.c` | **新增**：QWERTY + 数字行布局表 + 表长校验 |
| `mysrc/src/ui_login.c` | 版面压缩 + 换键盘 |
| `mysrc/src/net_chat.c` | 版面压缩 + 换键盘 + 补清 screen `SCROLLABLE`（原来漏了） |

### 3.4 待验证清单（下次上板点这些）

- [ ] mp3：能播，`play/next/prev/stop/BACK` 全有反应（全程不碰 fb0，页面保持可点）
- [ ] mp4：先出 1.2s 提示 → 全屏播放 → **点屏幕任意位置能停**（关键修复）
- [ ] 停视频后界面立刻正常（不黑屏/不花屏/不冻住）→ `fb_restore()` 生效
- [ ] 停视频后**不会**自动播下一首 → 防误触生效
- [ ] 视频自然播完自动回界面
- [ ] 键盘第一排就是 `1234567890`，字母区是 QWERTY，不必切页；`ABC`/`abc`/`1#` 切换正常；各键宽度对齐无错位
- [ ] 日志：有 `[MUSIC] tap on screen -> stop video`；**没有** `[KB] layout table mismatch`

> 分诊：完全无 `[MUSIC]` 日志 → 触摸事件没进来，核对 `/dev/input/event*`（`cat /proc/bus/input/devices`）；
> 有日志但画面不动 → 看 `fb_restore()` 的 perror 输出。

---

## 4. 常用命令

```bash
# —— VM 里编译（代码改在 Windows 正本，VM 共享目录直接可见）——
cd ~/Desktop/yan/smartterm/build
# ⚠️ 本轮新增了 mysrc/src/ui_kb.c。CMakeLists 用的是 aux_source_directory()，
#    它是**配置期**取的文件列表，已有的 build/ 不会自己发现新文件 ——
#    不重新 configure 就会以 "undefined reference to `smart_kb_apply'" 链接失败。
cmake .. && make
ls -l ../bin/main
# HGFS 时报时钟警告无害；改了源码没重编可先 touch mysrc/src/*.c

# —— 服务器（板端跑，让客户端连 127.0.0.1）——
# 把 server/chat_server_arm 传到板子
cd /jing && ./chat_server_arm &     # 后台起，终端留出来跑 main
./main                              # GUI 客户端；Server 填 127.0.0.1 / admin / 1234

# —— 服务器（本机 Windows 侧联调，走 WSL）——
wsl.exe bash -lc "cd /mnt/c/Users/yanji/Desktop/yan/smartterm/server && gcc chat_server.c cJSON.c -o chat_server -lpthread && nohup ./chat_server > /tmp/chat_server.log 2>&1 & disown"
wsl.exe bash -lc "ss -ltnp | grep 8888"      # 验证监听
wsl.exe bash -lc "pkill -f chat_server"      # 停止
```

---

## 5. 环境备忘

- Windows 用户名 `yanji`；工程正本 `C:\Users\yanji\Desktop\yan\smartterm`。
- 交叉工具链：`aarch64-linux-gnu-gcc`（CMakeLists 里写死 `/usr/local/arm-linux/bin/`）。
- 板子：RK3568，1024×600 触摸屏；触摸节点 `/dev/input/event6`（换板要核对）。
- 出厂 demo 抢屏：`/etc/init.d/S50launch_demo`，已用 `chmod -x` 禁用（`mount -o remount,rw /` 后可改）。
- 本机侧还有个 Qt 项目 `qt-industrial-equipment-monitor`，与 SmartTerm 是两条独立线。

---

## 6. 已知坑索引

完整版见 `DEVELOPMENT.md` 与 Obsidian `smartterm/05-踩坑记录/已知坑与防坑手册.md`。高频 TOP：
1. **framebuffer 抢屏 + 「停刷新 = 停输入」**（`lv_timer_handler()` 同时是画图和读触摸的入口，不调它按钮全失灵，见 §3.1）
2. **evdev 事件按 fd 各发一份**（自己读一遍 ≠ 清掉 LVGL 那份 → 用 `lv_indev_wait_release()`，见 §3.1）
3. SIGPIPE 杀 mplayer（stdout / stdin 都重定向 `/dev/null`）
4. 触摸键盘常显（建好即显示 + `lv_refr_now`）
5. **改键盘表 ctrl 数组短了会越界**（`smart_kb_apply()` 已加校验，见 §3.2）
6. **LVGL 滚动标志吞事件**（滑动/点击被 `SCROLLABLE` 吃掉，见 §2.2）
7. **`waitpid` 无限阻塞冻结 UI**（子进程回收必须非阻塞 + 超时兜底）
8. LVGL 对象禁跨线程（一律 `lv_async_call` 回主线程）

---

## 7. 逐日工作日志（原始记录）
- `docs/worklog/2026-10-11.md` —— 集成 2048、项目定名、GitHub 化、脱敏清洗
- `docs/worklog/2026-10-12.md` —— 聊天服务器启动、2048/music 两个 bug 定位与修复
- `docs/worklog/2026-10-13.md` —— **上板首测**：MP4 播完 UI 假死（`lv_timer_handler` 停输入）+ 键盘改 QWERTY 常驻数字行
- `docs/worklog/2026-10-15.md` —— 项目状态核对（工作区干净、本地领先远程 2 提交）、补全开发日志、准备推送 GitHub

