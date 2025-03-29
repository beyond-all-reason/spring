/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaSyncedTable.h"

#include "LuaInclude.h"

#include "LuaHandleSynced.h"
#include "LuaHashString.h"
#include "LuaUtils.h"


static int SyncTableIndex(lua_State* L);
static int SyncTableNewIndex(lua_State* L);
static int SyncTableMetatable(lua_State* L);

/******************************************************************************/

static int SyncTableIndex(lua_State* dstL)
{
	if (lua_isnoneornil(dstL, -1))
		return 0;

	auto slh = CSplitLuaHandle::GetSyncedHandle(dstL);
	if (!slh->IsValid())
		return 0;
	auto srcL = slh->GetLuaState();

	const int srcTop = lua_gettop(srcL);
	const int dstTop = lua_gettop(dstL);

	// copy the index & get value
	lua_pushvalue(srcL, LUA_GLOBALSINDEX);
	const int keyCopied = LuaUtils::CopyData(srcL, dstL, 1);
	assert(keyCopied > 0);
	lua_rawget(srcL, -2);

	// copy to destination
	const int valueCopied = LuaUtils::CopyData(dstL, srcL, 1);
	if (lua_istable(dstL, -1)) {
		// disallow writing in SYNCED[...]
		lua_createtable(dstL, 0, 2); {
			LuaPushNamedCFunc(dstL, "__newindex",  SyncTableNewIndex);
			LuaPushNamedCFunc(dstL, "__metatable", SyncTableMetatable);
		}
		lua_setmetatable(dstL, -2);
	}

	assert(valueCopied == 1);
	assert(dstTop + 1 == lua_gettop(dstL));

	lua_settop(srcL, srcTop);
	return valueCopied;
}


static int SyncTableNewIndex(lua_State* L)
{
	luaL_error(L, "Attempt to write to SYNCED table");
	return 0;
}


static int SyncTableMetatable(lua_State* L)
{
	luaL_error(L, "Attempt to access SYNCED metatable");
	return 0;
}


/******************************************************************************/
/******************************************************************************/

/***
 * Proxy table for reading synced global state in unsynced code.
 * 
 * **Generally not recommended.** Instead, listen to the same events as synced
 * and build the table in parallel
 * 
 * Unsynced code can read from the synced global table (`_G`) using the `SYNCED`
 * proxy table. e.g. `_G.foo` can be access from unsynced via `SYNCED.foo`.
 * 
 * This table makes *a copy* of the object on the other side, and only copies
 * numbers, strings, bools and tables (recursively but with the type
 * restriction), in particular this does not allow access to functions.
 * 
 * Note that this makes a copy on each access, so is very slow and will not
 * reflect changes. Cache it, but remember to refresh.
 * 
 * 
 * @global SYNCED table<string, any>
 */
bool LuaSyncedTable::PushEntries(lua_State* L)
{
	HSTR_PUSH(L, "SYNCED");
	lua_newtable(L); { // the proxy table

		lua_createtable(L, 0, 3); { // the metatable
			LuaPushNamedCFunc(L, "__index",     SyncTableIndex);
			LuaPushNamedCFunc(L, "__newindex",  SyncTableNewIndex);
			LuaPushNamedCFunc(L, "__metatable", SyncTableMetatable);
		}

		lua_setmetatable(L, -2);
	}
	lua_rawset(L, -3);

	return true;
}


/******************************************************************************/
/******************************************************************************/
