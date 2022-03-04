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

}

#endif