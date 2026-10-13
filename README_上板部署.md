# 嵌入式 Linux 智能终端系统 SmartTerm —— 上板部署说明

RK3568 开发板部署全流程。项目唯一位置：`C:\Users\yanji\Desktop\yan\smartterm`
（虚拟机里对应 `/mnt/hgfs/yan/smartterm`）。

---

## 0. 环境链路

```
Windows 写代码 (VS Code: Desktop\yan\smartterm)
      │  VMware 共享文件夹 yan → C:\Users\yanji\Desktop\yan
      ▼
Ubuntu 虚拟机交叉编译 (/mnt/hgfs/yan/smartterm, aarch64-linux-gnu-gcc)
      │  产物 bin/main
      ▼
RK3568 开发板 (/jing/)
```

> ⚠️ VMware 共享名 `yan` 指向的是 `Desktop\yan`，所以虚拟机里 `/mnt/hgfs/yan/smartterm`
> = Windows 的 `Desktop\yan\smartterm`。改代码一定要改**这一份**，否则编不到。

---

## 1. 编译（Ubuntu 虚拟机里）

```sh
cd /mnt/hgfs/yan/smartterm/build
make
```

- 产物：`bin/main`（≈2.3MB，动态链接的 aarch64 程序）
  - 虚拟机路径 `/mnt/hgfs/yan/smartterm/bin/main`
  - Windows 路径 `Desktop\yan\smartterm\bin\main`
- **改了源码 make 却说"没有可更新的"** → 先强制再编：
  ```sh
  touch /mnt/hgfs/yan/smartterm/mysrc/src/*.c
  make
  ```
- 出现 `检测到时钟错误。您的构建版本可能是不完整的` 是虚拟机 + HGFS 共享目录的
  时间戳偏差（零点几秒），**无害，可忽略**。

---

## 2. 把 main 传到开发板

三种方式任选（都能用）：

| 方式 | 操作 |
|---|---|
| 串口 rz/sz | 板子上 `cd /jing` 执行 `rz`，用 SecureCRT/Xshell 发送 `bin/main` |
| adb | 板子开着 adb（`/dev/usb-ffs/adb`）：`adb push bin/main /jing/main` |
| U 盘 | U 盘插**开发板自己的 USB Host 口**（插电脑/被虚拟机捕获则板子看不到）<br>`mount /dev/sda1 /mnt/usb && cp /mnt/usb/main /jing/main && umount /mnt/usb` |

传完确认大小对：
```sh
ls -l /jing/main      # 应约 2321240 字节
```

---

## 3. 开发板上的资源布局

```
/jing/
├── main          可执行文件（本项目）
├── pvzBMP/       PVZ 游戏素材（缺了游戏会报 bank.bmp NOT FOUND）
├── chat_server_arm  聊天服务器（aarch64 版，可选，板端本地起也行）
└── music/        音乐/视频文件（.mp4/.mp3/.wav/.flac）
```

- **2048 小游戏无需任何图片素材**：数字方块由 LVGL 控件 + 文字实时绘制，
  不必像 PVZ 那样准备 BMP，开箱即用。
- LVGL 的 `A:` 盘符映射板子根 `/`（`lv_conf.h`：`LV_FS_POSIX_LETTER 'A'` +
  `LV_FS_POSIX_PATH ""`），所以 `A:/jing/pvzBMP` = 板子上的 `/jing/pvzBMP`。
- **音乐必须平铺在 `/jing/music/` 根下**，放子文件夹里扫不到（代码只扫一层不递归）。
- 音乐目录优先级：`/mnt/usb`（U 盘）> `/jing/music`，插着 U 盘且里面有歌时会优先读 U 盘。

---

## 4. 关掉出厂 demo 自启（重要）

板子开机自动执行 `/etc/init.d/S50launch_demo`（厂商出厂测试程序，RTC/LED/Buzzer
测试界面）。它一直占用 framebuffer，会导致自己程序画面被覆盖。

```sh
mount -o remount,rw /                # 若提示只读，先让根分区可写
chmod -x /etc/init.d/S50launch_demo  # 去掉可执行位，rcS 启动时跳过它
sync
reboot
```

- 原理：BusyBox 的 `/etc/init.d/rcS` 只执行**有可执行权限**的 `S??` 脚本。
- 想恢复出厂 demo：`chmod +x /etc/init.d/S50launch_demo` 再重启。

---

## 5. 运行

```sh
cd /jing
./main
```

- 屏幕有残留画面时清屏（把 framebuffer 刷黑）：
  ```sh
  dd if=/dev/zero of=/dev/fb0 bs=1M count=3
  ```
  （1024×600×4 字节 ≈ 2.4MB，count=3 足够刷满全屏）
- 只有 `main` 一个程序在跑时，屏幕才会被它独占。

---

## 6. 聊天功能（需要先开服务器）

聊天是 TCP + JSON 的客户端/服务端结构，板子上的 main 要连服务器才能登录/聊天。

在任一有网络的机器（虚拟机即可）上启动服务器：
```sh
cd /mnt/hgfs/yan/smartterm/server
make                 # cc chat_server.c cJSON.c -o chat_server -lpthread
./chat_server        # 监听 8888 端口
```
登录界面的「Server」栏填服务器 IP（默认 `192.168.1.100`，按实际网段改）。
预置账号：`admin / 1234`。

---

## 7. 常见问题

| 现象 | 原因 / 解决 |
|---|---|
| 登录页没有键盘 | main 不是重新编译的新版；确认 `bin/main` 是 make 之后的新产物 |
| 屏幕被旧画面覆盖 | 出厂 demo 还在跑 → 按第 4 节禁用自启；`killall` 掉占用 framebuffer 的进程 |
| PVZ 报 `bank.bmp NOT FOUND` | `/jing/pvzBMP` 不存在或没传全，看终端打印的候选路径 |
| 音乐界面没有歌 | mp4 是否平铺在 `/jing/music/` 根下；是否插着 U 盘（U 盘优先） |
| 2048 不动 | 在棋盘区域内用手指滑动（上/下/左/右），点一下不算；凑到 2048 即胜利 |
| make 报时钟错误 | 虚拟机 + HGFS 时间戳偏差，无害 |
| `ps -ef` 报 unsupported SysV option | 板子是 BusyBox，用 `ps` 或 `ps aux` |
