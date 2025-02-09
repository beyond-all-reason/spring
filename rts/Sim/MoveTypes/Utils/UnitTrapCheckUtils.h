/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_TRAP_CHECK_UTILS_H__
#define UNIT_TRAP_CHECK_UTILS_H__

struct CFeature;
struct CUnit;

namespace MoveTypes {
    void RegisterFeatureForUnitTrapCheck(CFeature* object);
    void RegisterUnitForUnitTrapCheck(CUnit* object);
}

#endif