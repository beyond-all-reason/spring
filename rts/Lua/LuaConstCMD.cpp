/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstCMD.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"


/******************************************************************************
 * Command constants
 * @module CMD
 * @see rts/Lua/LuaConstCMD.cpp
******************************************************************************/

/*** @table CMD
 *
 * @param FIRESTATE_NONE -1
 * @param MOVESTATE_NONE -1
 * @param STOP 0
 * @param MOVESTATE_HOLDPOS 0
 * @param FIRESTATE_HOLDFIRE 0
 * @param INSERT 1
 * @param MOVESTATE_MANEUVER 1
 * @param FIRESTATE_RETURNFIRE 1
 * @param WAITCODE_TIME 1
 * @param WAITCODE_DEATH 2
 * @param MOVESTATE_ROAM 2
 * @param REMOVE 2
 * @param FIRESTATE_FIREATWILL 2
 * @param FIRESTATE_FIREATNEUTRAL 3
 * @param WAITCODE_SQUAD 3
 * @param OPT_META 4
 * @param WAITCODE_GATHER 4
 * @param WAIT 5
 * @param TIMEWAIT 6
 * @param DEATHWAIT 7
 * @param OPT_INTERNAL 8
 * @param SQUADWAIT 8
 * @param GATHERWAIT 9
 * @param MOVE 10
 * @param PATROL 15
 * @param FIGHT 16
 * @param OPT_RIGHT 16
 * @param LOOPBACKATTACK 20
 * @param ATTACK 20
 * @param AREA_ATTACK 21
 * @param GUARD 25
 * @param OPT_SHIFT 32
 * @param GROUPSELECT 35
 * @param GROUPADD 36
 * @param GROUPCLEAR 37
 * @param REPAIR 40
 * @param FIRE_STATE 45
 * @param MOVE_STATE 50
 * @param SETBASE 55
 * @param INTERNAL 60
 * @param OPT_CTRL 64
 * @param SELFD 65
 * @param SET_WANTED_MAX_SPEED 70
 * @param LOAD_UNITS 75
 * @param LOAD_ONTO 76
 * @param UNLOAD_UNITS 80
 * @param UNLOAD_UNIT 81
 * @param ONOFF 85
 * @param RECLAIM 90
 * @param CLOAK 95
 * @param STOCKPILE 100
 * @param MANUALFIRE 105
 * @param DGUN 105
 * @param RESTORE 110
 * @param REPEAT 115
 * @param TRAJECTORY 120
 * @param RESURRECT 125
 * @param OPT_ALT 128
 * @param CAPTURE 130
 * @param AUTOREPAIRLEVEL 135
 * @param IDLEMODE 145 
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
