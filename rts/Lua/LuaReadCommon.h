/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_READ_COMMON_H
#define LUA_READ_COMMON_H

#include "Sim/Units/Unit.h"

struct lua_State;

std::function<bool(const CUnit *)> GetIsUnitDisqualifiedTest(lua_State *L, int allegiance,
                                                             int readTeam, int readAllyTeam);

#endif // LUA_READ_COMMON_H