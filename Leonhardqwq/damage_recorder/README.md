
# PVZ-Damage-Recorder

记录每场战斗中僵尸受到的伤害及其来源，并在退出战斗时输出 CSV 到指定路径下，字段信息如下：

| 字段名 | 解释 |
| --- | --- |
| 波次 | 伤害发生波次 |
| 时间 | 伤害发生波内时间 |
| 绝对时间 | 战斗开始后的绝对时间，可用于计算波长和跨波相对时间 |
| 伤害 | 本次伤害数值；直接消灭为 `INT_MAX` |
| 僵尸ID | 受伤僵尸 ID |
| 僵尸类型 | 受伤僵尸类型 |
| 僵尸路 | 受伤僵尸所在行 |
| 僵尸坐标 | 受伤僵尸横坐标(int) |
| 僵尸波次 | 受伤僵尸所属波次 |
| 僵尸总血量 | 僵尸受伤前总血量 |
| 僵尸临界后 | 伤害发生前是否已经低于有效生命临界 |
| 子弹ID | 造成伤害的子弹 ID；无子弹则为空 |
| 子弹类型 | 造成伤害的子弹类型；无子弹则为空 |
| 植物ID | 伤害来源植物 ID；无法追溯则为空 |
| 植物类型 | 伤害来源植物类型；无法追溯则为空 |
| 植物路 | 伤害来源植物所在行；无法追溯则为空 |
| 植物列 | 伤害来源植物所在列；无法追溯则为空 |

特别指出，使用 `AReplay` 功能时，游戏对象 ID 可能重复。

## 使用示例
```cpp
#include "damage_recorder/damage_recorder.h"

DamageRecorder damageRecorder;
void AScript(){
    damageRecorder.SetPath("D:\\AAA_LH_SL");
    damageRecorder.Setup();
}
```

不调用 `SetPath()` 时，默认输出到：

```text
C:\ProgramData\PopCap Games\PlantsVsZombies
```

---
特别鸣谢：[vector-wlc](https://github.com/vector-wlc)

如有问题请联系开发者：[Leonhard_qwq(bilibili)](https://space.bilibili.com/171987343)