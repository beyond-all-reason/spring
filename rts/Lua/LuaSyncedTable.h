/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_SYNCED_TABLE_H
#define LUA_SYNCED_TABLE_H

// Adds the `SYNCED` proxy table

struct lua_State;

class LuaSyncedTable {
	public:
		static bool PushEntries(lua_State* L);
};

#endif /* LUA_SYNCED_TABLE_H */
