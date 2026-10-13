# 开发约定与已知坑

> 嵌入式 Linux 智能终端（RK3568 + LVGL 9 + C）。已知坑与约定集中在此，避免重复踩。
> **详细开发日志 / 调试纪要 / 待验证清单在 `DEVLOG.md`——见 DEVLOG.md。**

@DEVLOG.md

## 当前状态（2026-10-13）
- 五大模块（登录鉴权 / 网络聊天 / PVZ / 2048 / 音乐视频）代码已集成，语法自检通过；GitHub 已推送。
- ⏳ **从未上板实测**（没有可用板子）。板子到手第一件事：`cd build && make` → 传 `bin/main` → 按 `DEVLOG.md §2.5` 清单逐项验证。
- 最近一轮改动：修复「2048 滑不动 + music 按钮点不动」两个 bug，详见 `DEVLOG.md §2`。

## 环境
- 唯一正本：`C:\Users\yanji\Desktop\yan\项目`（VMware 共享 `yan` → `Desktop\yan`，VM 内 `/mnt/hgfs/yan/项目`）。**改代码只改这一份**，否则编不到。
- 交叉编译：`aarch64-linux-gnu-gcc`，CMakeLists 写死 `/usr/local/arm-linux/bin/`。换工具链改 `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER`。
- 构建：`cd build && make`；产物 `../bin/main`。HGFS 下 `make` 报时钟错误无害；改源码不重编先 `touch mysrc/src/*.c`。
- 板子：1024×600 触摸屏；触摸节点 `/dev/input/event6`（换板必须核对 `/dev/input/event*`）。

## 板端资源（路径写死，勿随意改）
- PVZ 素材：`A:/jing/pvzBMP`（LVGL `A:` 映射板根 `/`）。`game_pvz.c` 的 `RES_CANDIDATES[]` 有候选回退。
- 音乐/视频：`/jing/music`（平铺根下，不递归）；U 盘 `/mnt/usb` 优先。
- 聊天服务器：`127.0.0.1:8888`，预置 `admin/1234`。板端跑 `chat_server_arm`，客户端才能连 `127.0.0.1`。

## 已知坑
1. **framebuffer 抢屏（最重要）**：所有程序共用 `/dev/fb0`。播放视频时主循环必须暂停 LVGL 刷新（`music_video_active()` 守卫），否则界面重绘盖掉视频；结束恢复。`main.c` 主循环：`if(!music_video_active()) lv_timer_handler();`
2. **SIGPIPE 杀 mplayer**：子进程 stdout 接无读端管道，连接关闭触发 SIGPIPE 打死播放器。→ 子进程 stdout 重定向 `/dev/null`。
3. **触摸键盘不显示**：LVGL 内置键盘默认隐藏需上滑。→ 建好即常显 + `lv_refr_now` 重绘 + 清 `SCROLLABLE` 标志（`ui_login.c` / `net_chat.c`）。
4. **触摸节点名写死**：`lv_evdev_create(event6)`，换板核对 `/dev/input/event*`。
5. **出厂 demo 自启占屏**：`/etc/init.d/S50launch_demo` 开机占 fb0。→ `mount -o remount,rw /` + `chmod -x` 禁用。
6. **TCP 粘包**：统一「4 字节长度(网络字节序) + JSON」，`MSG_WAITALL` 按长度收全。
7. **LVGL 对象禁跨线程**：UI 刷新一律 `lv_async_call` 回主线程（聊天收包线程尤其注意）。
8. **2048 无需素材**：纯控件绘制，开箱即用；滑动手势在 `LV_EVENT_PRESSING` 阶段判定方向（不依赖 `RELEASED` 一定送达），并处理 `LV_EVENT_PRESS_LOST` 复位。
9. **资源大小写**：板端 Linux 区分大小写，`bank.bmp` 必须完全一致。
10. **mplayer 依赖**：板端需 `which mplayer`；没装则音乐/视频不可用。
11. **LVGL 滚动标志会吞掉点击/滑动 ⭐**：`lv_obj_create()` 的容器**默认带 `LV_OBJ_FLAG_SCROLLABLE`**；手指一滑 LVGL 就当成"滚动该容器"并抢走 press（`PRESS_LOST`），导致手势回调收不到、`CLICKED` 不触发（按钮全失灵）。→ 不可滚动的容器/棋盘/整屏统一 `lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE)`；触摸容错 `indev->scroll_limit = 25`（默认仅 10，见 `main.c`，本版 LVGL 无公开 setter，需 include `lvgl/src/indev/lv_indev_private.h`）。
12. **`waitpid` 无限阻塞会冻结整个 UI ⭐**：`mplayer_stop()` 里 `waitpid(pid,NULL,0)` 一旦 mplayer 不响应就永久卡死主线程，表现为"所有按钮点不动只能 Ctrl+C"。→ 一律**非阻塞轮询 + 超时 SIGKILL 兜底**，回收子进程绝不能阻塞 UI 线程。

## 模块速查
- `ui_login.c` 登录（IP/用户/密码 + 键盘常显）
- `ui_home.c` 主页四卡片（Chat/Game/2048/Music）
- `net_chat.c` 聊天（封包 + 收包线程 + `lv_async_call`）
- `app_music.c` 音乐/视频（扫描 + fork/mplayer/fifo + 看门狗）
- `game_pvz.c` 植物大战僵尸（对象池 + 波次）
- `game_2048.c` 2048（滑动手势 + 胜负遮罩）
- `server/chat_server.c` 服务器（pthread + 互斥锁 + 在线链表）
