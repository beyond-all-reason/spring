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

ALIAS_COMPONENT(BuildSpeed, float)
ALIAS_COMPONENT(BuildProgress, float)
ALIAS_COMPONENT(BuildTime, float)
ALIAS_COMPONENT(RepairSpeed, float)
ALIAS_COMPONENT_DEF(MaxRepairSpeed, float, defaultMaxRepairPowerRate)
//ALIAS_COMPONENT(MaxRepairSpeed, float)
ALIAS_COMPONENT(RepairRecieved, float)

struct BeingBuilt {};
struct BuildCost : public SResourcePack {
    using SResourcePack::operator=;
};

template<class Archive, class Snapshot>
void serializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component
        < ActiveBuild, ActiveRepair, BeingBuilt, BuildCost, BuildSpeed, BuildProgress, BuildTime
        , RepairSpeed, RepairRecieved
        >(archive);
}

}

#endif