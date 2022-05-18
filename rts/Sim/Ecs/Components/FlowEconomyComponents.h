#ifndef FLOW_ECONOMY_COMPONENTS_H__
#define FLOW_ECONOMY_COMPONENTS_H__

#include "Sim/Misc/Resource.h"

namespace FlowEconomy {

// struct MetalFixedIncome {
//     float value = 0.f;
// };

// struct MetalProratableIncome {
//     float value = 0.f;
// };

// struct MetalProratableUse {
//     float value = 0.f;
// };

// struct EnergyFixedIncome {
//     float value = 0.f;
// };

// struct EnergyFixedUse {
//     float value = 0.f;
// };

// struct EnergyProratableIncome {
//     float value = 0.f;
// };

// struct EnergyProratableUse {
//     float value = 0.f;
// };

struct ResourceOrder {
    SResourcePack add;
    SResourcePack use;
};

struct ResourceAdd : public SResourcePack {
    using SResourcePack::operator=;
};

struct ResourceUse : public SResourcePack {
    using SResourcePack::operator=;
};

struct BuildRate {
    float value = 0.f;
};

// struct AwaitingEconomyAssignment {
// };

struct IsEconomyTask {
};

struct IsConditionalEconomyTask {
};

}

#endif