# 落点显示器

当一次性植物生效时，在 PvZ 画面上展示其信息：

- 冰、核、樱、辣：生效时间、所在格
- 窝瓜：生效时间、所在格、落点横坐标
- 炮弹：生效时间、落点横坐标
  - 在天台场景下，额外显示发射源所在格以及[上界之风](https://wiki.pvz1.com/doku.php?id=%E6%94%BB%E7%95%A5:%E6%96%9C%E5%9D%A1%E4%B8%8E%E4%B8%8A%E7%95%8C%E4%B9%8B%E9%A3%8E)导致的纵向偏移量

为方便展示，落点坐标保留小数点后两位；这一数据可以直接输入 AvZ 2.9.1 及以后的版本，不会损失任何信息。

用法：

```cpp
#include <cob_marker/cob_marker.h>

// 每条信息默认停留 300 帧，可自行修改
CobMarker cobMarker(300);

void AScript() {
    cobMarker.Start();
}
```
