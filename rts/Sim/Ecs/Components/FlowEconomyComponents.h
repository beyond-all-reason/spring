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

template<class Archive>
void serialize(Archive &ar, AllocatedUnusedResource &c) { ar(c.res, c.prorationRate); }

ALIAS_COMPONENT(BuildRate, float)

struct IsEconomyTask {};
struct IsConditionalEconomyTask {};
struct IsPassiveEconomyTask {};

template<class Archive, class Snapshot>
void serializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component
        < AllocatedUnusedResource, BuildRate
            , IsConditionalEconomyTask, IsEconomyTask, IsPassiveEconomyTask
            , ResourceAdd, ResourceUse
        >(archive);
}

}

#endif