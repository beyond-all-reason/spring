/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstCMD.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"


/***
 * Command constants.
 * @enum CMD
 */

bool LuaConstCMD::PushEntries(lua_State* L)
{
	/*** @field CMD.OPT_ALT 128 */
	LuaPushNamedNumber(L, "OPT_ALT",      ALT_KEY);
	/*** @field CMD.OPT_CTRL 64 */
	LuaPushNamedNumber(L, "OPT_CTRL",     CONTROL_KEY);
	/*** @field CMD.OPT_SHIFT 32 */
	LuaPushNamedNumber(L, "OPT_SHIFT",    SHIFT_KEY);
	/*** @field CMD.OPT_RIGHT 16 */
	LuaPushNamedNumber(L, "OPT_RIGHT",    RIGHT_MOUSE_KEY);
	/*** @field CMD.OPT_INTERNAL 8 */
	LuaPushNamedNumber(L, "OPT_INTERNAL", INTERNAL_ORDER);
	/*** @field CMD.OPT_META 4 */
	LuaPushNamedNumber(L, "OPT_META",     META_KEY);

	/*** @field CMD.MOVESTATE_NONE -1 */
	LuaPushNamedNumber(L, "MOVESTATE_NONE"    , MOVESTATE_NONE    );
	/*** @field CMD.MOVESTATE_HOLDPOS 0 */
	LuaPushNamedNumber(L, "MOVESTATE_HOLDPOS" , MOVESTATE_HOLDPOS );
	/*** @field CMD.MOVESTATE_MANEUVER 1 */
	LuaPushNamedNumber(L, "MOVESTATE_MANEUVER", MOVESTATE_MANEUVER);
	/*** @field CMD.MOVESTATE_ROAM 2 */
	LuaPushNamedNumber(L, "MOVESTATE_ROAM"    , MOVESTATE_ROAM    );

	/*** @field CMD.FIRESTATE_NONE -1 */
	LuaPushNamedNumber(L, "FIRESTATE_NONE"         , FIRESTATE_NONE         );
	/*** @field CMD.FIRESTATE_HOLDFIRE 0 */
	LuaPushNamedNumber(L, "FIRESTATE_HOLDFIRE"     , FIRESTATE_HOLDFIRE     );
	/*** @field CMD.FIRESTATE_RETURNFIRE 1 */
	LuaPushNamedNumber(L, "FIRESTATE_RETURNFIRE"   , FIRESTATE_RETURNFIRE   );
	/*** @field CMD.FIRESTATE_FIREATWILL 2 */
	LuaPushNamedNumber(L, "FIRESTATE_FIREATWILL"   , FIRESTATE_FIREATWILL   );
	/*** @field CMD.FIRESTATE_FIREATNEUTRAL 3 */
	LuaPushNamedNumber(L, "FIRESTATE_FIREATNEUTRAL", FIRESTATE_FIREATNEUTRAL);

	/*** @field CMD.WAITCODE_TIME 1 */
	LuaPushNamedNumber(L, "WAITCODE_TIME",   CMD_WAITCODE_TIMEWAIT);
	/*** @field CMD.WAITCODE_DEATH 2 */
	LuaPushNamedNumber(L, "WAITCODE_DEATH",  CMD_WAITCODE_DEATHWAIT);
	/*** @field CMD.WAITCODE_SQUAD 3 */
	LuaPushNamedNumber(L, "WAITCODE_SQUAD",  CMD_WAITCODE_SQUADWAIT);
	/*** @field CMD.WAITCODE_GATHER 4 */
	LuaPushNamedNumber(L, "WAITCODE_GATHER", CMD_WAITCODE_GATHERWAIT);

#define PUSH_CMD(cmd) LuaInsertDualMapPair(L, #cmd, CMD_ ## cmd);

	/*** @field CMD.STOP 0 */
	PUSH_CMD(STOP);
	/*** @field CMD.INSERT 1 */
	PUSH_CMD(INSERT);
	/*** @field CMD.REMOVE 2 */
	PUSH_CMD(REMOVE);
	/*** @field CMD.WAIT 5 */
	PUSH_CMD(WAIT);
	/*** @field CMD.TIMEWAIT 6 */
	PUSH_CMD(TIMEWAIT);
	/*** @field CMD.DEATHWAIT 7 */
	PUSH_CMD(DEATHWAIT);
	/*** @field CMD.SQUADWAIT 8 */
	PUSH_CMD(SQUADWAIT);
	/*** @field CMD.GATHERWAIT 9 */
	PUSH_CMD(GATHERWAIT);
	/*** @field CMD.MOVE 10 */
	PUSH_CMD(MOVE);
	/*** @field CMD.PATROL 15 */
	PUSH_CMD(PATROL);
	/*** @field CMD.FIGHT 16 */
	PUSH_CMD(FIGHT);
	/*** @field CMD.ATTACK 20 */
	PUSH_CMD(ATTACK);
	/*** @field CMD.AREA_ATTACK 21 */
	PUSH_CMD(AREA_ATTACK);
	/*** @field CMD.GUARD 25 */
	PUSH_CMD(GUARD);
	/*** @field CMD.GROUPSELECT 35 */
	PUSH_CMD(GROUPSELECT);
	/*** @field CMD.GROUPADD 36 */
	PUSH_CMD(GROUPADD);
	/*** @field CMD.GROUPCLEAR 37 */
	PUSH_CMD(GROUPCLEAR);
	/*** @field CMD.REPAIR 40 */
	PUSH_CMD(REPAIR);
	/*** @field CMD.FIRE_STATE 45 */
	PUSH_CMD(FIRE_STATE);
	/*** @field CMD.MOVE_STATE 50 */
	PUSH_CMD(MOVE_STATE);
	/*** @field CMD.SETBASE 55 */
	PUSH_CMD(SETBASE);
	/*** @field CMD.INTERNAL 60 */
	PUSH_CMD(INTERNAL);
	/*** @field CMD.SELFD 65 */
	PUSH_CMD(SELFD);
	/*** @field CMD.LOAD_UNITS 75 */
	PUSH_CMD(LOAD_UNITS);
	/*** @field CMD.LOAD_ONTO 76 */
	PUSH_CMD(LOAD_ONTO);
	/*** @field CMD.UNLOAD_UNITS 80 */
	PUSH_CMD(UNLOAD_UNITS);
	/*** @field CMD.UNLOAD_UNIT 81 */
	PUSH_CMD(UNLOAD_UNIT);
	/*** @field CMD.ONOFF 85 */
	PUSH_CMD(ONOFF);
	/*** @field CMD.RECLAIM 90 */
	PUSH_CMD(RECLAIM);
	/*** @field CMD.CLOAK 95 */
	PUSH_CMD(CLOAK);
	/*** @field CMD.STOCKPILE 100 */
	PUSH_CMD(STOCKPILE);
	/*** @field CMD.MANUALFIRE 105 */
	PUSH_CMD(MANUALFIRE);
	/*** @field CMD.DGUN 105 */
	LuaInsertDualMapPair(L, "DGUN", CMD_MANUALFIRE); // backward compatibility (TODO: find a way to print a warning when used!)
	/*** @field CMD.RESTORE 110 */
	PUSH_CMD(RESTORE);
	/*** @field CMD.REPEAT 115 */
	PUSH_CMD(REPEAT);
	/*** @field CMD.TRAJECTORY 120 */
	PUSH_CMD(TRAJECTORY);
	/*** @field CMD.RESURRECT 125 */
	PUSH_CMD(RESURRECT);
	/*** @field CMD.CAPTURE 130 */
	PUSH_CMD(CAPTURE);
	/*** @field CMD.AUTOREPAIRLEVEL 135 */
	PUSH_CMD(AUTOREPAIRLEVEL);
	/*** @field CMD.LOOPBACKATTACK 20 */
	LuaInsertDualMapPair(L, "LOOPBACKATTACK", CMD_ATTACK); // backward compatibility (TODO: find a way to print a warning when used!)
	/*** @field CMD.IDLEMODE 145  */
	PUSH_CMD(IDLEMODE);

	return true;
}