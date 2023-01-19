/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstCMDTYPE.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"


/******************************************************************************
 * Command type constants
 * @module CMDTYPE
 * @see rts/Lua/LuaConstCMDTYPE.cpp
******************************************************************************/

/*** Note, the CMDTYPE[] table is bidirectional. That means: CMDTYPE[CMDTYPE.ICON] := "CMDTYPE_ICON"
 *
 * @table CMDTYPE
 * @number ICON expect 0 parameters in return
 * @number ICON_MODE expect 1 parameter in return (number selected mode)
 * @number ICON_MAP expect 3 parameters in return (mappos)
 * @number ICON_AREA expect 4 parameters in return (mappos+radius)
 * @number ICON_UNIT expect 1 parameters in return (unitid)
 * @number ICON_UNIT_OR_MAP expect 1 parameters in return (unitid) or 3 parameters in return (mappos)
 * @number ICON_FRONT expect 3 or 6 parameters in return (middle and right side of front if a front was defined)
 * @number COMBO_BOX expect 1 parameter in return (number selected option)
 * @number ICON_UNIT_OR_AREA expect 1 parameter in return (unitid) or 4 parameters in return (mappos+radius)
 * @number ICON_UNIT_FEATURE_OR_AREA expect 1 parameter in return (unitid or Game.maxUnits+featureid) or 4 parameters in return (mappos+radius)
 * @number ICON_BUILDING expect 3 parameters in return (mappos)
 * @number ICON_UNIT_OR_RECTANGLE expect 1 parameter in return (unitid) or 3 parameters in return (mappos) or 6 parameters in return (startpos+endpos)
 * @number NUMBER expect 1 parameter in return (number)
 * @number CUSTOM used with CMD_INTERNAL
 * @number NEXT next command page used with CMD_INTERNAL
 * @number PREV previous command page used with CMD_INTERNAL
 */

bool LuaConstCMDTYPE::PushEntries(lua_State* L)
{
#define PUSH_CMDTYPE(cmd) LuaInsertDualMapPair(L, #cmd, CMDTYPE_ ## cmd)

	PUSH_CMDTYPE(ICON);
	PUSH_CMDTYPE(ICON_MODE);
	PUSH_CMDTYPE(ICON_MAP);
	PUSH_CMDTYPE(ICON_AREA);
	PUSH_CMDTYPE(ICON_UNIT);
	PUSH_CMDTYPE(ICON_UNIT_OR_MAP);
	PUSH_CMDTYPE(ICON_FRONT);
	PUSH_CMDTYPE(ICON_UNIT_OR_AREA);
	PUSH_CMDTYPE(NEXT);
	PUSH_CMDTYPE(PREV);
	PUSH_CMDTYPE(ICON_UNIT_FEATURE_OR_AREA);
	PUSH_CMDTYPE(ICON_BUILDING);
	PUSH_CMDTYPE(CUSTOM);
	PUSH_CMDTYPE(ICON_UNIT_OR_RECTANGLE);
	PUSH_CMDTYPE(NUMBER);

	return true;
}


/******************************************************************************/
/******************************************************************************/
