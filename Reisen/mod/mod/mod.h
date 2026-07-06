#ifndef _REISEN_MOD_MOD_H
#define _REISEN_MOD_MOD_H

#include "avz.h"

#if __AVZ_VERSION__ > 221001
#include "mod_avz2.h"
#include "rnd_avz2.h"
#else
#include "mod_avz1.h"
#endif

// 简化部分非战斗逻辑，加速游戏运行（实验性，建议配合跳帧使用）
Mod AccelerateGame{
    {0x40ef00, {0xc2, 0x04, 0x00}, {0x6a, 0xff, 0x64}},
    {0x455930, '\xc3', '\x80'},
    {0x45b260, '\xc3', '\x8b'},
    {0x461123, {0xeb, 0x6d}, {0x8b, 0x9f}},
    {0x4635c0, {0xc2, 0x04, 0x00}, {0x55, 0x8b, 0xec}},
    {0x465040, {0xd9, 0xee, 0xc3}, {0x55, 0x8b, 0xec}},
    {0x471dcf, {0xeb, 0x24, 0x90}, {0x8b, 0x4e, 0x0c}},
    {0x515020, {0xc2, 0x04, 0x00}, {0x55, 0x8b, 0xec}},
    {0x5336d1, '\xeb', '\x74'},
    {0x515020, {0xc2, 0x04, 0x00}, {0x55, 0x8b, 0xec}},
    {0x517670, {0x90, 0x90}, {0x74, 0x17}},
    {0x51767b, {0x90, 0x90}, {0x75, 0x0c}},
    {0x6a66f4, '\x00', '\x01'},
};

// 玉米投手只投黄油
Mod AlwaysButter{
    {0x45f1ec, '\x70', '\x75'},
};

// 玉米投手只投玉米粒
Mod AlwaysKernel{
    {0x45f1ec, '\xeb', '\x75'},
};

// 寒冰菇和灰烬放置立即生效
Mod AshInstantExplode{
    {0x463408, '\x70', '\x75'}
};

// 禁用“引信延迟”机制，开启后炸陆地的玉米加农炮生效延迟固定373cs，水池仍为378cs
Mod CobFixedDelay{
    {0x46d672, {0x00, 0x97}, {0xd0, 0x96}},
};

// 玉米加农炮立即装填（实际仍有476cs冷却时间）
Mod CobInstantRecharge{
    {0x46103b, '\x80', '\x85'},
};

// 小丑僵尸与辣椒僵尸不会爆炸
Mod DisableJackExplode{
    {0x526afc, '\x81', '\x8f'},
    {0x5275dd, '\x81', '\x85'},
};

// 不掉战利品
Mod DisableItemDrop{
    {0x530276, '\x66', '\x5b'},
};

// 不生成墓碑、墓碑僵尸、珊瑚僵尸、空降僵尸
Mod DisableSpecialAttack{
    {0x413083, '\xeb', '\x75'},
    {0x42694a, '\xeb', '\x75'},
};

// 取消天降阳光
Mod DisableSunDrop{
    {0x413b83, '\xeb', '\x75'},
};

// 修复“上界之风”
Mod FixCobDrift{
    {0x46dce3, {0x83, 0x7e, 0x5c, 0x0b, 0x75, 0x04, 0xdd, 0xd8, 0xeb, 0x1b}, {0x75, 0x08, 0xd9, 0x46, 0x34, 0xd8, 0xc1, 0xd9, 0x5e, 0x34}},
};

// 卡片不消耗阳光且无冷却时间，紫卡可直接种植
Mod FreePlantingCheat{
    {{0x6a9ec0, 0x814}, '\x01', '\x00'},
    #if __AVZ_VERSION__ <= 221001
    {0x487296, '\x70', '\x7e'},
    {0x488250, '\xeb', '\x75'},
    #endif
};

// 蘑菇免唤醒
Mod MushroomAwake{
    {0x45de8e, '\xeb', '\x74'},
};

// 僵尸进家即死
Mod NeverFail{
    {0x52b308, {0x90, 0x90}, {0x74, 0x07}},
};

// 无视所有种植限制条件（此修改允许重叠种植，慎用）
Mod PlantAnywhere{
    {0x40fe30, '\x81', '\x84'},
    {0x42a2d9, '\x8d', '\x84'},
    {0x438e40, '\xeb', '\x74'},
};

// 植物无敌
Mod PlantInvincible{
    {0x0041cc2f, '\xeb', '\x74'},
    {0x0045ec66, '\x00', '\xce'},
    {0x0045ee0a, '\x70', '\x75'},
    {0x00462b80, {0xc2, 0x04, 0x00}, {0x53, 0x55, 0x8b}},
    {0x0046cfeb, {0x90, 0x90, 0x90}, {0x29, 0x50, 0x40}},
    {0x0046d7a6, {0x90, 0x90, 0x90}, {0x29, 0x4e, 0x40}},
    {0x005276ea, '\xeb', '\x75'},
    {0x0052e93b, '\xeb', '\x74'},
    {0x0052fcf3, '\x00', '\xfc'},
};

// 植物虚弱（受到伤害直接死亡）
Mod PlantWeak{
    {0x0045ec66, '\x00', '\xce'},
    {0x0045ee0a, '\xeb', '\x75'},
    {0x0046cfec, '\x40', '\x50'},
    {0x0046d7a7, '\x76', '\x4e'},
    {0x0052fcf1, '\x66', '\x46'},
    {0x0052fcf3, '\x00', '\xfc'},
};

// 清除浓雾
Mod RemoveFog{
    {0x41a68d, {0x31, 0xd2}, {0x3b, 0xf2}},
};

// 存档只读
Mod SaveDataReadOnly{
    {0x482149, '\x2e', '\x13'},
    {0x54b267, '\x70', '\x74'},
};

// 阳光陈化（去除游戏中前 52 个天降阳光掉落更快的设定）
Mod StabilizeSunDrop{
    {{0x6a9ec0, 0x768, 0x553c}, 52},
};

// 暂停刷新
Mod StopZombieSpawn{
    {0x4265dc, '\xeb', '\x74'},
};

// 种卡不消耗阳光
Mod UnlimitedSun{
    {0x41ba72, '\x70', '\x7f'},
    {0x41ba74, '\x3b', '\x2b'},
    {0x41bac0, '\x91', '\x9e'},
    {0x427a92, '\x80', '\x8f'},
    {0x427dfd, '\x80', '\x8f'},
    {0x42487f, '\xeb', '\x74'},
};
#endif
