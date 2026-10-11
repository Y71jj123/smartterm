# -*- coding: utf-8 -*-
"""把 pvzBMP 里的素材按真实坐标合成一张游戏界面预览图（同时用来校验布局坐标）"""
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "pvzBMP")
FONT = "C:/Windows/Fonts/msyhbd.ttc"

W, H = 800, 480
GX, GY = 78, 98
CW, CH = 80, 76
COLS, ROWS = 9, 5

CARD_X0, CARD_Y = 118, 6
CARD_W, CARD_H, CARD_GAP = 64, 88, 66


def load(name):
    """加载 BMP（新格式与 LVGL example_32bit.bmp 一致，Pillow 可直接读 RGBA）"""
    return Image.open(os.path.join(RES, name + ".bmp")).convert("RGBA")


def paste(base, im, x, y):
    base.alpha_composite(im, (int(x), int(y)))


def main():
    scr = load("bg").copy()
    d = ImageDraw.Draw(scr)
    f = ImageFont.truetype(FONT, 26)
    fs = ImageFont.truetype(FONT, 22)

    # 阳光面板 + 数字
    paste(scr, load("bank"), 8, 6)
    d.text((62, 30), "175", font=f, fill=(255, 240, 120), anchor="lm",
           stroke_width=3, stroke_fill=(50, 34, 18))

    # 卡片
    for i, n in enumerate(["card_sunflower", "card_peashooter", "card_wallnut",
                           "card_cherry", "card_snowpea"]):
        paste(scr, load(n), CARD_X0 + i * CARD_GAP, CARD_Y)
    # 第 2 张卡冷却遮罩示意
    mask = Image.new("RGBA", (CARD_W, 40), (10, 10, 10, 150))
    scr.alpha_composite(mask, (CARD_X0 + 64, CARD_Y))

    # 铲子 / 波次 / 重来
    paste(scr, load("shovel"), 462, 20)
    paste(scr, load("wave"), 532, 8)
    d.text((84, 32), "3", font=f, fill=(255, 255, 255), anchor="mm",
           stroke_width=3, stroke_fill=(40, 56, 26))
    paste(scr, load("btn_restart"), 708, 8)

    # 小推车
    for r in range(ROWS):
        paste(scr, load("mower"), 12, GY + r * CH + 15)

    # 植物
    def cell_xy(r, c):
        return GX + c * CW, GY + r * CH

    def put_plant(name, r, c):
        x, y = cell_xy(r, c)
        im = load(name)
        paste(scr, im, x + (CW - im.width) // 2, y + (CH - im.height) + 2)

    put_plant("plant_sunflower", 0, 0)
    put_plant("plant_sunflower", 1, 0)
    put_plant("plant_peashooter_2", 0, 2)
    put_plant("plant_peashooter_1", 1, 2)
    put_plant("plant_snowpea_1", 3, 2)
    put_plant("plant_wallnut_1", 2, 4)
    put_plant("plant_wallnut_3", 4, 4)
    put_plant("plant_cherry", 2, 1)
    put_plant("plant_sunflower", 4, 0)

    # 僵尸
    for i, (name, r) in enumerate([("zombie1_w1", 0), ("zombie2_w1", 1),
                                   ("zombie3_w1", 2), ("zombie1_eat", 3)]):
        im = load(name)
        paste(scr, im, 520 + (i % 2) * 40, GY + r * CH + CH - im.height)

    # 子弹
    paste(scr, load("bullet_pea"), 320, GY + 0 * CH + 26)
    paste(scr, load("bullet_pea"), 380, GY + 1 * CH + 26)
    paste(scr, load("bullet_snow"), 340, GY + 3 * CH + 26)

    # 阳光
    paste(scr, load("sun_1"), 430, 150)
    paste(scr, load("sun_2"), 250, 260)

    scr.save(os.path.join(ROOT, "pvz_preview.png"))
    print("预览图已生成:", os.path.join(ROOT, "pvz_preview.png"))

    # 素材总览图
    names = [n[:-4] for n in sorted(os.listdir(RES)) if n.endswith(".bmp")]
    cell = 110
    cols = 8
    rows = (len(names) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * cell, rows * (cell + 16)), (36, 36, 40, 255))
    sd = ImageDraw.Draw(sheet)
    for i, n in enumerate(names):
        im = load(n)
        im.thumbnail((cell - 8, cell - 8), Image.LANCZOS)
        x = (i % cols) * cell + (cell - im.width) // 2
        y = (i // cols) * (cell + 16) + 4
        sheet.alpha_composite(im, (x, y))
        sd.text(((i % cols) * cell + cell // 2, (i // cols) * (cell + 16) + cell + 2),
                n, font=ImageFont.truetype(FONT, 11), fill=(220, 220, 220), anchor="ma")
    sheet.save(os.path.join(ROOT, "pvz_assets_sheet.png"))
    print("素材总览已生成:", os.path.join(ROOT, "pvz_assets_sheet.png"))


if __name__ == "__main__":
    main()
