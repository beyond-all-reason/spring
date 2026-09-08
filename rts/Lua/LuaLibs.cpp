/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "LuaLibs.h"

#include "LuaIO.h"
#include "lib/lua/include/LuaInclude.h"

#include <initializer_list>

static void RemoveMethods(lua_State* L, const char *lib, const std::initializer_list <const char *> &methods)
{
	lua_getglobal(L, lib);
	for (const auto &method : methods) {
		lua_pushstring(L, method);
		lua_pushnil(L);
		lua_rawset(L, -3);
	}
	lua_pop(L, 1);
}

namespace LuaLibs {

	void OpenSynced(lua_State* L, bool registerCreg)
	{
		if (registerCreg) {
			SPRING_LUA_OPEN_LIB(L, luaopen_base);
			SPRING_LUA_OPEN_LIB(L, luaopen_math);
			SPRING_LUA_OPEN_LIB(L, luaopen_table);
			SPRING_LUA_OPEN_LIB(L, luaopen_string);
		} else {
			LUA_OPEN_LIB(L, luaopen_base);
			LUA_OPEN_LIB(L, luaopen_math);
			LUA_OPEN_LIB(L, luaopen_table);
			LUA_OPEN_LIB(L, luaopen_string);
		}
	}

	void OpenUnsynced(lua_State* L)
	{
		OpenSynced(L, false);
		LUA_OPEN_LIB(L, luaopen_io);
		LUA_OPEN_LIB(L, luaopen_os);
		LUA_OPEN_LIB(L, luaopen_debug);

		lua_set_fopen (L, LuaIO::fopen);
		lua_set_system(L, LuaIO::system);
		lua_set_remove(L, LuaIO::remove);
		lua_set_rename(L, LuaIO::rename);

		RemoveMethods(L, "io",
			{ "popen"
		});
		RemoveMethods(L, "os",
			{ "exit"
			, "execute"
			, "tmpname"
			, "getenv"
		});
	}

} // namespace LuaLibs