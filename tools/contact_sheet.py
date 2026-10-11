# -*- coding: utf-8 -*-
"""生成 pvzBMP 全部资源对照表 PNG（棋盘格底显示透明区域）"""
import os
from PIL import Image

RES = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "pvzBMP")
OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "pvz_res_preview.png")

COLS = 8
CELL = 96          # 每格大小
PAD = 6
LABEL_H = 14

names = sorted(n[:-4] for n in os.listdir(RES) if n.endswith(".bmp"))
rows = (len(names) + COLS - 1) // COLS

W = COLS * (CELL + PAD) + PAD
H = rows * (CELL + PAD + LABEL_H) + PAD
sheet = Image.new("RGB", (W, H), (245, 245, 245))

# 棋盘格画笔
def checker(size, sq=12):
    im = Image.new("RGB", (size, size), (255, 255, 255))
    px = im.load()
    for y in range(size):
        for x in range(size):
            if ((x // sq) + (y // sq)) % 2:
                px[x, y] = (204, 204, 204)
    return im

from PIL import ImageDraw
draw = ImageDraw.Draw(sheet)
try:
    from PIL import ImageFont
    font = ImageFont.load_default()
except Exception:
    font = None

for idx, name in enumerate(names):
    r, c = divmod(idx, COLS)
    x0 = PAD + c * (CELL + PAD)
    y0 = PAD + r * (CELL + PAD + LABEL_H)
    img = Image.open(os.path.join(RES, name + ".bmp")).convert("RGBA")
    bg = checker(CELL)
    scaled = img.copy()
    if img.width > CELL or img.height > CELL:
        ratio = min(CELL / img.width, CELL / img.height)
        scaled = img.resize((max(1, int(img.width * ratio)), max(1, int(img.height * ratio))), Image.NEAREST)
    bg.paste(scaled, ((CELL - scaled.width) // 2, (CELL - scaled.height) // 2), scaled)
    sheet.paste(bg, (x0, y0))
    label = "%s %dx%d" % (name, img.width, img.height)
    if len(label) > 22:
        label = label[:22]
    draw.text((x0 + 2, y0 + CELL + 1), label, fill=(60, 60, 60), font=font)

sheet.save(OUT)
print("saved:", OUT, sheet.size, "%d images" % len(names))
