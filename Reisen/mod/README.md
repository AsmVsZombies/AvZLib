# AvZ 修改器库

将修改器的一些常用功能移植到了 AvZ 中，方便脚本使用。

目前包含以下修改：

- `AccelerateGame`：简化部分非战斗逻辑，加速游戏运行（实验性，建议配合跳帧使用）
- `AlwaysButter`：玉米投手只投黄油
- `AlwaysKernel`：玉米投手只投玉米粒
- `AshInstantExplode`：寒冰菇和灰烬放置后立即生效
- `CobFixedDelay`：禁用“引信延迟”机制，开启后炸陆地的玉米加农炮生效延迟固定373cs，水池仍为378cs
- `CobInstantRecharge`：玉米加农炮立即装填（实际仍有476cs冷却时间）
- `DisableJackExplode`：小丑僵尸与辣椒僵尸不会爆炸
- `DisableItemDrop`：不掉战利品
- `DisableSpecialAttack`：不生成墓碑、墓碑僵尸、珊瑚僵尸、空降僵尸
- `DisableSunDrop`：取消天降阳光
- `FixCobDrift`：修复“上界之风”
- `FreePlantingCheat`：卡片不消耗阳光且无冷却时间，紫卡可直接种植（在AvZ1中实际仍有1cs冷却时间，AvZ2不存在此问题）
- `MushroomAwake`：蘑菇免唤醒
- `NeverFail`：僵尸进家即死
- `PlantAnywhere`：无视所有种植限制条件（此修改允许重叠种植，慎用）
- `PlantInvincible`：植物无敌
- `PlantWeak`：植物虚弱（受到伤害直接死亡）
- `RemoveFog`：清除浓雾
- `SaveDataReadOnly`：存档只读
- `StabilizeSunDrop`：阳光陈化（去除游戏中前 52 个天降阳光掉落更快的设定）
- `StopZombieSpawn`：暂停刷新
- `UnlimitedSun`：不消耗阳光

## 使用方法

```cpp
// AvZ1
void Script() {
    // EnableModsScoped 会开启修改，并在选卡结束或用户回到主菜单时自动关闭
    EnableModsScoped(SaveDataReadOnly, RemoveFog, UnlimitedSun);
    // EnableModsGlobal 开启的修改会一直生效，需要手动关闭
    // 这两个函数都是 Not In Queue，定时生效需要配合 InsertTimeOperation 而非 SetTime
    InsertTimeOperation(0, 1, [](){ EnableModsGlobal(DisableJackExplode); });
    InsertTimeOperation(0, 2, [](){ DisableMods(DisableJackExplode); });
}

// AvZ2
void AScript() {
    EnableModsScoped(SaveDataReadOnly, RemoveFog, UnlimitedSun);
    // 注意：SaveDataReadOnly 在选卡后启动会导致存档被删除，因此 EnableMods 最好放在选卡之前
    ASelectCards({...});
    AConnect(ATime(1, 0), [](){ EnableModsGlobal(DisableJackExplode); });
    AConnect(ATime(2, 0), [](){ DisableMods(DisableJackExplode); });
}
```

## Rnd（仅 AvZ2）

`Rnd` 命名空间提供对游戏内随机数的精确控制。每个成员均有 `Set(value)`、`Set(func)` 和 `Unset()` 三个方法，分别用于固定值、自定义函数和恢复随机。战斗结束时所有设置自动清除。

```cpp
// 固定值
Rnd::firstFreeze.Set(400); // 一次冻结固定为 400cs
Rnd::jackEarly.Set(true);  // 小丑全部早爆

// 自定义函数（每次触发时调用）
Rnd::pogoPhase.Set([]{ static int x; return x++ % 80; });

// 恢复随机
Rnd::firstFreeze.Unset();
```

`Set` 的第二个参数 `protect`（默认为 `true`）启用值域检查，超出范围时会报错并取消设置。

可控随机数列表：

| 成员 | 说明 | 值域 |
|------|------|------|
| `firstFreeze` | 一次冻结时长 | [400, 600] |
| `secondFreeze` | 二次冻结时长 | [300, 400] |
| `refreshCd` | 每波自然刷新时间上限 | [2500, 3099] |
| `sunDropCd` | 天降阳光间隔上限 | [950, 1225] |
| `sunshroomXOffset` | 阳光菇横坐标偏移 | [-5, 4] |
| `sunshroomYOffset` | 阳光菇纵坐标偏移 | [-5, 4] |
| `puffshroomXOffset` | 小喷菇横坐标偏移 | [-5, 4] |
| `puffshroomYOffset` | 小喷菇纵坐标偏移 | [-3, 2] |
| `butter` | 锁定黄油（true 为投黄油） | {false, true} |
| `jackCd` | 小丑爆炸倒计时 | [450, 749] |
| `jackEarly` | 小丑早爆（true 为早爆） | {false, true} |
| `otherSpawn` | 大部分僵尸出生横坐标 | [780, 819] |
| `poleSpawn` | 撑杆僵尸出生横坐标 | [870, 879] |
| `zomboniSpawn` | 冰车僵尸出生横坐标 | [800, 809] |
| `catapultSpawn` | 投篮僵尸出生横坐标 | [825, 834] |
| `gargSpawn` | 巨人僵尸出生横坐标 | [845, 854] |
| `garlicDirection` | 啃蒜僵尸移动方向（0 向下，1 向上） | {0, 1} |
| `jalapenoCd` | 辣椒僵尸爆炸倒计时 | [275, 449] |
| `bungeeHeight` | 小偷出生高度 | [3000, 3150] |
| `dancerSlideCd` | 舞王滑步时间 | [300, 311] |
| `yetiEscapeCd` | 雪人逃跑时间 | [1500, 2000] |
| `pogoPhase` | 跳跳初始相位 | [0, 79] |
| `impParam` | 小鬼飞出垂向速度参数 | [0.0, 100.0] |
| `refreshRatio` | 激活下一波刷新所需血量比例 | [0.5, 0.65] |
| `normalSpeed` | 普通僵尸相对速度 | [0.23, 0.37] |
| `jackSpeed` | 小丑等快速僵尸相对速度 | [0.66, 0.68] |
| `ladderSpeed` | 梯子、倭瓜僵尸相对速度 | [0.79, 0.81] |
| `dolphinSpeed` | 海豚、愤怒报纸僵尸相对速度 | [0.89, 0.91] |

## 自定义 Mod

一个 `Mod` 的基本结构如下：

```cpp
Mod ModName{
    {内存地址1, 修改值1, 原始值1},
    {内存地址2, 修改值2, 原始值2},
    ...
};
```

内存地址可以是一个数，也可以是用花括号括起的一列数（第一个数为基址，后面的数为各级偏移）。

修改值可以是 `char`（1 字节）、`uint32_t`（4 字节）或 `vector<uint8_t>`（任意长度）。
**注意，在大多数机器上整数类型的存储是以字节为单位从低位到高位存储的，也就是说 `0xdeadbeef` 相当于 `{0xef, 0xbe, 0xad, 0xde}`。**

示例见 `mod/mod.h`。

**`Mod` 最好被定义为全局变量。**通过 `EnableModsScoped` 启动的局部变量 `Mod` 受对象生命周期的影响，可能会意外地提前关闭。
