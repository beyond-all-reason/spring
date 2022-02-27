#ifndef BUILD_COMPONENTS_H__
#define BUILD_COMPONENTS_H__

#include "Sim/Ecs/EcsMain.h"

namespace Build {

enum class ProrationRate {
    PRORATION_ONLY_ENERGY,
    PRORATION_ONLY_METAL,
    PRORATION_ALL
};

// builder
struct ActiveBuild {
    entt::entity buildTarget = entt::null;
    ProrationRate prorationType = ProrationRate::PRORATION_ALL;
};

struct BuildPower {
    float value;
};

struct RepairPower {
    float value;
};

// buildee
struct BuildProgress {
    float value;
};

struct BuildComplete {
};

struct BuildCostMetal {
    float value;
};

struct BuildCostEnergy {
    float value;
};

struct BuildTime {
    float value;
};

}

#endif