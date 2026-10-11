# 植物大战僵尸经典版（LVGL 9 / ARM 板）— pvzProject

独立的植物大战僵尸工程（从 2048 项目分离而来），玩法参照经典版白天草坪关卡，
`/dev/fb0` 显示 + `/dev/input/event6` 触摸，与 2048 的运行环境完全一致。

## 工程结构

| 路径 | 说明 |
|---|---|
| `main.c` | 入口：初始化 fb0 + 触摸，启动 `game_menu()` |
| `mysrc/src/game_pvz.c` | 游戏全部逻辑（约 1200 行） |
| `mysrc/src/game_menu.c` | 开始菜单：绿色全屏 + 标题 +「开始游戏」按钮 |
| `mysrc/inc/game_pvz.h` | 对外接口 `void game_pvz(void);` |
| `mysrc/inc/game_menu.h` | 对外接口 `void game_menu(void);` |
| `pvzBMP/` | 38 张 32 位带透明通道 BMP 美术资源（含新增 `btn_exit.bmp`） |
| `lvgl/` + `lv_conf.h` | LVGL 9.0 源码与配置（与 2048 工程完全一致，矢量图形保持开启） |
| `CMakeLists.txt` / `Makefile` | 与 2048 相同的交叉编译配置，无需改动 |
| `tools/gen_pvz_res.py` | 美术资源生成脚本（Pillow 程序化绘制，已适配 1024×600） |
| `tools/preview_pvz.py` | 生成 `pvz_preview.png` / `pvz_assets_sheet.png` 预览图 |
| `tools/build_check.py` + `tools/link_check.py` | 用 zig cc 交叉验证编译/链接（Windows 本机即可跑） |
| `pvz_bin_arm64` | aarch64-linux-musl 静态二进制（11.7 MB） |
| `bin/main` | 已同步为最新 `pvz_bin_arm64`，可直接拷到板子运行 |
| `pvz_res_preview.png` | 全部 38 张资源对照图（棋盘格底查看透明） |

`CMakeLists.txt` 用 `aux_source_directory` 收源码，`mysrc/src` 下的 `game_pvz.c` 自动编入。

## 屏幕与界面

- **全屏分辨率 1024×600**：`game_menu.c` 与 `game_pvz.c` 均按 1024×600 布局，
  草坪 9 列 × 5 行，每格 96×92，占满开发板 `fb0` 整个屏幕。
- **开始界面**：绿色渐变全屏 + 木质上沿 +「植物大战僵尸」标题 + 玩法说明 +
  中央「开始游戏」按钮，点击后进入游戏。
- **游戏界面**：顶部工具栏从左到右依次是阳光、5 张植物卡片、铲子、波次、重来、退出。
- **退出按钮**：游戏界面右上角「重来」右侧新增「退出」按钮，点击后停止游戏 tick、
  清理当前屏幕并回到开始界面。

## 玩法

- 5 行 × 9 列草坪，顶部卡片栏：**向日葵(50) / 豌豆射手(100) / 寒冰射手(175) / 坚果墙(50) / 樱桃炸弹(150)**
- 天降阳光 + 向日葵产阳光，点击收集；卡片有冷却（半透明 = 冷却中）
- 3 种僵尸：普通 / 路障（血厚）/ 铁桶（最厚），路障和铁桶被打掉头盔后变普通状态
- 寒冰射手子弹减速僵尸（变蓝、行走变慢）
- 樱桃炸弹秒杀 3×3 范围内全部僵尸
- 每行左侧一辆小推车，僵尸越过最左格时自动启动清行（一次性）
- 铲子可移除植物（不退阳光）
- 10 波僵尸，波次越大出怪越快越密；撑过第 10 波 = 胜利，僵尸进家门 = 失败
- 右上角「重来」「退出」按钮随时重开一局或返回菜单

## 部署到开发板

**推荐目录**：和 2048 的 `2048BMP` 并列，把 `pvzBMP/` 拷到板子的 **`/jing/pvzBMP`**。

```bash
# 在板子上验证文件真的存在（大小写要完全一致）
ls /jing/pvzBMP/bank.bmp
ls /jing/pvzBMP/bg.bmp
```

> 注意：不要把 `pvzBMP` 文件夹再套一层，否则会变成 `/jing/pvzBMP/pvzBMP/bank.bmp`，探测不到。

运行时会按以下顺序自动探测（第一个能打开 `bank.bmp` 的生效，
也可自行在 `game_pvz.c` 的 `RES_CANDIDATES[]` 里加路径）：

`A:/jing/pvzBMP` → `A:/pvzBMP` → `A:/pvz` → `A:/root/pvzBMP` → `A:/mnt/pvzBMP` → `A:/home/pvzBMP` → `A:/sdcard/pvzBMP`

然后运行：

```bash
chmod +x bin/main
./bin/main
```

程序启动时会打印 `[PVZ] resources OK: A:/jing/pvzBMP/bank.bmp`；
如果找不到资源，会打印 `ERROR: bank.bmp NOT FOUND!` 以及所有尝试过的路径。

### 快捷方式（可选）

`bin/main` 与 `pvz_bin_arm64` 都是 Windows 上用 `zig cc` 交叉编译出的 aarch64-linux-musl **静态**可执行文件，
不依赖板子上的 glibc，拷到板上直接运行即可。

## Windows 本机验证编译（不依赖交叉工具链）

```
python tools/build_check.py   # 319 个 .c 全部编译（LVGL + thorvg + 工程 4）
python tools/link_check.py    # 链接出 pvz_bin_arm64
```

## 改素材 / 改数值

- 美术：改 `tools/gen_pvz_res.py` 里的绘制函数后运行，自动重写 `pvzBMP/`
- 对照图：运行 `python tools/contact_sheet.py` 生成 `pvz_res_preview.png`，
  可和板子显示效果对比，确认资源有没有被正确加载
- 数值：植物血量/伤害/阳光产出/僵尸血量/速度/波次表都在 `game_pvz.c` 顶部的
  `PLANT_INFO[]`、`ZOMBIE_INFO[]`、波次配置区，注释清晰，直接改数字即可

## 常见问题

| 现象 | 原因 | 解决 |
|---|---|---|
| 启动后打印 `[PVZ] ERROR: bank.bmp NOT FOUND!` | 板子上没有 `/jing/pvzBMP/bank.bmp` | 把 `pvzBMP/` 整个目录拷到 `/jing/pvzBMP`（和 `2048BMP` 同级） |
| 终端报 `failed to get image info: A:/xxx/yyy.bmp` | 运行中按探测到的路径找不到该 BMP | 同上，确认文件名大小写一致 |
| 图片位置/大小不对 | LVGL 中 `lv_obj_set_pos/set_size` 与父对象没对齐 | 在 `game_pvz.c` 里调对应 `lv_obj_set_pos`/`set_size` |
| 图片显示但颜色/透明不对 | BMP 格式不是 32bit V5 / BI_BITFIELDS | 用 `python tools/gen_pvz_res.py` 重新生成 |
