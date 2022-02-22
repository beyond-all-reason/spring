#ifndef UNIT_COMPONENTS_H__
#define UNIT_COMPONENTS_H__

#include "Sim/Units/Unit.h"

class UnitDef;

namespace Units {

struct UnitId {
    int unitId;
};

struct Team {
    int value;
};

struct UnitDefRef {
    const UnitDef* unitDefRef = nullptr;
};

}

#endif