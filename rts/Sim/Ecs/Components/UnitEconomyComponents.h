#ifndef UNIT_ECONOMY_COMPONENTS_H__
#define UNIT_ECONOMY_COMPONENTS_H__

#include "Sim/Misc/Resource.h"

namespace UnitEconomy {

struct ResourcesCurrentMake : public SResourcePack {
    using SResourcePack::operator=;
};

struct ResourcesCurrentUsage : public SResourcePack {
    using SResourcePack::operator=;
};


struct ResourcesComponentBase {
    SResourcePack resources;
};

template<class Archive>
void serialize(Archive &ar, ResourcesComponentBase &c) { ar(c.resources); }

struct ResourcesConditionalUse : public ResourcesComponentBase {
};

struct ResourcesConditionalMake : public ResourcesComponentBase {
};

struct ResourcesUnconditionalUse : public ResourcesComponentBase {
};

struct ResourcesUnconditionalMake : public ResourcesComponentBase {
};

template<class Archive, class Snapshot>
void serializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component
        < ResourcesComponentBase, ResourcesConditionalMake, ResourcesConditionalUse, ResourcesCurrentMake
            , ResourcesCurrentUsage, ResourcesUnconditionalMake, ResourcesUnconditionalUse
        >(archive);
}

}

#endif