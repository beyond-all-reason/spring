#include "SaveLoadUtils.h"

#include <sstream>

#include "cereal/archives/binary.hpp"

#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyReportComponents.h"
#include "Sim/Ecs/Utils/SystemUtils.h"

#include "Sim/Misc/Resource.h"
#include "System/Log/ILog.h"

// temporary measure
#include "Sim/Ecs/Systems/BuildSystem.h"
#include "Sim/Ecs/Systems/EnvEconomySystem.h"
#include "Sim/Ecs/Systems/EnvResourceSystem.h"
#include "Sim/Ecs/Systems/FlowEconomySystem.h"
#include "Sim/Ecs/Systems/FluxEconomySystem.h"
#include "Sim/Ecs/Systems/UnitEconomyReportSystem.h"
#include "SystemGlobalUtils.h"


// Partial Specialization of Function Templates are not allowed so we can't do this
//
// template<class Archive, class BaseComponentType>
// void serialize(Archive &ar, BaseComponentType &c) { ar(c.value); }
//
// and then the other type specializations

template<class Archive>
void serialize(Archive &ar, BasicComponentType<float> &c) { ar(c.value); }

template<class Archive>
void serialize(Archive &ar, BasicComponentType<int> &c) { ar(c.value); }

template<class Archive>
void serialize(Archive &ar, BasicComponentType<entt::entity> &c) { ar(c.value); }

template<class Archive>
void serialize(Archive &ar, BasicComponentType<std::size_t> &c) { ar(c.value); }

template<class Archive>
void serialize(Archive &ar, SResourcePack &c) { ar(c.res1, c.res2, c.res3, c.res4); }

template<class Archive>
void serialize(Archive &ar, float3 &c) { ar(c.x, c.y, c.z); }

namespace Build {
    template<class Archive>
    void serialize(Archive &ar, ActiveBuild &c) { ar(c.buildTarget, c.currentBuildpower); }
}

namespace FlowEconomy {
    template<class Archive>
    void serialize(Archive &ar, AllocatedUnusedResource &c) { ar(c.res, c.prorationRate); }
}

namespace SystemGlobals {
    template<class Archive>
    void serialize(Archive &ar, EnvResourceComponent &c)
        { ar( c.curTidalStrength, c.curWindStrength, c.newWindStrength, c.minWindStrength, c.maxWindStrength
            , c.curWindDir, c.curWindVec, c.newWindVec, c.oldWindVec, c.windDirTimer
        );}

    template<class Archive>
    void serialize(Archive &ar, FlowEconomySystemComponent &c) { ar(c.economyMultiplier); }

    template<class Archive>
    void serialize(Archive &ar, UnitEconomyReportSystemComponent &c) { ar(c.activeBuffer); }
}

namespace Units {
    template<class Archive>
    void serialize(Archive &ar, ChainEntity &c) { ar(c.prev, c.next); }
}

namespace UnitEconomy {
    template<class Archive>
    void serialize(Archive &ar, ResourcesComponentBase &c) { ar(c.resources); }
}

namespace UnitEconomyReport {
    template<class Archive>
    void serialize(Archive &ar, SnapshotBase &c) { ar(c.resources); }
}


template<class S, class T>
void ProcessComponents(T&& archive, S&& regSnapshot) {
    regSnapshot.entities(archive);

    {
        using namespace Build;
        regSnapshot.template component
            < ActiveBuild, BeingBuilt, BuildCost, BuildPower, BuildProgress, BuildTime, RepairPower
            >(archive);
    }
    {
        using namespace EnvEconomy;
        regSnapshot.template component
            < NewWindGenerator, WindEconomyTaskRef, WindEnergy, WindGenerator, WindGeneratorActive
            >(archive);        
    }
    {
        using namespace FlowEconomy;
        regSnapshot.template component
            < AllocatedUnusedResource, BuildRate
            , IsConditionalEconomyTask, IsEconomyTask, IsPassiveEconomyTask
            , ResourceAdd, ResourceUse
            >(archive);
    }
    {
        using namespace SolidObject;
        regSnapshot.template component
            < Health, MaxHealth
            >(archive);
    }
    {
        using namespace SystemGlobals;
        regSnapshot.template component
            < BuildSystemComponent, EnvResourceComponent, FlowEconomySystemComponent, UnitEconomyReportSystemComponent
            >(archive);
    }
    {
        using namespace Units;
        regSnapshot.template component
            < ChainEntity, ConditionalEnergyUseEconomyTaskRef, ConditionalMetalUseEconomyTaskRef, EconomyTasks
            , EnergyUpKeepEconomyTaskRef, MakeDrainResourcesEconomyTaskRef, MakeResourcesEconomyTaskRef
            , MetalUpKeepEconomyTaskRef, OwningEntity, Team, UnitDefRef, UnitId
            >(archive);
    }
    {
        using namespace UnitEconomy;
        regSnapshot.template component
            < ResourcesComponentBase, ResourcesConditionalMake, ResourcesConditionalUse, ResourcesCurrentMake
            , ResourcesCurrentUsage, ResourcesUnconditionalMake, ResourcesUnconditionalUse
            >(archive);
    }
    {
        using namespace UnitEconomyReport;
        regSnapshot.template component
            < SnapshotMake, SnapshotUsage
            >(archive);
    }
}


using namespace SystemGlobals;

void SaveLoadUtils::LoadComponents(std::stringstream &iss) {
    SystemUtils::systemUtils.NotifyPreLoad();
    
    auto archive = cereal::BinaryInputArchive{iss};
    LOG("%s: Entities before clear is %d", __func__, (int)EcsMain::registry.alive());
    EcsMain::registry.each([](entt::entity entity) { EcsMain::registry.destroy(entity); });
    LOG("%s: Entities after clear is %d (%d)", __func__, (int)EcsMain::registry.alive(), (int)iss.tellg());
    {ProcessComponents<entt::snapshot_loader>(archive, entt::snapshot_loader{EcsMain::registry});}
    LOG("%s: Entities after load is %d (%d)", __func__, (int)EcsMain::registry.alive(), (int)iss.tellg());
    EcsMain::registry.each([](entt::entity entity) { LOG("%s: Entity %d added (%d)", __func__, entt::to_entity(entity), entt::to_integral(entity)); });

    LOG("%s: eco multiplier is %f", __func__, EcsMain::registry.get<SystemGlobals::FlowEconomySystemComponent>(entt::entity(0)).economyMultiplier);
    {
        using namespace SystemGlobals;
        archive(systemGlobals);
    }
    SystemUtils::systemUtils.NotifyPostLoad();
}

void SaveLoadUtils::SaveComponents(std::stringstream &oss) {
    auto archive = cereal::BinaryOutputArchive{oss};
    LOG("%s: Entities before save is %d (%d)", __func__, (int)EcsMain::registry.alive(), (int)oss.tellp());
    {ProcessComponents<entt::snapshot>(archive, entt::snapshot{EcsMain::registry});}
    LOG("%s: Save bytes writen %d", __func__, (int)oss.tellp());
    EcsMain::registry.each([](entt::entity entity) { LOG("%s: Entity %d saved (%d)", __func__, entt::to_entity(entity), entt::to_integral(entity)); });

    {
        using namespace SystemGlobals;
        archive(systemGlobals);
    }
}
