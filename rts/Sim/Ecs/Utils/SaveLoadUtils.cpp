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


// template<class Archive, class BaseComponentType>
// void serialize(Archive &ar, BaseComponentType &c) { ar(c.value); }

// template<class Archive>
// void serialize(Archive &ar, Build::BuildPower &c) { ar(c.value); }

/*
template<class S, class T>
void ProcessComponents(T&& archive, entt::registry& reg) {
    auto regSnapshot = S(reg);

    regSnapshot.entities(archive);

    {
        using namespace Build;
        regSnapshot.component< ActiveBuild, BeingBuilt, BuildCost, BuildPower, BuildProgress, BuildTime, RepairPower
            >(archive);
    }
    {
        using namespace EnvEconomy;
        regSnapshot.component
            < NewWindGenerator, WindEconomyTaskRef, WindEnergy, WindGenerator, WindGeneratorActive
            >(archive);
    }
    {
        using namespace FlowEconomy;
        regSnapshot.component
            < AllocatedUnusedResource, BuildRate
            , IsConditionalEconomyTask, IsEconomyTask, IsPassiveEconomyTask
            , ResourceAdd, ResourceOrder, ResourceUse
            >(archive);
    }
    {
        using namespace SolidObject;
        regSnapshot.component
            < Health, MaxHealth
            >(archive);
    }
    {
        using namespace SystemGlobals;
        regSnapshot.component
            < BuildSystemComponent, EnvResourceComponent, FlowEconomySystemComponent, UnitEconomyReportSystemComponent
            >(archive);
    }
    {
        using namespace Units;
        regSnapshot.component
            < ChainEntity, ConditionalEnergyUseEconomyTaskRef, ConditionalMetalUseEconomyTaskRef, EconomyTasks
            , EnergyUpKeepEconomyTaskRef, MakeDrainResourcesEconomyTaskRef, MakeResourcesEconomyTaskRef
            , MetalUpKeepEconomyTaskRef, OwningEntity, Team, UnitDefRef, UnitId
            >(archive);
    }
    {
        using namespace UnitEconomy;
        regSnapshot.component
            < ResourcesComponentBase, ResourcesConditionalMake, ResourcesConditionalUse, ResourcesCurrentMake
            , ResourcesCurrentUsage, ResourcesUnconditionalMake, ResourcesUnconditionalUse
            >(archive);
    }
    {
        using namespace UnitEconomyReport;
        regSnapshot.component
            < SnapshotMake, SnapshotUsage
            >(archive);
    }
}*/


// Partial Specialization of Function Templates are not allowed so we can't do this
//
// template<class Archive, class BaseComponentType>
// void serialize(Archive &ar, BaseComponentType &c) { ar(c.value); }
//
// and then the other type specializations

template<class Archive>
void serialize(Archive &ar, BasicComponentType<float> &c) { ar(c.value); }

template<class Archive>
void serialize(Archive &ar, SResourcePack &c) { ar(c.res1, c.res2, c.res3, c.res4); }

// Need to figure out why Build::ActiveBuild doesn't work and why namespace does...
namespace Build {
    template<class Archive>
    void serialize(Archive &ar, ActiveBuild &c) { ar(c.buildTarget, c.currentBuildpower); }
}


using namespace SystemGlobals;

void SaveLoadUtils::LoadComponents(creg::CInputStreamSerializer &os, std::stringstream &oss) {
    //auto regSnapshot = entt::snapshot_loader{EcsMain::registry};
    //ProcessComponents<entt::snapshot_loader>(cereal::BinaryInputArchive{oss}, EcsMain::registry);
}

void SaveLoadUtils::SaveComponents(creg::COutputStreamSerializer &os, std::stringstream &oss) {
    auto regSnapshot = entt::snapshot{EcsMain::registry};

    auto archive = cereal::BinaryOutputArchive{oss};

    regSnapshot.entities(archive);

    {
        using namespace Build;
        regSnapshot.component
            < ActiveBuild, BeingBuilt, BuildCost, BuildPower, BuildProgress, BuildTime, RepairPower
            >(archive);
    }

    //ProcessComponents<entt::snapshot>(cereal::BinaryOutputArchive{oss}, EcsMain::registry);
}


// class TArchive {
// public:
//     TArchive(creg::COutputStreamSerializer &os)
//       : os(os)
//     {}

//     template<class T>
//     void operator()(T input) {
//         oss.write(componentName.begin(), componentName.size());
//     }

// private:
//     creg::COutputStreamSerializer &os
// };