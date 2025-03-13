/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaConstEngine.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Game/GameVersion.h"
#include "System/Platform/Misc.h"
#include "Rendering/Fonts/glFont.h"

/******************************************************************************
 * Engine constants
 * @see rts/Lua/LuaConstEngine.cpp
******************************************************************************/

/***
 * @class FeatureSupport
 * @field NegativeGetUnitCurrentCommand boolean Whether Spring.GetUnitCurrentCommand allows negative indices to look from the end
 * @field hasExitOnlyYardmaps boolean Whether yardmaps accept 'e' (exit only) and 'u' (unbuildable, walkable)
 * @field rmlUiApiVersion integer Version of Recoil's rmlUI API
 * @field noAutoShowMetal boolean Whether the engine switches to the metal view when selecting a "build metal extractor" command (yes if false)
 * @field maxPiecesPerModel integer How many pieces supported for 3d models?
 * @field gunshipCruiseAltitudeMultiplier number For gunships, the cruiseAltitude from the unit def is multiplied by this much
 * @field noRefundForConstructionDecay boolean Whether there is no refund for construction decay (100% metal back if false)
 * @field noRefundForFactoryCancel boolean Whether there is no refund for factory cancel (100% metal back if false)
 * @field noOffsetForFeatureID boolean Whether featureID from various interfaces (targetID for Reclaim commands, ownerID from SpringGetGroundDecalOwner, etc) needs to be offset by `Game.maxUnits`
 */

/***
 * Engine specific information.
 *
 * @table Engine
 * @field version string "Major.Minor.PatchSet" for releases, "Major.Minor.PatchSet-CommitNum-gHash branch" otherwise
 * @field versionFull string "Major.Minor.PatchSet" for releases, "Major.Minor.PatchSet-CommitNum-gHash branch" otherwise. Will also include (buildFlags), if there're any.
 * @field versionMajor string Major part of the named release version
 * @field versionMinor string Minor part of the named release version
 * @field versionPatchSet string Build number of the named release version
 * @field commitsNumber string Number of commits after the latest named release, non-zero indicates a "dev" build
 * @field buildFlags string Gets additional engine buildflags, e.g. "Debug" or "Sync-Debug"
 * @field featureSupport FeatureSupport Table containing various engine features as keys; use for cross-version compat
 * @field wordSize number Indicates the build type always 64 these days
 * @field gameSpeed number Number of simulation gameframes per second
 * @field textColorCodes TextColorCode Table containing keys that represent the color code operations during font rendering
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

	LuaPushNamedNumber(L, "gameSpeed", GAME_SPEED);

	/* If possible, entries should be bools that resolve to false in the "old" version
	 * and to true in the "new" version; this is because any version beforehand has it
	 * "set" to nil which is booleanly false as well. This way games doing the check:
	 *
	 *  if Engine.FeatureSupport.Foo then
	 *
	 * will be compatible even on engines that don't yet know about the entry at all. */
	lua_pushliteral(L, "FeatureSupport");
	lua_createtable(L, 0, 9);
		LuaPushNamedBool(L, "NegativeGetUnitCurrentCommand", true);
		LuaPushNamedBool(L, "hasExitOnlyYardmaps", true);
		LuaPushNamedNumber(L, "rmlUiApiVersion", 1);
		LuaPushNamedBool(L, "noAutoShowMetal", false);
		LuaPushNamedNumber(L, "maxPiecesPerModel", MAX_PIECES_PER_MODEL);
		LuaPushNamedBool(L, "transformsInGL4", true);
		LuaPushNamedNumber(L, "gunshipCruiseAltitudeMultiplier", 1.5f); // see https://github.com/beyond-all-reason/spring/issues/1028
		LuaPushNamedBool(L, "noRefundForConstructionDecay", false);
		LuaPushNamedBool(L, "noRefundForFactoryCancel", false);
		LuaPushNamedBool(L, "noOffsetForFeatureID", false);
	lua_rawset(L, -3);

	lua_pushliteral(L, "textColorCodes");
	lua_createtable(L, 0, 3);
		LuaPushNamedChar(L, "Color"          , static_cast<char>(CglFont::ColorCodeIndicator  ));
		LuaPushNamedChar(L, "ColorAndOutline", static_cast<char>(CglFont::ColorCodeIndicatorEx));
		LuaPushNamedChar(L, "Reset"          , static_cast<char>(CglFont::ColorResetIndicator ));
	lua_rawset(L, -3);

	return true;
}
