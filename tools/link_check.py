# -*- coding: utf-8 -*-
"""链接所有 .o 为 aarch64-linux-musl 静态可执行文件（不依赖 glibc）"""
import os, sys, subprocess, glob

ROOT = r"C:/Users/yanji/Desktop/yan/3-第三阶段/02-网编/smart_center"
OUT  = r"C:/Users/yanji/AppData/Local/Temp/sc_build"
LVGL_OBJ_DIR = os.path.join(OUT, "lvgl")
ZIG = r"C:/Users/yanji/.workbuddy/binaries/python/versions/3.13.12/Lib/site-packages/ziglang/zig.exe"

objs = sorted(glob.glob(os.path.join(LVGL_OBJ_DIR, "*.o")))
print(f"链接 {len(objs)} 个 .o ...")

# 用 zig cc 链接（同时也是 aarch64-linux-musl 链接器）
out_bin = os.path.join(ROOT, "smart_center_bin_arm64").replace("\\","/")
r = subprocess.run([ZIG, "c++",
                    "-target", "aarch64-linux-musl",
                    "-O2", "-static",
                    "-o", out_bin,
                    "-lm", "-pthread"]
                   + [o.replace("\\","/") for o in objs],
                   cwd=ROOT.replace("\\","/"),
                   capture_output=True, text=True)
print("rc =", r.returncode)
if r.returncode != 0:
    print("STDERR:")
    print(r.stderr[-2000:])
else:
    sz = os.path.getsize(out_bin)
    print(f"\n生成成功: {out_bin} ({sz/1024/1024:.1f} MB)")