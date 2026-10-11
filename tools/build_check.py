# -*- coding: utf-8 -*-
"""用 zig cc 批量编译 LVGL 源 + 本工程源，验证整个项目能编译"""
import os, sys, subprocess

ROOT = r"C:/Users/yanji/Desktop/yan/3-第三阶段/02-网编/smart_center"
OUT  = r"C:/Users/yanji/AppData/Local/Temp/sc_build"
LVGL_DIR = os.path.join(OUT, "lvgl")
ZIG = r"C:/Users/yanji/.workbuddy/binaries/python/versions/3.13.12/Lib/site-packages/ziglang/zig.exe"

os.makedirs(LVGL_DIR, exist_ok=True)

CFLAGS = [
    "cc",
    "-DLV_CONF_INCLUDE_SIMPLE",
    "-target", "aarch64-linux-musl",
    "-O0", "-std=gnu99",
    "-I" + ROOT, "-Ilvgl", "-Imysrc/inc",
    "-fno-sanitize=undefined",
    "-Wall",
    "-Wno-unused-function", "-Wno-unused-variable",
    "-Wno-unused-but-set-variable",
    "-Wno-missing-field-initializers",
    "-Wno-format-truncation", "-Wno-format-nonliteral",
    "-Wno-sign-compare",
]

# thorvg（矢量图形库，LV_USE_VECTOR_GRAPHIC=1 时必须编译并链接）
CXXFLAGS = [
    "c++",
    "-DLV_CONF_INCLUDE_SIMPLE",
    "-target", "aarch64-linux-musl",
    "-O0", "-std=c++17", "-fno-exceptions", "-fno-rtti",
    "-I" + ROOT, "-Ilvgl", "-Ilvgl/src/libs/thorvg",
    "-fno-sanitize=undefined",
    "-Wno-everything",
]

# 收集 LVGL 源文件（lvgl/src 下所有 .c；排除演示、test、cursor；thorvg 在第二遍用 c++ 编译）
srcs_path = os.path.join(OUT, "srcs.txt")
if not os.path.exists(srcs_path):
    srcs, cpp_srcs = [], []
    for r, _, fs in os.walk(os.path.join(ROOT, "lvgl/src")):
        rel = os.path.relpath(r, ROOT).replace("\\", "/")
        for f in fs:
            full = os.path.join(r, f)
            p = os.path.relpath(full, ROOT).replace("\\", "/")
            if "demo" in p or "example" in p or "test" in p or "mouse_cursor" in p:
                continue
            if f.endswith(".cpp"):
                cpp_srcs.append(p)
            elif f.endswith(".c"):
                srcs.append(p)
    srcs.sort()
    cpp_srcs.sort()
    with open(srcs_path, "w") as f:
        f.write("\n".join(srcs))
    with open(os.path.join(OUT, "cpp_srcs.txt"), "w") as f:
        f.write("\n".join(cpp_srcs))
else:
    with open(srcs_path) as f:
        srcs = [l.strip().replace("\\","/") for l in f if l.strip()]
    cpp_srcs = []
    cpp_path = os.path.join(OUT, "cpp_srcs.txt")
    if os.path.exists(cpp_path):
        with open(cpp_path) as f:
            cpp_srcs = [l.strip().replace("\\","/") for l in f if l.strip()]

# 我们的工程文件
ours = ["main.c", "mouse_cursor_icon.c",
        "mysrc/src/game_pvz.c", "mysrc/src/game_menu.c",
        "mysrc/src/cJSON.c",
        "mysrc/src/ui_login.c", "mysrc/src/ui_home.c",
        "mysrc/src/net_chat.c", "mysrc/src/app_music.c"]

def compile(src, cxx=False):
    name = src.replace("/", "_").replace(".", "_") + ".o"
    obj = (os.path.join(LVGL_DIR, name)).replace("\\", "/")
    if os.path.exists(obj):
        return True, ""
    flags = CXXFLAGS if cxx else CFLAGS
    r = subprocess.run([ZIG] + flags + ["-o", obj, "-c", src.replace("\\","/")],
                       cwd=ROOT.replace("\\","/"), capture_output=True, text=True, timeout=300)
    if r.returncode != 0 or not os.path.exists(obj):
        return False, r.stderr[:500]
    return True, ""

fails = []
total = 0
ok = 0
for s in srcs + ours:
    ok1, err = compile(s)
    total += 1
    if ok1:
        ok += 1
    else:
        fails.append((s, err))

# thorvg C++ 源
for s in cpp_srcs:
    ok1, err = compile(s, cxx=True)
    total += 1
    if ok1:
        ok += 1
    else:
        fails.append((s, err))

print(f"\n编译完成: {ok}/{total}")
if fails:
    print("失败列表:")
    for s, e in fails:
        print("=" * 60)
        print(s)
        print(e)
sys.exit(1 if fails else 0)