/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "UnitTrapCheckUtils.h"

#include "Sim/Ecs/Registry.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/Components/MoveTypesComponents.h"

#include <tracy/Tracy.hpp>

using namespace MoveTypes;

void MoveTypes::RegisterFeatureForUnitTrapCheck(CFeature* object) {
    //ZoneScoped;
    if (gs->frameNum < 0)
        return;

    assert(Sim::registry.valid(object->entityReference));

    Sim::registry.emplace_or_replace<UnitTrapCheck>(object->entityReference
            , UnitTrapCheckType::TRAPPER_IS_FEATURE
            , object->id);
}
