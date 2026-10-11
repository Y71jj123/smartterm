#include "lvgl/lvgl.h"          
#include "lvgl/src/misc/lv_fs.h"  
#include <stdio.h>               
#include <stdlib.h>              
#include <stdint.h>              
#include <string.h>              
#include <time.h>                
#include <unistd.h>              
#include "game_pvz.h"          

/* --- 屏幕尺寸 --- */
#define SCR_W     1024            /* 屏幕宽度（像素） */
#define SCR_H     600             /* 屏幕高度（像素） */
#define TOP_H     110             /* 顶部 UI 区域高度

/* --- 草坪网格 --- */
#define COLS      9              /* 草坪列数（9列，和原版 PVZ 一致） */
#define ROWS      5              /* 草坪行数（5行 = 5条跑道） */
#define GRID_X0   80              /* 草坪左上角 X 坐标：(1024 - 9*96) / 2 = 80，居中放置 */
#define GRID_Y0   115             /* 草坪左上角 Y 坐标：在顶部 UI 下方 */
#define CELL_W    96              /* 每个格子的宽度（像素） */
#define CELL_H    92              /* 每个格子的高度（像素） */

/* --- 顶部 UI 坐标（从左到右依次排布，避免重叠） --- */
#define BANK_X    8               /* 阳光面板左上角 X */
#define BANK_Y    30               /* 阳光面板左上角 Y */
#define BANK_W    104              /* 阳光面板宽度 */
#define BANK_H    48               /* 阳光面板高度 */
#define BANK_LBL_X 58              /* 阳光数字标签的 X 坐标（面板内部偏右居中） */
#define BANK_LBL_Y 38              /* 阳光数字标签的 Y 坐标 */

#define CARD_X0   130             /* 第 1 张卡片左上角 X；5 张卡片从 X=130 排到 X=458 */
#define CARD_Y    10               /* 卡片左上角 Y */
#define CARD_W    64               /* 卡片宽度 */
#define CARD_H    88               /* 卡片高度 */
#define CARD_GAP  66              /* 卡片间距（相邻卡片左上角 X 的差值） */

#define SHOVEL_X  486             /* 铲子左上角 X（卡片右侧） */
#define SHOVEL_Y  30              /* 铲子左上角 Y */

#define WAVE_X    572             /* 波次面板左上角 X */
#define WAVE_Y    30              /* 波次面板左上角 Y */
#define WAVE_LBL_X 636            /* 波次数字标签 X */
#define WAVE_LBL_Y 38             /* 波次数字标签 Y */

/* 波次进度条：竖放在草坪右侧空白区（草坪右边界 944 ~ 屏幕 1024），仿原版 PVZ */
#define BAR_X    970              /* 进度条左上角 X（草坪右侧空白区） */
#define BAR_Y    150              /* 进度条左上角 Y */
#define BAR_W    28               /* 进度条宽度（竖直方向） */
#define BAR_H    340              /* 进度条高度 */

/* --- 右上角按钮坐标 --- */
#define BTN_RESTART_X 754         /* 重启按钮 X */
#define BTN_RESTART_Y 30          /* 重启按钮 Y */
#define BTN_RESTART_W 88           /* 重启按钮宽 */
#define BTN_RESTART_H 48           /* 重启按钮高 */

#define BTN_EXIT_X  854            /* 退出按钮 X（紧贴重启按钮右侧） */
#define BTN_EXIT_Y  30             /* 退出按钮 Y */
#define BTN_EXIT_W  88             /* 退出按钮宽 */
#define BTN_EXIT_H  48             /* 退出按钮高 */

/* --- 实体尺寸（植物/僵尸/子弹/阳光，单位=像素） --- */
#define PLANT_W   82              /* 植物宽度（跟 96px 格子匹配，略小留边距） */
#define PLANT_H   92              /* 植物高度 */
#define ZOMBIE_W  72              /* 普通僵尸宽度 */
#define ZOMBIE_H  108             /* 普通僵尸高度 */
#define BOSS_W    120             /* 僵王宽度（奶龙造型，比普通僵尸大一圈） */
#define BOSS_H    150             /* 僵王高度 */
#define BOSS_SUMMON_CD   9000     /* 僵王召唤小弟的冷却间隔（毫秒） */
#define BOSS_SUMMON_MAX  4       /* 单只僵王最多召唤几只小弟（防止无限刷导致清不完场） */
#define BULLET_SZ 28              /* 子弹尺寸（豌豆/雪球的宽高） */
#define SUN_SZ    62              /* 阳光尺寸（宽高） */

/* ============================================================
 *  BMP 资源路径探测
 * ============================================================
 *  LVGL 的文件系统用盘符 "A:" 访问开发板上的文件。
 *  开发板上 BMP 文件夹的位置不确定，所以准备多个候选路径，
 *  启动时依次尝试打开 bank.bmp，第一个成功的就用。
 *  ============================================================ */
static const char *RES_CANDIDATES[] = {
    "A:/jing/pvzBMP",       /* 板子：A: 映射根目录 /，即 /jing/pvzBMP */
    "./pvzBMP",             /* 本地调试：程序同级目录 */
    NULL                    /* 哨兵：NULL 表示数组结束 */
};
/* 实际使用的资源路径前缀（pvz_res_init 中会根据探测结果更新） */
static char g_res_prefix[64] = "A:/jing/pvzBMP";

/* ============================================================
 *  资源文件名（不带路径和 .bmp 后缀，pvz_path 函数会自动拼接）
 * ============================================================
 *  每个 #define 就是一个 BMP 文件的名字（不含 .bmp 后缀）。
 *  调用 pvz_set_img_src(obj, F_BG) 时会自动拼成 "A:/jing/pvzBMP/bg.bmp"
 * ============================================================ */

/* --- UI 静态资源 --- */
#define F_BG              "bg"             /* 游戏背景图 */
#define F_BANK            "bank"           /* 阳光面板背景图 */
#define F_WAVE            "wave"           /* 波次面板背景图 */
#define F_BTN_RESTART     "btn_restart"   /* 重启按钮图 */
#define F_BTN_EXIT        "btn_exit"       /* 退出按钮图 */
#define F_SHOVEL          "shovel"         /* 铲子图 */
#define F_BOOM            "boom"           /* 樱桃爆炸动画图 */
#define F_BULLET_PEA      "bullet_pea"    /* 豌豆子弹图 */
#define F_BULLET_SNOW     "bullet_snow"   /* 雪球子弹图（冰豌豆） */
#define F_SUN_1           "sun_1"          /* 阳光第 1 帧 */
#define F_SUN_2           "sun_2"          /* 阳光第 2 帧（旋转动画切换） */
#define F_MOWER           "mower"          /* 小推车图 */
#define F_WIN             "win"            /* 胜利横幅图 */
#define F_LOSE            "lose"           /* 失败横幅图 */

/* --- 植物图片 --- */
#define F_PLANT_SUN       "plant_sunflower"            /* 向日葵（静态，只有1帧） */
#define F_PLANT_PEA1      "plant_peashooter_1"         /* 豌豆射手第 1 帧（射击前） */
#define F_PLANT_PEA2      "plant_peashooter_2"         /* 豌豆射手第 2 帧（射击后坐力） */
#define F_PLANT_SNOW1     "plant_snowpea_1"            /* 冰豌豆射手第 1 帧 */
#define F_PLANT_SNOW2     "plant_snowpea_2"            /* 冰豌豆射手第 2 帧 */
#define F_PLANT_NUT1      "plant_wallnut_1"            /* 坚果第 1 阶段（满血） */
#define F_PLANT_NUT2      "plant_wallnut_2"            /* 坚果第 2 阶段（中等损伤） */
#define F_PLANT_NUT3      "plant_wallnut_3"            /* 坚果第 3 阶段（严重损伤） */
#define F_PLANT_CHERRY    "plant_cherry"                /* 樱桃炸弹 */

/* --- 僵尸图片（每种僵尸有走路2帧+吃植物1帧） --- */
#define F_ZOMBIE1_W1      "zombie1_w1"     /* 普通僵尸 走路第1帧 */
#define F_ZOMBIE1_W2      "zombie1_w2"     /* 普通僵尸 走路第2帧 */
#define F_ZOMBIE1_EAT     "zombie1_eat"    /* 普通僵尸 吃植物 */
#define F_ZOMBIE2_W1      "zombie2_w1"     /* 路障僵尸 走路第1帧 */
#define F_ZOMBIE2_W2      "zombie2_w2"     /* 路障僵尸 走路第2帧 */
#define F_ZOMBIE2_EAT     "zombie2_eat"    /* 路障僵尸 吃植物 */
#define F_ZOMBIE3_W1      "zombie3_w1"     /* 铁桶僵尸 走路第1帧 */
#define F_ZOMBIE3_W2      "zombie3_w2"     /* 铁桶僵尸 走路第2帧 */
#define F_ZOMBIE3_EAT     "zombie3_eat"    /* 铁桶僵尸 吃植物 */

/* --- 僵王图片（仅最后一波出现，奶龙造型） --- */
#define F_ZOMBIE_BOSS_W1  "zombie_boss_w1"  /* 僵王 走路第1帧 */
#define F_ZOMBIE_BOSS_W2  "zombie_boss_w2"  /* 僵王 走路第2帧 */
#define F_ZOMBIE_BOSS_EAT "zombie_boss_eat" /* 僵王 吃植物 */

/* ============================================================
 *  植物类型枚举
 * ============================================================
 *  每种植物对应一个编号，用于数组索引和 switch 判断。
 *  PT_NONE=0 表示空格子（没种植物）。
 * ============================================================ */
typedef enum {
    PT_NONE = 0,       /* 空（格子没种植物） */
    PT_SUNFLOWER,      /* 向日葵：每 8 秒产 25 阳光 */
    PT_PEASHOOTER,     /* 豌豆射手：每 1.5 秒射豌豆，伤害 20 */
    PT_WALLNUT,        /* 坚果：400 血，纯肉盾，三阶段外观 */
    PT_CHERRY,         /* 樱桃炸弹：1.2 秒后爆炸，3×3 范围 900 伤害 */
    PT_SNOWPEA,        /* 冰豌豆：射雪球，伤害 20 + 减速 5 秒 */
    PT_COUNT           /* 植物种类总数（用于数组大小） */
} plant_type_t;

/* ============================================================
 *  植物属性配置表（每种植物的"身份证"）
 * ============================================================
 *  每个字段含义：
 *    cost           花费多少阳光
 *    hp             血量
 *    cooldown_ms    卡片冷却时间（种一次后等多久才能再种）
 *    shoot_interval 射击间隔（毫秒），0=不射击
 *    produce_interval 阳光生产间隔（毫秒），0=不产阳光
 *    fuse_ms        樱桃引信时间（毫秒），0=不是樱桃
 *    bullet_dmg     子弹伤害
 *    bullet_slow_ms 子弹减速时长（毫秒），0=不减速
 *    card_file      卡片图片文件名
 *    in_game[2]     射手类两帧动画（射击前/后坐力），NULL=没有动画
 *    in_game_static 静态植物图片（向日葵/坚果/樱桃），NULL=用动画帧
 * ============================================================ */
typedef struct {
    int          cost;             /* 阳光花费 */
    int          hp;               /* 血量 */
    int          cooldown_ms;      /* 卡片冷却时间 */
    int          shoot_interval;   /* 射击间隔(ms)，0=不射击 */
    int          produce_interval; /* 阳光生产间隔(ms)，0=不产阳光 */
    int          fuse_ms;          /* 樱桃引信(ms)，0=非樱桃 */
    int          bullet_dmg;       /* 子弹伤害 */
    int          bullet_slow_ms;   /* 子弹减速时长(ms) */
    const char  *card_file;        /* 卡片图片名 */
    const char  *in_game[2];       /* 射手类两帧动画图 */
    const char  *in_game_static;   /* 静态植物图（非射手类） */
} plant_info_t;

/* 植物配置表：用 [PT_XXX] 索引，一次查到该植物的所有属性 */
static const plant_info_t PLANT_INFO[PT_COUNT] = {
    [PT_NONE]        = {0},       /* 空格子，无属性 */
    [PT_SUNFLOWER]   = { 50, 100,  7000, 0,    8000, 0,   0, 0,
                         "card_sunflower", {0,0}, F_PLANT_SUN },
    [PT_PEASHOOTER]  = {100, 100,  7000, 1500, 0,    0,  20, 0,
                         "card_peashooter", {F_PLANT_PEA1, F_PLANT_PEA2}, NULL },
    [PT_WALLNUT]     = { 50, 400, 20000, 0,    0,    0,   0, 0,
                         "card_wallnut", {0,0}, F_PLANT_NUT1 },
    [PT_CHERRY]      = {150, 100, 30000, 0,    0,  1200,   0, 0,
                         "card_cherry", {0,0}, F_PLANT_CHERRY },
    [PT_SNOWPEA]     = {175, 100,  7000, 1500, 0,    0,  20, 5000,
                         "card_snowpea", {F_PLANT_SNOW1, F_PLANT_SNOW2}, NULL }
};

/* 卡片栏的排列顺序（从左到右 5 张卡） */
static const plant_type_t CARD_ORDER[5] = {
    PT_SUNFLOWER, PT_PEASHOOTER, PT_WALLNUT, PT_CHERRY, PT_SNOWPEA
};

/* ============================================================
 *  僵尸类型枚举
 * ============================================================ */
typedef enum {
    ZT_NORMAL,    /* 普通僵尸：100 血，14 px/s */
    ZT_CONE,      /* 路障僵尸：280 血（头上顶橙色路障） */
    ZT_BUCKET,    /* 铁桶僵尸：650 血（身上裹铁桶） */
    ZT_BOSS,      /* 僵王（奶龙造型）：8000 血，仅最后一波出现 */
    ZT_COUNT      /* 僵尸种类总数 */
} zombie_type_t;

/* ============================================================
 *  实体结构定义（植物/子弹/阳光/推车/僵尸的"数据结构"）
 * ============================================================
 *  每个结构体描述一种游戏对象在内存中的数据布局。
 *  img 字段是 LVGL 图片控件指针，用来在屏幕上显示。
 * ============================================================ */

/* --- 植物实体 --- */
typedef struct {
    plant_type_t type;     /* 植物类型 */
    int          row, col; /* 所在草坪行列 */
    int          hp, hp_max;  /* 当前血量 / 最大血量 */
    int          shoot_t;     /* 射击计时器：累加 dt，到 shoot_interval 就开火 */
    int          prod_t;      /* 阳光生产计时器：累加 dt，到 produce_interval 就产阳光 */
    int          fuse_t;      /* 樱桃引信倒计时：到 0 就爆炸 */
    int          anim_t;      /* 动画计时器：累加 dt，到阈值就切帧 */
    int          frame;       /* 当前帧 0 或 1（射手后坐力恢复用） */
    int          wallnut_stage;/* 坚果外观阶段 1/2/3（满血/中等/严重损伤） */
    bool         dead;        /* 是否已死亡 */
    lv_obj_t    *img;         /* LVGL 图片控件指针（屏幕上显示的植物图片） */
} plant_t;

/* --- 子弹实体 --- */
typedef struct {
    float        x;       /* 当前 X 坐标（浮点，因为要平滑移动） */
    int          row;     /* 所在行（子弹沿同一行水平飞行） */
    int          dmg;     /* 伤害值 */
    int          slow_ms; /* 减速时长（雪球才有，豌豆为 0） */
    bool         ice;     /* true=雪球（冰豌豆），false=豌豆 */
    bool         dead;    /* 是否已销毁 */
    lv_obj_t    *img;     /* LVGL 图片控件指针 */
} bullet_t;

/* --- 阳光实体 --- */
typedef struct {
    float        x, y;      /* 当前坐标 */
    float        vy;       /* 垂直速度（下落用） */
    int          target_y;  /* 落地目标 Y 坐标 */
    int          value;     /* 点击后获得多少阳光 */
    int          phase;     /* 旋转动画相位 0/1（两帧切换） */
    int          life;      /* 落地后停留计时（到 3.5 秒自动消失） */
    bool         falling;   /* true=正在下落，false=已落地 */
    bool         dead;      /* 是否已销毁 */
    lv_obj_t    *img;       /* LVGL 图片控件指针 */
} sun_t;

/* --- 小推车实体 --- */
typedef struct {
    int          row;     /* 所在行 */
    float        x;       /* 当前 X 坐标 */
    float        vx;      /* 水平速度（被触发后 320 px/s 向右冲） */
    bool         armed;    /* 是否还有（没被用过） */
    bool         active;  /* 是否正在运行（推出去碾僵尸） */
    bool         dead;    /* 是否已销毁 */
    lv_obj_t    *img;     /* LVGL 图片控件指针 */
} mower_t;

/* --- 僵尸实体 --- */
typedef struct {
    int          row;     /* 所在行 */
    float        x;       /* 当前 X 坐标（从右往左走，逐渐减小） */
    zombie_type_t type;   /* 僵尸类型 */
    int          w, h;    /* 宽高（普通 72x108，僵王 120x150） */
    int          hp, hp_max;  /* 当前血量 / 最大血量 */
    int          speed;   /* 移动速度（px/s，普通 14，僵王 8） */
    int          eat_t;   /* 吃植物计时器：每 1000ms 扣一次伤害 */
    int          slow_t;  /* 减速剩余时间（被雪球打中后>0，期间速度减半） */
    int          anim_t;  /* 走路动画计时器：到 350ms 切一帧 */
    bool         eating;  /* 是否正在啃植物 */
    bool         dead;    /* 是否已死亡 */
    bool         gear_broken; /* 装备是否已碎（路障/铁桶被打掉后变普通僵尸） */
    int          summon_t;     /* 僵王召唤计时器（ms） */
    int          summon_count;  /* 僵王已召唤的小弟数量 */
    bool         is_minion;     /* 是否是僵王召唤的（不计入波次进度统计） */
    lv_obj_t    *img;     /* LVGL 图片控件指针 */
} zombie_t;

/* ============================================================
 *  全局状态
 * ============================================================
 *  所有游戏数据都用 static 全局变量保存，整个文件可见。
 *  重新开始游戏时，pvz_init() 会把所有变量重置为初始值。
 * ============================================================ */

/* --- 实体数组容量上限（防止无限分配内存） --- */
#define MAX_ZOMBIES 40    /* 最多同时存在 40 只僵尸 */
#define MAX_BULLETS 80    /* 最多同时存在 80 颗子弹 */
#define MAX_SUNS    16    /* 最多同时存在 16 个阳光 */

/* --- 实体数组（游戏中的所有对象都存在这些数组里） --- */
static plant_t  g_plants[ROWS][COLS];  /* 植物：5行×9列，每个格子最多1株 */
static zombie_t g_zombies[MAX_ZOMBIES]; /* 僵尸池 */
static bullet_t g_bullets[MAX_BULLETS]; /* 子弹池 */
static sun_t    g_suns[MAX_SUNS];        /* 阳光池 */
static mower_t  g_mowers[ROWS];          /* 小推车：每行1辆，共5辆 */

/* --- 玩家状态 --- */
static int      g_sun_amount = 100;       /* 当前阳光数量（初始 100） */
static int      g_selected_card = -1;     /* 当前选中的卡片索引(-1=没选中) */
static bool     g_shovel_selected = false; /* 铲子是否被选中 */
static bool     g_game_over = false;      /* 游戏是否失败 */
static bool     g_game_won = false;       /* 游戏是否胜利 */

/* --- 卡片冷却计时器（5 张卡各一个） --- */
static int      g_card_cd_t[5] = {0};    /* >0 表示正在冷却，每 tick 减 30 */

/* --- 波次系统状态 --- */
static int      g_wave_index = 0;              /* 当前波次（1~5） */
static int      g_wave_to_spawn = 0;            /* 本波还需生成多少只僵尸 */
static int      g_wave_spawn_t = 0;             /* 生成间隔计时器 */
static int      g_wave_spawn_interval = 3500;   /* 间隔多少 ms 生成一只 */
static int      g_wave_between_t = 0;           /* 波次间倒计时 */
static bool     g_wave_active = false;          /* 是否正在刷僵尸 */
static const int TOTAL_WAVES = 5;               /* 总波数 */

static int      g_sky_sun_t = 0;        /* 天空掉落阳光的计时器 */
static int      g_tick_count = 0;       /* 全局 tick 计数（用于动画相位计算） */

/* ============================================================
 *  UI 控件句柄（指向屏幕上各个 LVGL 控件的指针）
 * ============================================================
 *  这些都是 lv_obj_t* 指针，在 pvz_init() 中创建并赋值。
 *  后续通过这些指针来更新控件的显示内容（改文字、改位置、改图片等）。
 * ============================================================ */
static lv_obj_t *g_bg_obj;               /* 背景图 */
static lv_obj_t *g_bank_obj;             /* 阳光面板图 */
static lv_obj_t *g_sun_num_label;        /* 阳光数字标签(Label) */
static lv_obj_t *g_wave_obj;             /* 波次面板图 */
static lv_obj_t *g_wave_num_label;       /* 波次数字标签(Label) */
static lv_obj_t *g_wave_bar;             /* 波次进度条(Bar) */
static lv_obj_t *g_wave_bar_label;      /* 进度条文字标签：显示 "3/5" 或 "FINAL" */
static lv_obj_t *g_shovel_obj;           /* 铲子容器(Object) */
static lv_obj_t *g_btn_restart;          /* 重启按钮(Object) */
static lv_obj_t *g_btn_exit;             /* 退出按钮(Object) */
static lv_obj_t *g_banner_obj;           /* 胜负横幅(Image) */
static lv_obj_t *g_card_objs[5];         /* 5 张卡片容器(Object) */
static lv_obj_t *g_card_img[5];          /* 5 张卡片内的图片(Image) */
static lv_obj_t *g_card_cooldown[5];    /* 5 张卡片的冷却遮罩(Object) */
static lv_obj_t *g_cells[ROWS][COLS];   /* 45 个草坪格子(Object) */

/* ============================================================
 *  样式（用 lv_style_t 定义，多个控件可共享同一个样式）
 * ============================================================ */
static lv_style_t g_st_card;          /* 卡片普通态：透明背景、无边框 */
static lv_style_t g_st_card_sel;      /* 卡片选中态：金色边框 */
static lv_style_t g_st_overlay;      /* 冷却遮罩：半透明黑色覆盖在卡片上 */
static lv_style_t g_st_overlay_dim;  /* 阳光不足遮罩（更淡的黑） */
static lv_style_t g_st_cell;         /* 草坪格子：透明、无边框（只做点击区） */
static lv_style_t g_st_banner;       /* 胜负横幅：透明、无边框 */

/* ============================================================
 *  工具函数
 * ============================================================ */

/*
 * pvz_path：拼接 BMP 文件的完整路径
 *   name = 资源名（如 "bg"），out = 输出缓冲区，n = 缓冲区大小
 *   结果示例："A:/jing/pvzBMP/bg.bmp"
 */
static void pvz_path(const char *name, char *out, int n)
{
    snprintf(out, n, "%s/%s.bmp", g_res_prefix, name);
}

/*
 * pvz_resource_exists：检测 BMP 资源文件夹是否存在
 *   用 LVGL 文件系统 API 打开 bank.bmp，能打开说明路径正确。
 *   lv_fs_open / lv_fs_close：LVGL 文件系统接口
 *   LV_FS_MODE_RD：只读模式打开
 *   LV_FS_RES_OK：打开成功
 */
static bool pvz_resource_exists(void)
{
    char p[128];
    pvz_path(F_BANK, p, sizeof(p));
    lv_fs_file_t f;
    if (lv_fs_open(&f, p, LV_FS_MODE_RD) == LV_FS_RES_OK) {
        lv_fs_close(&f);
        return true;
    }
    return false;
}

/*
 * pvz_res_init：初始化资源路径
 *   遍历候选路径数组，找到第一个能打开 bank.bmp 的路径。
 *   找到后更新 g_res_prefix 并返回；全部找不到就打印错误提示。
 */
static void pvz_res_init(void)
{
    for (int i = 0; RES_CANDIDATES[i]; ++i) {
        strncpy(g_res_prefix, RES_CANDIDATES[i], sizeof(g_res_prefix) - 1);
        if (pvz_resource_exists()) {
            printf("[PVZ] resources OK: %s/bank.bmp\n", g_res_prefix);
            return;
        }
    }
    strncpy(g_res_prefix, RES_CANDIDATES[0], sizeof(g_res_prefix) - 1);
    printf("[PVZ] ================================================\n");
    printf("[PVZ] ERROR: bank.bmp NOT FOUND! Tried:\n");
    for (int i = 0; RES_CANDIDATES[i]; ++i)
        printf("[PVZ]   %s/bank.bmp\n", RES_CANDIDATES[i]);
    printf("[PVZ] Please copy the pvzBMP folder to the board, e.g.:\n");
    printf("[PVZ]   /jing/pvzBMP/bank.bmp   (next to 2048BMP)\n");
    printf("[PVZ] ================================================\n");
    fflush(stdout);
}

/*
 * pvz_set_img_src：给图片控件设置 BMP 源
 *   obj = LVGL 图片控件指针，name = 资源名（如 "bg"）
 *   内部调用 pvz_path 拼路径，再调用 lv_image_set_src 设置图片源
 */
static void pvz_set_img_src(lv_obj_t *obj, const char *name)
{
    char p[128];
    pvz_path(name, p, sizeof(p));
    lv_image_set_src(obj, p);
}

/* ============================================================
 *  子弹 / 阳光 / 僵尸 / 小推车的创建与销毁
 * ============================================================
 *  这些函数负责在实体数组中找空位（dead=true 的槽位），
 *  填充属性，创建 LVGL 图片控件显示在屏幕上。
 *  销毁时删掉 LVGL 控件并标记 dead=true，让槽位可复用。
 * ============================================================ */

/* --- 子弹 --- */

/* pvz_bullet_get_slot：在子弹池中找一个已销毁的空位 */
static bullet_t *pvz_bullet_get_slot(void)
{
    for (int i = 0; i < MAX_BULLETS; ++i)
        if (g_bullets[i].dead) return &g_bullets[i];
    return NULL;   /* 池满了，返回 NULL，调用者会跳过 */
}

/*
 * pvz_spawn_bullet：创建一颗子弹
 *   row = 所在行，x/y = 起始坐标，dmg = 伤害，slow_ms = 减速时长，ice = 是否雪球
 *   用到 LVGL 控件：lv_image_create（创建图片控件显示子弹图片）
 */
static void pvz_spawn_bullet(int row, int x, int y, int dmg, int slow_ms, bool ice)
{
    bullet_t *b = pvz_bullet_get_slot();
    if (!b) return;
    b->row = row;
    b->x = x;
    b->dmg = dmg;
    b->slow_ms = slow_ms;
    b->ice = ice;
    b->dead = false;
    /* lv_image_create(父对象)：创建图片控件，父对象=活动屏幕 */
    b->img = lv_image_create(lv_screen_active());
    /* 根据是否冰属性选择不同子弹图片 */
    pvz_set_img_src(b->img, ice ? F_BULLET_SNOW : F_BULLET_PEA);
    lv_obj_set_pos(b->img, x, y);
}

/* --- 阳光 --- */

/* pvz_sun_get_slot：在阳光池中找一个已销毁的空位 */
static sun_t *pvz_sun_get_slot(void)
{
    for (int i = 0; i < MAX_SUNS; ++i)
        if (g_suns[i].dead) return &g_suns[i];
    return NULL;
}

static void pvz_sun_click_cb(lv_event_t *e);   /* 前向声明：点击阳光的回调函数 */

/*
 * pvz_spawn_sun：创建一个阳光
 *   sx = X 坐标，ty = 目标 Y 坐标，value = 点击后获得的阳光值，from_sky = 从天空落 or 植物产出
 *   用到 LVGL 控件：
 *     lv_image_create（图片控件显示阳光图片）
 *     lv_obj_add_flag（添加可点击标志 LV_OBJ_FLAG_CLICKABLE）
 *     lv_obj_add_event_cb（注册点击事件 → pvz_sun_click_cb）
 */
static void pvz_spawn_sun(int sx, int ty, int value, bool from_sky)
{
    sun_t *s = pvz_sun_get_slot();
    if (!s) return;
    s->x = sx;
    if (from_sky) {
        s->y = 96;                    /* 从屏幕顶部开始下落 */
        s->vy = 30 + (rand() % 25);  /* 随机下落速度 30~54 px/s */
    } else {
        s->y = ty;
        s->vy = -55;                  /* 植物产出：先向上弹起再回落 */
    }
    s->target_y = ty;
    s->value = value;
    s->phase = rand() % 2;           /* 随机初始帧 */
    s->life = 0;
    s->falling = from_sky;
    s->dead = false;
    /* 创建图片控件显示阳光 */
    s->img = lv_image_create(lv_screen_active());
    pvz_set_img_src(s->img, s->phase ? F_SUN_2 : F_SUN_1);
    /* lv_obj_add_flag：给阳光图片添加"可点击"标志，否则点击事件不会触发 */
    lv_obj_add_flag(s->img, LV_OBJ_FLAG_CLICKABLE);
    /* lv_obj_add_event_cb：注册点击事件，点击阳光时调用 pvz_sun_click_cb 收集阳光 */
    lv_obj_add_event_cb(s->img, pvz_sun_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_pos(s->img, sx, (int)s->y);
}

/* --- 僵尸 --- */

/* pvz_zombie_get_slot：在僵尸池中找一个已销毁的空位 */
static zombie_t *pvz_zombie_get_slot(void)
{
    for (int i = 0; i < MAX_ZOMBIES; ++i)
        if (g_zombies[i].dead) return &g_zombies[i];
    return NULL;
}

/*
 * pvz_zombie_sprite_name：根据僵尸类型和状态返回对应的图片文件名
 *   walking = true 返回走路帧，false 返回吃植物帧
 *   装备击碎后(gear_broken)统一显示普通僵尸外观
 */
static const char *pvz_zombie_sprite_name(zombie_t *z, bool walking)
{
    if (z->type == ZT_BOSS) {
        return walking ? F_ZOMBIE_BOSS_W1 : F_ZOMBIE_BOSS_EAT;
    }
    int t = z->type;
    if (z->gear_broken) t = ZT_NORMAL;   /* 装备碎了→显示普通僵尸 */
    if (t == ZT_CONE)        return walking ? F_ZOMBIE2_W1 : F_ZOMBIE2_EAT;
    else if (t == ZT_BUCKET) return walking ? F_ZOMBIE3_W1 : F_ZOMBIE3_EAT;
    else                     return walking ? F_ZOMBIE1_W1 : F_ZOMBIE1_EAT;
}

/* pvz_refresh_zombie_sprite：刷新僵尸图片（走路↔吃植物切换时调用） */
static void pvz_refresh_zombie_sprite(zombie_t *z)
{
    pvz_set_img_src(z->img, pvz_zombie_sprite_name(z, !z->eating));
}

/*
 * pvz_spawn_zombie：创建一只僵尸
 *   zt = 僵尸类型，row = 所在行，wave = 当前波次（影响血量倍率）
 *   用到 LVGL 控件：lv_image_create（创建图片控件显示僵尸）
 */
static zombie_t *pvz_spawn_zombie(zombie_type_t zt, int row, int wave)
{
    zombie_t *z = pvz_zombie_get_slot();
    if (!z) return NULL;
    z->type = zt;
    z->row = row;
    z->x = (float)SCR_W;     /* 从屏幕右边缘出现 */

    /* 设置血量、体型、速度 */
    int base_hp;
    if (zt == ZT_BOSS) {
        base_hp = 100 * 80;        /* 普通 100，僵王 80 倍 = 8000 */
        z->w = BOSS_W;
        z->h = BOSS_H;
        z->speed = 8;              /* 僵王走得慢一些，给玩家时间 */
    } else {
        base_hp = (zt == ZT_BUCKET) ? 650 : (zt == ZT_CONE) ? 280 : 100;
        z->w = ZOMBIE_W;
        z->h = ZOMBIE_H;
        z->speed = 14;
    }
    /* 每波 +5% 血量（wave 从 1 开始） */
    float mul = 1.0f + 0.05f * (wave - 1);
    z->hp_max = (int)(base_hp * mul + 0.5f);
    z->hp = z->hp_max;

    z->eat_t = 0;
    z->slow_t = 0;
    z->anim_t = 0;
    z->eating = false;
    z->dead = false;
    z->gear_broken = false;
    z->summon_t = 0;
    z->summon_count = 0;
    z->is_minion = false;
    /* 创建图片控件并设置初始图片 */
    z->img = lv_image_create(lv_screen_active());
    pvz_refresh_zombie_sprite(z);
    int y = GRID_Y0 + row * CELL_H + CELL_H - z->h;
    lv_obj_set_pos(z->img, (int)z->x, y);
    return z;
}

/* --- 销毁函数（删掉 LVGL 控件 + 标记 dead=true 释放槽位） --- */

/* pvz_destroy_zombie：删掉僵尸图片，标记死亡 */
static void pvz_destroy_zombie(zombie_t *z)
{
    if (z->img) { lv_obj_delete(z->img); z->img = NULL; }
    z->dead = true;
}

/* pvz_destroy_bullet：删掉子弹图片，标记死亡 */
static void pvz_destroy_bullet(bullet_t *b)
{
    if (b->img) { lv_obj_delete(b->img); b->img = NULL; }
    b->dead = true;
}

/* pvz_destroy_sun：删掉阳光图片，标记死亡 */
static void pvz_destroy_sun(sun_t *s)
{
    if (s->img) { lv_obj_delete(s->img); s->img = NULL; }
    s->dead = true;
}

/* pvz_destroy_plant：删掉植物图片，清零该格子的植物数据 */
static void pvz_destroy_plant(int row, int col)
{
    plant_t *p = &g_plants[row][col];
    if (p->img) { lv_obj_delete(p->img); p->img = NULL; }
    memset(p, 0, sizeof(*p));   /* 清零整个植物结构体，type 变回 PT_NONE */
}

/* ============================================================
 *  UI 构建辅助函数（刷新显示内容）
 * ============================================================ */

/* pvz_refresh_sun_label：更新阳光数字标签显示当前阳光数 */
static void pvz_refresh_sun_label(void)
{
    /* lv_label_set_text_fmt：类似 printf 的格式化设置标签文字 */
    lv_label_set_text_fmt(g_sun_num_label, "%d", g_sun_amount);
}

/* pvz_wave_total：计算第 w 波有多少只僵尸（公式：4 + w*2，上限 20） */
static int pvz_wave_total(int w)
{
    int n = 4 + w * 2;
    return n > 20 ? 20 : n;
}

/*
 * pvz_refresh_wave_bar：刷新右侧进度条(Bar控件)
 *
 * 进度计算公式：
 *   进度% = (前几波总数 + 本波已杀数) × 100 / 全场总僵尸数
 *
 * 僵王召唤的小弟(is_minion=true)不计入统计，
 * 否则进度条会被召唤物拖住、僵王没死却永远走不到 100%。
 *
 * 用到 LVGL 控件操作：
 *   lv_bar_set_value：设置进度条数值
 *   lv_label_set_text / lv_label_set_text_fmt：更新标签文字
 *   lv_obj_set_style_text_color：改文字颜色（最后一波飘红）
 *   lv_obj_set_style_bg_color：改进度条颜色（最后一波飘红）
 *   LV_PART_INDICATOR：进度条的"已填充"部分
 *   LV_PART_MAIN：进度条的"未填充"背景部分
 */
static void pvz_refresh_wave_bar(void)
{
    if (!g_wave_bar) return;

    int pct;
    if (g_game_won) {
        pct = 100;    /* 胜利了直接满格 */
    } else {
        /* 计算全场总僵尸数 */
        int total_all = 0;
        for (int w = 1; w <= TOTAL_WAVES; ++w) total_all += pvz_wave_total(w);

        /* 前面几波的总数（已全部消灭） */
        int done_before = 0;
        for (int w = 1; w < g_wave_index; ++w) done_before += pvz_wave_total(w);

        /* 本波已生成数 = 本波总数 - 还没生成的 */
        int wave_total = pvz_wave_total(g_wave_index);
        int spawned    = wave_total - g_wave_to_spawn;
        if (spawned < 0) spawned = 0;

        /* 当前场上还活着的（不算僵王召唤的小弟） */
        int alive = 0;
        for (int i = 0; i < MAX_ZOMBIES; ++i)
            if (!g_zombies[i].dead && !g_zombies[i].is_minion) alive++;

        /* 本波已杀 = 已生成 - 还活着 */
        int killed = spawned - alive;
        if (killed < 0) killed = 0;

        pct = (done_before + killed) * 100 / total_all;
        if (pct > 100) pct = 100;
        if (pct < 0)   pct = 0;
    }

    /* lv_bar_set_value：设置进度条数值，LV_ANIM_OFF=不加动画直接跳 */
    lv_bar_set_value(g_wave_bar, pct, LV_ANIM_OFF);

    if (!g_wave_bar_label) return;
    /* 最后一波（含僵王）：进度条和文字一起飘红 */
    if (g_wave_index >= TOTAL_WAVES) {
        lv_label_set_text(g_wave_bar_label, "FINAL");
        lv_obj_set_style_text_color(g_wave_bar_label, lv_color_hex(0xFF5252), 0);
        /* LV_PART_INDICATOR：进度条的已填充部分 */
        lv_obj_set_style_bg_color(g_wave_bar, lv_color_hex(0xC62828), LV_PART_INDICATOR);
    } else {
        lv_label_set_text_fmt(g_wave_bar_label, "%d/%d", g_wave_index, TOTAL_WAVES);
        lv_obj_set_style_text_color(g_wave_bar_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_color(g_wave_bar, lv_color_hex(0x66BB6A), LV_PART_INDICATOR);
    }
}

/* pvz_refresh_wave_label：更新波次数字标签 + 刷新进度条 */
static void pvz_refresh_wave_label(void)
{
    lv_label_set_text_fmt(g_wave_num_label, "%d", g_wave_index);
    pvz_refresh_wave_bar();
}

/*
 * pvz_refresh_card_visuals：刷新 5 张卡片的视觉状态
 *
 * 每张卡片有 3 个视觉层：
 *   1. 选中高亮：用 LV_STATE_CHECKED 状态触发金色边框样式
 *   2. 冷却遮罩：一个半透明黑色 Object 覆盖在卡片上，高度按冷却进度调整
 *   3. 阳光不足变暗：用 lv_obj_set_style_opa 降低图片透明度
 *
 * 用到 LVGL 操作：
 *   lv_obj_add_state / lv_obj_remove_state：切换选中状态
 *   lv_obj_set_size / lv_obj_set_pos：调整遮罩高度
 *   lv_obj_clear_flag(LV_OBJ_FLAG_HIDDEN) / lv_obj_add_flag(LV_OBJ_FLAG_HIDDEN)：显隐遮罩
 *   lv_obj_set_style_opa：设置透明度（LV_OPA_50=半透明，LV_OPA_COVER=不透明）
 */
static void pvz_refresh_card_visuals(void)
{
    for (int i = 0; i < 5; ++i) {
        plant_type_t pt = CARD_ORDER[i];
        const plant_info_t *info = &PLANT_INFO[pt];

        /* 选中卡片→加 LV_STATE_CHECKED 状态→触发金色边框样式 */
        if (i == g_selected_card)
            lv_obj_add_state(g_card_objs[i], LV_STATE_CHECKED);
        else
            lv_obj_remove_state(g_card_objs[i], LV_STATE_CHECKED);

        /* 冷却遮罩：高度按剩余冷却时间比例调整 */
        lv_obj_t *ov = g_card_cooldown[i];
        if (g_card_cd_t[i] > 0 && info->cooldown_ms > 0) {
            int h = CARD_H * g_card_cd_t[i] / info->cooldown_ms;
            if (h < 0) h = 0;
            if (h > CARD_H) h = CARD_H;
            lv_obj_set_size(ov, CARD_W, h);
            lv_obj_set_pos(ov, 0, 0);
            /* lv_obj_clear_flag：移除"隐藏"标志→显示遮罩 */
            lv_obj_clear_flag(ov, LV_OBJ_FLAG_HIDDEN);
        } else {
            /* lv_obj_add_flag：添加"隐藏"标志→隐藏遮罩（冷却结束） */
            lv_obj_add_flag(ov, LV_OBJ_FLAG_HIDDEN);
        }

        /* 阳光不足时卡片变暗（半透明），够时正常显示（不透明） */
        if (g_sun_amount < info->cost)
            lv_obj_set_style_opa(g_card_img[i], LV_OPA_50, 0);    /* 半透明=变暗 */
        else
            lv_obj_set_style_opa(g_card_img[i], LV_OPA_COVER, 0); /* 不透明=正常 */
    }
}

/* ============================================================
 *  事件回调函数（用户点击屏幕时 LVGL 自动调用）
 * ============================================================
 *  每个回调函数的参数都是 lv_event_t *e，由 LVGL 在事件触发时传入。
 *  通过 lv_event_get_user_data 可以拿到注册事件时传进去的用户数据。
 * ============================================================ */

/*
 * pvz_card_click_cb：点击卡片
 *   注册时传入的 user_data 是卡片索引(0~4)
 *   点击卡片→选中该卡片（高亮），再点一次→取消选中
 *   如果之前选了铲子，先取消铲子
 */
static void pvz_card_click_cb(lv_event_t *e)
{
    /* lv_event_get_user_data：取出注册事件时传入的卡片索引 */
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (g_game_over || g_game_won) return;

    /* 如果之前选了铲子，取消铲子选中状态 */
    if (g_shovel_selected) {
        g_shovel_selected = false;
        lv_obj_remove_state(g_shovel_obj, LV_STATE_CHECKED);
    }
    /* 再次点击已选中的卡片→取消选中 */
    if (g_selected_card == idx) {
        g_selected_card = -1;
    } else {
        g_selected_card = idx;
    }
    pvz_refresh_card_visuals();
}

/*
 * pvz_cell_click_cb：点击草坪格子
 *   注册时传入的 user_data 是 row*10+col 编码的行列号
 *   两种操作模式：
 *     铲子模式→移除该格子的植物
 *     选卡模式→在该格子种植物（扣阳光+启动冷却）
 *
 *   用到 LVGL 控件：lv_image_create（创建植物图片）
 */
static void pvz_cell_click_cb(lv_event_t *e)
{
    if (g_game_over || g_game_won) return;
    /* 解码行列号：rc / 10 = 行，rc % 10 = 列 */
    int rc = (int)(intptr_t)lv_event_get_user_data(e);
    int row = rc / 10;
    int col = rc % 10;
    plant_t *p = &g_plants[row][col];

    /* ---- 铲子模式：移除植物 ---- */
    if (g_shovel_selected) {
        if (p->type != PT_NONE) {
            pvz_destroy_plant(row, col);
            g_shovel_selected = false;
            lv_obj_remove_state(g_shovel_obj, LV_STATE_CHECKED);
        }
        return;
    }

    /* ---- 选卡模式：种植物 ---- */
    if (g_selected_card >= 0) {
        if (p->type != PT_NONE) return;        /* 已占用 */
        if (g_card_cd_t[g_selected_card] > 0) return;  /* 还在冷却 */
        plant_type_t pt = CARD_ORDER[g_selected_card];
        const plant_info_t *info = &PLANT_INFO[pt];
        if (g_sun_amount < info->cost) return;  /* 阳光不够 */

        /* 扣阳光 + 启动冷却 */
        g_sun_amount -= info->cost;
        g_card_cd_t[g_selected_card] = info->cooldown_ms;

        /* 初始化植物实体数据 */
        p->type = pt;
        p->row = row;
        p->col = col;
        p->hp = info->hp;
        p->hp_max = info->hp;
        p->shoot_t = 0;
        p->prod_t = 0;
        p->fuse_t = info->fuse_ms;
        p->anim_t = 0;
        p->frame = 0;
        p->wallnut_stage = 1;
        p->dead = false;

        /* 创建植物图片控件 */
        p->img = lv_image_create(lv_screen_active());
        if (info->in_game_static)
            pvz_set_img_src(p->img, info->in_game_static);  /* 静态植物（向日葵/坚果/樱桃） */
        else
            pvz_set_img_src(p->img, info->in_game[0]);      /* 射手第一帧 */

        /* 计算植物在格子中的位置（水平居中，底部对齐格子） */
        int x = GRID_X0 + col * CELL_W + (CELL_W - PLANT_W) / 2;
        int y = GRID_Y0 + row * CELL_H + CELL_H - PLANT_H;
        lv_obj_set_pos(p->img, x, y);

        /* 刷新阳光数和卡片状态 */
        pvz_refresh_sun_label();
        pvz_refresh_card_visuals();
    }
}

/*
 * pvz_sun_click_cb：点击阳光收集
 *   lv_event_get_current_target：获取触发事件的对象（就是被点击的阳光图片）
 *   遍历阳光池找到匹配的→加阳光→销毁→刷新显示
 */
static void pvz_sun_click_cb(lv_event_t *e)
{
    if (g_game_over || g_game_won) return;
    lv_obj_t *img = lv_event_get_current_target(e);
    for (int i = 0; i < MAX_SUNS; ++i) {
        if (!g_suns[i].dead && g_suns[i].img == img) {
            g_sun_amount += g_suns[i].value;
            pvz_destroy_sun(&g_suns[i]);
            pvz_refresh_sun_label();
            pvz_refresh_card_visuals();
            return;
        }
    }
}

/*
 * pvz_shovel_click_cb：点击铲子
 *   切换铲子的选中/取消状态
 *   选中铲子时取消卡片选中（互斥）
 */
static void pvz_shovel_click_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if (g_game_over || g_game_won) return;
    g_shovel_selected = !g_shovel_selected;
    if (g_shovel_selected) {
        g_selected_card = -1;   /* 互斥：选铲子时取消卡片选中 */
        lv_obj_add_state(g_shovel_obj, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(g_shovel_obj, LV_STATE_CHECKED);
    }
    pvz_refresh_card_visuals();
}

/* 声明主界面函数（在 ui_home.c 中实现）：游戏退出后回主界面 */
extern void ui_home_show(void);

/* 全局唯一 tick 定时器句柄，重开/退出都释放 */
static lv_timer_t *g_tick_timer = NULL;
/* 标记当前是否在游戏界面（防止退出后定时器还在跑） */
static bool       g_in_game    = false;

/*
 * pvz_restart_cb：点击重启按钮
 *   直接重新调用 game_pvz()，里面会清屏+重新初始化
 */
static void pvz_restart_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    game_pvz();
}

/*
 * pvz_exit_cb：点击退出按钮
 *   1. 删掉定时器（停止游戏循环）
 *   2. 清屏
 *   3. 切换回菜单界面
 *   用到 LVGL 控件操作：
 *     lv_timer_delete：删除定时器
 *     lv_obj_clean：清空屏幕上所有控件
 */
static void pvz_exit_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if (g_tick_timer) {
        lv_timer_delete(g_tick_timer);   /* 删掉定时器，停止游戏循环 */
        g_tick_timer = NULL;
    }
    g_in_game = false;
    g_wave_bar = NULL;        /* 置空指针，防止定时器回调访问已删除的控件 */
    g_wave_bar_label = NULL;
    lv_obj_clean(lv_screen_active());   /* 清空屏幕上所有控件 */
    ui_home_show();         /* 回到主界面 */
}

/* ============================================================
 *  战斗逻辑
 * ============================================================ */
static bool pvz_lane_has_zombie_ahead(int row, float from_x)
{
    for (int i = 0; i < MAX_ZOMBIES; ++i) {
        if (!g_zombies[i].dead && g_zombies[i].row == row && g_zombies[i].x > from_x)
            return true;
    }
    return false;
}

static void pvz_zombie_damage(zombie_t *z, int dmg, int slow_ms)
{
    if (z->dead) return;
    z->hp -= dmg;
    /* 装备击碎切换外观 */
    if (z->type == ZT_CONE && !z->gear_broken && z->hp <= 100) {
        z->gear_broken = true;
        z->type = ZT_NORMAL;
        pvz_refresh_zombie_sprite(z);
    } else if (z->type == ZT_BUCKET && !z->gear_broken && z->hp <= 280) {
        z->gear_broken = true;
        z->type = ZT_NORMAL;
        pvz_refresh_zombie_sprite(z);
    }
    if (slow_ms > 0) {
        z->slow_t = slow_ms;
    }
    if (z->hp <= 0) {
        pvz_destroy_zombie(z);
        return;
    }
    /* 坚果三阶段（单独处理） */
    /* 这里其实普通植物也会经过 hp 0 检查 */
}

/* 秒杀伤害：小推车碾压普通僵尸用，直接死 */
#define INSTAKILL_DMG  9999
/*
 * 樱桃炸弹固定伤害：不分普通僵尸还是僵王，一律 700。
 * 之前用 9999 必杀值会把 9600 血的僵王一颗带走，这里改成按血量结算。
 */
#define CHERRY_DMG      900

static void pvz_boom_destroy_later(lv_obj_t *o);   /* forward decl */

static void pvz_explode_cherry(int row, int col)
{
    /* 在樱桃位置生成爆炸动画 */
    lv_obj_t *boom = lv_image_create(lv_screen_active());
    pvz_set_img_src(boom, F_BOOM);
    int cx = GRID_X0 + col * CELL_W + CELL_W / 2;
    int cy = GRID_Y0 + row * CELL_H + CELL_H / 2;
    lv_obj_set_pos(boom, cx - 55, cy - 55);

    /* 删掉樱桃本体 */
    pvz_destroy_plant(row, col);

    /* 3x3 范围内的僵尸全部死亡 */
    for (int i = 0; i < MAX_ZOMBIES; ++i) {
        if (g_zombies[i].dead) continue;
        if (g_zombies[i].row < row - 1 || g_zombies[i].row > row + 1) continue;
            float zx = g_zombies[i].x + g_zombies[i].w / 2;
            float cell_l = GRID_X0 + (col - 1) * CELL_W;
            float cell_r = GRID_X0 + (col + 1) * CELL_W + CELL_W;
            if (zx >= cell_l && zx <= cell_r)
                pvz_zombie_damage(&g_zombies[i], CHERRY_DMG, 0);
    }

    /* 爆炸动画 1s 后删除 */
    pvz_boom_destroy_later(boom);
}

/* 用一个简单的辅助结构跟踪要延迟销毁的 boom 动画 */
#define MAX_BOOMS 8
typedef struct {
    lv_obj_t *img;
    int       t;
} boom_t;
static boom_t g_booms[MAX_BOOMS];

static void pvz_boom_destroy_later(lv_obj_t *o)
{
    for (int i = 0; i < MAX_BOOMS; ++i) {
        if (g_booms[i].img == NULL) {
            g_booms[i].img = o;
            g_booms[i].t = 0;
            return;
        }
    }
}

static void pvz_update_booms(int dt_ms)
{
    for (int i = 0; i < MAX_BOOMS; ++i) {
        if (g_booms[i].img == NULL) continue;
        g_booms[i].t += dt_ms;
        if (g_booms[i].t >= 1000) {
            lv_obj_delete(g_booms[i].img);
            g_booms[i].img = NULL;
        }
    }
}

static void pvz_check_mower(int row)
{
    mower_t *m = &g_mowers[row];
    if (!m->armed || m->active || m->dead) return;
    m->active = true;
    m->vx = 320;        /* 推车速度 */
    m->x = GRID_X0 - 10;
    lv_obj_set_pos(m->img, (int)m->x,
                   GRID_Y0 + row * CELL_H + (CELL_H - 46) / 2);
}

static void pvz_kill_row_zombies(int row)
{
    for (int i = 0; i < MAX_ZOMBIES; ++i) {
        if (g_zombies[i].dead) continue;
        if (g_zombies[i].row == row)
            pvz_zombie_damage(&g_zombies[i], INSTAKILL_DMG, 0);
    }
}

/* ============================================================
 *  主定时器：游戏循环（每 30ms 由 LVGL 定时器自动调用一次）
 * ============================================================
 *  这是整个游戏的心脏函数，所有的游戏逻辑都在这里：
 *
 *    1. 天空阳光   → 每 8 秒从天空掉一个阳光
 *    2. 卡片冷却   → 倒计时递减
 *    3. 阳光实体   → 下落动画 + 落地停留 3.5 秒消失 + 旋转帧切换
 *    4. 植物逻辑   → 向日葵产阳光 / 射手射击 / 樱桃爆炸 / 坚果阶段切换
 *    5. 子弹更新   → 水平移动 + 碰撞检测（打中僵尸扣血）
 *    6. 僵尸更新   → 走路 / 啃植物 / 僵王召唤小弟 / 走路动画 / 到左边界→失败
 *    7. 小推车     → 被触发后向右冲碾僵尸
 *    8. 爆炸动画   → 1 秒后删除
 *    9. 波次进度条 → 刷新显示
 *   10. 波次系统   → 按时间生成僵尸 / 全清后进入下一波 / 5 波全清→胜利
 *   11. 卡片视觉   → 刷新选中/冷却/阳光不足状态
 *
 *  用到 LVGL 控件操作：
 *    lv_obj_set_pos：更新所有实体的屏幕位置（每帧移动）
 *    lv_obj_set_style_opa：樱桃闪烁（半透明↔不透明切换）
 *    pvz_set_img_src → lv_image_set_src：切换动画帧
 *    lv_bar_set_value：更新进度条
 *    lv_label_set_text_fmt：更新标签文字
 *    lv_image_create：胜利/失败时创建横幅图片
 *    lv_obj_add_flag(LV_OBJ_FLAG_CLICKABLE)：胜利后启用重启按钮
 * ============================================================ */
static void pvz_game_tick(lv_timer_t *t)
{
    LV_UNUSED(t);
    const int dt = 30;          /* 固定步长 30ms（和定时器周期一致） */
    g_tick_count += dt;

    if (g_game_over || g_game_won) {
        pvz_update_booms(dt);
        return;
    }

    /* ---- 阳光 ---- */
    g_sky_sun_t += dt;
    if (g_sky_sun_t >= 8000) {
        g_sky_sun_t = 0;
        int sx = GRID_X0 + (rand() % (COLS * CELL_W - SUN_SZ));
        pvz_spawn_sun(sx, 110 + (rand() % 320), 25, true);
    }

    /* ---- 卡片冷却 ---- */
    for (int i = 0; i < 5; ++i) {
        if (g_card_cd_t[i] > 0) {
            g_card_cd_t[i] -= dt;
            if (g_card_cd_t[i] < 0) g_card_cd_t[i] = 0;
        }
    }

    /* ---- 阳光实体更新 ---- */
    for (int i = 0; i < MAX_SUNS; ++i) {
        sun_t *s = &g_suns[i];
        if (s->dead) continue;
        if (!s->img) continue;
        if (s->falling) {
            s->y += s->vy * (dt / 1000.0f);
            if (s->y >= s->target_y) {
                s->y = s->target_y;
                s->falling = false;
                s->vy = 0;
                s->life = 0;
            }
        } else {
            s->life += dt;
            if (s->life >= 3500) {       /* 落地停留 3.5s 后消失（用户要求） */
                pvz_destroy_sun(s);
                continue;
            }
        }
        /* 旋转 */
        int phase_now = (g_tick_count / 400) % 2;
        if (phase_now != s->phase) {
            s->phase = phase_now;
            pvz_set_img_src(s->img, s->phase ? F_SUN_2 : F_SUN_1);
        }
        lv_obj_set_pos(s->img, (int)s->x, (int)s->y);
    }

    /* ---- 植物逻辑 ---- */
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            plant_t *p = &g_plants[r][c];
            if (p->type == PT_NONE || p->dead) continue;

            const plant_info_t *info = &PLANT_INFO[p->type];

            /* 樱桃倒计时 */
            if (p->type == PT_CHERRY) {
                p->fuse_t -= dt;
                if (p->fuse_t <= 0) {
                    pvz_explode_cherry(r, c);
                    continue;
                } else if (p->fuse_t < 500 && (g_tick_count / 80) % 2 == 0) {
                    /* 闪烁 */
                    lv_obj_set_style_opa(p->img, LV_OPA_50, 0);
                } else {
                    lv_obj_set_style_opa(p->img, LV_OPA_COVER, 0);
                }
            }

            /* 向日葵生产 */
            if (p->type == PT_SUNFLOWER && info->produce_interval > 0) {
                p->prod_t += dt;
                if (p->prod_t >= info->produce_interval) {
                    p->prod_t = 0;
                    int x = GRID_X0 + c * CELL_W + CELL_W / 2 - SUN_SZ / 2;
                    int y = GRID_Y0 + r * CELL_H + CELL_H / 2 - SUN_SZ / 2;
                    pvz_spawn_sun(x, y - 30, 25, false);
                }
            }

            /* 射手开火 */
            if ((p->type == PT_PEASHOOTER || p->type == PT_SNOWPEA)
                && info->shoot_interval > 0) {
                p->shoot_t += dt;
                int px = GRID_X0 + c * CELL_W + (CELL_W - PLANT_W) / 2;
                int py = GRID_Y0 + r * CELL_H + CELL_H - PLANT_H;
                int bx = px + PLANT_W - 8;
                int by = py + PLANT_H / 2 - BULLET_SZ / 2 + 4;
                if (p->shoot_t >= info->shoot_interval) {
                    if (pvz_lane_has_zombie_ahead(r, bx)) {
                        p->shoot_t = 0;
                        p->frame = 1;     /* 后坐力 */
                        pvz_set_img_src(p->img, info->in_game[1]);
                        bool ice = (p->type == PT_SNOWPEA);
                        pvz_spawn_bullet(r, bx, by,
                                         info->bullet_dmg,
                                         info->bullet_slow_ms, ice);
                    } else {
                        p->shoot_t = info->shoot_interval - 200; /* 没目标时短暂降速 */
                    }
                }
                /* 后坐力恢复 */
                p->anim_t += dt;
                if (p->frame == 1 && p->anim_t >= 200) {
                    p->frame = 0;
                    p->anim_t = 0;
                    pvz_set_img_src(p->img, info->in_game[0]);
                }
            }

            /* 坚果阶段更新 */
            if (p->type == PT_WALLNUT) {
                int ratio = p->hp * 100 / p->hp_max;
                int stage = (ratio > 66) ? 1 : (ratio > 33) ? 2 : 3;
                if (stage != p->wallnut_stage) {
                    p->wallnut_stage = stage;
                    pvz_set_img_src(p->img,
                        stage == 1 ? F_PLANT_NUT1 :
                        stage == 2 ? F_PLANT_NUT2 : F_PLANT_NUT3);
                }
            }
        }
    }

    /* ---- 子弹更新 ---- */
    for (int i = 0; i < MAX_BULLETS; ++i) {
        bullet_t *b = &g_bullets[i];
        if (b->dead) continue;
        if (!b->img) continue;
        b->x += 240.0f * (dt / 1000.0f);   /* 子弹速度 240 px/s */
        lv_obj_set_pos(b->img, (int)b->x,
                       GRID_Y0 + b->row * CELL_H + CELL_H / 2 - BULLET_SZ / 2 + 2);
        if (b->x > SCR_W) {
            pvz_destroy_bullet(b);
            continue;
        }
        /* 命中僵尸 */
        for (int z = 0; z < MAX_ZOMBIES; ++z) {
            zombie_t *zz = &g_zombies[z];
            if (zz->dead || zz->row != b->row) continue;
            if (b->x + BULLET_SZ >= zz->x && b->x <= zz->x + zz->w) {
                pvz_zombie_damage(zz, b->dmg, b->slow_ms);
                pvz_destroy_bullet(b);
                break;
            }
        }
    }

    /* ---- 僵尸更新 ---- */
    for (int i = 0; i < MAX_ZOMBIES; ++i) {
        zombie_t *z = &g_zombies[i];
        if (z->dead) continue;
        if (!z->img) continue;

        /* 僵王技能：定期在身后召唤小弟（啃植物时也照召唤） */
        if (z->type == ZT_BOSS) {
            z->summon_t += dt;
            if (z->summon_t >= BOSS_SUMMON_CD) {
                z->summon_t = 0;
                /* 残血或已达上限就收手，否则无限刷会导致永远清不完场 */
                if (z->hp > z->hp_max * 30 / 100 &&
                    z->summon_count < BOSS_SUMMON_MAX) {
                    zombie_t *minion = pvz_spawn_zombie(ZT_NORMAL, z->row, g_wave_index);
                    if (minion) {
                        minion->is_minion = true;
                        minion->x = z->x + z->w + 8;
                        if (minion->x > SCR_W) minion->x = SCR_W;
                        lv_obj_set_pos(minion->img, (int)minion->x,
                                       GRID_Y0 + minion->row * CELL_H + CELL_H - minion->h);
                        z->summon_count++;
                    }
                }
            }
        }

        /* 找同一行最左的植物 */
        plant_t *target = NULL;
        for (int c = 0; c < COLS; ++c) {
            plant_t *p = &g_plants[z->row][c];
            if (p->type != PT_NONE && !p->dead) {
                int pl_x = GRID_X0 + c * CELL_W;
                int pl_right = pl_x + CELL_W;
                if (z->x + z->w * 0.55f > pl_x &&
                    z->x + z->w * 0.55f < pl_right + 12) {
                    target = p;
                    break;
                }
            }
        }

        if (target) {
            z->eating = true;
            z->eat_t += dt;
            /* 普通僵尸 50 dmg/s，僵王 400 dmg/s（8 倍） */
            int eat_dmg = (z->type == ZT_BOSS) ? 400 : 50;
            if (z->eat_t >= 1000) {
                int times = z->eat_t / 1000;
                z->eat_t -= times * 1000;
                target->hp -= eat_dmg * times;
            }
            if (target->hp <= 0) {
                /* 植物被吃掉 */
                int r = target->row, c = target->col;
                pvz_destroy_plant(r, c);
                z->eating = false;
                z->eat_t = 0;
            }
            pvz_refresh_zombie_sprite(z);
        } else {
            z->eating = false;
            pvz_refresh_zombie_sprite(z);
            int s = z->speed;
            if (z->slow_t > 0) {
                s = s / 2;
                z->slow_t -= dt;
            }
            z->x -= s * (dt / 1000.0f);
            z->anim_t += dt;
            if (z->anim_t >= 350) {
                z->anim_t = 0;
                /* 走路 w1/w2 切换 */
                if (z->type == ZT_BOSS) {
                    /* 僵王：直接基于当前是否显示 W1 切换（更直观） */
                    const char *cur = lv_image_get_src(z->img);
                    if (cur && strstr(cur, "_w1")) {
                        pvz_set_img_src(z->img, F_ZOMBIE_BOSS_W2);
                    } else {
                        pvz_set_img_src(z->img, F_ZOMBIE_BOSS_W1);
                    }
                } else {
                    const char *cur_name = pvz_zombie_sprite_name(z, true);
                    const char *other = strstr(cur_name, "_w1") ?
                        (z->type == ZT_NORMAL && !z->gear_broken ? F_ZOMBIE1_W2 :
                         z->type == ZT_NORMAL && z->gear_broken ? F_ZOMBIE1_W2 :
                         z->type == ZT_CONE ? F_ZOMBIE2_W2 :
                         F_ZOMBIE3_W2) :
                        (z->type == ZT_NORMAL && !z->gear_broken ? F_ZOMBIE1_W1 :
                         z->type == ZT_NORMAL && z->gear_broken ? F_ZOMBIE1_W1 :
                         z->type == ZT_CONE ? F_ZOMBIE2_W1 :
                         F_ZOMBIE3_W1);
                    pvz_set_img_src(z->img, other);
                }
            }
        }
        int y = GRID_Y0 + z->row * CELL_H + CELL_H - z->h;
        lv_obj_set_pos(z->img, (int)z->x, y);

        /* 触发小推车 */
        if (z->x < GRID_X0 && !g_mowers[z->row].active
            && !g_mowers[z->row].dead && g_mowers[z->row].armed) {
            pvz_check_mower(z->row);
        }
        /* 到达左边界 */
        if (z->x < -z->w) {
            pvz_destroy_zombie(z);
            g_game_over = true;
            if (!g_banner_obj) {
                g_banner_obj = lv_image_create(lv_screen_active());
                pvz_set_img_src(g_banner_obj, F_LOSE);
                lv_obj_set_pos(g_banner_obj,
                               (SCR_W - 420) / 2, (SCR_H - 150) / 2);
                lv_obj_add_flag(g_btn_restart, LV_OBJ_FLAG_CLICKABLE);
            }
        }
    }

    /* ---- 小推车更新 ---- */
    for (int r = 0; r < ROWS; ++r) {
        mower_t *m = &g_mowers[r];
        if (m->dead) continue;
        if (!m->active) continue;
        m->x += m->vx * (dt / 1000.0f);
        int y = GRID_Y0 + r * CELL_H + (CELL_H - 46) / 2;
        lv_obj_set_pos(m->img, (int)m->x, y);
        /* 碾压同行僵尸 */
        for (int i = 0; i < MAX_ZOMBIES; ++i) {
            zombie_t *z = &g_zombies[i];
            if (z->dead || z->row != r) continue;
            if (m->x + 54 >= z->x && m->x <= z->x + z->w) {
                if (z->type == ZT_BOSS) {
                    /* 僵王一脚踩碎小推车，自己毫发无损。
                       碾压伤害每帧都会触发，若按血量扣会瞬间秒杀，
                       所以这里让僵王免疫推车。 */
                    if (m->img) { lv_obj_delete(m->img); m->img = NULL; }
                    m->dead = true;
                    break;
                }
                pvz_zombie_damage(z, 9999, 0);
            }
        }
        if (m->x > SCR_W + 30) {
            if (m->img) { lv_obj_delete(m->img); m->img = NULL; }
            m->dead = true;
        }
    }

    /* ---- 爆炸动画 ---- */
    pvz_update_booms(dt);

    /* ---- 波次进度条 ---- */
    pvz_refresh_wave_bar();

    /* ---- 波次 ---- */
    if (g_wave_active) {
        g_wave_spawn_t += dt;
        if (g_wave_to_spawn > 0 && g_wave_spawn_t >= g_wave_spawn_interval) {
            g_wave_spawn_t = 0;
            int row = rand() % ROWS;
            zombie_type_t zt;
            /* 最后一波最后一只：僵王（奶龙）；其余按波次混编 */
            if (g_wave_index == TOTAL_WAVES && g_wave_to_spawn == 1) {
                zt = ZT_BOSS;
            } else if (g_wave_index <= 2) {
                zt = ZT_NORMAL;
            } else {
                int r = rand() % 100;
                zt = (r < 60) ? ZT_NORMAL : (r < 90) ? ZT_CONE : ZT_BUCKET;
            }
            pvz_spawn_zombie(zt, row, g_wave_index);
            g_wave_to_spawn--;
        }
        /* 全部生成完且场上无僵尸 → 进入间歇 */
        if (g_wave_to_spawn <= 0) {
            bool any_alive = false;
            for (int i = 0; i < MAX_ZOMBIES; ++i)
                if (!g_zombies[i].dead) { any_alive = true; break; }
            if (!any_alive) {
                if (g_wave_index >= TOTAL_WAVES) {
                    g_game_won = true;
                    g_wave_active = false;
                    if (!g_banner_obj) {
                        g_banner_obj = lv_image_create(lv_screen_active());
                        pvz_set_img_src(g_banner_obj, F_WIN);
                        lv_obj_set_pos(g_banner_obj,
                                       (SCR_W - 420) / 2, (SCR_H - 150) / 2);
                        lv_obj_add_flag(g_btn_restart, LV_OBJ_FLAG_CLICKABLE);
                    }
                } else {
                    g_wave_active = false;
                    g_wave_between_t = 3500;
                }
            }
        }
    } else {
        /* 间歇期或游戏开始前 */
        if (g_wave_index == 0) {
            g_wave_index = 1;
            g_wave_to_spawn = 4 + g_wave_index * 2;
            g_wave_spawn_interval = 3500;
            g_wave_active = true;
            pvz_refresh_wave_label();
        } else if (g_wave_between_t > 0) {
            g_wave_between_t -= dt;
            if (g_wave_between_t <= 0) {
                g_wave_index++;
                g_wave_to_spawn = 4 + g_wave_index * 2;
                if (g_wave_to_spawn > 20) g_wave_to_spawn = 20;
                g_wave_spawn_interval = 3500 - g_wave_index * 250;
                if (g_wave_spawn_interval < 1200) g_wave_spawn_interval = 1200;
                g_wave_active = true;
                pvz_refresh_wave_label();
            }
        }
    }

    pvz_refresh_card_visuals();
}

/* ============================================================
 *  初始化函数：构建整个游戏界面 + 启动定时器
 * ============================================================
 *  这是游戏入口，完成以下工作（按执行顺序）：
 *
 *    1. 清屏 + 重置所有全局状态（植物/僵尸/子弹/阳光/波次等全部清零）
 *    2. 初始化 6 套样式（卡片普通/选中、冷却遮罩×2、草坪格子、横幅）
 *    3. 创建背景图片(Image)
 *    4. 创建阳光面板(Image) + 阳光数字标签(Label)
 *    5. 创建 5 张卡片(Object容器+Image图片+Object冷却遮罩)
 *    6. 创建铲子(Object+Image)
 *    7. 创建波次面板(Image) + 波次数字标签(Label)
 *    8. 创建波次进度条(Bar) + 进度条标签(Label)
 *    9. 创建重启按钮(Object+Image) + 退出按钮(Object+Image)
 *   10. 创建 45 个草坪格子(Object，5行×9列，每个都注册点击事件)
 *   11. 创建 5 辆小推车(Image)
 *   12. 启动 30ms 定时器(Timer) → 驱动 pvz_game_tick
 *
 *  每个控件的创建方式：
 *    Image:  lv_image_create(父对象) + pvz_set_img_src(图片, 文件名)
 *    Label:  lv_label_create(父对象) + lv_obj_set_style_text_*() 设字体颜色
 *    Object: lv_obj_create(父对象) + lv_obj_remove_style_all() + lv_obj_add_style()
 *    Bar:    lv_bar_create(父对象) + lv_bar_set_range()
 *    Timer:  lv_timer_create(回调, 周期ms, NULL)
 *
 *  事件注册：
 *    卡片: lv_obj_add_event_cb(card, pvz_card_click_cb, LV_EVENT_CLICKED, 索引)
 *    格子: lv_obj_add_event_cb(cell, pvz_cell_click_cb, LV_EVENT_CLICKED, 行列编码)
 *    铲子: lv_obj_add_event_cb(shovel, pvz_shovel_click_cb, LV_EVENT_CLICKED, NULL)
 *    重启: lv_obj_add_event_cb(restart, pvz_restart_cb, LV_EVENT_CLICKED, NULL)
 *    退出: lv_obj_add_event_cb(exit, pvz_exit_cb, LV_EVENT_CLICKED, NULL)
 * ============================================================ */
static void pvz_init(void)
{
    printf("[PVZ] pvz_init start\n"); fflush(stdout);
    srand((unsigned)time(NULL));    /* 用当前时间初始化随机数种子 */

    /* 清空整个屏幕（重新开始游戏时把上一次的植物/僵尸/UI 都清掉） */
    lv_obj_clean(lv_screen_active());
    printf("[PVZ] screen cleaned\n"); fflush(stdout);
    g_banner_obj = NULL;
    memset(g_plants, 0, sizeof(g_plants));
    memset(g_zombies, 0, sizeof(g_zombies));
    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_suns, 0, sizeof(g_suns));
    memset(g_booms, 0, sizeof(g_booms));
    memset(&g_mowers, 0, sizeof(g_mowers));

    /* 静态变量默认 dead=false，必须显式标记为死亡，否则第一个 tick 会访问空 img 指针 */
    for (int i = 0; i < MAX_ZOMBIES; ++i) g_zombies[i].dead = true;
    for (int i = 0; i < MAX_BULLETS; ++i) g_bullets[i].dead = true;
    for (int i = 0; i < MAX_SUNS; ++i) g_suns[i].dead = true;
    for (int i = 0; i < MAX_BOOMS; ++i) g_booms[i].img = NULL;

    for (int i = 0; i < 5; ++i) g_card_cd_t[i] = 0;
    g_sun_amount = 100;
    g_selected_card = -1;
    g_shovel_selected = false;
    g_game_over = false;
    g_game_won = false;
    g_wave_index = 0;
    g_wave_active = false;
    g_wave_between_t = 3500;     /* 给 3.5s 让玩家看清首屏 */
    g_wave_to_spawn = 0;
    g_wave_spawn_t = 0;
    g_sky_sun_t = 0;
    g_tick_count = 0;

    /* 资源路径探测 */
    pvz_res_init();

    /* ---------- 样式 ---------- */
    printf("[PVZ] init styles...\n"); fflush(stdout);
    lv_style_init(&g_st_card);
    lv_style_set_bg_opa(&g_st_card, LV_OPA_TRANSP);
    lv_style_set_border_opa(&g_st_card, LV_OPA_TRANSP);
    lv_style_set_border_width(&g_st_card, 0);

    lv_style_init(&g_st_card_sel);
    lv_style_set_bg_opa(&g_st_card_sel, LV_OPA_TRANSP);
    lv_style_set_border_color(&g_st_card_sel, lv_color_hex(0xFFD700));
    lv_style_set_border_opa(&g_st_card_sel, LV_OPA_COVER);
    lv_style_set_border_width(&g_st_card_sel, 3);
    lv_style_set_border_post(&g_st_card_sel, 1);    /* 边框画在外面 */

    lv_style_init(&g_st_overlay);
    lv_style_set_bg_color(&g_st_overlay, lv_color_hex(0x0A0A0A));
    lv_style_set_bg_opa(&g_st_overlay, LV_OPA_70);
    lv_style_set_border_width(&g_st_overlay, 0);

    lv_style_init(&g_st_overlay_dim);
    lv_style_set_bg_color(&g_st_overlay_dim, lv_color_hex(0x0A0A0A));
    lv_style_set_bg_opa(&g_st_overlay_dim, LV_OPA_50);
    lv_style_set_border_width(&g_st_overlay_dim, 0);

    lv_style_init(&g_st_cell);
    lv_style_set_bg_opa(&g_st_cell, LV_OPA_TRANSP);
    lv_style_set_border_width(&g_st_cell, 0);

    lv_style_init(&g_st_banner);
    lv_style_set_bg_opa(&g_st_banner, LV_OPA_TRANSP);
    lv_style_set_border_width(&g_st_banner, 0);

    /* ---------- 背景 ---------- */
    printf("[PVZ] create bg...\n"); fflush(stdout);
    g_bg_obj = lv_image_create(lv_screen_active());
    pvz_set_img_src(g_bg_obj, F_BG);
    lv_obj_set_pos(g_bg_obj, 0, 0);
    lv_obj_move_background(g_bg_obj);

    /* ---------- 阳光面板 ---------- */
    printf("[PVZ] create sun bank...\n"); fflush(stdout);
    g_bank_obj = lv_image_create(lv_screen_active());
    pvz_set_img_src(g_bank_obj, F_BANK);
    lv_obj_set_pos(g_bank_obj, BANK_X, BANK_Y);

    g_sun_num_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_color(g_sun_num_label, lv_color_hex(0xFFF078), 0);
    lv_obj_set_style_text_font(g_sun_num_label, &lv_font_montserrat_20, 0);
    lv_obj_set_size(g_sun_num_label, BANK_W - 16, BANK_H - 16);
    lv_obj_set_style_text_align(g_sun_num_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(g_sun_num_label, "100");
    lv_obj_set_pos(g_sun_num_label, BANK_LBL_X, BANK_LBL_Y);

    /* ---------- 卡片 ---------- */
    printf("[PVZ] create cards...\n"); fflush(stdout);
    for (int i = 0; i < 5; ++i) {
        plant_type_t pt = CARD_ORDER[i];
        const plant_info_t *info = &PLANT_INFO[pt];

        lv_obj_t *card = lv_obj_create(lv_screen_active());
        lv_obj_remove_style_all(card);
        lv_obj_set_size(card, CARD_W, CARD_H);
        lv_obj_set_pos(card, CARD_X0 + i * CARD_GAP, CARD_Y);
        lv_obj_add_style(card, &g_st_card, 0);
        lv_obj_add_style(card, &g_st_card_sel, LV_STATE_CHECKED);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, pvz_card_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        g_card_objs[i] = card;

        lv_obj_t *img = lv_image_create(card);
        pvz_set_img_src(img, info->card_file);
        lv_obj_set_pos(img, 0, 0);
        g_card_img[i] = img;

        lv_obj_t *ov = lv_obj_create(card);
        lv_obj_add_style(ov, &g_st_overlay, 0);
        lv_obj_set_size(ov, CARD_W, CARD_H);
        lv_obj_set_pos(ov, 0, 0);
        lv_obj_add_flag(ov, LV_OBJ_FLAG_HIDDEN);
        g_card_cooldown[i] = ov;
    }
    printf("[PVZ] cards done\n"); fflush(stdout);

    /* ---------- 铲子 ---------- */
    printf("[PVZ] create shovel...\n"); fflush(stdout);
    g_shovel_obj = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(g_shovel_obj);
    lv_obj_set_size(g_shovel_obj, 58, 58);
    lv_obj_set_pos(g_shovel_obj, SHOVEL_X, SHOVEL_Y);
    lv_obj_add_style(g_shovel_obj, &g_st_card, 0);
    lv_obj_add_style(g_shovel_obj, &g_st_card_sel, LV_STATE_CHECKED);
    lv_obj_add_flag(g_shovel_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_shovel_obj, pvz_shovel_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *shovel_img = lv_image_create(g_shovel_obj);
    pvz_set_img_src(shovel_img, F_SHOVEL);
    lv_obj_set_pos(shovel_img, 0, 0);

    /* ---------- 波次面板 ---------- */
    printf("[PVZ] create wave panel...\n"); fflush(stdout);
    g_wave_obj = lv_image_create(lv_screen_active());
    pvz_set_img_src(g_wave_obj, F_WAVE);
    lv_obj_set_pos(g_wave_obj, WAVE_X, WAVE_Y);
    g_wave_num_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_color(g_wave_num_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_wave_num_label, &lv_font_montserrat_28, 0);
    lv_obj_set_size(g_wave_num_label, 60, 36);
    lv_obj_set_style_text_align(g_wave_num_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(g_wave_num_label, "0");
    lv_obj_set_pos(g_wave_num_label, WAVE_LBL_X, WAVE_LBL_Y);

    /* ---------- 波次进度条（竖直，草坪右侧空白区） ---------- */
    printf("[PVZ] create wave bar...\n"); fflush(stdout);
    g_wave_bar = lv_bar_create(lv_screen_active());
    lv_obj_set_size(g_wave_bar, BAR_W, BAR_H);
    lv_obj_set_pos(g_wave_bar, BAR_X, BAR_Y);
    lv_bar_set_range(g_wave_bar, 0, 100);
    lv_bar_set_value(g_wave_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g_wave_bar, lv_color_hex(0x1B3D1B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_wave_bar, lv_color_hex(0x66BB6A), LV_PART_INDICATOR);
    lv_obj_set_style_radius(g_wave_bar, 6, LV_PART_MAIN);
    lv_obj_set_style_radius(g_wave_bar, 6, LV_PART_INDICATOR);

    g_wave_bar_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(g_wave_bar_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(g_wave_bar_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(g_wave_bar_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_size(g_wave_bar_label, 80, 26);
    lv_label_set_text(g_wave_bar_label, "1/5");
    lv_obj_set_pos(g_wave_bar_label, BAR_X - 26, BAR_Y + BAR_H + 8);

    /* ---------- 重启按钮 ---------- */
    printf("[PVZ] create restart button...\n"); fflush(stdout);
    g_btn_restart = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(g_btn_restart);
    lv_obj_set_size(g_btn_restart, BTN_RESTART_W, BTN_RESTART_H);
    lv_obj_set_pos(g_btn_restart, BTN_RESTART_X, BTN_RESTART_Y);
    lv_obj_add_flag(g_btn_restart, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_btn_restart, pvz_restart_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *br_img = lv_image_create(g_btn_restart);
    pvz_set_img_src(br_img, F_BTN_RESTART);
    lv_obj_set_pos(br_img, 0, 0);

    /* ---------- 退出按钮 ---------- */
    printf("[PVZ] create exit button...\n"); fflush(stdout);
    g_btn_exit = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(g_btn_exit);
    lv_obj_set_size(g_btn_exit, BTN_EXIT_W, BTN_EXIT_H);
    lv_obj_set_pos(g_btn_exit, BTN_EXIT_X, BTN_EXIT_Y);
    lv_obj_add_flag(g_btn_exit, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_btn_exit, pvz_exit_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ex_img = lv_image_create(g_btn_exit);
    pvz_set_img_src(ex_img, F_BTN_EXIT);
    lv_obj_set_pos(ex_img, 0, 0);

    /* ---------- 草坪格子（点击区） ---------- */
    printf("[PVZ] create grid cells...\n"); fflush(stdout);
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            lv_obj_t *cell = lv_obj_create(lv_screen_active());
            lv_obj_remove_style_all(cell);
            lv_obj_set_size(cell, CELL_W, CELL_H);
            lv_obj_set_pos(cell, GRID_X0 + c * CELL_W, GRID_Y0 + r * CELL_H);
            lv_obj_add_style(cell, &g_st_cell, 0);
            lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
            int rc = r * 10 + c;
            lv_obj_add_event_cb(cell, pvz_cell_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)rc);
            g_cells[r][c] = cell;
        }
    }
    printf("[PVZ] grid done\n"); fflush(stdout);

    /* ---------- 小推车 ---------- */
    printf("[PVZ] create mowers...\n"); fflush(stdout);
    for (int r = 0; r < ROWS; ++r) {
        mower_t *m = &g_mowers[r];
        m->row = r;
        m->x = (float)14;
        m->vx = 0;
        m->armed = true;
        m->active = false;
        m->dead = false;
        m->img = lv_image_create(lv_screen_active());
        pvz_set_img_src(m->img, F_MOWER);
        lv_obj_set_pos(m->img, 14, GRID_Y0 + r * CELL_H + (CELL_H - 46) / 2);
    }
    printf("[PVZ] mowers done\n"); fflush(stdout);

    printf("[PVZ] refresh UI...\n"); fflush(stdout);
    pvz_refresh_card_visuals();
    pvz_refresh_sun_label();
    pvz_refresh_wave_label();

    /* ---------- 启动定时器 ---------- */
    /* lv_timer_create(回调函数, 周期ms, NULL)：
       创建一个 30ms 周期的定时器，每 30ms 自动调用 pvz_game_tick。
       这就是整个游戏循环的驱动源——定时器不停，游戏就不停。 */
    printf("[PVZ] start timer...\n"); fflush(stdout);
    if (g_tick_timer == NULL) {
        g_tick_timer = lv_timer_create(pvz_game_tick, 30, NULL);
    }
    g_in_game = true;
    printf("[PVZ] init done\n"); fflush(stdout);
}

/*
 * game_pvz：游戏对外入口函数（在 game_menu.c 中被 menu_start_cb 调用）
 * 也是重启按钮 pvz_restart_cb 调用的函数。
 * 内部直接调用 pvz_init() 完成全部初始化。
 */
void game_pvz(void)
{
    pvz_init();
}