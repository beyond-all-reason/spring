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

#include "Sim/Misc/Resource.h"

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

void SaveLoadUtils::LoadComponents(std::stringstream &oss) {
    ProcessComponents<entt::snapshot_loader>(cereal::BinaryInputArchive{oss}, entt::snapshot_loader{EcsMain::registry});
}

void SaveLoadUtils::SaveComponents(std::stringstream &oss) {
    ProcessComponents<entt::snapshot>(cereal::BinaryOutputArchive{oss}, entt::snapshot{EcsMain::registry});
}
