# Roadmap — SmartTerm

> 嵌入式 Linux 智能终端（RK3568 + LVGL 9 + C）。迭代计划。

## 已完成

| 版本 | 内容 |
| --- | --- |
| v0.1 | 登录鉴权 + 主页门户（LVGL 9 + fbdev/evdev 跑通） |
| v0.2 | 网络聊天室：TCP + cJSON + pthread 多线程，lv_async_call 跨线程刷新 |
| v0.3 | 植物大战僵尸：对象池、30ms tick、5 波递增 + 僵尸王、BMP 资源 |
| v0.4 | 音乐/视频播放：fork + mplayer -slave + FIFO；fb0 释放解决抢屏 |
| v0.5 | 2048：纯 LVGL 绘制 + 触摸滑动手势 + 胜负遮罩 |
| v0.6 | 工程化：关出厂 demo、资源路径板端化、README/DEVELOPMENT/ROADMAP 文档体系 |

## 规划中

- [ ] `docs/screenshots/` 补充各模块真实运行截图 / 录屏，丰富 README 预览
- [ ] 聊天历史消息落盘（本地文件或简单存储）
- [ ] CMake Presets 区分「板端交叉编译」与「x86 本机仿真（SDL）」
- [ ] 主页卡片图标用 LV_SYMBOL 统一，补全 Chat/Game 语义图标
- [ ] 触摸节点名改为运行时探测 `/dev/input/event*`，去硬编码
- [ ] 登录失败 / 网络异常的友好提示与重连

## 已验证 / 待板验
- ✅ 本地语法自检（MinGW gcc -fsyntax-only，LVGL 头文件）
- ⏳ 板端二次复验（清单见 DEVLOG §3.4）：MP4 点屏停播、防误触下一首、键盘 QWERTY 对齐
