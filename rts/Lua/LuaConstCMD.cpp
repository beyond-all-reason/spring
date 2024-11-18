/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstCMD.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"


/******************************************************************************
 * Command constants
 * @see rts/Lua/LuaConstCMD.cpp
******************************************************************************/

/***
 * @enum CMD
 * @field FIRESTATE_NONE -1
 * @field MOVESTATE_NONE -1
 * @field STOP 0
 * @field MOVESTATE_HOLDPOS 0
 * @field FIRESTATE_HOLDFIRE 0
 * @field INSERT 1
 * @field MOVESTATE_MANEUVER 1
 * @field FIRESTATE_RETURNFIRE 1
 * @field WAITCODE_TIME 1
 * @field WAITCODE_DEATH 2
 * @field MOVESTATE_ROAM 2
 * @field REMOVE 2
 * @field FIRESTATE_FIREATWILL 2
 * @field FIRESTATE_FIREATNEUTRAL 3
 * @field WAITCODE_SQUAD 3
 * @field OPT_META 4
 * @field WAITCODE_GATHER 4
 * @field WAIT 5
 * @field TIMEWAIT 6
 * @field DEATHWAIT 7
 * @field OPT_INTERNAL 8
 * @field SQUADWAIT 8
 * @field GATHERWAIT 9
 * @field MOVE 10
 * @field PATROL 15
 * @field FIGHT 16
 * @field OPT_RIGHT 16
 * @field LOOPBACKATTACK 20
 * @field ATTACK 20
 * @field AREA_ATTACK 21
 * @field GUARD 25
 * @field OPT_SHIFT 32
 * @field GROUPSELECT 35
 * @field GROUPADD 36
 * @field GROUPCLEAR 37
 * @field REPAIR 40
 * @field FIRE_STATE 45
 * @field MOVE_STATE 50
 * @field SETBASE 55
 * @field INTERNAL 60
 * @field OPT_CTRL 64
 * @field SELFD 65
 * @field SET_WANTED_MAX_SPEED 70
 * @field LOAD_UNITS 75
 * @field LOAD_ONTO 76
 * @field UNLOAD_UNITS 80
 * @field UNLOAD_UNIT 81
 * @field ONOFF 85
 * @field RECLAIM 90
 * @field CLOAK 95
 * @field STOCKPILE 100
 * @field MANUALFIRE 105
 * @field DGUN 105
 * @field RESTORE 110
 * @field REPEAT 115
 * @field TRAJECTORY 120
 * @field RESURRECT 125
 * @field OPT_ALT 128
 * @field CAPTURE 130
 * @field AUTOREPAIRLEVEL 135
 * @field IDLEMODE 145 
 */


bool LuaConstCMD::PushEntries(lua_State* L)
{
	LuaPushNamedNumber(L, "OPT_ALT",      ALT_KEY);
	LuaPushNamedNumber(L, "OPT_CTRL",     CONTROL_KEY);
	LuaPushNamedNumber(L, "OPT_SHIFT",    SHIFT_KEY);
	LuaPushNamedNumber(L, "OPT_RIGHT",    RIGHT_MOUSE_KEY);
	LuaPushNamedNumber(L, "OPT_INTERNAL", INTERNAL_ORDER);
	LuaPushNamedNumber(L, "OPT_META",     META_KEY);

	LuaPushNamedNumber(L, "MOVESTATE_NONE"    , MOVESTATE_NONE    );
	LuaPushNamedNumber(L, "MOVESTATE_HOLDPOS" , MOVESTATE_HOLDPOS );
	LuaPushNamedNumber(L, "MOVESTATE_MANEUVER", MOVESTATE_MANEUVER);
	LuaPushNamedNumber(L, "MOVESTATE_ROAM"    , MOVESTATE_ROAM    );

	LuaPushNamedNumber(L, "FIRESTATE_NONE"         , FIRESTATE_NONE         );
	LuaPushNamedNumber(L, "FIRESTATE_HOLDFIRE"     , FIRESTATE_HOLDFIRE     );
	LuaPushNamedNumber(L, "FIRESTATE_RETURNFIRE"   , FIRESTATE_RETURNFIRE   );
	LuaPushNamedNumber(L, "FIRESTATE_FIREATWILL"   , FIRESTATE_FIREATWILL   );
	LuaPushNamedNumber(L, "FIRESTATE_FIREATNEUTRAL", FIRESTATE_FIREATNEUTRAL);

	LuaPushNamedNumber(L, "WAITCODE_TIME",   CMD_WAITCODE_TIMEWAIT);
	LuaPushNamedNumber(L, "WAITCODE_DEATH",  CMD_WAITCODE_DEATHWAIT);
	LuaPushNamedNumber(L, "WAITCODE_SQUAD",  CMD_WAITCODE_SQUADWAIT);
	LuaPushNamedNumber(L, "WAITCODE_GATHER", CMD_WAITCODE_GATHERWAIT);

#define PUSH_CMD(cmd) LuaInsertDualMapPair(L, #cmd, CMD_ ## cmd);

	PUSH_CMD(STOP);
	PUSH_CMD(INSERT);
	PUSH_CMD(REMOVE);
	PUSH_CMD(WAIT);
	PUSH_CMD(TIMEWAIT);
	PUSH_CMD(DEATHWAIT);
	PUSH_CMD(SQUADWAIT);
	PUSH_CMD(GATHERWAIT);
	PUSH_CMD(MOVE);
	PUSH_CMD(PATROL);
	PUSH_CMD(FIGHT);
	PUSH_CMD(ATTACK);
	PUSH_CMD(AREA_ATTACK);
	PUSH_CMD(GUARD);
	PUSH_CMD(GROUPSELECT);
	PUSH_CMD(GROUPADD);
	PUSH_CMD(GROUPCLEAR);
	PUSH_CMD(REPAIR);
	PUSH_CMD(FIRE_STATE);
	PUSH_CMD(MOVE_STATE);
	PUSH_CMD(SETBASE);
	PUSH_CMD(INTERNAL);
	PUSH_CMD(SELFD);
	PUSH_CMD(LOAD_UNITS);
	PUSH_CMD(LOAD_ONTO);
	PUSH_CMD(UNLOAD_UNITS);
	PUSH_CMD(UNLOAD_UNIT);
	PUSH_CMD(ONOFF);
	PUSH_CMD(RECLAIM);
	PUSH_CMD(CLOAK);
	PUSH_CMD(STOCKPILE);
	PUSH_CMD(MANUALFIRE);
	LuaInsertDualMapPair(L, "DGUN", CMD_MANUALFIRE); // backward compability (TODO: find a way to print a warning when used!)
	PUSH_CMD(RESTORE);
	PUSH_CMD(REPEAT);
	PUSH_CMD(TRAJECTORY);
	PUSH_CMD(RESURRECT);
	PUSH_CMD(CAPTURE);
	PUSH_CMD(AUTOREPAIRLEVEL);
	LuaInsertDualMapPair(L, "LOOPBACKATTACK", CMD_ATTACK); // backward compability (TODO: find a way to print a warning when used!)
	PUSH_CMD(IDLEMODE);

	return true;
}


/******************************************************************************/
/******************************************************************************/
