/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaMetalMap.h"

#include "LuaInclude.h"
#include "LuaUtils.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"

#include "System/Misc/TracyDefs.h"

/******************************************************************************
 * Metal Map Lua API
 * @see rts/Lua/LuaMetalMap.cpp
******************************************************************************/

bool LuaMetalMap::PushReadEntries(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	REGISTER_LUA_CFUNC(GetMetalMapSize);
	REGISTER_LUA_CFUNC(GetMetalAmount);
	REGISTER_LUA_CFUNC(GetMetalExtraction);
	return true;
}

bool LuaMetalMap::PushCtrlEntries(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	REGISTER_LUA_CFUNC(SetMetalAmount);
	return true;
}

/***
 * @function Spring.GetMetalMapSize
 * @return integer x X coordinate in worldspace / `Game.metalMapSquareSize`.
 * @return integer z Z coordinate in worldspace / `Game.metalMapSquareSize`.
 */
int LuaMetalMap::GetMetalMapSize(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	lua_pushnumber(L, metalMap.GetSizeX());
	lua_pushnumber(L, metalMap.GetSizeZ());
	return 2;
}

/***
 * Returns the amount of metal on a single square.
 * @function Spring.GetMetalAmount
 * @param x integer X coordinate in worldspace / `Game.metalMapSquareSize`.
 * @param z integer Z coordinate in worldspace / `Game.metalMapSquareSize`.
 * @return number amount
 */
int LuaMetalMap::GetMetalAmount(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int x = luaL_checkint(L, 1);
	const int z = luaL_checkint(L, 2);
	// GetMetalAmount automatically clamps the value
	lua_pushnumber(L, metalMap.GetMetalAmount(x, z));
	return 1;
}

/***
 * Sets the amount of metal on a single square.
 * @function Spring.SetMetalAmount
 * @param x integer X coordinate in worldspace / `Game.metalMapSquareSize`.
 * @param z integer Z coordinate in worldspace / `Game.metalMapSquareSize`.
 * @param metalAmount number must be between 0 and 255*maxMetal (with maxMetal from the .smd or mapinfo.lua).
 * @return nil
 */
int LuaMetalMap::SetMetalAmount(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int x = luaL_checkint(L, 1);
	const int z = luaL_checkint(L, 2);
	const float m = luaL_checkfloat(L, 3);
	// SetMetalAmount automatically clamps the value
	metalMap.SetMetalAmount(x, z, m);
	return 0;
}

/***
 * @function Spring.GetMetalExtraction
 * @param x integer X coordinate in worldspace / `Game.metalMapSquareSize`.
 * @param z integer Z coordinate in worldspace / `Game.metalMapSquareSize`.
 * @return number extraction
 */
int LuaMetalMap::GetMetalExtraction(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int x = luaL_checkint(L, 1);
	const int z = luaL_checkint(L, 2);
	// GetMetalExtraction automatically clamps the value
	lua_pushnumber(L, metalMap.GetMetalExtraction(x, z));
	return 1;
}




/******************************************************************************/
/******************************************************************************/
