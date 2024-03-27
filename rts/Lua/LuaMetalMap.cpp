/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaMetalMap.h"

#include "LuaInclude.h"
#include "LuaUtils.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"

#include <tracy/Tracy.hpp>

/******************************************************************************
 * Metal Map Lua API
 * @module MetalMap
 * @see rts/Lua/LuaMetalMap.cpp
******************************************************************************/

bool LuaMetalMap::PushReadEntries(lua_State* L)
{
	//ZoneScoped;
	REGISTER_LUA_CFUNC(GetMetalMapSize);
	REGISTER_LUA_CFUNC(GetMetalAmount);
	REGISTER_LUA_CFUNC(GetMetalExtraction);
	return true;
}

bool LuaMetalMap::PushCtrlEntries(lua_State* L)
{
	//ZoneScoped;
	REGISTER_LUA_CFUNC(SetMetalAmount);
	return true;
}

int LuaMetalMap::GetMetalMapSize(lua_State* L)
{
	//ZoneScoped;
	lua_pushnumber(L, metalMap.GetSizeX());
	lua_pushnumber(L, metalMap.GetSizeZ());
	return 2;
}

int LuaMetalMap::GetMetalAmount(lua_State* L)
{
	//ZoneScoped;
	const int x = luaL_checkint(L, 1);
	const int z = luaL_checkint(L, 2);
	// GetMetalAmount automatically clamps the value
	lua_pushnumber(L, metalMap.GetMetalAmount(x, z));
	return 1;
}

/***
 * @function Spring.SetMetalAmount
 * @number x in worldspace/16.
 * @number z in worldspace/16.
 * @number metalAmount must be between 0 and 255*maxMetal (with maxMetal from the .smd or mapinfo.lua).
 * @treturn nil
 *
 *
 */
int LuaMetalMap::SetMetalAmount(lua_State* L)
{
	//ZoneScoped;
	const int x = luaL_checkint(L, 1);
	const int z = luaL_checkint(L, 2);
	const float m = luaL_checkfloat(L, 3);
	// SetMetalAmount automatically clamps the value
	metalMap.SetMetalAmount(x, z, m);
	return 0;
}

int LuaMetalMap::GetMetalExtraction(lua_State* L)
{
	//ZoneScoped;
	const int x = luaL_checkint(L, 1);
	const int z = luaL_checkint(L, 2);
	// GetMetalExtraction automatically clamps the value
	lua_pushnumber(L, metalMap.GetMetalExtraction(x, z));
	return 1;
}




/******************************************************************************/
/******************************************************************************/
