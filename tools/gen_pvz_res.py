# -*- coding: utf-8 -*-
"""
植物大战僵尸（经典版）—— LVGL 素材生成脚本
==========================================
用 Pillow 程序化绘制全部美术资源，导出为 32bit BGRA 的 BMP（带 alpha 通道），
LVGL 的 BMP 解码器（LV_USE_BMP=1）可直接以 "A:/xxx/yyy.bmp" 加载。

用法：  python gen_pvz_res.py [输出目录]
默认输出目录：脚本同级的 ../pvzBMP
"""
import os
import sys
import math
import random
from PIL import Image, ImageDraw, ImageFont

SS = 3                      # 超采样倍数（先放大画，再缩小 = 抗锯齿）
FONT_PATH = "C:/Windows/Fonts/msyhbd.ttc"   # 微软雅黑粗体
if not os.path.exists(FONT_PATH):
    FONT_PATH = "C:/Windows/Fonts/msyh.ttc"

# ---------------- 调色板 ----------------
C_OUT      = (38, 32, 26)     # 描边
C_LEAF     = (88, 168, 58)
C_LEAF_D   = (54, 118, 38)
C_PEA      = (108, 192, 74)
C_PEA_D    = (64, 138, 44)
C_ICE      = (150, 214, 240)
C_ICE_D    = (86, 158, 200)
C_PETAL    = (255, 216, 66)
C_PETAL_D  = (232, 172, 34)
C_SEED     = (146, 92, 42)
C_SEED_D   = (104, 62, 26)
C_NUT      = (206, 142, 72)
C_NUT_D    = (154, 98, 44)
C_CHERRY   = (222, 58, 52)
C_CHERRY_D = (168, 32, 30)
C_ZSKIN    = (172, 194, 152)
C_ZSKIN_D  = (124, 146, 108)
C_ZSHIRT   = (92, 104, 148)
C_ZPANT    = (72, 76, 96)
C_CONE     = (242, 142, 48)
C_BUCKET   = (156, 166, 176)
C_BUCKET_D = (104, 114, 126)
C_SUN      = (255, 226, 72)
C_SUN_D    = (240, 170, 30)
C_WOOD     = (150, 104, 58)
C_WOOD_D   = (108, 72, 38)
C_GRASS_A  = (108, 176, 74)
C_GRASS_B  = (96, 160, 66)

# 僵王（奶龙）调色板
C_NAI_BODY      = (66, 162, 220)   # 奶龙主色：圆胖蓝身
C_NAI_BELLY     = (148, 214, 240)  # 浅蓝肚皮
C_NAI_BODY_D    = (36, 110, 168)   # 主色阴影
C_NAI_OUT       = (28, 50, 78)     # 描边（深蓝黑）
C_NAI_EYE_W     = (255, 255, 255)  # 眼白
C_NAI_EYE_B     = (24, 24, 36)     # 眼瞳
C_NAI_MOUTH     = (54, 32, 30)     # 嘴内
C_NAI_TONGUE    = (220, 88, 102)   # 舌头红
C_NAI_TOOTH     = (244, 236, 218)  # 牙齿米白
C_NAI_HORN      = (220, 168, 64)   # 头顶小角（呆毛）金黄


# ---------------- 基础工具 ----------------
def new_canvas(w, h):
    im = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)


def finish(im, size=None):
    if size is None:
        size = (im.width // SS, im.height // SS)
    return im.resize(size, Image.LANCZOS)


def _s(v):
    return int(round(v * SS))


def box(x0, y0, x1, y1):
    return [_s(x0), _s(y0), _s(x1), _s(y1)]


def ell(d, b, fill=None, outline=None, width=0):
    x0, y0, x1, y1 = box(*b)
    d.ellipse([x0, y0, x1, y1], fill=fill, outline=outline, width=_s(width))


def rrect(d, b, radius=6, fill=None, outline=None, width=0):
    x0, y0, x1, y1 = box(*b)
    d.rounded_rectangle([x0, y0, x1, y1], radius=_s(radius), fill=fill,
                        outline=outline, width=_s(width))


def poly(d, pts, fill=None, outline=None, width=0):
    p = [(_s(x), _s(y)) for x, y in pts]
    d.polygon(p, fill=fill, outline=outline, width=_s(width)) if outline else d.polygon(p, fill=fill)


def line(d, pts, fill=C_OUT, width=2, joint="curve"):
    p = [(_s(x), _s(y)) for x, y in pts]
    d.line(p, fill=fill, width=_s(width), joint=joint)


def font(size):
    return ImageFont.truetype(FONT_PATH, size)


def text(d, xy, s, f, fill=(255, 255, 255), stroke=0, stroke_fill=(30, 20, 10), anchor=None):
    d.text(xy, s, font=f, fill=fill, stroke_width=stroke, stroke_fill=stroke_fill, anchor=anchor)


def save_bmp(im, path):
    """导出 32bit BGRA、自下而上的 BMP（与 LVGL example_32bit.bmp 同格式）"""
    im = im.convert("RGBA")
    w, h = im.size
    r, g, b, a = im.split()
    bgra = Image.merge("RGBA", (b, g, r, a))
    data = bgra.tobytes()

    HEADER_SIZE = 124             # BITMAPV5HEADER
    PX_OFFSET   = 14 + HEADER_SIZE
    ROW_SZ      = w * 4
    SIZE_IMAGE  = ROW_SZ * h
    FILE_SIZE   = PX_OFFSET + SIZE_IMAGE

    with open(path, "wb") as f:
        # 文件头 14 字节
        f.write(b"BM")
        f.write(FILE_SIZE.to_bytes(4, "little"))
        f.write((0).to_bytes(2, "little"))          # 保留
        f.write((0).to_bytes(2, "little"))          # 保留
        f.write(PX_OFFSET.to_bytes(4, "little"))

        # BITMAPV5HEADER 124 字节
        f.write(HEADER_SIZE.to_bytes(4, "little"))  # 0e: DIB header size
        f.write(w.to_bytes(4, "little", signed=True))  # 12: width
        f.write(h.to_bytes(4, "little", signed=True))  # 16: height
        f.write((1).to_bytes(2, "little"))          # 1a: planes
        f.write((32).to_bytes(2, "little"))         # 1c: bpp
        f.write((3).to_bytes(4, "little"))          # 1e: compression = BI_BITFIELDS
        f.write(SIZE_IMAGE.to_bytes(4, "little"))   # 22: size image
        f.write((0).to_bytes(4, "little"))          # 26: X ppm
        f.write((0).to_bytes(4, "little"))          # 2a: Y ppm
        f.write((0).to_bytes(4, "little"))          # 2e: colors used
        f.write((0).to_bytes(4, "little"))          # 32: colors important
        # 颜色掩码（BGRA）
        f.write((0x00FF0000).to_bytes(4, "little")) # 36: Red mask
        f.write((0x0000FF00).to_bytes(4, "little")) # 3a: Green mask
        f.write((0x000000FF).to_bytes(4, "little")) # 3e: Blue mask
        f.write((0xFF000000).to_bytes(4, "little")) # 42: Alpha mask
        # 颜色空间 / endpoints / gamma（填 0）
        for _ in range((HEADER_SIZE - 56) // 4):
            f.write((0).to_bytes(4, "little"))

        # 像素数据（自下而上）
        for y in range(h - 1, -1, -1):
            f.write(data[y * ROW_SZ:(y + 1) * ROW_SZ])


# ---------------- 通用零件 ----------------
def draw_stem(d, cx, top, bottom, leaves=True, color=C_LEAF, dark=C_LEAF_D):
    """画茎 + 两片叶子"""
    rrect(d, [cx - 4, top, cx + 4, bottom], radius=4, fill=color, outline=C_OUT, width=1.5)
    if leaves:
        ell(d, [cx - 26, bottom - 22, cx - 2, bottom - 4], fill=color, outline=C_OUT, width=1.5)
        ell(d, [cx + 2, bottom - 34, cx + 26, bottom - 16], fill=dark, outline=C_OUT, width=1.5)
        line(d, [(cx - 24, bottom - 13), (cx - 6, bottom - 14)], fill=dark, width=1.5)
        line(d, [(cx + 6, bottom - 25), (cx + 24, bottom - 26)], fill=color, width=1.5)


def draw_face(d, cx, cy, s=1.0, happy=True, color=C_OUT):
    """眼睛 + 嘴"""
    ew = 3.2 * s
    ell(d, [cx - 9 * s, cy - 4 * s, cx - 9 * s + ew * 2, cy - 4 * s + ew * 2.4], fill=color)
    ell(d, [cx + 9 * s - ew * 2, cy - 4 * s, cx + 9 * s, cy - 4 * s + ew * 2.4], fill=color)
    if happy:
        line(d, [(cx - 7 * s, cy + 8 * s), (cx, cy + 12 * s), (cx + 7 * s, cy + 8 * s)],
             fill=color, width=2.2 * s)
    else:
        line(d, [(cx - 7 * s, cy + 10 * s), (cx + 7 * s, cy + 10 * s)], fill=color, width=2.2 * s)


# ---------------- 植物 ----------------
def plant_sunflower(size=(72, 82)):
    im, d = new_canvas(*size)
    draw_stem(d, 36, 42, 80)
    # 花瓣
    for i in range(12):
        a = math.radians(i * 30)
        px = 36 + math.cos(a) * 20
        py = 30 + math.sin(a) * 20
        ell(d, [px - 8, py - 6, px + 8, py + 6], fill=C_PETAL, outline=C_PETAL_D, width=1.5)
    # 花盘
    ell(d, [36 - 17, 30 - 17, 36 + 17, 30 + 17], fill=C_SEED, outline=C_OUT, width=2)
    ell(d, [36 - 13, 30 - 13, 36 + 13, 30 + 13], fill=C_SEED_D)
    random.seed(7)
    for i in range(18):
        a = random.random() * 6.28
        r = random.random() * 10
        ell(d, [36 + math.cos(a) * r - 1.6, 30 + math.sin(a) * r - 1.6,
                36 + math.cos(a) * r + 1.6, 30 + math.sin(a) * r + 1.6], fill=C_SEED)
    draw_face(d, 36, 30, s=1.0)
    return finish(im)


def plant_shooter(size=(72, 82), ice=False, recoil=0):
    im, d = new_canvas(*size)
    body, body_d = (C_ICE, C_ICE_D) if ice else (C_PEA, C_PEA_D)
    draw_stem(d, 32, 44, 80, color=body, dark=body_d)
    rx = -recoil
    # 头
    ell(d, [12 + rx, 8, 56 + rx, 46], fill=body, outline=C_OUT, width=2)
    ell(d, [18 + rx, 12, 46 + rx, 28], fill=(255, 255, 255, 60))
    # 炮管
    tube = (C_ICE_D, C_ICE) if ice else (C_PEA_D, C_PEA)
    rrect(d, [46 + rx, 16, 68 + rx - recoil * 3, 34], radius=8, fill=tube[1], outline=C_OUT, width=2)
    ell(d, [62 + rx - recoil * 3, 18, 70 + rx - recoil * 3, 32], fill=tube[0], outline=C_OUT, width=1.5)
    # 眼
    ell(d, [20 + rx, 14, 34 + rx, 28], fill=(255, 255, 255), outline=C_OUT, width=1.5)
    ell(d, [26 + rx, 18, 33 + rx, 25], fill=C_OUT)
    ell(d, [28 + rx, 19, 31 + rx, 22], fill=(255, 255, 255))
    if ice:
        ell(d, [12 + rx, 8, 30 + rx, 20], fill=(235, 250, 255, 150))
    return finish(im)


def plant_wallnut(size=(70, 82), stage=1):
    im, d = new_canvas(*size)
    if stage == 3:
        b = [10, 22, 60, 76]
    elif stage == 2:
        b = [8, 18, 62, 78]
    else:
        b = [6, 14, 64, 80]
    ell(d, b, fill=C_NUT, outline=C_OUT, width=2.5)
    ell(d, [b[0] + 6, b[1] + 6, b[0] + 26, b[1] + 26], fill=(232, 176, 110))
    # 表情
    cy = (b[1] + b[3]) / 2 - 6
    cx = (b[0] + b[2]) / 2
    draw_face(d, cx, cy, s=1.15, happy=(stage == 1))
    # 裂纹
    if stage >= 2:
        line(d, [(cx + 12, b[1] + 8), (cx + 4, cy), (cx + 14, cy + 16), (cx + 6, b[3] - 10)],
             fill=C_NUT_D, width=2.5)
        line(d, [(cx - 18, b[1] + 14), (cx - 12, cy + 6)], fill=C_NUT_D, width=2)
    if stage >= 3:
        line(d, [(cx - 22, cy - 10), (cx - 8, cy - 2), (cx - 16, cy + 14)], fill=C_NUT_D, width=3)
        line(d, [(cx + 2, b[1] + 4), (cx - 2, cy - 8)], fill=C_NUT_D, width=2)
        ell(d, [cx - 26, cy + 8, cx - 18, cy + 16], fill=C_NUT_D)
    return finish(im)


def plant_cherry(size=(74, 82)):
    im, d = new_canvas(*size)
    # 梗
    line(d, [(36, 40), (30, 18), (44, 6)], fill=C_LEAF_D, width=3)
    line(d, [(36, 40), (48, 20), (58, 12)], fill=C_LEAF_D, width=3)
    ell(d, [50, 2, 70, 16], fill=C_LEAF, outline=C_OUT, width=1.5)
    ell(d, [22, 0, 44, 14], fill=C_LEAF, outline=C_OUT, width=1.5)
    # 樱桃
    for cx, cy in ((24, 58), (50, 56)):
        ell(d, [cx - 18, cy - 18, cx + 18, cy + 18], fill=C_CHERRY, outline=C_OUT, width=2)
        ell(d, [cx - 13, cy - 14, cx - 4, cy - 5], fill=(255, 150, 140))
        ell(d, [cx - 6, cy - 4, cx + 2, cy + 4], fill=C_OUT)
        ell(d, [cx + 6, cy - 4, cx + 12, cy + 2], fill=C_OUT)
        line(d, [(cx - 8, cy + 9), (cx + 8, cy + 9)], fill=C_OUT, width=2)
    return finish(im)


def boom(size=(110, 110)):
    im, d = new_canvas(*size)
    pts = []
    for i in range(20):
        a = math.radians(i * 18)
        r = 52 if i % 2 == 0 else 30
        pts.append((55 + math.cos(a) * r, 55 + math.sin(a) * r))
    poly(d, pts, fill=(255, 236, 120), outline=(255, 140, 30))
    ell(d, [55 - 34, 55 - 34, 55 + 34, 55 + 34], fill=(255, 190, 60), outline=(255, 120, 20), width=2)
    ell(d, [55 - 20, 55 - 20, 55 + 20, 55 + 20], fill=(255, 250, 200))
    return finish(im)


# ---------------- 子弹 / 阳光 / 道具 ----------------
def bullet(ice=False, size=(26, 26)):
    im, d = new_canvas(*size)
    c, cd = (C_ICE, C_ICE_D) if ice else ((122, 214, 86), C_PEA_D)
    ell(d, [1, 1, 25, 25], fill=c, outline=C_OUT, width=2)
    ell(d, [6, 5, 14, 12], fill=(255, 255, 255, 190))
    if ice:
        line(d, [(13, 2), (13, 24)], fill=(255, 255, 255, 160), width=1.5)
        line(d, [(2, 13), (24, 13)], fill=(255, 255, 255, 160), width=1.5)
    return finish(im)


def sun_sprite(phase=0, size=(58, 58)):
    im, d = new_canvas(*size)
    for i in range(12):
        a = math.radians(i * 30 + phase * 15)
        x0, y0 = 29 + math.cos(a) * 15, 29 + math.sin(a) * 15
        x1, y1 = 29 + math.cos(a) * 28, 29 + math.sin(a) * 28
        na = a + 1.57
        poly(d, [(x1, y1), (x0 + math.cos(na) * 5, y0 + math.sin(na) * 5),
                 (x0 - math.cos(na) * 5, y0 - math.sin(na) * 5)],
             fill=C_SUN, outline=C_SUN_D)
    ell(d, [29 - 19, 29 - 19, 29 + 19, 29 + 19], fill=C_SUN, outline=C_SUN_D, width=2)
    ell(d, [29 - 12, 29 - 12, 29 + 5, 29 + 5], fill=(255, 250, 190))
    return finish(im)


def mower(size=(54, 46)):
    im, d = new_canvas(*size)
    rrect(d, [4, 8, 50, 34], radius=8, fill=(214, 66, 58), outline=C_OUT, width=2)
    rrect(d, [10, 4, 44, 18], radius=6, fill=(238, 96, 84), outline=C_OUT, width=2)
    ell(d, [6, 28, 22, 46], fill=(60, 56, 56), outline=C_OUT, width=2)
    ell(d, [32, 28, 48, 46], fill=(60, 56, 56), outline=C_OUT, width=2)
    ell(d, [11, 33, 17, 41], fill=(150, 150, 150))
    ell(d, [37, 33, 43, 41], fill=(150, 150, 150))
    rrect(d, [44, 20, 54, 30], radius=3, fill=(180, 186, 194), outline=C_OUT, width=1.5)
    return finish(im)


def shovel(size=(58, 58)):
    im, d = new_canvas(*size)
    rrect(d, [30, 4, 40, 26], radius=4, fill=C_WOOD, outline=C_OUT, width=2)
    line(d, [(33, 8), (37, 8), (37, 24), (33, 24)], fill=C_WOOD_D, width=1.5)
    poly(d, [(10, 24), (46, 24), (40, 52), (16, 52)], fill=(176, 184, 192), outline=C_OUT, width=2)
    poly(d, [(16, 30), (40, 30), (36, 46), (20, 46)], fill=(206, 214, 222))
    line(d, [(16, 52), (10, 24)], fill=C_OUT, width=2)
    return finish(im)


# ---------------- 僵尸 ----------------
def zombie(gear="none", frame=0, size=(62, 94)):
    """frame: 0/1 = 走路两帧，2 = 啃食"""
    im, d = new_canvas(*size)
    eating = (frame == 2)
    lean = 6 if eating else 0
    leg_a = 0 if frame == 1 else (5 if frame == 0 else 2)
    hx, hy = 30 - lean * 0.6, 26 + (8 if eating else 0)

    # 腿
    for i, lx in enumerate((14, 36)):
        off = leg_a if i == 0 else -leg_a
        rrect(d, [lx + off, 62, lx + 13 + off, 90], radius=5, fill=C_ZPANT, outline=C_OUT, width=2)
        rrect(d, [lx + off - 2, 86, lx + 15 + off, 92], radius=3, fill=(46, 44, 44), outline=C_OUT, width=1.5)
    # 身体（衬衫）
    rrect(d, [12 - lean * 0.4, 40, 48 - lean * 0.4, 68], radius=8, fill=C_ZSHIRT, outline=C_OUT, width=2)
    # 破洞
    ell(d, [18 - lean * 0.4, 58, 26 - lean * 0.4, 66], fill=(60, 66, 96))
    poly(d, [(30 - lean * 0.4, 66), (36 - lean * 0.4, 66), (33 - lean * 0.4, 58)], fill=(60, 66, 96))
    # 手臂（朝左前伸）
    if eating:
        rrect(d, [4, 48, 30, 58], radius=6, fill=C_ZSKIN, outline=C_OUT, width=2)
        rrect(d, [10, 58, 34, 66], radius=6, fill=C_ZSKIN, outline=C_OUT, width=2)
    else:
        rrect(d, [0, 40, 26, 50], radius=6, fill=C_ZSKIN, outline=C_OUT, width=2)
        rrect(d, [2, 52, 28, 62], radius=6, fill=C_ZSKIN_D, outline=C_OUT, width=2)
    # 头
    ell(d, [hx - 16, hy - 18, hx + 16, hy + 18], fill=C_ZSKIN, outline=C_OUT, width=2.2)
    ell(d, [hx - 16, hy + 2, hx + 16, hy + 18], fill=C_ZSKIN_D)
    # 头发
    for i in range(4):
        line(d, [(hx - 12 + i * 7, hy - 14), (hx - 14 + i * 7, hy - 22)], fill=(72, 70, 58), width=2)
    # 眼睛
    ell(d, [hx - 12, hy - 8, hx - 3, hy + 1], fill=(40, 38, 34))
    ell(d, [hx + 2, hy - 8, hx + 11, hy + 1], fill=(40, 38, 34))
    ell(d, [hx - 11, hy - 7, hx - 7, hy - 3], fill=(240, 240, 230))
    ell(d, [hx + 4, hy - 7, hx + 8, hy - 3], fill=(240, 240, 230))
    # 嘴
    if eating:
        ell(d, [hx - 12, hy + 5, hx + 6, hy + 15], fill=(50, 34, 34), outline=C_OUT, width=1.5)
        for i in range(4):
            poly(d, [(hx - 10 + i * 4, hy + 6), (hx - 7 + i * 4, hy + 6), (hx - 8 + i * 4, hy + 11)],
                 fill=(240, 238, 226))
    else:
        line(d, [(hx - 10, hy + 8), (hx + 6, hy + 8)], fill=(50, 34, 34), width=2)
        for i in range(3):
            poly(d, [(hx - 8 + i * 5, hy + 8), (hx - 5 + i * 5, hy + 8), (hx - 6 + i * 5, hy + 13)],
                 fill=(240, 238, 226))
    # 头顶装备
    if gear == "cone":
        poly(d, [(hx - 13, hy - 14), (hx + 13, hy - 14), (hx + 3, hy - 38), (hx - 3, hy - 38)],
             fill=C_CONE, outline=C_OUT, width=2)
        poly(d, [(hx - 8, hy - 22), (hx + 8, hy - 22), (hx + 6, hy - 28), (hx - 6, hy - 28)],
             fill=(250, 250, 245))
    elif gear == "bucket":
        poly(d, [(hx - 15, hy - 14), (hx + 15, hy - 14), (hx + 11, hy - 34), (hx - 11, hy - 34)],
             fill=C_BUCKET, outline=C_OUT, width=2)
        rrect(d, [hx - 13, hy - 38, hx + 13, hy - 30], radius=3, fill=C_BUCKET_D, outline=C_OUT, width=2)
        line(d, [(hx - 6, hy - 32), (hx - 6, hy - 16)], fill=(220, 226, 232), width=2)
    return finish(im)


# ---------------- 僵王（奶龙风格） ----------------
def zombie_boss(frame=0, size=(120, 150)):
    """
    僵王：用 PIL 程序化绘制"奶龙"风格（参考 2024 年爆火的奶龙表情包）：
        圆胖蓝色身体、大眼睛、露齿咧嘴笑、头顶小角。
    frame: 0/1=走路两帧（w1/w2），2=啃食
    """
    im, d = new_canvas(*size)
    eating = (frame == 2)
    lean = 4 if eating else 0            # 啃食时身体略前倾
    leg_off = 6 if frame == 1 else 0     # w2 走路帧腿分开

    cx_head = 60 - lean
    cy_head = 44
    # ---- 头顶金黄小角（呆毛） ----
    poly(d, [(cx_head - 6, cy_head - 22), (cx_head + 6, cy_head - 22),
             (cx_head + 2, cy_head - 38), (cx_head - 2, cy_head - 38)],
         fill=C_NAI_HORN, outline=C_NAI_OUT, width=2)
    ell(d, [cx_head - 5, cy_head - 38, cx_head + 5, cy_head - 32],
        fill=C_NAI_HORN, outline=C_NAI_OUT, width=1.5)

    # ---- 腿（先画，被身体遮住一半） ----
    for i, lx in enumerate((30 + leg_off, 72 - leg_off)):
        rrect(d, [lx, 124, lx + 20, 146], radius=6, fill=C_NAI_BODY_D,
              outline=C_NAI_OUT, width=2)
        rrect(d, [lx - 2, 142, lx + 22, 150], radius=3, fill=(40, 64, 90),
              outline=C_NAI_OUT, width=1.5)

    # ---- 身体（圆胖，浅蓝肚皮） ----
    ell(d, [60 - 52, 86, 60 + 52, 134], fill=C_NAI_BODY,
        outline=C_NAI_OUT, width=2.5)
    ell(d, [60 - 42, 96, 60 + 42, 130], fill=C_NAI_BELLY)
    # 肚皮上的"奶龙"高光线
    ell(d, [60 - 28, 100, 60 + 8, 122], fill=(200, 234, 248), outline=None)
    # 破烂：裂痕
    line(d, [(60 - 18, 116), (60 - 10, 122), (60 - 16, 130)],
         fill=C_NAI_BODY_D, width=2.5)
    line(d, [(60 + 12, 102), (60 + 22, 110), (60 + 18, 120)],
         fill=C_NAI_BODY_D, width=2)

    # ---- 手臂 ----
    if eating:
        # 前伸啃食
        rrect(d, [4, 100, 36, 116], radius=8, fill=C_NAI_BODY,
              outline=C_NAI_OUT, width=2)
        rrect(d, [10, 112, 38, 124], radius=6, fill=C_NAI_BODY_D,
              outline=C_NAI_OUT, width=1.5)
        # 右臂靠后
        rrect(d, [82, 100, 114, 114], radius=8, fill=C_NAI_BODY,
              outline=C_NAI_OUT, width=2)
    else:
        # 走路两帧：手自然下垂
        for lx in (12, 90):
            rrect(d, [lx, 96, lx + 22, 112], radius=8, fill=C_NAI_BODY,
                  outline=C_NAI_OUT, width=2)
            rrect(d, [lx - 2, 108, lx + 24, 122], radius=6, fill=C_NAI_BODY_D,
                  outline=C_NAI_OUT, width=1.5)

    # ---- 头（圆胖蓝色） ----
    ell(d, [cx_head - 42, cy_head - 38, cx_head + 42, cy_head + 32],
        fill=C_NAI_BODY, outline=C_NAI_OUT, width=2.5)
    # 下半部阴影
    ell(d, [cx_head - 40, cy_head + 10, cx_head + 40, cy_head + 30],
        fill=C_NAI_BODY_D)
    # 头顶亮斑
    ell(d, [cx_head - 30, cy_head - 32, cx_head - 10, cy_head - 18],
        fill=(190, 226, 246), outline=None)

    # ---- 大眼睛 ----
    eye_y = cy_head - 6
    for ex in (cx_head - 18, cx_head + 18):
        ell(d, [ex - 11, eye_y - 13, ex + 11, eye_y + 13],
            fill=C_NAI_EYE_W, outline=C_NAI_OUT, width=2)
        # 眼瞳
        ell(d, [ex - 6, eye_y - 6, ex + 6, eye_y + 6],
            fill=C_NAI_EYE_B)
        # 眼瞳高光
        ell(d, [ex - 4, eye_y - 4, ex, eye_y],
            fill=(255, 255, 255))
        # 红色凶恶眼线（僵王特征）
        ell(d, [ex - 12, eye_y - 14, ex + 12, eye_y - 11],
            fill=(190, 60, 60, 180))

    # ---- 嘴（大咧嘴露齿） ----
    mouth_y = cy_head + 18
    if eating:
        # 啃食：张得更大，舌头伸出
        ell(d, [cx_head - 26, mouth_y - 12, cx_head + 26, mouth_y + 16],
            fill=C_NAI_MOUTH, outline=C_NAI_OUT, width=2)
        # 舌头
        ell(d, [cx_head - 14, mouth_y + 4, cx_head + 14, mouth_y + 14],
            fill=C_NAI_TONGUE)
        # 上排牙齿（5 颗白色三角）
        for i in range(5):
            x = cx_head - 20 + i * 10
            poly(d, [(x, mouth_y - 12), (x + 8, mouth_y - 12), (x + 4, mouth_y - 2)],
                 fill=C_NAI_TOOTH, outline=C_NAI_OUT, width=1)
        # 下排牙齿（4 颗）
        for i in range(4):
            x = cx_head - 14 + i * 10
            poly(d, [(x, mouth_y + 10), (x + 7, mouth_y + 10), (x + 3, mouth_y + 4)],
                 fill=C_NAI_TOOTH, outline=C_NAI_OUT, width=1)
    else:
        # 咆哮：嘴张但没舌头
        ell(d, [cx_head - 24, mouth_y - 10, cx_head + 24, mouth_y + 14],
            fill=C_NAI_MOUTH, outline=C_NAI_OUT, width=2)
        for i in range(5):
            x = cx_head - 18 + i * 9
            poly(d, [(x, mouth_y - 10), (x + 7, mouth_y - 10), (x + 3, mouth_y - 1)],
                 fill=C_NAI_TOOTH, outline=C_NAI_OUT, width=1)
        for i in range(4):
            x = cx_head - 12 + i * 9
            poly(d, [(x, mouth_y + 8), (x + 6, mouth_y + 8), (x + 3, mouth_y + 2)],
                 fill=C_NAI_TOOTH, outline=C_NAI_OUT, width=1)

    return finish(im)


# ---------------- UI ----------------
def seed_card(icon, cost, size=(64, 88)):
    im = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    rrect(d, [0, 0, 63, 87], radius=8, fill=(196, 186, 152), outline=(60, 48, 34), width=3)
    rrect(d, [4, 4, 59, 45], radius=5, fill=(232, 226, 200), outline=(120, 104, 74), width=2)
    rrect(d, [4, 50, 59, 83], radius=5, fill=(168, 156, 122), outline=(120, 104, 74), width=2)
    ic = icon.copy()
    ic.thumbnail((46, 46), Image.LANCZOS)
    im.alpha_composite(ic, ((64 - ic.width) // 2, (49 - ic.height) // 2 + 2))
    f = font(20)
    text(d, (32, 84), str(cost), f, fill=(255, 245, 140), stroke=3, stroke_fill=(50, 34, 20), anchor="ms")
    return im


def bank_panel(size=(104, 48)):
    im = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    rrect(d, [0, 0, 103, 47], radius=8, fill=(120, 84, 46), outline=(52, 34, 18), width=3)
    rrect(d, [4, 4, 99, 43], radius=6, fill=(196, 160, 100), outline=(140, 108, 60), width=2)
    s = sun_sprite(0, size=(40, 40))
    im.alpha_composite(s, (2, 4))
    return im


def wave_panel(size=(168, 48)):
    im = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    rrect(d, [0, 0, 167, 47], radius=8, fill=(96, 128, 60), outline=(40, 56, 26), width=3)
    rrect(d, [4, 4, 163, 43], radius=6, fill=(146, 190, 92), outline=(96, 128, 60), width=2)
    f = font(22)
    text(d, (14, 24), "第", f, fill=(255, 255, 255), stroke=3, stroke_fill=(40, 56, 26), anchor="lm")
    text(d, (128, 24), "波", f, fill=(255, 255, 255), stroke=3, stroke_fill=(40, 56, 26), anchor="lm")
    return im


def btn(size, label, bg=(150, 104, 58), fg=(255, 255, 255)):
    im = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    rrect(d, [0, 0, size[0] - 1, size[1] - 1], radius=8, fill=bg, outline=(52, 34, 18), width=3)
    rrect(d, [4, 4, size[0] - 5, size[1] - 5], radius=6, fill=(int(bg[0] * 1.28), int(bg[1] * 1.28), int(bg[2] * 1.28)),
          outline=(120, 88, 48), width=2)
    f = font(22)
    text(d, (size[0] // 2, size[1] // 2), label, f, fill=fg, stroke=3, stroke_fill=(52, 34, 18), anchor="mm")
    return im


def banner(size, title, sub, bg):
    im = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    rrect(d, [0, 0, size[0] - 1, size[1] - 1], radius=18, fill=bg, outline=(40, 30, 20), width=4)
    rrect(d, [6, 6, size[0] - 7, size[1] - 7], radius=14,
          fill=(int(bg[0] * 1.2), int(bg[1] * 1.2), int(bg[2] * 1.2)), outline=(255, 255, 255, 120), width=2)
    f1, f2 = font(44), font(22)
    text(d, (size[0] // 2, size[1] // 2 - 16), title, f1, fill=(255, 245, 160), stroke=4, stroke_fill=(40, 26, 14), anchor="mm")
    text(d, (size[0] // 2, size[1] // 2 + 30), sub, f2, fill=(255, 255, 255), stroke=3, stroke_fill=(40, 26, 14), anchor="mm")
    return im


# ---------------- 背景 ----------------
def background(w=1024, h=600, gx=80, gy=115, gw=864, gh=460, cols=9, rows=5):
    im = Image.new("RGBA", (w, h), (48, 34, 22, 255))
    d = ImageDraw.Draw(im)
    # 顶部木质卡片栏
    rrect(d, [0, 0, w, 110], radius=0, fill=C_WOOD_D)
    for y in range(0, 110, 18):
        d.line([(0, y), (w, y)], fill=(88, 58, 30), width=2)
    rrect(d, [0, 100, w, 110], radius=0, fill=(70, 46, 24))
    d.line([(0, 110), (w, 110)], fill=(40, 26, 14), width=3)
    # 左侧水泥地 + 房子墙
    rrect(d, [0, 110, gx, h], radius=0, fill=(148, 140, 128))
    for i in range(rows):
        y0 = gy + i * (gh // rows)
        d.line([(0, y0), (gx, y0)], fill=(120, 112, 100), width=2)
    # 右侧泥土
    rrect(d, [gx + gw, 110, w, h], radius=0, fill=(126, 92, 56))
    # 草坪
    cw, ch = gw // cols, gh // rows
    random.seed(2024)
    for r in range(rows):
        for c in range(cols):
            base = C_GRASS_A if (r + c) % 2 == 0 else C_GRASS_B
            x0, y0 = gx + c * cw, gy + r * ch
            d.rectangle([x0, y0, x0 + cw - 1, y0 + ch - 1], fill=base)
            for _ in range(34):
                gx0 = x0 + random.randint(2, cw - 3)
                gy0 = y0 + random.randint(2, ch - 3)
                col = (base[0] + random.randint(-12, 14), base[1] + random.randint(-12, 14),
                       base[2] + random.randint(-12, 14))
                d.line([(gx0, gy0), (gx0 + random.randint(-3, 3), gy0 - random.randint(3, 7))],
                       fill=col, width=2)
            d.line([(x0, y0), (x0 + cw, y0)], fill=(80, 140, 56), width=1)
            d.line([(x0, y0), (x0, y0 + ch)], fill=(80, 140, 56), width=1)
    # 草坪边框
    d.rectangle([gx - 3, gy - 3, gx + gw + 2, gy + gh + 2], outline=(58, 92, 40), width=4)
    return im


# ---------------- 主流程 ----------------
def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "pvzBMP")
    out_dir = os.path.abspath(out_dir)
    os.makedirs(out_dir, exist_ok=True)

    # ---------- 1024×600 全屏背景 ----------
    imgs = {}
    imgs["bg"] = background()

    # ---------- 精灵尺寸（与 game_pvz.c 里的 PLANT_W/H 等保持一致） ----------
    PLANT_SIZE  = (82, 92)
    ZOMBIE_SIZE = (72, 108)
    BOSS_SIZE   = (120, 150)
    BULLET_SIZE = (28, 28)
    SUN_SIZE    = (62, 62)
    SHOVEL_SIZE = (58, 58)
    MOWER_SIZE  = (60, 52)

    sf = plant_sunflower(size=PLANT_SIZE)
    ps1 = plant_shooter(size=PLANT_SIZE, ice=False, recoil=0)
    ps2 = plant_shooter(size=PLANT_SIZE, ice=False, recoil=4)
    sp1 = plant_shooter(size=PLANT_SIZE, ice=True, recoil=0)
    sp2 = plant_shooter(size=PLANT_SIZE, ice=True, recoil=4)
    wn1 = plant_wallnut(size=(78, 92), stage=1)
    wn2 = plant_wallnut(size=(78, 92), stage=2)
    wn3 = plant_wallnut(size=(78, 92), stage=3)
    ch = plant_cherry(size=(86, 92))

    imgs["plant_sunflower"] = sf
    imgs["plant_peashooter_1"] = ps1
    imgs["plant_peashooter_2"] = ps2
    imgs["plant_snowpea_1"] = sp1
    imgs["plant_snowpea_2"] = sp2
    imgs["plant_wallnut_1"] = wn1
    imgs["plant_wallnut_2"] = wn2
    imgs["plant_wallnut_3"] = wn3
    imgs["plant_cherry"] = ch
    imgs["boom"] = boom(size=(130, 130))
    imgs["bullet_pea"] = bullet(ice=False, size=BULLET_SIZE)
    imgs["bullet_snow"] = bullet(ice=True, size=BULLET_SIZE)
    imgs["sun_1"] = sun_sprite(phase=0, size=SUN_SIZE)
    imgs["sun_2"] = sun_sprite(phase=1, size=SUN_SIZE)
    imgs["mower"] = mower(size=MOWER_SIZE)
    imgs["shovel"] = shovel(size=SHOVEL_SIZE)

    for i, gear in enumerate(("none", "cone", "bucket"), start=1):
        imgs["zombie%d_w1" % i] = zombie(gear, 0, size=ZOMBIE_SIZE)
        imgs["zombie%d_w2" % i] = zombie(gear, 1, size=ZOMBIE_SIZE)
        imgs["zombie%d_eat" % i] = zombie(gear, 2, size=ZOMBIE_SIZE)

    # 僵王：奶龙造型，仅最后一波出现
    imgs["zombie_boss_w1"]  = zombie_boss(0, size=BOSS_SIZE)
    imgs["zombie_boss_w2"]  = zombie_boss(1, size=BOSS_SIZE)
    imgs["zombie_boss_eat"] = zombie_boss(2, size=BOSS_SIZE)

    # 卡片尺寸保持原样（顶部 UI 高度固定 88）
    imgs["card_sunflower"] = seed_card(sf, 50)
    imgs["card_peashooter"] = seed_card(ps1, 100)
    imgs["card_wallnut"] = seed_card(wn1, 50)
    imgs["card_cherry"] = seed_card(ch, 150)
    imgs["card_snowpea"] = seed_card(sp1, 175)
    imgs["bank"] = bank_panel(size=(104, 48))
    imgs["wave"] = wave_panel(size=(168, 48))
    imgs["btn_restart"] = btn((88, 48), "重来", bg=(150, 104, 58))
    imgs["btn_exit"]    = btn((88, 48), "退出", bg=(150, 64, 64))
    imgs["btn_start"] = btn((150, 52), "开始游戏", bg=(96, 140, 62))
    imgs["win"] = banner((420, 150), "守住了！", "点击「重来」再战一局", (86, 130, 54))
    imgs["lose"] = banner((420, 150), "被吃掉了脑子", "点击「重来」再战一局", (128, 52, 48))

    for name, im in imgs.items():
        save_bmp(im, os.path.join(out_dir, name + ".bmp"))

    print("素材已生成：%s （共 %d 个）" % (out_dir, len(imgs)))
    for k in sorted(imgs):
        print("  %-20s %s" % (k + ".bmp", imgs[k].size))


if __name__ == "__main__":
    main()
