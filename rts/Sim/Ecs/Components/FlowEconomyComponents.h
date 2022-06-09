#ifndef FLOW_ECONOMY_COMPONENTS_H__
#define FLOW_ECONOMY_COMPONENTS_H__

#include "Sim/Misc/Resource.h"

namespace FlowEconomy {

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

struct IsEconomyTask {
};

struct IsConditionalEconomyTask {
};

struct IsGeneralPurposeEconomyTask {
};

}

#endif