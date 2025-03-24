/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "LuaReadCommon.h"
#include "Lua/LuaUtils.h"
#include "lua.h"

/**
 * @brief Gets a lambda which checks if a unit is disqualified from being
 * returned by a spatial query.
 *
 * In particular, the lambda checks if the unit has the desired allegiance and
 * if it is visible.
 */
std::function<bool(const CUnit *)> GetIsUnitDisqualifiedTest(lua_State *L, int allegiance,
                                                             int readTeam, int readAllyTeam) {
    switch(allegiance) {
    case LuaUtils::AllUnits:
        return [L](const CUnit *unit) -> bool {
            return !LuaUtils::IsUnitVisible(L, unit);
        };
    case LuaUtils::MyUnits:
        return [L, readTeam](const CUnit *unit) -> bool {
            return unit->team != readTeam || !LuaUtils::IsUnitVisible(L, unit);
        };
    case LuaUtils::AllyUnits:
        return [L, readAllyTeam](const CUnit *unit) -> bool {
            return unit->allyteam != readAllyTeam || !LuaUtils::IsUnitVisible(L, unit);
        };
    case LuaUtils::EnemyUnits:
        return [L, readAllyTeam](const CUnit *unit) -> bool {
            return unit->allyteam == readAllyTeam || !LuaUtils::IsUnitVisible(L, unit);
        };
    default:
        return [allegiance, L](const CUnit *unit) -> bool {
            return unit->team != allegiance || !LuaUtils::IsUnitVisible(L, unit);
        };
    }
}