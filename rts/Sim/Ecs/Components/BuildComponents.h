#ifndef BUILD_COMPONENTS_H__
#define BUILD_COMPONENTS_H__

#include "Sim/Ecs/EcsMain.h"
#include "BaseComponents.h"

#include "Sim/Misc/Resource.h"

namespace Build {

// builder
struct ActiveBuild {
    entt::entity buildTarget = entt::null;
    float currentBuildpower = 0.f;
};

ALIAS_COMPONENT(BuildPower, float)
ALIAS_COMPONENT(RepairPower, float)
ALIAS_COMPONENT(BuildProgress, float)
ALIAS_COMPONENT(BuildTime, float)

struct BeingBuilt {};

struct BuildCost : public SResourcePack {
    using SResourcePack::operator=;
};

}

#endif