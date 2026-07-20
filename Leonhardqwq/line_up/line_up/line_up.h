#ifndef LINE_UP_H
#define LINE_UP_H

#include <cstdio>

#include "avz.h"

namespace line_up {

constexpr int ROWS = 6;
constexpr int COLS = 9;
constexpr int LADDER = 0x30;
constexpr int RAKE = 0x31;
constexpr int GRAVESTONE = 0x32;

struct Grid {
    int plant = -1;
    bool plantImitator = false;
    bool awake = true;
    int base = -1;
    bool baseImitator = false;
    bool pumpkin = false;
    bool pumpkinImitator = false;
    bool coffee = false;
    bool coffeeImitator = false;
    bool ladder = false;
};

struct Lineup {
    std::array<Grid, ROWS * COLS> grids {};
    int scene = -1;
    int rakeRow = -1;

    Grid& at(int row, int col) {
        return grids[row * COLS + col];
    }

    const Grid& at(int row, int col) const {
        return grids[row * COLS + col];
    }
};

namespace detail {

inline bool isDayScene(int scene) {
    return ARangeIn(scene, {0, 2, 4});
}

inline bool canSleep(int type) {
    return ARangeIn(type, {APUFF_SHROOM, ASUN_SHROOM, AFUME_SHROOM,
        AHYPNO_SHROOM, ASCAREDY_SHROOM, AICE_SHROOM, ADOOM_SHROOM,
        ASEA_SHROOM, AMAGNET_SHROOM, AGLOOM_SHROOM});
}

inline int rowCount(int scene) {
    return ARangeIn(scene, {2, 3}) ? 6 : 5;
}

inline bool isValid(const Lineup& lineup) {
    if (lineup.scene < 0 || lineup.scene > 5 || lineup.rakeRow < -1
        || (lineup.rakeRow >= rowCount(lineup.scene))
        || (lineup.rakeRow >= 0 && ARangeIn(lineup.scene, {2, 3})
            && ARangeIn(lineup.rakeRow, {2, 3})))
        return false;

    int rows = rowCount(lineup.scene);
    for (int i = 0; i < ROWS * COLS; ++i) {
        const Grid& grid = lineup.grids[i];
        bool occupied = grid.plant != -1 || grid.base != -1 || grid.pumpkin
            || grid.coffee || grid.ladder;
        if ((i / COLS >= rows && occupied)
            || (grid.plant != -1
                && (grid.plant < APEASHOOTER || grid.plant > ACOB_CANNON
                    || ARangeIn(grid.plant, {ALILY_PAD, APUMPKIN, AFLOWER_POT, ACOFFEE_BEAN})))
            || (grid.plant == -1 && grid.plantImitator)
            || (grid.base != -1 && !ARangeIn(grid.base, {ALILY_PAD, AFLOWER_POT, GRAVESTONE}))
            || (grid.base == -1 && grid.baseImitator)
            || (grid.base == GRAVESTONE && grid.baseImitator)
            || (!grid.pumpkin && grid.pumpkinImitator)
            || (!grid.coffee && grid.coffeeImitator))
            return false;
    }
    return true;
}

inline APlant* putPlant(int row, int col, int type, bool imitator) {
    int seedType = imitator ? AIMITATOR : type;
    int imitatorType = imitator ? type : -1;
    APlant* plant;
    asm volatile(
        "movl %[row], %%eax;"
        "movl %[col], %%edi;"
        "movl %[seedType], %%ebx;"
        "movl %[imitatorType], %%esi;"
        "movl 0x6a9ec0, %%ecx;"
        "movl 0x768(%%ecx), %%ecx;"
        "pushl %%esi;"
        "pushl %%ebx;"
        "pushl %%edi;"
        "pushl %%ecx;"
        "movl $0x40d120, %%edx;"
        "call *%%edx;"
        "movl %%eax, %[plant];"
        : [plant] "=m"(plant)
        : [row] "m"(row), [col] "m"(col), [seedType] "m"(seedType),
          [imitatorType] "m"(imitatorType)
        : ASaveAllRegister, "memory");
    if (!plant || !imitator)
        return plant;
    asm volatile(
        "movl %[plant], %%esi;"
        "movl $0x466b80, %%eax;"
        "call *%%eax;"
        :
        : [plant] "m"(plant)
        : ASaveAllRegister, "memory");
    auto* main = AGetMainObject();
    int index = plant->Index();
    plant->Id() = main->PlantNext();
    main->PlantNext() = index;
    --main->PlantCount();
    return AGetPlantPtr(row + 1, col + 1, type);
}

inline void setSleeping(APlant* plant, bool sleeping) {
    int isSleeping = sleeping;
    asm volatile(
        "movl %[plant], %%eax;"
        "movl %[sleeping], %%ecx;"
        "pushl %%ecx;"
        "movl $0x45e860, %%edx;"
        "call *%%edx;"
        :
        : [plant] "m"(plant), [sleeping] "m"(isSleeping)
        : ASaveAllRegister, "memory");
}

inline void removePlaceItem(APlaceItem* item) {
    asm volatile(
        "movl %[item], %%esi;"
        "movl $0x44d000, %%eax;"
        "call *%%eax;"
        :
        : [item] "m"(item)
        : ASaveAllRegister, "memory");
}

inline void putGravestone(int row, int col) {
    asm volatile(
        "movl %[row], %%edi;"
        "movl %[col], %%ebx;"
        "movl 0x6a9ec0, %%edx;"
        "movl 0x768(%%edx), %%edx;"
        "movl 0x160(%%edx), %%edx;"
        "pushl %%edx;"
        "movl $0x426620, %%eax;"
        "call *%%eax;"
        :
        : [row] "m"(row), [col] "m"(col)
        : ASaveAllRegister, "memory");
}

inline void putLadder(int row, int col) {
    asm volatile(
        "movl %[row], %%edi;"
        "movl %[col], %%edx;"
        "pushl %%edx;"
        "movl 0x6a9ec0, %%eax;"
        "movl 0x768(%%eax), %%eax;"
        "movl $0x408f40, %%edx;"
        "call *%%edx;"
        :
        : [row] "m"(row), [col] "m"(col)
        : ASaveAllRegister, "memory");
}

inline void putRake(int row) {
    auto rowCode = AMRef<std::array<uint8_t, 7>>(0x40bb25);
    auto colCode = AMRef<std::array<uint8_t, 8>>(0x40ba8e);
    uint16_t unlimitedCode = AMRef<uint16_t>(0x40b9e2);

    AMRef<std::array<uint8_t, 7>>(0x40bb25) = {0xba, 0, 0, 0, 0, 0x90, 0x90};
    AMRef<int>(0x40bb26) = row;
    AMRef<int>(0x40ba92) = 7;
    AMRef<uint16_t>(0x40b9e2) = 0x800f;
    asm volatile("" ::: "memory");
    asm volatile(
        "movl 0x6a9ec0, %%edx;"
        "movl 0x768(%%edx), %%edx;"
        "pushl %%edx;"
        "movl $0x40b9c0, %%eax;"
        "call *%%eax;"
        :
        :
        : ASaveAllRegister, "memory");

    AMRef<std::array<uint8_t, 7>>(0x40bb25) = rowCode;
    AMRef<std::array<uint8_t, 8>>(0x40ba8e) = colCode;
    AMRef<uint16_t>(0x40b9e2) = unlimitedCode;
}

inline void setScene(int scene) {
    auto* main = AGetMainObject();
    auto* mowers = main->MPtr<uint8_t>(0x100);
    for (int i = 0; i < main->MRef<int>(0x104); ++i) {
        auto* mower = mowers + i * 0x48;
        if (mower[0x30])
            continue;
        asm volatile(
            "movl %[mower], %%eax;"
            "movl $0x458d10, %%edx;"
            "call *%%edx;"
            :
            : [mower] "m"(mower)
            : ASaveAllRegister, "memory");
    }
    for (int row = 0; row < ROWS; ++row)
        if (main->MRef<int>(0x624 + row * 4) > 0)
            main->MRef<int>(0x624 + row * 4) = 1;

    bool hasPool = ARangeIn(scene, {2, 3});
    for (int row = 0; row < ROWS; ++row) {
        int rowType = hasPool && ARangeIn(row, {2, 3}) ? 2 : (!hasPool && row == 5 ? 0 : 1);
        main->MRef<int>(0x5d8 + row * 4) = rowType;
        for (int col = 0; col < COLS; ++col)
            main->MRef<int>(0x168 + (col * ROWS + row) * 4)
                = rowType == 2 ? 3 : (rowType == 0 ? 2 : 1);
    }
    if (isDayScene(scene) && main->MRef<int>(0x5538) <= 0)
        main->MRef<int>(0x5538) = 425;

    main->Scene() = scene;
    asm volatile(
        "movl 0x6a9ec0, %%esi;"
        "movl 0x768(%%esi), %%esi;"
        "movl $0x40a160, %%eax;"
        "call *%%eax;"
        :
        :
        : ASaveAllRegister, "memory");
}

} // namespace detail

inline std::string toString(const Lineup& lineup) {
    static constexpr int sceneCodes[] = {2, 3, 0, 1, 4, 5};
    if (!detail::isValid(lineup))
        return {};

    std::string result = std::to_string(sceneCodes[lineup.scene]);
    auto add = [&](int type, int row, int col, int state = 0, bool imitator = false) {
        result += std::format(",{:X} {} {} {} 0 {}", type, row + 1, col + 1,
            state, int(imitator));
    };
    if (lineup.rakeRow >= 0)
        add(RAKE, lineup.rakeRow, 7);

    for (int i = 0; i < ROWS * COLS; ++i) {
        const Grid& grid = lineup.grids[i];
        int row = i / COLS;
        int col = i % COLS;
        if (ARangeIn(grid.base, {ALILY_PAD, AFLOWER_POT}))
            add(grid.base, row, col, 0, grid.baseImitator);
        if (grid.plant != -1)
            add(grid.plant, row, col,
                int(detail::canSleep(grid.plant) && grid.awake), grid.plantImitator);
        if (grid.pumpkin)
            add(APUMPKIN, row, col, 0, grid.pumpkinImitator);
        if (grid.coffee)
            add(ACOFFEE_BEAN, row, col, 0, grid.coffeeImitator);
        if (grid.base == GRAVESTONE)
            add(GRAVESTONE, row, col);
        if (grid.ladder)
            add(LADDER, row, col);
    }
    return result;
}

inline Lineup fromString(const std::string& text) {
    static constexpr int scenes[] = {2, 3, 0, 1, 4, 5};
    Lineup lineup;
    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return lineup;
    size_t last = text.find_last_not_of(" \t\r\n");
    std::string code = text.substr(first, last - first + 1);
    if (code.back() == ',' || code[0] < '0' || code[0] > '5'
        || (code.size() > 1 && code[1] != ','))
        return lineup;
    lineup.scene = scenes[code[0] - '0'];

    for (size_t begin = 2; begin < code.size();) {
        size_t end = code.find(',', begin);
        std::string item = code.substr(begin, end - begin);
        unsigned type;
        int row, col, state, reserved, imitator, used = 0;
        if (std::sscanf(item.c_str(), "%x %d %d %d %d %d %n",
                &type, &row, &col, &state, &reserved, &imitator, &used)
                != 6
            || used != int(item.size()) || type > GRAVESTONE
            || row < 1 || row > ROWS || col < 1 || col > COLS
            || state < 0 || state > 2 || reserved < 0 || reserved > 4
            || imitator < 0 || imitator > 1)
            return {};

        Grid& grid = lineup.at(row - 1, col - 1);
        switch (type) {
            case ALILY_PAD:
            case AFLOWER_POT:
                grid.base = type;
                grid.baseImitator = imitator;
                break;
            case APUMPKIN:
                grid.pumpkin = true;
                grid.pumpkinImitator = imitator;
                break;
            case ACOFFEE_BEAN:
                grid.coffee = true;
                grid.coffeeImitator = imitator;
                break;
            case LADDER:
                grid.ladder = true;
                break;
            case RAKE:
                lineup.rakeRow = row - 1;
                break;
            case GRAVESTONE:
                grid.base = GRAVESTONE;
                grid.baseImitator = false;
                break;
            default:
                grid.plant = type;
                grid.plantImitator = imitator;
                grid.awake = state != 0 || !detail::isDayScene(lineup.scene)
                    || !detail::canSleep(type);
        }
        begin = end == std::string::npos ? code.size() : end + 1;
    }
    return detail::isValid(lineup) ? lineup : Lineup {};
}

inline Lineup getLineup() {
    Lineup lineup;
    if (!AGetPvzBase() || !AGetMainObject()
        || !ARangeIn(AGetPvzBase()->GameUi(), {2, 3}))
        return lineup;

    lineup.scene = AGetMainObject()->Scene();
    for (auto& plant : aAlivePlantFilter) {
        if (plant.Row() < 0 || plant.Row() >= ROWS
            || plant.Col() < 0 || plant.Col() >= COLS
            || plant.Type() < APEASHOOTER || plant.Type() > ACOB_CANNON)
            continue;

        Grid& grid = lineup.at(plant.Row(), plant.Col());
        bool imitator = plant.MRef<int>(0x138) == AIMITATOR;
        switch (plant.Type()) {
            case ALILY_PAD:
            case AFLOWER_POT:
                grid.base = plant.Type();
                grid.baseImitator = imitator;
                break;
            case APUMPKIN:
                grid.pumpkin = true;
                grid.pumpkinImitator = imitator;
                break;
            case ACOFFEE_BEAN:
                grid.coffee = true;
                grid.coffeeImitator = imitator;
                break;
            default:
                grid.plant = plant.Type();
                grid.plantImitator = imitator;
                grid.awake = !plant.IsSleeping();
        }
    }

    for (auto& item : aAlivePlaceItemFilter) {
        if (item.Row() < 0 || item.Row() >= ROWS
            || item.Col() < 0 || item.Col() >= COLS)
            continue;
        Grid& grid = lineup.at(item.Row(), item.Col());
        if (item.Type() == APlaceItemType::GRAVESTONE) {
            grid.base = GRAVESTONE;
            grid.baseImitator = false;
        } else if (item.Type() == APlaceItemType::LADDER)
            grid.ladder = true;
        else if (item.Type() == APlaceItemType::RAKE)
            lineup.rakeRow = item.Row();
    }
    return lineup;
}

inline bool setLineup(const Lineup& lineup, int reservedSlots = 0) {
    if (!AGetPvzBase() || !AGetMainObject()
        || !ARangeIn(AGetPvzBase()->GameUi(), {2, 3})
        || !detail::isValid(lineup))
        return false;

    auto* main = AGetMainObject();
    int plantCount = 0;
    bool hasImitator = false;
    for (const auto& grid : lineup.grids) {
        plantCount += int(ARangeIn(grid.base, {ALILY_PAD, AFLOWER_POT}))
            + int(grid.plant != -1) + int(grid.pumpkin) + int(grid.coffee);
        hasImitator |= grid.baseImitator || grid.plantImitator
            || grid.pumpkinImitator || grid.coffeeImitator;
    }
    if (reservedSlots < 0
        || reservedSlots + plantCount + int(hasImitator) > main->PlantLimit())
        return false;

    std::vector<APlaceItem*> items;
    for (auto& item : aAlivePlaceItemFilter)
        if (ARangeIn(item.Type(), {APlaceItemType::GRAVESTONE, APlaceItemType::CRATER,
                APlaceItemType::LADDER, APlaceItemType::RAKE}))
            items.push_back(&item);
    for (APlaceItem* item : items)
        detail::removePlaceItem(item);

    APlant* plants = main->PlantArray();
    for (int i = 0; i < main->PlantCountMax(); ++i) {
        if (!(plants[i].Id() >> 16))
            continue;
        if (!plants[i].IsDisappeared())
            AAsm::RemovePlant(plants + i);
        plants[i].Id() = 0;
    }
    main->PlantCountMax() = main->PlantNext() = reservedSlots;
    main->PlantCount() = 0;
    for (int i = 0; i < reservedSlots; ++i)
        plants[i].Id() = 0;

    if (lineup.scene != AGetMainObject()->Scene())
        detail::setScene(lineup.scene);

    if (lineup.rakeRow >= 0)
        detail::putRake(lineup.rakeRow);

    for (int i = 0; i < ROWS * COLS; ++i) {
        const Grid& grid = lineup.grids[i];
        if (ARangeIn(grid.base, {ALILY_PAD, AFLOWER_POT}))
            detail::putPlant(i / COLS, i % COLS, grid.base, grid.baseImitator);
    }
    for (int i = 0; i < ROWS * COLS; ++i) {
        const Grid& grid = lineup.grids[i];
        if (grid.plant == -1)
            continue;
        APlant* plant = detail::putPlant(i / COLS, i % COLS,
            grid.plant, grid.plantImitator);
        if (!plant)
            continue;
        if (detail::isDayScene(lineup.scene) && detail::canSleep(grid.plant) && grid.awake)
            detail::setSleeping(plant, false);
        if (ARangeIn(grid.plant, {APOTATO_MINE, ASUN_SHROOM}))
            plant->StateCountdown() = 1;
        if (grid.plant == ACOB_CANNON) {
            plant->State() = 37;
            plant->StateCountdown() = 0;
            float rate = 12.0f;
            asm volatile(
                "movl %[plant], %%edi;"
                "movl %[rate], %%eax;"
                "pushl %%eax;"
                "movl $0x468280, %%edx;"
                "call *%%edx;"
                :
                : [plant] "m"(plant), [rate] "m"(rate)
                : ASaveAllRegister, "memory");
        }
    }
    for (int i = 0; i < ROWS * COLS; ++i)
        if (lineup.grids[i].pumpkin)
            detail::putPlant(i / COLS, i % COLS, APUMPKIN,
                lineup.grids[i].pumpkinImitator);
    for (int i = 0; i < ROWS * COLS; ++i)
        if (lineup.grids[i].coffee)
            detail::putPlant(i / COLS, i % COLS, ACOFFEE_BEAN,
                lineup.grids[i].coffeeImitator);
    for (int i = 0; i < ROWS * COLS; ++i)
        if (lineup.grids[i].base == GRAVESTONE)
            detail::putGravestone(i / COLS, i % COLS);
    for (int i = 0; i < ROWS * COLS; ++i)
        if (lineup.grids[i].ladder)
            detail::putLadder(i / COLS, i % COLS);
    for (int i = reservedSlots - 1; i >= 0; --i) {
        plants[i].Id() = main->PlantNext();
        main->PlantNext() = i;
    }
    return true;
}

inline std::string getLineupString() {
    return toString(getLineup());
}

inline bool setLineup(const std::string& text, int reservedSlots = 0) {
    return setLineup(fromString(text), reservedSlots);
}

} // namespace line_up

#endif
