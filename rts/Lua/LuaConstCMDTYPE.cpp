/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstCMDTYPE.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"

/*** 
 * Command type constants
 * 
 * Note, the `CMDTYPE[]` table is bidirectional. That means: `CMDTYPE[CMDTYPE.ICON] := "CMDTYPE_ICON"`
 * 
 * @enum CMDTYPE
 */

bool LuaConstCMDTYPE::PushEntries(lua_State* L)
{
#define PUSH_CMDTYPE(cmd) LuaInsertDualMapPair(L, #cmd, CMDTYPE_ ## cmd)
	/*** @field CMDTYPE.ICON integer Expect 0 parameters in return. */
	PUSH_CMDTYPE(ICON);
	/*** @field CMDTYPE.ICON_MODE integer Expect 1 parameter in return (number selected mode). */
	PUSH_CMDTYPE(ICON_MODE);
	/*** @field CMDTYPE.ICON_MAP integer Expect 3 parameters in return (mappos). */
	PUSH_CMDTYPE(ICON_MAP);
	/*** @field CMDTYPE.ICON_AREA integer Expect 4 parameters in return (mappos+radius). */
	PUSH_CMDTYPE(ICON_AREA);
	/*** @field CMDTYPE.ICON_UNIT integer Expect 1 parameters in return (unitid). */
	PUSH_CMDTYPE(ICON_UNIT);
	/*** @field CMDTYPE.ICON_UNIT_OR_MAP integer Expect 1 parameters in return (unitid) or 3 parameters in return (mappos). */
	PUSH_CMDTYPE(ICON_UNIT_OR_MAP);
	/*** @field CMDTYPE.ICON_FRONT integer Expect 3 or 6 parameters in return (middle and right side of front if a front was defined). */
	PUSH_CMDTYPE(ICON_FRONT);
	/*** @field CMDTYPE.ICON_UNIT_OR_AREA integer Expect 1 parameter in return (unitid) or 4 parameters in return (mappos+radius). */
	PUSH_CMDTYPE(ICON_UNIT_OR_AREA);
	/*** @field CMDTYPE.NEXT integer Expect command page used with `CMD_INTERNAL`. */
	PUSH_CMDTYPE(NEXT);
	/*** @field CMDTYPE.PREV integer Expect command page used with `CMD_INTERNAL`. */
	PUSH_CMDTYPE(PREV);
	/*** @field CMDTYPE.ICON_UNIT_FEATURE_OR_AREA integer Expect 1 parameter in return (unitid or Game.maxUnits+featureid) or 4 parameters in return (mappos+radius). */
	PUSH_CMDTYPE(ICON_UNIT_FEATURE_OR_AREA);
	/*** @field CMDTYPE.ICON_BUILDING integer Expect 3 parameters in return (mappos). */
	PUSH_CMDTYPE(ICON_BUILDING);
	/*** @field CMDTYPE.CUSTOM integer Expect with `CMD_INTERNAL`. */
	PUSH_CMDTYPE(CUSTOM);
	/*** @field CMDTYPE.ICON_UNIT_OR_RECTANGLE integer Expect 1 parameter in return (unitid) or 3 parameters in return (mappos) or 6 parameters in return (startpos+endpos). */
	PUSH_CMDTYPE(ICON_UNIT_OR_RECTANGLE);
	/*** @field CMDTYPE.NUMBER integer Expect 1 parameter in return (number). */
	PUSH_CMDTYPE(NUMBER);

	return true;
}