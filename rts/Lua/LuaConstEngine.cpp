/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaConstEngine.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Game/GameVersion.h"
#include "System/Platform/Misc.h"

/******************************************************************************
 * Engine constants
 * @module Engine
 * @see rts/Lua/LuaConstEngine.cpp
******************************************************************************/

/*** Engine specific information
 *
 * @table Engine
 * @string version Returns the same as `spring  *sync-version`, e.g. "92"
 * @string versionFull 
 * @string versionPatchSet 
 * @string buildFlags (unsynced only) Gets additional engine buildflags, e.g. "OMP" or "MT-Sim DEBUG"
 * @number wordSize indicates the build type and is either 32 or 64 (or 0 in synced code)
 */

bool LuaConstEngine::PushEntries(lua_State* L)
{
	LuaPushNamedString(L, "version"        ,                                    SpringVersion::GetSync()          );
	LuaPushNamedString(L, "versionFull"    , (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetFull()      : "");
	LuaPushNamedString(L, "versionPatchSet", (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetPatchSet()  : "");
	LuaPushNamedString(L, "buildFlags"     , (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetAdditional(): "");

	#if 0
	LuaPushNamedNumber(L, "nativeWordSize", (!CLuaHandle::GetHandleSynced(L))? Platform::NativeWordSize() * 8: 0); // engine
	LuaPushNamedNumber(L, "systemWordSize", (!CLuaHandle::GetHandleSynced(L))? Platform::SystemWordSize() * 8: 0); // op-sys
	#else
	LuaPushNamedNumber(L, "wordSize", (!CLuaHandle::GetHandleSynced(L))? Platform::NativeWordSize() * 8: 0);
	#endif

	// temporary, see #1028
	LuaPushNamedNumber(L, "cruiseAltitudeMultiplier", 1.0f);

	return true;
}

