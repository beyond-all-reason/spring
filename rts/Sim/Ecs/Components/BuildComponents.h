#ifndef BUILD_COMPONENTS_H__
#define BUILD_COMPONENTS_H__

#include "Sim/Ecs/EcsMain.h"

namespace Build {

enum class ProrationRate {
    PRORATION_NONE,
    PRORATION_ONLY_METAL,
    PRORATION_ONLY_ENERGY,
    PRORATION_ALL
};

// builder
struct ActiveBuild {
    entt::entity buildTarget = entt::null;
    float currentBuildpower = 0.f;
    ProrationRate prorationType = ProrationRate::PRORATION_NONE;
};

struct BuildPower {
    float value = 0.f;
};

struct RepairPower {
    float value = 0.f;
};

// buildee
struct BuildProgress {
    float value = 0.f;
};

struct BuildComplete {
};

struct BuildCostMetal {
    float value = 0.f;
};

struct BuildCostEnergy {
    float value = 0.f;
};

struct BuildTime {
    float value = 0.f;
};

}

#endif