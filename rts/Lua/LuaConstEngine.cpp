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
 * @string version "Major.Minor.PatchSet" for releases, "Major.Minor.PatchSet-CommitNum-gHash branch" otherwise
 * @string versionFull "Major.Minor.PatchSet" for releases, "Major.Minor.PatchSet-CommitNum-gHash branch" otherwise. Will also include (buildFlags), if there're any.
 * @string versionMajor - Major part of the named release version
 * @string versionMinor - Minor part of the named release version
 * @string versionPatchSet - Build numbert of the named release version
 * @string commitsNumber - number of commits after the latest named release, non-zero indicates a "dev" build
 * @string buildFlags Gets additional engine buildflags, e.g. "Debug" or "Sync-Debug"
 * @string FeatureSupport table containing various engine features as keys; use for cross-version compat
 * @number wordSize indicates the build type always 64 these days
 */

bool LuaConstEngine::PushEntries(lua_State* L)
{
	LuaPushNamedString(L, "version"        , SpringVersion::GetSync()      );
	LuaPushNamedString(L, "versionFull"    , SpringVersion::GetFull()      );
	LuaPushNamedString(L, "versionMajor"   , SpringVersion::GetMajor()     );
	LuaPushNamedString(L, "versionMinor"   , SpringVersion::GetMinor()     );
	LuaPushNamedString(L, "versionPatchSet", SpringVersion::GetPatchSet()  );
	LuaPushNamedString(L, "commitsNumber"  , SpringVersion::GetCommits()   );
	LuaPushNamedString(L, "buildFlags"     , SpringVersion::GetAdditional());
	LuaPushNamedNumber(L, "wordSize", (!CLuaHandle::GetHandleSynced(L))? Platform::NativeWordSize() * 8: 0);


	/* If possible, entries should be bools that resolve to false in the "old" version
	 * and to true in the "new" version; this is because any version beforehand has it
	 * "set" to nil which is booleanly false as well. This way games doing the check:
	 *
	 *  if Engine.FeatureSupport.Foo then
	 *
	 * will be compatible even on engines that don't yet know about the entry at all. */
	lua_pushliteral(L, "FeatureSupport");
	lua_createtable(L, 0, 3);
		LuaPushNamedBool(L, "NegativeGetUnitCurrentCommand", true);
		LuaPushNamedBool(L, "hasExitOnlyYardmaps", true);
		LuaPushNamedNumber(L, "rmlUiApiVersion", 1);
	lua_rawset(L, -3);

	return true;
}

