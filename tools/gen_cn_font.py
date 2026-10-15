# -*- coding: utf-8 -*-
"""生成界面中文字体（Noto Sans SC 子集 -> LVGL 字体 C 文件）

用法（项目根目录下执行）：
    python tools/gen_cn_font.py

依赖：
    - Node.js
    - lv_font_conv（npm 包；脚本会自动在常见路径里找，也可用 NODE / LV_FONT_CONV 环境变量指定）

字体源：
    项目内 lvgl 自带的 NotoSansSC-Regular.ttf（思源黑体系，SIL OFL 协议，可自由分发），
    因此无需额外下载，也不引入任何第三方版权字体。

输出：
    mysrc/src/ui_font_cn_{14,16,20,28,32,36,48}.c

说明：
    只把**字符串字面量里出现的非 ASCII 字符**收进字库（注释里的字不算），
    再加 ASCII 0x20~0x7E，所以体积可控。界面上新出现了字，重跑本脚本即可。
"""
import io, os, re, glob, subprocess, sys, shutil

try:
    sys.stdout.reconfigure(encoding="utf-8")
except Exception:
    pass

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "mysrc", "src")
TTF = os.path.join(ROOT, "lvgl", "tests", "src", "test_files", "fonts", "noto",
                   "NotoSansSC-Regular.ttf")
SIZES = (14, 16, 20, 28, 32, 36, 48)


def find_lv_font_conv():
    """先看环境变量，再到常见的 node_modules 位置找。"""
    env = os.environ.get("LV_FONT_CONV")
    if env and os.path.isfile(env):
        return env
    cands = [
        os.path.join(ROOT, "node_modules", "lv_font_conv", "lv_font_conv.js"),
        os.path.expanduser("~/.workbuddy/binaries/node/workspace/node_modules/lv_font_conv/lv_font_conv.js"),
    ]
    for c in cands:
        if os.path.isfile(c):
            return c
    return None


def collect_symbols():
    """取所有 .c/.h 里**字符串字面量**中的非 ASCII 字符（注释里的不算）。"""
    chars = set()
    for p in sorted(glob.glob(os.path.join(SRC, "*.c")) + glob.glob(os.path.join(SRC, "*.h"))):
        if os.path.basename(p).startswith("ui_font_cn_"):
            continue                      # 字库自身不参与收集
        t = io.open(p, encoding="utf-8").read()
        t = re.sub(r"/\*.*?\*/", "", t, flags=re.S)      # 去块注释
        t = re.sub(r"//[^\n]*", "", t)                   # 去行注释
        for m in re.finditer(r'"((?:[^"\\]|\\.)*)"', t):  # 字符串字面量
            for c in m.group(1):
                if ord(c) >= 0x80:
                    chars.add(c)
    return "".join(sorted(chars))


def main():
    node = os.environ.get("NODE") or shutil.which("node") or "node"
    conv = find_lv_font_conv()
    if not conv:
        print("找不到 lv_font_conv.js，请先 `npm i lv_font_conv`，或用 LV_FONT_CONV 指定路径")
        return 1
    if not os.path.isfile(TTF):
        print("找不到字体源:", TTF)
        return 1

    syms = collect_symbols()
    print("界面用非 ASCII 字符 %d 个：%s" % (len(syms), syms))

    total = 0
    for sz in SIZES:
        out = os.path.join(SRC, "ui_font_cn_%d.c" % sz)
        cmd = [node, conv, "--font", TTF, "--size", str(sz), "--bpp", "4",
               "--format", "lvgl", "--symbols", syms, "--range", "0x20-0x7E",
               "--lv-include", "lvgl/lvgl.h", "-o", out]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            print("  %dpx 失败：%s" % (sz, r.stderr[-200:]))
            continue
        kb = os.path.getsize(out) / 1024.0
        total += kb
        print("  %2dpx -> %7.1f KB" % (sz, kb))
    print("合计 %.1f KB" % total)
    return 0


if __name__ == "__main__":
    sys.exit(main())
