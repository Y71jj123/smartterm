/* ui_font.h - 界面中文字体
 *
 * 由 Noto Sans SC（思源黑体系，SIL OFL 开源协议）经 lv_font_conv 生成的**子集**字库：
 * 只包含界面实际用到的汉字 + ASCII(0x20~0x7E)，所以体积可控（7 档合计约 2.5MB）。
 * 字体源用的是项目里 lvgl 自带的 NotoSansSC-Regular.ttf，无需外部下载。
 *
 * 重新生成：python tools/gen_cn_font.py
 */
#ifndef UI_FONT_H
#define UI_FONT_H

#include "lvgl/lvgl.h"

LV_FONT_DECLARE(ui_font_cn_14)
LV_FONT_DECLARE(ui_font_cn_16)
LV_FONT_DECLARE(ui_font_cn_20)
LV_FONT_DECLARE(ui_font_cn_28)
LV_FONT_DECLARE(ui_font_cn_32)
LV_FONT_DECLARE(ui_font_cn_36)
LV_FONT_DECLARE(ui_font_cn_48)

#endif /* UI_FONT_H */
