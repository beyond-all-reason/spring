#ifndef DAMAGE_COMPONENTS_H__
#define DAMAGE_COMPONENTS_H__

#include "Sim/Units/Unit.h"

namespace Damage {

struct Collidee {

    Collidee()
        : type(Type::NONE)
    {}

    Collidee(CUnit* newCollidee) { Assign(newCollidee); }

    void Assign(CUnit* newCollidee) {
        type = Type::UNIT;
        collidee.unitRef = newCollidee;
    }

    enum class Type
        { NONE
        , FEATURE
        , PROJECTILE
        , UNIT
        };
    
    Type type;
    union CollideeData {
        CollideeData() { unitRef = nullptr; }

        CUnit* unitRef;
    } collidee;
};


}

#endif