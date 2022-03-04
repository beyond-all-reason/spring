#ifndef FLOW_ECONOMY_COMPONENTS_H__
#define FLOW_ECONOMY_COMPONENTS_H__

namespace FlowEconomy {

struct MetalFixedIncome {
    float value = 0.f;
};

struct MetalFixedUse {
    float value = 0.f;
};

struct MetalProratableIncome {
    float value = 0.f;
};

struct MetalProratableUse {
    float value = 0.f;
};

struct EnergyFixedIncome {
    float value = 0.f;
};

struct EnergyFixedUse {
    float value = 0.f;
};

struct EnergyProratableIncome {
    float value = 0.f;
};

struct EnergyProratableUse {
    float value = 0.f;
};

struct BuildRate {
    float value = 0.f;
};

struct NewUnitAddToSlowUpdate {};
struct NewUnitAddToCurrent {};

}

#endif