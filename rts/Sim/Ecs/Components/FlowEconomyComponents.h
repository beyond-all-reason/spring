#ifndef FLOW_ECONOMY_COMPONENTS_H__
#define FLOW_ECONOMY_COMPONENTS_H__

namespace FlowEconomy {

// MetalIncome (fixed/proratable)
// EnergyIncome (fixed/proratable)

// MetalExpense (proratable)
// EnergyExpense (proratable)

// Units
// unconditional eco    (fixed value always applied)

// Unit eco activated
// conditional eco      (fixed vlaue applied on/off)
// tidal                (fixed value always applied fir tidal gens)
// wind                 (variable value always applied)

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

struct AwaitingEconomyAssignment {
};

}

#endif