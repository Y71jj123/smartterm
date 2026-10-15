# 开发约定与已知坑

> 嵌入式 Linux 智能终端（RK3568 + LVGL 9 + C）。已知坑与约定集中在此，避免重复踩。
> **详细开发日志 / 调试纪要 / 待验证清单见 `DEVLOG.md`。**

## 当前状态（2026-10-13）
- 五大模块（登录鉴权 / 网络聊天 / PVZ / 2048 / 音乐视频）代码已集成。
- ✅ **已上板实测**（RK3568 到手）。首轮实测暴露两个问题并已修复：MP4 播放后 UI 假死、键盘布局不顺手，
  详见 `docs/worklog/2026-10-13.md`。**改动尚未上板复验**。
- 历史：修复「2048 滑不动 + music 按钮点不动」两个 bug，见 `DEVLOG.md §2`（已提交 `92ca0ba`）。

## 环境
- 唯一正本：`C:\Users\yanji\Desktop\yan\smartterm`（VMware 共享 `yan` → `Desktop\yan`，VM 内 `/mnt/hgfs/yan/smartterm`）。**改代码只改这一份**，否则编不到。
- 交叉编译：`aarch64-linux-gnu-gcc`，CMakeLists 写死 `/usr/local/arm-linux/bin/`。换工具链改 `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER`。
- 构建：`cd build && cmake .. && make`；产物 `../bin/main`。HGFS 下 `make` 报时钟错误无害；改源码不重编先 `touch mysrc/src/*.c`。
  ⚠️ `CMakeLists.txt` 用 `aux_source_directory(./mysrc/src SRCS)` —— **配置期**取列表，
  **新增 .c 文件后必须重跑 `cmake ..`**，否则报 `undefined reference`（如本轮新增 `ui_kb.c`）。
- 板子：1024×600 触摸屏；触摸节点 `/dev/input/event6`（换板必须核对 `/dev/input/event*`）。

## 板端资源（路径写死，勿随意改）
- PVZ 素材：`A:/jing/pvzBMP`（LVGL `A:` 映射板根 `/`）。`game_pvz.c` 的 `RES_CANDIDATES[]` 有候选回退。
- 音乐/视频：`/jing/music`（平铺根下，不递归）；U 盘 `/mnt/usb` 优先。
- 聊天服务器：`127.0.0.1:8888`，预置 `admin/1234`。板端跑 `chat_server_arm`，客户端才能连 `127.0.0.1`。

## 已知坑
1. **framebuffer 抢屏 + "停刷新 = 停输入"（最重要，一对孪生坑 ⭐⭐）**：所有程序共用 `/dev/fb0`。
   - 播放视频时若照常刷 LVGL，界面会重绘盖掉视频 → 必须暂停刷新；
   - **但 `lv_timer_handler()` 同时是"读触摸"的入口**（LVGL 9 里输入读取和画面刷新挂同一个 handler）。
     完全不调它 = 视频照播（mplayer 是 fork 出去的独立进程）而**所有按钮全部失灵，连 BACK 都没用**。
     这是"进了 music 点 MP4 后 UI 假死"的真因，别再往"音频盖住 UI"方向查。
   - 正确做法（`main.c`）：视频期间跳过 `lv_timer_handler()`，改为 `music_video_poll_touch()` 自己读 evdev，
     点屏任意位置 = `music_video_stop()` 停视频回界面。**纯音频必须走 `-vo null -novideo`**，压根不碰 fb0，就不需要这套。
   - 配套：`fb_restore()` 复位 mplayer 改过的 fb var info（被 SIGKILL 时它自己不会恢复），
     否则 LVGL 往 mmap 的旧偏移画 → 画面"冻死"（症状和上面一模一样，极易混淆）。
2. **evdev 事件按 fd 各发一份，不是取走就没 ⭐**：视频期间我们读的是自己 open 的 fd，
   LVGL 那份还原封不动 → 停视频后 LVGL 会把"刚才那一下"当成真点击，在音乐列表上误触下一首。
   → 用官方 `lv_indev_wait_release(indev)` 让 LVGL 忽略输入直到抬起（**不要**试图自己 read 到抬起为止，没用）。
   反向的坑：视频开播后的**第一轮 poll 要丢弃**（`g_touch_armed`）——"点开这首歌"那一按还躺在缓冲里，否则刚开播就秒关。
3. **SIGPIPE 杀 mplayer**：子进程 stdout 接无读端管道，连接关闭触发 SIGPIPE 打死播放器。→ 子进程 stdout **和 stdin** 都重定向 `/dev/null`（stdin 是为了不让 mplayer 抢终端改乱 termios）。
4. **触摸键盘不显示**：LVGL 内置键盘默认隐藏需上滑。→ 建好即常显 + `lv_refr_now` 重绘 + 清 `SCROLLABLE` 标志（`ui_login.c` / `net_chat.c`）。
5. **改键盘布局：ctrl 表短了会越界读内存 ⭐**：`lv_keyboard_update_ctrl_map()` 按 `btnm->btn_cnt` 从 ctrl 表拷固定条数，ctrl 表比 map 的键少 → 读数组外内存，**不报错不崩**，只表现为"键宽是乱数/点下去崩"。→ 本项目统一走 `smart_kb_apply()`（`ui_kb.c`），它会先数按键数做校验，对不上就拒绝装表并打 `[KB] layout table mismatch`。另：**模式切换是按按键文字硬编码匹配的**（`"ABC"`→大写页 / `"abc"`→小写页 / `"1#"`→符号页），两张表里都得放对应文字；每行宽度单位之和要相等，否则键位错位。
6. **触摸节点名写死**：`lv_evdev_create(event6)`，换板核对 `/dev/input/event*`。
7. **出厂 demo 自启占屏**：`/etc/init.d/S50launch_demo` 开机占 fb0。→ `mount -o remount,rw /` + `chmod -x` 禁用。
8. **TCP 粘包**：统一「4 字节长度(网络字节序) + JSON」，`MSG_WAITALL` 按长度收全。
9. **LVGL 对象禁跨线程**：UI 刷新一律 `lv_async_call` 回主线程（聊天收包线程尤其注意）。
10. **2048 无需素材**：纯控件绘制，开箱即用；滑动手势在 `LV_EVENT_PRESSING` 阶段判定方向（不依赖 `RELEASED` 一定送达），并处理 `LV_EVENT_PRESS_LOST` 复位。
11. **资源大小写**：板端 Linux 区分大小写，`bank.bmp` 必须完全一致。
12. **mplayer 依赖**：板端需 `which mplayer`；没装则音乐/视频不可用。
13. **LVGL 滚动标志会吞掉点击/滑动 ⭐**：`lv_obj_create()` 的容器**默认带 `LV_OBJ_FLAG_SCROLLABLE`**；手指一滑 LVGL 就当成"滚动该容器"并抢走 press（`PRESS_LOST`），导致手势回调收不到、`CLICKED` 不触发（按钮全失灵）。→ 不可滚动的容器/棋盘/整屏统一 `lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE)`；触摸容错 `indev->scroll_limit = 25`（默认仅 10，见 `main.c`，本版 LVGL 无公开 setter，需 include `lvgl/src/indev/lv_indev_private.h`）。
14. **`waitpid` 无限阻塞会冻结整个 UI ⭐**：`mplayer_stop()` 里 `waitpid(pid,NULL,0)` 一旦 mplayer 不响应就永久卡死主线程，表现为"所有按钮点不动只能 Ctrl+C"。→ 一律**非阻塞轮询 + 超时 SIGKILL 兜底**，回收子进程绝不能阻塞 UI 线程。
15. **中文字库是子集，新写了汉字必须重生成 ⭐**：`mysrc/src/ui_font_cn_*.c` 只包含"字符串字面量里出现过的字 + ASCII"，
    界面上新用一个没收录的汉字、又没重跑 `tools/gen_cn_font.py`，那个字会显示成**方块**（不报错、不崩溃，很难查）。
    另外默认字体挂在根 screen 上（`main.c`），页面切换用 `lv_obj_clean()` 只清子对象、不会清掉它，所以设一次就够；
    若哪天改成换 screen（`lv_obj_create(NULL)`），记得在新 screen 上重设字体，否则中文全变方块。

## 模块速查
- `ui_login.c` 登录（IP/用户/密码 + 键盘常显）
- `ui_home.c` 主页四卡片（Chat/Game/2048/Music）
- `ui_kb.c` / `ui_kb.h` **触摸键盘统一布局**：PC 式 QWERTY + 常驻数字行，登录页/聊天页共用 `smart_kb_apply()`
- `ui_font.h` / `ui_font_cn_*.c` **界面中文字库**：思源黑体子集（14~48px 共 7 档），只含界面实际用到的字；
  界面文字改动后重跑 `python tools/gen_cn_font.py` 重新生成
- `net_chat.c` 聊天（封包 + 收包线程 + `lv_async_call`）
- `app_music.c` 音乐/视频（扫描 + fork/mplayer/fifo + 看门狗 + 点屏停播 + `fb_restore`）
- `game_pvz.c` 植物大战僵尸（对象池 + 波次）
- `game_2048.c` 2048（滑动手势 + 胜负遮罩）
- `server/chat_server.c` 服务器（pthread + 互斥锁 + 在线链表）
