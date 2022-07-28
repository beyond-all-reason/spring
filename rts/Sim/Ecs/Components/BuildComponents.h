#ifndef BUILD_COMPONENTS_H__
#define BUILD_COMPONENTS_H__

#include "Sim/Ecs/EcsMain.h"
#include "BaseComponents.h"

#include "Sim/Misc/Resource.h"

namespace Build {

constexpr float defaultMaxRepairPowerRate = 1000000.f;

// builder
struct ActiveBuild {
    entt::entity buildTarget = entt::null;
    float currentBuildpower = 0.f;
};

template<class Archive>
void serialize(Archive &ar, ActiveBuild &c) { ar(c.buildTarget, c.currentBuildpower); }

struct ActiveRepair {
    entt::entity buildTarget = entt::null;
    float currentBuildpower = 0.f;
};

template<class Archive>
void serialize(Archive &ar, ActiveRepair &c) { ar(c.buildTarget, c.currentBuildpower); }

ALIAS_COMPONENT(BuildPower, float)
ALIAS_COMPONENT(BuildProgress, float)
ALIAS_COMPONENT(BuildTime, float)
ALIAS_COMPONENT(RepairPower, float)
ALIAS_COMPONENT_DEF(MaxRepairPowerRate, float, defaultMaxRepairPowerRate)
//ALIAS_COMPONENT(MaxRepairPowerRate, float)
ALIAS_COMPONENT(RepairPowerRecieved, float)

struct BeingBuilt {};
struct BuildCost : public SResourcePack {
    using SResourcePack::operator=;
};

template<class Archive, class Snapshot>
void serializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component
        < ActiveBuild, ActiveRepair, BeingBuilt, BuildCost, BuildPower, BuildProgress, BuildTime
        , RepairPower, RepairPowerRecieved
        >(archive);
}

}

#endif