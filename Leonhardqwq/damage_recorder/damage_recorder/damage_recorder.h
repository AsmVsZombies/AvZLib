//#pragma once
#ifndef __DAMAGE_RECORDER_H__
#define __DAMAGE_RECORDER_H__

#include "asm_insert_code/asm_insert_code.h"
#include <climits>


class BasicInfo{
public:
    struct ZombieInfo{
        uint32_t id;
        int type;
        int wave;
        int row;
        int xint;
        int hp;
        bool below_critical;

        ZombieInfo(): id(0), type(-1), wave(-1), row(-1), xint(-1), hp(-1), below_critical(false) {}
        ZombieInfo(AZombie* zombie){
            id = zombie->Id();
            type = zombie->Type();
            wave = zombie->AtWave();
            row = zombie->Row();
            xint = zombie->MRef<int>(0x8);
            hp = zombie->Hp() + zombie->OneHp() + zombie->TwoHp();
            below_critical = !zombie->MRef<bool>(0xBA);
        }
    };

    struct PlantInfo{
        uint32_t id;
        int type;
        int row;
        int col;

        PlantInfo(): id(0), type(-1), row(-1), col(-1) {}
        PlantInfo(APlant* plant){
            id = plant->Id();
            type = plant->Type();
            row = plant->Row();
            col = plant->Col();
        }
    };

    struct ProjectileInfo{
        uint32_t id;
        int type;
        int time_exist;

        ProjectileInfo(): id(0), type(-1), time_exist(-1) {}
        ProjectileInfo(AProjectile* projectile){
            id = projectile->Id();
            type = projectile->Type();
        }
    };

    ZombieInfo z;
    PlantInfo p;
    ProjectileInfo proj;
    BasicInfo(): z(), p(), proj() {}

    void GetZombieInfo(AZombie* zombie){z = ZombieInfo(zombie);}
    void GetPlantInfo(APlant* plant){p = PlantInfo(plant);}
    void GetProjectileInfo(AProjectile* projectile){proj = ProjectileInfo(projectile);}
    void GetProjectileExistTime(AProjectile* projectile){
        proj.time_exist = projectile->ExistTime();
    }
};

class DamageInfo : public BasicInfo {
public:
    int damage;
    ATime time;
    int time_clock; 
    DamageInfo():damage(-1), time(), time_clock(-1) {}
};


class DamageRecorder : public AStateHook {
protected:
    char damage_record_path[1000] = "C:\\ProgramData\\PopCap Games\\PlantsVsZombies";
    std::string record_time_str;
    int record_start_clock = 0;
    ATickRunner trace_record_runner;
    static BasicInfo projectile_mark;
    static std::vector<DamageInfo> damage_records;
    static std::vector<DamageInfo> projectile_records;
    static std::vector<BasicInfo> source_records;


    static void trace_record(){
        // 回溯弹出记录
        ATime now_time = ANowTime();
        while(!damage_records.empty()){
            auto& dmg = damage_records.back();
            if( now_time.wave > dmg.time.wave 
            ||  (now_time.wave == dmg.time.wave && now_time.time >= dmg.time.time) )
                break;
            damage_records.pop_back();
        }
        while(!projectile_records.empty()){
            auto& dmg = projectile_records.back();
            if( now_time.wave > dmg.time.wave 
            ||  (now_time.wave == dmg.time.wave && now_time.time >= dmg.time.time) )
                break;
            projectile_records.pop_back();
        }
    }

    virtual void _EnterFight() override {
        damage_records.clear();
        projectile_records.clear();
        projectile_mark = BasicInfo();
        source_records.clear();
        record_start_clock = AGetMainObject()->GameClock();
        auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%Y-%m-%d-%H-%M-%S");
        record_time_str = ss.str();
        trace_record_runner.Start(trace_record, ATickRunner::GLOBAL);
    }

    virtual void _ExitFight() override {
        if (damage_records.empty()) {
            projectile_records.clear();
            return;
        }

        std::map<std::pair<uint32_t, int>, BasicInfo::PlantInfo> projectile_index;
        // 重复 key 时保留最后一条，与原先反向遍历 projectile_records 的逻辑一致
        for (const auto& record : projectile_records)
            projectile_index[{record.proj.id, record.time_clock}] = record.p;

        for (auto& dmg : damage_records){
            // 合并子弹表到伤害表
            if (dmg.proj.id != 0){
                auto it = projectile_index.find({dmg.proj.id, dmg.time_clock + 1 - dmg.proj.time_exist});
                if (it != projectile_index.end())
                    dmg.p = it->second;
            }
        }

        // 写入文件

        // 构造文件路径
        std::string file_path = damage_record_path + std::string("/") + "Records-" + record_time_str + ".csv";

        // 打开文件
        std::ofstream ofs(file_path);

        // 子弹名字
        std::map<int, std::string> projectile_type_names = {
            {0, "豌豆"},
            {1, "冰豌豆"},
            {2, "卷心菜"},
            {3, "西瓜"},
            {4, "孢子"},
            {5, "冰瓜"},
            {6, "火豌豆"},
            {7, "星星"},
            {8, "尖刺"},
            {9, "篮球"},
            {10, "玉米粒"},
            {11, "玉米炮"},
            {12, "黄油"},
            {13, "僵尸豌豆"},
        };

        // 写入表头
        ofs << "波次,时间,绝对时间,伤害,"
            << "僵尸ID,僵尸类型,僵尸路,僵尸坐标,僵尸波次,僵尸总血量,僵尸临界后,"
            << "子弹ID,子弹类型,"
            << "植物ID,植物类型,植物路,植物列\n";

        // 写入数据
        for (const auto& dmg : damage_records){
            ofs << dmg.time.wave << ","
                << dmg.time.time + 1 << ","
                << dmg.time_clock + 1 - record_start_clock << ","
                << dmg.damage << ",";

            // 僵尸信息
            ofs << dmg.z.id << ",";
            ofs << std::format("{}", static_cast<AZombieType>(dmg.z.type));
            ofs << ",";
            ofs << dmg.z.row+1 << ","
                << dmg.z.xint << ","
                << dmg.z.wave+1 << ","
                << dmg.z.hp << ","
                << dmg.z.below_critical << ",";
            // 子弹信息
            if (dmg.proj.id != 0)    ofs << dmg.proj.id;
            ofs << ",";
            if (dmg.proj.type >= 0) {
                auto pt_it = projectile_type_names.find(dmg.proj.type);
                if (pt_it != projectile_type_names.end())    ofs << pt_it->second;
            }
            ofs << ",";
            // 植物信息
            if (dmg.p.id != 0)    ofs << dmg.p.id;
            ofs << ",";
            if (dmg.p.type >= 0)    ofs << __aCardManager.GetCardName(static_cast<APlantType>(dmg.p.type));
            ofs << ",";
            if (dmg.p.row >= 0)    ofs << dmg.p.row+1;
            ofs << ",";
            if (dmg.p.col >= 0)    ofs << dmg.p.col+1;
            ofs << "\n";
        }
        ofs.close();

        damage_records.clear();
        projectile_records.clear();
    }

    static bool isInvalidZombieForTakeDamage(AZombie* zombie){
        return zombie == nullptr || zombie->IsDead() || zombie->State() == 16;
    }

    static BasicInfo makePlantSource(APlant* plant){
        BasicInfo mark;
        if (plant != nullptr) mark.GetPlantInfo(plant);
        return mark;
    }

    static BasicInfo makeProjectileSource(AProjectile* projectile){
        BasicInfo mark;
        if (projectile != nullptr) {
            mark.GetProjectileInfo(projectile);
            mark.GetProjectileExistTime(projectile);
        }
        return mark;
    }

    static void pushSource(const BasicInfo& source){
        source_records.push_back(source);
    }

    static void popSource(){
        if (!source_records.empty()) source_records.pop_back();
    }

    static void pushDamage(AZombie* zombie, int damage, const BasicInfo& source){
        trace_record();

        DamageInfo dmg_info;
        // 记录伤害与时间
        dmg_info.damage = damage;
        dmg_info.time = ANowTime();
        dmg_info.time_clock = AGetMainObject()->GameClock();
        // 记录僵尸信息
        dmg_info.GetZombieInfo(zombie);
        // 记录来源信息
        dmg_info.p = source.p;
        dmg_info.proj = source.proj;

        damage_records.push_back(dmg_info);
    }

    static void recordDamage(AZombie* zombie, int damage, const BasicInfo& source){
        if (isInvalidZombieForTakeDamage(zombie)) return;
        pushDamage(zombie, damage, source);
    }

    static void recordCurrentSourceDamage(AZombie* zombie, int damage){
        if (source_records.empty()) return;
        recordDamage(zombie, damage, source_records.back());
    }

    static void recordDirectKill(AZombie* zombie, const BasicInfo& source){
        if (zombie == nullptr) return;
        pushDamage(zombie, INT_MAX, source);
    }

    static void recordCurrentSourceDirectKill(AZombie* zombie){
        if (source_records.empty()) return;
        recordDirectKill(zombie, source_records.back());
    }

    static void recordProjectileSource(AProjectile* projectile, APlant* plant){
        if (projectile == nullptr || plant == nullptr) return;

        trace_record();

        projectile_mark = BasicInfo();
        projectile_mark.GetPlantInfo(plant);

        DamageInfo proj_info;
        proj_info.time = ANowTime();
        proj_info.time_clock = AGetMainObject()->GameClock();
        proj_info.GetProjectileInfo(projectile);
        proj_info.p = projectile_mark.p;

        projectile_records.push_back(proj_info);
    }

    void record_precise_damage(){
        // 记录大喷菇、忧郁菇、地刺、地刺王等行/范围植物的实际扣血
        AInsertUniqueAsmCode(
            // Plant::DoRowAreaDamage(int theDamage, unsigned int theDamageFlags): aZombie->TakeDamage(aDamage, theDamageFlags) 调用前
            0x45EE22, [](AAsmCodeContext * context) __stdcall {
                auto damage = AMRef<int>(context->esp);
                auto zombie = (AZombie*)context->esi;
                auto plant = (APlant*)context->ebp;
                recordDamage(zombie, damage, makePlantSource(plant));
        });

        // 记录窝瓜落地范围内造成的 1800 伤害
        AInsertUniqueAsmCode(
            // Plant::DoSquashDamage(): aZombie->TakeDamage(1800, 18U) 调用前
            0x4607B2, [](AAsmCodeContext * context) __stdcall {
                auto damage = AMRef<int>(context->esp);
                auto zombie = (AZombie*)context->esi;
                auto plant = (APlant*)AMRef<uintptr_t>(context->esp + 0x40);
                recordDamage(zombie, damage, makePlantSource(plant));
        });

        // 记录溅射子弹对直接命中目标和范围目标造成的伤害
        AInsertUniqueAsmCode(
            // Projectile::DoSplashDamage(Zombie* theZombie): aZombie->TakeDamage(aOriginalDamage/aSplashDamage, aDamageFlags) 调用前
            0x46D468, [](AAsmCodeContext * context) __stdcall {
                auto damage = AMRef<int>(context->esp);
                auto zombie = (AZombie*)context->esi;
                auto projectile = (AProjectile*)context->edi;
                recordDamage(zombie, damage, makeProjectileSource(projectile));
        });

        // 记录普通非溅射子弹命中目标造成的直伤
        AInsertUniqueAsmCode(
            // Projectile::DoImpact(Zombie* theZombie): theZombie->TakeDamage(GetProjectileDef().mDamage, aDamageFlags) 调用前
            0x46E07B, [](AAsmCodeContext * context) __stdcall {
                auto damage = AMRef<int>(context->esp);
                auto zombie = (AZombie*)context->esi;
                auto projectile = (AProjectile*)context->edi;
                recordDamage(zombie, damage, makeProjectileSource(projectile));
        });

        // 记录寒冰菇点冰命中时附带的 20 伤害
        AInsertUniqueAsmCode(
            // Zombie::HitIceTrap(): TakeDamage(20, 1U) 调用前
            0x532499, [](AAsmCodeContext * context) __stdcall {
                auto damage = AMRef<int>(context->esp);
                auto zombie = (AZombie*)context->esi;
                recordCurrentSourceDamage(zombie, damage);
        });

        // 记录巨人砸地刺王时受到的 20 反伤
        AInsertUniqueAsmCode(
            // Zombie::UpdateZombieGargantuar(): TakeDamage(20, 32U) 调用前
            0x526D81, [](AAsmCodeContext * context) __stdcall {
                auto damage = AMRef<int>(context->esp);
                auto zombie = (AZombie*)context->esi;
                auto plant = (APlant*)context->edi;
                recordDamage(zombie, damage, makePlantSource(plant));
        });

        // 记录范围结算中非燃烧分支造成的 1800 伤害
        AInsertUniqueAsmCode(
            // Board::KillAllZombiesInRadius(...): aZombie->TakeDamage(1800, 18U) 调用前
            0x41D93A, [](AAsmCodeContext * context) __stdcall {
                auto damage = AMRef<int>(context->esp);
                auto zombie = (AZombie*)context->esi;
                recordCurrentSourceDamage(zombie, damage);
        });

        // 记录灰烬烧灼命中高血量僵尸或僵王时的 1800 伤害
        AInsertUniqueAsmCode(
            // Zombie::ApplyBurn(): TakeDamage(1800, 18U) 调用前
            0x532FE5, [](AAsmCodeContext * context) __stdcall {
                auto damage = AMRef<int>(context->esp);
                auto zombie = (AZombie*)context->esi;
                recordCurrentSourceDamage(zombie, damage);
        });

        // 记录大嘴花咬巨人等不可吞噬目标时造成的 40 伤害
        AInsertUniqueAsmCode(
            // Plant::UpdateChomper(): aZombie->TakeDamage(40, 0U) 调用前
            0x4614E0, [](AAsmCodeContext * context) __stdcall {
                auto damage = AMRef<int>(context->esp);
                auto zombie = (AZombie*)context->esi;
                auto plant = (APlant*)context->edi;
                recordDamage(zombie, damage, makePlantSource(plant));
        });
    }

    void record_direct_kill(){
        // 记录灰烬烧灼直接消灭僵尸，伤害输出为 INT_MAX
        AInsertUniqueAsmCode(
            // Zombie::ApplyBurn(): DieWithLoot() 调用前
            0x532FC2, [](AAsmCodeContext * context) __stdcall {
                auto zombie = (AZombie*)(context->ecx ? context->ecx : context->esi);
                recordCurrentSourceDirectKill(zombie);
        });

        // 记录大嘴花普通吞噬直接消灭僵尸，伤害输出为 INT_MAX
        AInsertUniqueAsmCode(
            // Plant::UpdateChomper(): aZombie->DieWithLoot() 调用前
            0x4614F9, [](AAsmCodeContext * context) __stdcall {
                auto zombie = (AZombie*)(context->ecx ? context->ecx : context->esi);
                auto plant = (APlant*)context->edi;
                recordDirectKill(zombie, makePlantSource(plant));
        });

        // 记录缠绕水草直接消灭僵尸，伤害输出为 INT_MAX
        AInsertUniqueAsmCode(
            // Plant::UpdateTangleKelp(): aZombie->DieWithLoot() 调用前
            0x4679F7, [](AAsmCodeContext * context) __stdcall {
                auto zombie = (AZombie*)(context->ecx ? context->ecx : context->eax);
                auto plant = (APlant*)context->ebp;
                recordDirectKill(zombie, makePlantSource(plant));
        });
    }

    void mark_precise_damage_source(){
        // 压入寒冰菇来源，供 HitIceTrap 内部 20 点伤害使用
        AInsertUniqueAsmCode(
            // Plant::IceZombies(): aZombie->HitIceTrap() 调用前
            0x466444, [](AAsmCodeContext * context) __stdcall {
                auto plant = (APlant*)context->edi;
                pushSource(makePlantSource(plant));
        });
        // 弹出寒冰菇来源，避免影响后续伤害记录
        AInsertUniqueAsmCode(
            // Plant::IceZombies(): aZombie->HitIceTrap() 调用后
            0x466449, [](AAsmCodeContext * context) __stdcall {
                popSource();
        });

        // 压入樱桃炸弹来源，供范围灰烬结算使用
        AInsertUniqueAsmCode(
            // Plant::DoSpecial(): 樱桃炸弹 KillAllZombiesInRadius(...) 调用前
            0x4667E0, [](AAsmCodeContext * context) __stdcall {
                auto plant = (APlant*)context->ebx;
                pushSource(makePlantSource(plant));
        });
        // 弹出樱桃炸弹来源，结束本次范围灰烬上下文
        AInsertUniqueAsmCode(
            // Plant::DoSpecial(): 樱桃炸弹 KillAllZombiesInRadius(...) 调用后
            0x4667E5, [](AAsmCodeContext * context) __stdcall {
                popSource();
        });

        // 压入毁灭菇来源，供范围灰烬结算使用
        AInsertUniqueAsmCode(
            // Plant::DoSpecial(): 毁灭菇 KillAllZombiesInRadius(...) 调用前
            0x466841, [](AAsmCodeContext * context) __stdcall {
                auto plant = (APlant*)context->ebx;
                pushSource(makePlantSource(plant));
        });
        // 弹出毁灭菇来源，结束本次范围灰烬上下文
        AInsertUniqueAsmCode(
            // Plant::DoSpecial(): 毁灭菇 KillAllZombiesInRadius(...) 调用后
            0x466846, [](AAsmCodeContext * context) __stdcall {
                popSource();
        });

        // 压入土豆雷来源，供非燃烧范围 1800 伤害使用
        AInsertUniqueAsmCode(
            // Plant::DoSpecial(): 土豆雷 KillAllZombiesInRadius(...) 调用前
            0x466A6A, [](AAsmCodeContext * context) __stdcall {
                auto plant = (APlant*)context->ebx;
                pushSource(makePlantSource(plant));
        });
        // 弹出土豆雷来源，结束本次范围伤害上下文
        AInsertUniqueAsmCode(
            // Plant::DoSpecial(): 土豆雷 KillAllZombiesInRadius(...) 调用后
            0x466A6F, [](AAsmCodeContext * context) __stdcall {
                popSource();
        });

        // 压入玉米炮弹来源，后续再通过 projectile_records 回填植物来源
        AInsertUniqueAsmCode(
            // Projectile::UpdateLobMotion(): 玉米炮弹 KillAllZombiesInRadius(...) 调用前
            0x46D85B, [](AAsmCodeContext * context) __stdcall {
                auto projectile = (AProjectile*)context->ebp;
                pushSource(makeProjectileSource(projectile));
        });
        // 弹出玉米炮弹来源，结束本次范围灰烬上下文
        AInsertUniqueAsmCode(
            // Projectile::UpdateLobMotion(): 玉米炮弹 KillAllZombiesInRadius(...) 调用后
            0x46D860, [](AAsmCodeContext * context) __stdcall {
                popSource();
        });

        // 压入火爆辣椒来源，供 ApplyBurn 内部伤害或直接消灭使用
        AInsertUniqueAsmCode(
            // Plant::BurnRow(int theRow): aZombie->ApplyBurn() 调用前
            0x466528, [](AAsmCodeContext * context) __stdcall {
                auto plant = (APlant*)context->edi;
                pushSource(makePlantSource(plant));
        });
        // 弹出火爆辣椒来源，结束当前僵尸的燃烧上下文
        AInsertUniqueAsmCode(
            // Plant::BurnRow(int theRow): aZombie->ApplyBurn() 调用后
            0x46652D, [](AAsmCodeContext * context) __stdcall {
                popSource();
        });
    }

    void record_precise_projectile(){
        // 记录普通植物和玉米炮生成子弹时的植物来源
        AInsertUniqueAsmCode(
            // Plant::Fire(Zombie* theTargetZombie, int theRow, PlantWeapon thePlantWeapon): Board::AddProjectile(...) 返回后
            0x4672B5, [](AAsmCodeContext * context) __stdcall {
                auto plant = (APlant*)context->ebp;
                auto projectile = (AProjectile*)context->eax;
                recordProjectileSource(projectile, plant);
        });

        // 记录杨桃五发子弹生成时的植物来源
        AInsertUniqueAsmCode(
            // Plant::StarFruitFire(): ProjectileInitialize(...) 返回后
            0x45F80D, [](AAsmCodeContext * context) __stdcall {
                auto plant = (APlant*)context->esi;
                auto projectile = (AProjectile*)context->edi;
                recordProjectileSource(projectile, plant);
        });
    }

public:
    void SetPath(const char* path){std::strcpy(damage_record_path, path);}
    void Setup(){
        record_precise_damage();
        record_direct_kill();
        mark_precise_damage_source();
        record_precise_projectile();
    }
};

BasicInfo DamageRecorder::projectile_mark;
std::vector<DamageInfo> DamageRecorder::damage_records;
std::vector<DamageInfo> DamageRecorder::projectile_records;
std::vector<BasicInfo> DamageRecorder::source_records;

#endif
