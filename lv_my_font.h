/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --bpp 1 --size 16 --no-compress --stride 1 --align 1 --font ruimeijiazhangqingpingyingbixingshu.ttf --symbols  --range 0-255 --format lvgl -o lv_my_font.c
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif



#ifndef LV_MY_FONT
#define LV_MY_FONT 1
#endif

#if LV_MY_FONT

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0000 "\u0000" */
    0x0,

    /* U+000D "\r" */
    0x0,

    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0x1a, 0xa8, 0x8,

    /* U+0022 "\"" */
    0x16, 0x80,

    /* U+0023 "#" */
    0x42, 0xfc, 0xa7, 0xea, 0x40,

    /* U+0024 "$" */
    0x10, 0x8e, 0xc6, 0x18, 0xa5, 0xbb, 0x90,

    /* U+0025 "%" */
    0x8, 0x27, 0x1c, 0x10, 0x82, 0x96, 0x5a, 0x8,
    0x0,

    /* U+0026 "&" */
    0x30, 0x60, 0xc1, 0x2, 0x8d, 0x26, 0x4c, 0xb9,
    0xcc,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0xb, 0x49, 0x24, 0x48, 0x80,

    /* U+0029 ")" */
    0x48, 0x92, 0x49, 0x50,

    /* U+002A "*" */
    0x25, 0x6e, 0xcf, 0x90,

    /* U+002B "+" */
    0x10, 0x5f, 0x84, 0x10, 0x40,

    /* U+002C "," */
    0x16,

    /* U+002D "-" */
    0x3f, 0x0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x4, 0x10, 0x82, 0x10, 0x42, 0x8, 0x41, 0x8,
    0x0,

    /* U+0030 "0" */
    0x32, 0x63, 0x18, 0xca, 0x4c,

    /* U+0031 "1" */
    0x4, 0xa4, 0x96,

    /* U+0032 "2" */
    0x78, 0x20, 0x84, 0x21, 0x7, 0xe0,

    /* U+0033 "3" */
    0x32, 0x44, 0x47, 0x4, 0x22, 0xe0,

    /* U+0034 "4" */
    0x8, 0x51, 0x22, 0x89, 0x1f, 0xe4, 0x8, 0x10,
    0x20,

    /* U+0035 "5" */
    0xd, 0xc4, 0x1c, 0x88, 0x20, 0xa6, 0x70,

    /* U+0036 "6" */
    0x11, 0x11, 0xb, 0x66, 0x32, 0x60,

    /* U+0037 "7" */
    0x1, 0xf0, 0x84, 0x10, 0x41, 0x4, 0x10,

    /* U+0038 "8" */
    0x31, 0x18, 0x94, 0x21, 0x49, 0x24, 0x91, 0x80,

    /* U+0039 "9" */
    0x32, 0xe5, 0x6d, 0x10, 0x84, 0x21, 0x0,

    /* U+003A ":" */
    0x88,

    /* U+003B ";" */
    0xcd, 0x80,

    /* U+003C "<" */
    0x8, 0x41, 0x8, 0x42, 0x4, 0xc, 0x8, 0x10,

    /* U+003D "=" */
    0xe, 0x70, 0x0, 0xf, 0xf0,

    /* U+003E ">" */
    0x82, 0x8, 0x21, 0x88, 0x88, 0x84, 0x0,

    /* U+003F "?" */
    0x69, 0x13, 0x24, 0x40, 0x2,

    /* U+0040 "@" */
    0x1c, 0x49, 0xe, 0xda, 0xb5, 0x77, 0x50, 0xcc,
    0xf0,

    /* U+0041 "A" */
    0x0, 0x61, 0x8a, 0x29, 0x27, 0xd1, 0x86, 0x18,
    0x40,

    /* U+0042 "B" */
    0x18, 0x49, 0x12, 0x45, 0xcc, 0x50, 0xa1, 0x44,
    0xf0,

    /* U+0043 "C" */
    0x39, 0xa4, 0x20, 0x82, 0x8, 0x20, 0x49, 0xc0,

    /* U+0044 "D" */
    0x73, 0x24, 0x51, 0x45, 0x14, 0xd2, 0x51, 0x80,

    /* U+0045 "E" */
    0x19, 0x84, 0x10, 0x4d, 0xc4, 0x10, 0x5d, 0x80,

    /* U+0046 "F" */
    0x19, 0x84, 0x17, 0x62, 0x8, 0x20, 0x82, 0x0,

    /* U+0047 "G" */
    0x3b, 0x51, 0x9, 0xc6, 0x31, 0xdb, 0xc0,

    /* U+0048 "H" */
    0xa, 0x52, 0x97, 0xc6, 0x31, 0x8c, 0x40,

    /* U+0049 "I" */
    0x69, 0x24, 0x92, 0x5c,

    /* U+004A "J" */
    0x8, 0x42, 0x10, 0x84, 0x31, 0x93, 0x80,

    /* U+004B "K" */
    0x2, 0x89, 0x22, 0x86, 0xa, 0x12, 0x24, 0x85,
    0x4,

    /* U+004C "L" */
    0x42, 0x10, 0x84, 0x22, 0x10, 0x87, 0xc0,

    /* U+004D "M" */
    0x1, 0x31, 0x99, 0x4a, 0xa5, 0x54, 0xc6, 0x23,
    0x11, 0x80, 0xc0, 0x40,

    /* U+004E "N" */
    0x2, 0x85, 0x8b, 0x25, 0x4a, 0xb5, 0x46, 0x8d,
    0x10,

    /* U+004F "O" */
    0x32, 0x63, 0x18, 0xc6, 0x32, 0x60,

    /* U+0050 "P" */
    0x7b, 0x14, 0x51, 0x89, 0xc8, 0x20, 0x82, 0x0,

    /* U+0051 "Q" */
    0x32, 0x53, 0x18, 0xc6, 0x36, 0x93, 0x40,

    /* U+0052 "R" */
    0x7b, 0x14, 0x52, 0x91, 0xa8, 0xa3, 0x86, 0x10,

    /* U+0053 "S" */
    0x76, 0x21, 0x6, 0x8, 0x31, 0x9b, 0x80,

    /* U+0054 "T" */
    0x3f, 0x8, 0x42, 0x10, 0x84, 0x21, 0x0,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0xc9, 0xc0,

    /* U+0056 "V" */
    0x86, 0x14, 0xd2, 0x49, 0x22, 0x8c, 0x30, 0x40,

    /* U+0057 "W" */
    0x3, 0x26, 0x4c, 0x9b, 0x56, 0xab, 0x24, 0x48,
    0x90,

    /* U+0058 "X" */
    0x85, 0x32, 0x8a, 0x10, 0x42, 0x8a, 0x47, 0x10,

    /* U+0059 "Y" */
    0x0, 0x28, 0xb4, 0x70, 0x82, 0x8, 0x20, 0x82,
    0x0,

    /* U+005A "Z" */
    0x1e, 0x24, 0x4, 0x8, 0x10, 0x10, 0x20, 0x20,
    0x4f, 0xf0,

    /* U+005B "[" */
    0xf2, 0x49, 0x24, 0x9c,

    /* U+005C "\\" */
    0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10,

    /* U+005D "]" */
    0xe4, 0x92, 0x49, 0x3c,

    /* U+005E "^" */
    0x74, 0x0,

    /* U+005F "_" */
    0x7f, 0x0,

    /* U+0060 "`" */
    0xe0,

    /* U+0061 "a" */
    0x31, 0x69, 0xa4, 0xb3, 0x4c, 0xc0,

    /* U+0062 "b" */
    0x2, 0x10, 0x88, 0x7b, 0x31, 0x93, 0x80,

    /* U+0063 "c" */
    0x13, 0x61, 0x8, 0x41, 0xc0,

    /* U+0064 "d" */
    0x4, 0x10, 0x82, 0x29, 0x49, 0x24, 0xb3, 0x60,

    /* U+0065 "e" */
    0x39, 0xac, 0xbc, 0x82, 0xc, 0x5e,

    /* U+0066 "f" */
    0x19, 0x48, 0x43, 0xf0, 0x84, 0x22, 0x10, 0x80,

    /* U+0067 "g" */
    0x32, 0xe5, 0x6d, 0x9, 0x52, 0x93, 0x0,

    /* U+0068 "h" */
    0x41, 0x4, 0x10, 0x59, 0xa4, 0x92, 0x4e, 0x20,

    /* U+0069 "i" */
    0x46, 0xab,

    /* U+006A "j" */
    0x20, 0xb2, 0x52, 0x58,

    /* U+006B "k" */
    0x20, 0x40, 0x82, 0x45, 0x8d, 0x1c, 0x48, 0x8d,
    0x8,

    /* U+006C "l" */
    0x55, 0xaa, 0xb0,

    /* U+006D "m" */
    0x5f, 0xd5, 0x2a, 0x59, 0x32, 0x64, 0x80,

    /* U+006E "n" */
    0x5f, 0x52, 0x99, 0x4a, 0x20,

    /* U+006F "o" */
    0x32, 0x63, 0x18, 0xca, 0xdc,

    /* U+0070 "p" */
    0x1c, 0x64, 0x89, 0x63, 0x8, 0x10, 0x20, 0x80,
    0x0,

    /* U+0071 "q" */
    0x75, 0x99, 0xee, 0x22, 0x26, 0x0,

    /* U+0072 "r" */
    0xd3, 0x50, 0x88, 0x40, 0x0,

    /* U+0073 "s" */
    0x1b, 0x10, 0xc1, 0xa, 0x4c,

    /* U+0074 "t" */
    0x1, 0x89, 0xe4, 0x61, 0xa, 0x60,

    /* U+0075 "u" */
    0xcc, 0xa5, 0x29, 0x4a, 0x5e,

    /* U+0076 "v" */
    0x4b, 0x35, 0x14, 0x51, 0x86, 0x10,

    /* U+0077 "w" */
    0x2, 0x53, 0xd4, 0x54, 0x54, 0x78, 0x58, 0x50,

    /* U+0078 "x" */
    0xd9, 0x88, 0x42, 0x31, 0x92,

    /* U+0079 "y" */
    0xe, 0x9c, 0x21, 0xa, 0x94, 0x60,

    /* U+007A "z" */
    0x39, 0x41, 0x8, 0x41, 0xf8, 0x0,

    /* U+007B "{" */
    0x24, 0x44, 0x44, 0x84, 0x44, 0x44, 0x43,

    /* U+007C "|" */
    0x7f, 0xfc,

    /* U+007D "}" */
    0x12, 0x24, 0xd2, 0x49, 0x2c,

    /* U+007E "~" */
    0x61, 0x38, 0x0,

    /* U+5D4C "嵌" */
    0x4, 0x0, 0x28, 0x2, 0x81, 0x78, 0x19, 0x0,
    0x5e, 0x27, 0x41, 0xc8, 0xed, 0x1, 0xd2, 0x16,
    0x10,

    /* U+6D9B "涛" */
    0x4, 0x0, 0xc2, 0x60, 0xc, 0xd, 0xd1, 0xe1,
    0x9c, 0xba, 0x50, 0xa4, 0xa2, 0x8, 0x6, 0x0,
    0x80,

    /* U+738B "王" */
    0xe, 0x8, 0x2, 0x1, 0x83, 0xc2, 0x40, 0xe0,
    0x4f, 0xf8, 0x0,

    /* U+7CA4 "粤" */
    0x2, 0x0, 0x17, 0x1, 0xbc, 0x2, 0xf0, 0xb,
    0x20, 0x3e, 0x80, 0x9e, 0x3, 0x9e, 0x3, 0x84,
    0x78, 0xe, 0x3c, 0x0, 0x90, 0x0, 0x80, 0x2,
    0x0, 0x18, 0x0,

    /* U+8363 "荣" */
    0x8, 0x2a, 0x1c, 0x67, 0xb9, 0xc0, 0x8e, 0x30,
    0xf0, 0x5e, 0x51, 0x90, 0x10
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 0, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 64, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 64, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 128, .box_w = 2, .box_h = 11, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 6, .adv_w = 128, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 8, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 13, .adv_w = 128, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 20, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 29, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 128, .box_w = 1, .box_h = 3, .ofs_x = 5, .ofs_y = 8},
    {.bitmap_index = 39, .adv_w = 128, .box_w = 3, .box_h = 11, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 44, .adv_w = 128, .box_w = 3, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 48, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 52, .adv_w = 128, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 57, .adv_w = 128, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 58, .adv_w = 128, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 60, .adv_w = 128, .box_w = 1, .box_h = 1, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 128, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 75, .adv_w = 128, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 78, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 128, .box_w = 5, .box_h = 9, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 90, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 106, .adv_w = 128, .box_w = 5, .box_h = 9, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 112, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 119, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 127, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 128, .box_w = 1, .box_h = 5, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 135, .adv_w = 128, .box_w = 2, .box_h = 5, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 128, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 150, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 157, .adv_w = 128, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 162, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 205, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 213, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 228, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 235, .adv_w = 128, .box_w = 3, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 255, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 154, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 128, .box_w = 5, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 304, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 312, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 319, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 326, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 334, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 342, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 351, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 359, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 368, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 378, .adv_w = 128, .box_w = 3, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 382, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 390, .adv_w = 128, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 394, .adv_w = 128, .box_w = 3, .box_h = 3, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 396, .adv_w = 128, .box_w = 7, .box_h = 2, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 398, .adv_w = 128, .box_w = 1, .box_h = 3, .ofs_x = 4, .ofs_y = 9},
    {.bitmap_index = 399, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 412, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 417, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 425, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 439, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 446, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 454, .adv_w = 128, .box_w = 2, .box_h = 8, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 456, .adv_w = 128, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 460, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 469, .adv_w = 128, .box_w = 2, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 472, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 479, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 128, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 489, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 498, .adv_w = 128, .box_w = 4, .box_h = 11, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 504, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 509, .adv_w = 128, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 514, .adv_w = 128, .box_w = 5, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 520, .adv_w = 128, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 525, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 531, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 539, .adv_w = 128, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 544, .adv_w = 128, .box_w = 5, .box_h = 9, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 550, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 128, .box_w = 4, .box_h = 14, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 563, .adv_w = 128, .box_w = 1, .box_h = 14, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 565, .adv_w = 128, .box_w = 3, .box_h = 13, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 570, .adv_w = 128, .box_w = 7, .box_h = 3, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 573, .adv_w = 256, .box_w = 12, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 590, .adv_w = 256, .box_w = 10, .box_h = 13, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 607, .adv_w = 256, .box_w = 9, .box_h = 9, .ofs_x = 4, .ofs_y = 1},
    {.bitmap_index = 618, .adv_w = 256, .box_w = 14, .box_h = 15, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 645, .adv_w = 256, .box_w = 8, .box_h = 13, .ofs_x = 4, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0xd
};

static const uint16_t unicode_list_2[] = {
    0x0, 0x104f, 0x163f, 0x1f58, 0x2617
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 0, .range_length = 14, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 3,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 23884, .range_length = 9752, .glyph_id_start = 98,
        .unicode_list = unicode_list_2, .glyph_id_ofs_list = NULL, .list_length = 5, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 3,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_my_font = {
#else
lv_font_t lv_my_font = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    // .static_bitmap = 0,
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_MY_FONT*/
