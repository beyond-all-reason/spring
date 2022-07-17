#ifndef FLOW_ECONOMY_COMPONENTS_H__
#define FLOW_ECONOMY_COMPONENTS_H__

#include "Sim/Misc/Resource.h"
#include "BaseComponents.h"

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

struct AllocatedUnusedResource {
    SResourcePack res;
    float prorationRate = 0.f;
};

ALIAS_COMPONENT(BuildRate, float)

struct IsEconomyTask {};
struct IsConditionalEconomyTask {};
struct IsPassiveEconomyTask {};

}

#endif