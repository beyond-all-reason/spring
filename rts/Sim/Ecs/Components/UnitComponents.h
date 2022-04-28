#ifndef UNIT_COMPONENTS_H__
#define UNIT_COMPONENTS_H__

#include "Sim/Units/Unit.h"

class UnitDef;

namespace Units {

struct UnitId {
    int value;
};

struct Team {
    int value;
};

struct UnitDefRef {
    const UnitDef* value = nullptr;
};

struct OwningEntity {
    entt::entity value{entt::null};
};

struct EconomyTasks {
    std::size_t size = 0;
};

struct ChainEntity {
    entt::entity prev{entt::null};
    entt::entity next{entt::null};
};

struct ConditionalResourceMakeRef {
    entt::entity value{entt::null};
};

}

#endif