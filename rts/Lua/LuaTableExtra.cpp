/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaTableExtra.h"
#include "LuaUtils.h"

/******************************************************************************
 * Table extensions
******************************************************************************/

/*** Returns a table with preallocated memory
 *
 * Returns an empty table with more memory allocated.
 * This lets you microoptimize cases where a table receives
 * a lot of elements and you know the number beforehand,
 * such as one for each UnitDef, by avoiding reallocation.
 *
 * @function table.new
 * @param nArray number hint for count of array elements
 * @param nHashed number hint for count of hashtable elements
 * @return table
 */
static int TableExtra_new(lua_State* L)
{
	int nArray = luaL_optinteger(L, 1, 0);
	int nHash  = luaL_optinteger(L, 2, 0);

	if (nArray < 0)
		nArray = 0;
	if (nHash < 0)
		nHash = 0;

	lua_createtable(L, nArray, nHash);

	return 1;
}

bool LuaTableExtra::PushEntries(lua_State* L)
{
	LuaPushNamedCFunc(L, "new", TableExtra_new);
	return true;
}
