/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaRules.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "LuaObjectRendering.h"
#include "LuaCallInCheck.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/VFSModes.h" // for SPRING_VFS_*
#include "System/Threading/SpringThreading.h"

#include <cassert>

CLuaRules* luaRules = nullptr;

static const char* LuaRulesSyncedFilename   = "LuaRules/main.lua";
static const char* LuaRulesUnsyncedFilename = "LuaRules/draw.lua";


/******************************************************************************/
/******************************************************************************/

static spring::mutex m_singleton;

DECL_LOAD_SPLIT_HANDLER(CLuaRules, luaRules)
DECL_FREE_HANDLER(CLuaRules, luaRules)


/******************************************************************************/
/******************************************************************************/

CLuaRules::CLuaRules(bool dryRun): CSplitLuaHandle("LuaRules", LUA_HANDLE_ORDER_RULES)
{
	if (!IsValid())
		return;

	Init(dryRun);
}

CLuaRules::~CLuaRules()
{
	luaRules = nullptr;
}


std::string CLuaRules::GetUnsyncedFileName() const
{
	return LuaRulesUnsyncedFilename;
}

std::string CLuaRules::GetSyncedFileName() const
{
	return LuaRulesSyncedFilename;
}

std::string CLuaRules::GetInitFileModes() const
{
	return SPRING_VFS_MOD_BASE;
}

int CLuaRules::GetInitSelectTeam() const
{
	return CEventClient::AllAccessTeam;
}



/******************************************************************************
 * Lua Rules
 *
 * @see rts/Lua/LuaRules.cpp
******************************************************************************/


bool CLuaRules::AddSyncedCode(lua_State* L)
{
	lua_getglobal(L, "Script");
	LuaPushNamedCFunc(L, "PermitHelperAIs", PermitHelperAIs);
	lua_pop(L, 1);

	return true;
}


bool CLuaRules::AddUnsyncedCode(lua_State* L)
{
	lua_getglobal(L, "Spring");

	/*** @field Spring.UnitRendering ObjectRenderingTable */
	lua_pushliteral(L, "UnitRendering");
	lua_createtable(L, 0, 17);
	LuaObjectRendering<LUAOBJ_UNIT>::PushEntries(L);
	lua_rawset(L, -3);

	/*** @field Spring.FeatureRendering ObjectRenderingTable */
	lua_pushliteral(L, "FeatureRendering");
	lua_createtable(L, 0, 17);
	LuaObjectRendering<LUAOBJ_FEATURE>::PushEntries(L);
	lua_rawset(L, -3);

	lua_pop(L, 1); // Spring

	return true;
}



/******************************************************************************/
/******************************************************************************/
//
// LuaRules Call-Outs
//

/***
 *
 * @function Script.PermitHelperAIs
 * @param permit boolean
 */
int CLuaRules::PermitHelperAIs(lua_State* L)
{
	if (!lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect argument to PermitHelperAIs()");
	}
	gs->noHelperAIs = !lua_toboolean(L, 1);
	LOG("LuaRules has %s helper AIs",
			(gs->noHelperAIs ? "disabled" : "enabled"));
	return 0;
}

/******************************************************************************/
/******************************************************************************/
