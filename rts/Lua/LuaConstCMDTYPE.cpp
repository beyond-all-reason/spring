/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstCMDTYPE.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"


/******************************************************************************
 * Command type constants
 * @see rts/Lua/LuaConstCMDTYPE.cpp
******************************************************************************/

/*** Note, the CMDTYPE[] table is bidirectional. That means: CMDTYPE[CMDTYPE.ICON] := "CMDTYPE_ICON"
 *
 * @table CMDTYPE
 * @param ICON number expect 0 parameters in return
 * @param ICON_MODE number expect 1 parameter in return (number selected mode)
 * @param ICON_MAP number expect 3 parameters in return (mappos)
 * @param ICON_AREA number expect 4 parameters in return (mappos+radius)
 * @param ICON_UNIT number expect 1 parameters in return (unitid)
 * @param ICON_UNIT_OR_MAP number expect 1 parameters in return (unitid) or 3 parameters in return (mappos)
 * @param ICON_FRONT number expect 3 or 6 parameters in return (middle and right side of front if a front was defined)
 * @param COMBO_BOX number expect 1 parameter in return (number selected option)
 * @param ICON_UNIT_OR_AREA number expect 1 parameter in return (unitid) or 4 parameters in return (mappos+radius)
 * @param ICON_UNIT_FEATURE_OR_AREA number expect 1 parameter in return (unitid or Game.maxUnits+featureid) or 4 parameters in return (mappos+radius)
 * @param ICON_BUILDING number expect 3 parameters in return (mappos)
 * @param ICON_UNIT_OR_RECTANGLE number expect 1 parameter in return (unitid) or 3 parameters in return (mappos) or 6 parameters in return (startpos+endpos)
 * @param NUMBER number expect 1 parameter in return (number)
 * @param CUSTOM number used with CMD_INTERNAL
 * @param NEXT number next command page used with CMD_INTERNAL
 * @param PREV number previous command page used with CMD_INTERNAL
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
