#ifndef _REISEN_COB_MARKER_COB_MARKER_H_
#define _REISEN_COB_MARKER_COB_MARKER_H_

#include <avz.h>
#include <asm_insert_code/asm_insert_code.h>

class CobMarker : public AOrderedEnterFightHook<-1> {
protected:
    std::unordered_map<AProjectile*, AGrid> cobSource;
    AAsmCodeHandle ashHook {std::nullopt};
    AAsmCodeHandle squashHook {std::nullopt};
    AAsmCodeHandle registerCobHook {std::nullopt};
    AAsmCodeHandle cobHook {std::nullopt};

    int textDuration;
    std::vector<int> displayQueue[6];

    inline static const std::unordered_map<int, std::string> ashToName {
        {ACHERRY_BOMB, "A"},
        {AICE_SHROOM, "I"},
        {ADOOM_SHROOM, "N"},
        {AJALAPENO, "J"},
    };

    size_t _InsertToQueue(int idx) {
        int time = AGetPvzBase()->MjClock();
        for (size_t i = 0; i < displayQueue[idx].size(); ++i) {
            if (time - displayQueue[idx][i] > textDuration) {
                displayQueue[idx][i] = time;
                return i;
            }
        }
        displayQueue[idx].push_back(time);
        return displayQueue[idx].size() - 1;
    }

    std::pair<int, int> _GetDisplayPosition(int position, int row) {
        if (aFieldInfo.isRoof) {
            return {747 - 57 * position, row * 85 + 88};
        } else if (aFieldInfo.rowHeight == 100) {
            return {756 - 46 * (position / 2), row * 100 + 85 + (position % 2) * 40};
        } else {
            return {756 - 46 * (position / 2), row * 85 + 100 + (position % 2) * 40};
        }
    }

    virtual void _EnterFight() override {
        cobSource.clear();
        for (int i = 0; i < 6; ++i) {
            displayQueue[i].clear();
        }
    }

public:
    CobMarker(int textDuration = 300) : textDuration(textDuration) {}

    void Start() {
        ashHook = AInsertSharedAsmCode(0x4666a0, [this](AAsmCodeContext* context) {
            auto plant = *(APlant**)(context->esp + 4);
            if (!ashToName.contains(plant->Type())) {
                return;
            }
            std::string msg = std::format("{}\n{}-{}{}",
                ANowTime(false).time + 1,
                plant->Row() + 1, plant->Col() + 1,
                ashToName.at(plant->Type())
            );
            int textX = plant->Col() * 80 + 54;
            int textY = AAsm::GridToOrdinate(plant->Row(), plant->Col()) + 55;
            if (plant->Col() == 8) {
                int position = _InsertToQueue(plant->Row());
                std::tie(textX, textY) = _GetDisplayPosition(position, plant->Row());
            }
            aPainter.SetFontSize(20);
            aPainter.Draw(AText(msg, textX, textY), textDuration);
        });

        squashHook = AInsertSharedAsmCode(0x4606f0, [this](AAsmCodeContext* context) {
            auto plant = *(APlant**)(context->esp + 4);
            std::string msg = std::format("{}\n{}-{}W\n{:.02f}",
                ANowTime(false).time + 1,
                plant->Row() + 1, plant->Col() + 1,
                (plant->Abscissa() + 40) / 80.0f
            );
            int textX = plant->Col() * 80 + 54;
            int textY = AAsm::GridToOrdinate(plant->Row(), plant->Col()) + 35;
            if (plant->Col() == 8) {
                int position = _InsertToQueue(plant->Row());
                std::tie(textX, textY) = _GetDisplayPosition(position, plant->Row());
            }
            aPainter.SetFontSize(20);
            aPainter.Draw(AText(msg, textX, textY), textDuration);
        });

        registerCobHook = AInsertSharedAsmCode(0x46741b, [this](AAsmCodeContext* context) {
            auto cob = (AProjectile*)(context->esi);
            auto plant = (APlant*)(context->ebp);
            cobSource[cob] = {plant->Row() + 1, plant->Col() + 1};
        });

        auto cobHookFunc = [this](AAsmCodeContext* context) {
            auto p = (AProjectile*)(context->ebp);
            int row = p->CobTargetRow();
            float col = p->CobTargetAbscissa() / 80.0f;
            std::string msg = std::format("{}\n{:.02f}", ANowTime(false).time + 1, col);

            int position = _InsertToQueue(row);
            auto [textX, textY] = _GetDisplayPosition(position, row);
            aPainter.SetFontSize(20);
            aPainter.Draw(AText(msg, textX, textY), textDuration);
            if (cobSource.contains(p)) {
                cobSource.erase(p);
            }
        };

        auto cobHookFuncRoof = [this](AAsmCodeContext* context) {
            auto p = (AProjectile*)(context->ebp);
            int row = p->CobTargetRow();
            float col = p->CobTargetAbscissa() / 80.0f;

            std::string source = "?-?";
            if (cobSource.contains(p)) {
                auto [sRow, sCol] = cobSource[p];
                source = std::format("{}-{}", sRow, sCol);
                cobSource.erase(p);
            }

            int dropY = int(p->MRef<float>(0x34));
            int fixedY = int(70 + row * 85 + fmax(6 - col, 0) * 20);
            std::string delta;
            if (dropY < fixedY) {
                delta = std::format("↑{}", fixedY - dropY);
            } else if (dropY > fixedY) {
                delta = std::format("↓{}", dropY - fixedY);
            } else {
                delta = "=";
            }

            std::string msg = std::format("{:<5}\n{:.02f}\n{}\n{}",
                ANowTime(false).time + 1,
                col,
                source,
                delta
            );

            int position = _InsertToQueue(row);
            auto [textX, textY] = _GetDisplayPosition(position, row);
            aPainter.SetFontSize(20);
            aPainter.Draw(AText(msg, textX, textY), textDuration);
        };

        if (aFieldInfo.isRoof) {
            cobHook = AInsertSharedAsmCode(0x46d85b, cobHookFuncRoof);
        } else {
            cobHook = AInsertSharedAsmCode(0x46d85b, cobHookFunc);
        }
    }
};

#endif
