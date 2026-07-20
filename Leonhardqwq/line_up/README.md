# line_up

读取、输出和设置 PvZ 战场布阵，布阵码兼容 [PvZ Toolkit](https://github.com/lmintlcx/pvztoolkit)。

## 功能

- 读取当前植物、睡眠状态、模仿者、墓碑、梯子和钉耙。
- 在 `Lineup` 与 PvZ Toolkit 布阵码之间转换。
- 清空现有布阵并按目标场景重新布置。
- 生成的玉米加农炮可立即发射。
- 可预留指定数量的低编号植物内存槽位。

## 使用

```cpp
#include "line_up/line_up.h"

void AScript()
{
    std::string code = line_up::getLineupString();
    line_up::setLineup(code);
}
```

预留前 `20` 个植物内存槽位：

```cpp
line_up::setLineup(code, 20);
```

## 接口

| 接口 | 作用 |
| --- | --- |
| `getLineup()` | 读取当前布阵；失败时返回 `scene == -1` 的 `Lineup` |
| `setLineup(lineup, reservedSlots)` | 设置 `Lineup`，第二个参数默认为 `0` |
| `getLineupString()` | 输出当前布阵的 PvZ Toolkit 布阵码 |
| `toString(lineup)` | 将 `Lineup` 转为布阵码 |
| `fromString(code)` | 将布阵码解析为 `Lineup` |

## 注意

- 仅在选卡或战斗界面使用。
- `setLineup()` 会清除当前植物及相关场地物件；无效输入返回 `false` 且不会清场。
- `Lineup::at()` 的行列下标从 `0` 开始，布阵码中的行列从 `1` 开始。
