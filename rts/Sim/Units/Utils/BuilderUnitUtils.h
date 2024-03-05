/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BUILDER_UNIT_UTILS_H__
#define BUILDER_UNIT_UTILS_H__

#include "Sim/Ecs/Registry.h"
#include "Sim/Units/Components/UnitComponents.h"
#include "Sim/Units/Unit.h"


void SetBuildStance(const CUnit* unit) {
    Sim::registry.emplace_or_replace<Unit::InBuildStance>(unit->entityReference);
}

void ClearBuildStance(const CUnit* unit) {
    Sim::registry.remove<Unit::InBuildStance>(unit->entityReference);
}

#endif