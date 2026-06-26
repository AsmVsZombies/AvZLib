#include <avz.h>

template <size_t Addr, typename T>
struct _RndHook {
    static inline std::function<T()> func;
    static inline size_t origAddr;

    static bool IsInstalled() {
        return origAddr != 0;
    }

    static T Hook() {
        return func();
    };

    static void Install(auto&& f) {
        func = std::forward<decltype(f)>(f);
        if (!IsInstalled()) {
            origAddr = *(size_t*)(Addr);
            *(char**)(Addr) = (char*)Hook - Addr - 4;
        }
    }

    static void Uninstall() {
        if (IsInstalled()) {
            *(size_t*)(Addr) = origAddr;
            origAddr = 0;
        }
    }
};

namespace Rnd {
#define DECLARE_RND_CLASS(addr, type, transform, validate, fallback, message) \
    class : public AOrderedExitFightHook<0> {                                 \
    private:                                                                  \
        using R = _RndHook<addr, type>;                                       \
        static type TransformValue(type x) {                                  \
            return transform;                                                 \
        }                                                                     \
        static bool IsValid(type x) {                                         \
            return validate;                                                  \
        }                                                                     \
        void _ExitFight() override {                                          \
            R::Uninstall();                                                   \
        }                                                                     \
                                                                              \
    public:                                                                   \
        template <typename T>                                                 \
            requires std::is_invocable_r_v<type, T>                           \
        void Set(T&& func, bool protect = true) {                             \
            if (!protect) {                                                   \
                R::Install([f = std::forward<T>(func)]() {                    \
                    return TransformValue(f());                               \
                });                                                           \
            } else {                                                          \
                R::Install([f = std::forward<T>(func)]() {                    \
                    type result = f();                                        \
                    if (!IsValid(result)) {                                   \
                        aLogger->Error(message);                              \
                        R::Uninstall();                                       \
                        return fallback;                                      \
                    }                                                         \
                    return TransformValue(result);                            \
                });                                                           \
            }                                                                 \
        }                                                                     \
        void Set(type value, bool protect = true) {                           \
            if (protect && !IsValid(value)) {                                 \
                aLogger->Error(message);                                      \
                return;                                                       \
            }                                                                 \
            R::Install([value]() {                                            \
                return TransformValue(value);                                 \
            });                                                               \
        }                                                                     \
        void Unset() {                                                        \
            R::Uninstall();                                                   \
        }                                                                     \
    }

/*
eax:
a+b型，修改eax可影响b
0x532420 + 5, 400+201, ice
0x53240F + 5, 300+101, ice
0x4140C4 + 5, 2500+600, wave_cd
0x413BBE + 5, <=950+275, sun_cd
0x45E3CC + 5, -5+10, sunshroom_x
0x45E3DC + 5, -5+10, sunshroom_y
0x45E66F + 5, -5+10, puffshroom_x
0x45E67F + 5, -3+6, puffshroom_y
0x522FD2 + 5, 450+300, jack_cd
0x522FE8 + 5, 1/20, jack_early
0x52259F + 5, 780+40, zombie_x
0x522CDB + 5, 870+10, zombie_x
0x522DEF + 5, 800+10, zombie_x
0x522E91 + 5, 825+10, zombie_x
0x523D38 + 5, 845+10, zombie_x
0x45F8BA + 5, x-(15), plant_cd
0x45F1E5 + 5, 1/4, butter
0x4630F4 + 5, 1/2, bowling_down
0x52B907 + 5, 1/2, garlic_up
0x523A8B + 5, 275+175, zombie_jalapeno_cd
0x522A26 + 5, 3000+151, zombie_bungee_height
0x5234FE + 5, 300+12, zombie_mj_cd
0x522972 + 5, 1500+501, zombie_yeti_cd
0x5232B5 + 5, 1+80, zombie_pogo_position
0x45DED0 + 5, 300+(x-299), plant_produce_init_cd
0x45FA91 + 5, (x-150)+151, planr_produce_cd
0x45DEE2 + 5, 0+(x+1), plant_init_cd
0x41CEA8 + 5, 0+(30000), coin_drop
0x42AFA6 + 5, x+y, ize_puffshroom_num
0x42B019 + 5, x+y, ize_puffshroom_num
fstack:
a~b型，fstp+fld可影响结果
0x52708A + 5, 0-100, imp_v
0x414087 + 5, 0.5-0.65, refresh_hp
0x524C3D + 5, 0.66-0.68, zombie_speed
0x524C14 + 5, 0.79-0.81, zombie_speed
0x524BEB + 5, 0.89-0.91, zombie_speed
0x524B97 + 5, 0.23-0.37, zombie_speed

以下用"Rnd"代指随机数：
Zombies:
小丑倒数：小丑爆炸倒计时为(450+Rnd)*2/小丑速度参数，该参数取0.66-0.68;
小丑早爆：取0时为早爆丑，爆炸倒计时变为1/3;
其他出生：大部分僵尸出生点为780+Rnd;
撑杆出生：撑杆僵尸出生点为870+Rnd;
冰车出生：冰车僵尸出生点为800+Rnd;
投篮出生：投篮僵尸出生点为825+Rnd;
巨人出生：巨人僵尸出生点为845+Rnd;
大蒜方向：取0时，啃蒜僵尸将向下移动;
辣椒倒数：火爆辣椒僵尸爆炸倒计时为(275+Rnd)*2/僵尸速度参数，该参数取0.23-0.37;
小偷高度：小偷出生时高度为3000+Rnd;
舞王滑步：舞王滑步时间为300+Rnd;
雪人逃跑：雪人逃跑时间为1500+Rnd;
跳跳初始：跳跳初始相位为Rnd，取0-79;
物品掉落：影响掉落物的参数，数值越小则掉落物品价值越高，礼物盒子>钻石>金币>银币>无掉落；
PLants:
一次冻结：原速僵尸受寒冰菇冻结时间为400+Rnd；
二次冻结：减速僵尸受寒冰菇冻结时间为300+Rnd;
刷新倒数：每波自然刷新时间上限为2500+Rnd;
天降间隔：天降阳光时间间隔上限为950+Rnd;
小喷横移：小喷菇与海蘑菇横坐标偏移为-5+Rnd;
小喷横移：小喷菇与海蘑菇横坐标偏移为-3+Rnd;
阳光横移：阳光菇横坐标偏移为-5+Rnd;
阳光横移：阳光菇横坐标偏移为-5+Rnd;
锁定黄油：取0时玉米投手将投掷黄油;
保龄方向：取0时，坚果保龄球将向下弹射;
生产初始：阳光植物第一次生产所需时间为300+Rnd;
生产间隔：阳光植物生产间隔为2350+Rnd;
攻击初始：攻击植物第一次索敌所需时间为Rnd;
攻击间隔：植物攻击周期为最大周期-Rnd，最大周期取300(投手)，200(忧郁蘑菇)，150(其他，不包含地刺类);
Float:
激活比例：激活下一波刷新所需的本波僵尸血量比例，参考值0.5-0.65;
小丑速度：小丑、橄榄、奔跑的撑杆、挖掘的矿工、奔跑的潜水的相对速度，参考值0.66-0.68;
梯子速度：有梯的梯子、倭瓜僵尸的相对速度，参考值0.79-0.81;
海豚速度：海豚、愤怒的报纸的相对速度，参考值0.89-0.91;
普僵速度：大部分僵尸的相对速度，参考值0.23-0.37;
小鬼参数：小鬼飞出时垂向速度为(x-Rnd)/120，其中x为巨人横坐标-360(若为天台则再-180)，参考值0.0-100.0;
*/

// 一次冻结：400~600
DECLARE_RND_CLASS(
    0x532421,
    int,
    x - 400,
    400 <= x && x <= 600,
    0,
    "一次冻结的值域为 [400, 600]"
) inline firstFreeze;

// 二次冻结：300~400
DECLARE_RND_CLASS(
    0x532410,
    int,
    x - 300,
    300 <= x && x <= 400,
    0,
    "二次冻结的值域为 [300, 400]"
) inline secondFreeze;

// 刷新倒数：2500~3099
DECLARE_RND_CLASS(
    0x4140c5,
    int,
    x - 2500,
    2500 <= x && x <= 3099,
    0,
    "刷新倒数的值域为 [2500, 3099]"
) inline refreshCd;

// 天降间隔：950~1225
DECLARE_RND_CLASS(
    0x413bbf,
    int,
    x - 950,
    950 <= x && x <= 1225,
    0,
    "天降间隔的值域为 [950, 1225]"
) inline sunDropCd;

// 阳光横移 x：-5~4
DECLARE_RND_CLASS(
    0x45e3cd,
    int,
    x + 5,
    -5 <= x && x <= 4,
    0,
    "阳光横移 x 的值域为 [-5, 4]"
) inline sunshroomXOffset;

// 阳光横移 y：-5~4
DECLARE_RND_CLASS(
    0x45e3dd,
    int,
    x + 5,
    -5 <= x && x <= 4,
    0,
    "阳光横移 y 的值域为 [-5, 4]"
) inline sunshroomYOffset;

// 小喷横移 x：-5~4
DECLARE_RND_CLASS(
    0x45e670,
    int,
    x + 5,
    -5 <= x && x <= 4,
    0,
    "小喷横移 x 的值域为 [-5, 4]"
) inline puffshroomXOffset;

// 小喷横移 y：-3~2
DECLARE_RND_CLASS(
    0x45e680,
    int,
    x + 3,
    -3 <= x && x <= 2,
    0,
    "小喷横移 y 的值域为 [-3, 2]"
) inline puffshroomYOffset;

// 锁定黄油：false/true
DECLARE_RND_CLASS(
    0x45f1e6,
    int,
    !x,
    x == 0 || x == 1,
    1,
    "锁定黄油的值域为 {0, 1}"
) inline butter;

// 小丑倒数：450~749
DECLARE_RND_CLASS(
    0x522fd3,
    int,
    x - 450,
    450 <= x && x <= 749,
    0,
    "小丑倒数的值域为 [450, 749]"
) inline jackCd;

// 小丑早爆：false/true
DECLARE_RND_CLASS(
    0x522fe9,
    int,
    !x,
    x == 0 || x == 1,
    1,
    "小丑早爆的值域为 {0, 1}"
) inline jackEarly;

// 其他出生：780~819
DECLARE_RND_CLASS(
    0x5225a0,
    int,
    x - 780,
    780 <= x && x <= 819,
    0,
    "其他出生的值域为 [780, 819]"
) inline otherSpawn;

// 撑杆出生：870~879
DECLARE_RND_CLASS(
    0x522cdc,
    int,
    x - 870,
    870 <= x && x <= 879,
    0,
    "撑杆出生的值域为 [870, 879]"
) inline poleSpawn;

// 冰车出生：800~809
DECLARE_RND_CLASS(
    0x522df0,
    int,
    x - 800,
    800 <= x && x <= 809,
    0,
    "冰车出生的值域为 [800, 809]"
) inline zomboniSpawn;

// 投篮出生：825~834
DECLARE_RND_CLASS(
    0x522e92,
    int,
    x - 825,
    825 <= x && x <= 834,
    0,
    "投篮出生的值域为 [825, 834]"
) inline catapultSpawn;

// 巨人出生：845~854
DECLARE_RND_CLASS(
    0x523d39,
    int,
    x - 845,
    845 <= x && x <= 854,
    0,
    "巨人出生的值域为 [845, 854]"
) inline gargSpawn;

// 大蒜方向：0 为向下，1 为向上
DECLARE_RND_CLASS(
    0x52b908,
    int,
    x,
    x == 0 || x == 1,
    0,
    "大蒜方向的值域为 {0, 1}"
) inline garlicDirection;

// 辣椒倒数：275~449
DECLARE_RND_CLASS(
    0x523a8c,
    int,
    x - 275,
    275 <= x && x <= 449,
    0,
    "辣椒倒数的值域为 [275, 449]"
) inline jalapenoCd;

// 小偷高度：3000~3150
DECLARE_RND_CLASS(
    0x522a27,
    int,
    x - 3000,
    3000 <= x && x <= 3150,
    0,
    "小偷高度的值域为 [3000, 3150]"
) inline bungeeHeight;

// 舞王滑步：300~311
DECLARE_RND_CLASS(
    0x5234ff,
    int,
    x - 300,
    300 <= x && x <= 311,
    0,
    "舞王滑步的值域为 [300, 311]"
) inline dancerSlideCd;

// 雪人逃跑：1500~2000
DECLARE_RND_CLASS(
    0x522973,
    int,
    x - 1500,
    1500 <= x && x <= 2000,
    0,
    "雪人逃跑的值域为 [1500, 2000]"
) inline yetiEscapeCd;

// 跳跳初始：0~79
DECLARE_RND_CLASS(
    0x5232b6,
    int,
    x,
    0 <= x && x <= 79,
    0,
    "跳跳初始的值域为 [0, 79]"
) inline pogoPhase;

// 小鬼参数：0.0~100.0
DECLARE_RND_CLASS(
    0x52708b,
    float,
    x,
    0.0f <= x && x <= 100.0f,
    0.0f,
    "小鬼参数的值域为 [0.0, 100.0]"
) inline impParam;

// 激活比例：0.5~0.65
DECLARE_RND_CLASS(
    0x414088,
    float,
    x,
    0.5f <= x && x <= 0.65f,
    0.5f,
    "激活比例的值域为 [0.5, 0.65]"
) inline refreshRatio;

// 普僵速度：0.23~0.37
DECLARE_RND_CLASS(
    0x524b98,
    float,
    x,
    0.23f <= x && x <= 0.37f,
    0.23f,
    "普僵速度的值域为 [0.23, 0.37]"
) inline normalSpeed;

// 小丑速度：0.66~0.68
DECLARE_RND_CLASS(
    0x524c3e,
    float,
    x,
    0.66f <= x && x <= 0.68f,
    0.66f,
    "小丑速度的值域为 [0.66, 0.68]"
) inline jackSpeed;

// 梯子速度：0.79~0.81
DECLARE_RND_CLASS(
    0x524c15,
    float,
    x,
    0.79f <= x && x <= 0.81f,
    0.79f,
    "梯子速度的值域为 [0.79, 0.81]"
) inline ladderSpeed;

// 海豚速度：0.89~0.91
DECLARE_RND_CLASS(
    0x524bec,
    float,
    x,
    0.89f <= x && x <= 0.91f,
    0.89f,
    "海豚速度的值域为 [0.89, 0.91]"
) inline dolphinSpeed;

#undef DECLARE_RND_CLASS
} // namespace Rnd
