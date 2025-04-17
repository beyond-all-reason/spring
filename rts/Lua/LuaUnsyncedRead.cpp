/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaUnsyncedRead.h"

#include "LuaConfig.h"
#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"
#include "LuaRules.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/IVideoCapturing.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/TraceRay.h"
#include "Game/Camera/CameraController.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/ScanCodes.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Game/UI/PlayerRoster.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/BaseGroundTextures.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Menu/LuaMenuController.h"
#include "Net/GameServer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/Particles/Classes/NanoProjectile.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "Game/UI/Groups/Group.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "Net/Protocol/NetProtocol.h" // NETMSG_*
#include "System/TimeProfiler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Config/ConfigVariable.h"
#include "System/Input/KeyInput.h"
#include "System/LoadSave/DemoReader.h"
#include "System/Log/DefaultFilter.h"
#include "System/Platform/SDL1_keysym.h"
#include "System/Platform/Misc.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"
#include "System/StringUtil.h"
#include "System/Misc/SpringTime.h"
#include "System/ScopedResource.h"
#include "System/Math/NURBS.h"

#if !defined(HEADLESS) && !defined(NO_SOUND)
	#include "System/Sound/OpenAL/EFX.h"
	#include "System/Sound/OpenAL/EFXPresets.h"
#endif

#include <cctype>
#include <algorithm>

#include <SDL_keyboard.h>
#include <SDL_clipboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>


/******************************************************************************
 * Callouts to get state
 *
 * @see rts/Lua/LuaUnsyncedRead.cpp
******************************************************************************/

bool LuaUnsyncedRead::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(IsReplay);
	REGISTER_LUA_CFUNC(GetReplayLength);

	REGISTER_LUA_CFUNC(GetGameName);
	REGISTER_LUA_CFUNC(GetMenuName);

	REGISTER_LUA_CFUNC(GetProfilerTimeRecord);
	REGISTER_LUA_CFUNC(GetProfilerRecordNames);

	REGISTER_LUA_CFUNC(GetLuaMemUsage);
	REGISTER_LUA_CFUNC(GetVidMemUsage);

	REGISTER_LUA_CFUNC(GetDrawFrame);
	REGISTER_LUA_CFUNC(GetFrameTimeOffset);
	REGISTER_LUA_CFUNC(GetGameSecondsInterpolated);
	REGISTER_LUA_CFUNC(GetLastUpdateSeconds);
	REGISTER_LUA_CFUNC(GetVideoCapturingMode);

	REGISTER_LUA_CFUNC(GetNumDisplays);
	REGISTER_LUA_CFUNC(GetViewGeometry);
	REGISTER_LUA_CFUNC(GetDualViewGeometry);
	REGISTER_LUA_CFUNC(GetWindowGeometry);
	REGISTER_LUA_CFUNC(GetWindowDisplayMode);
	REGISTER_LUA_CFUNC(GetScreenGeometry);
	REGISTER_LUA_CFUNC(GetMiniMapGeometry);
	REGISTER_LUA_CFUNC(GetMiniMapDualScreen);
	REGISTER_LUA_CFUNC(GetMiniMapRotation);
	REGISTER_LUA_CFUNC(GetSelectionBox);
	REGISTER_LUA_CFUNC(IsAboveMiniMap);
	REGISTER_LUA_CFUNC(IsGUIHidden);

	REGISTER_LUA_CFUNC(IsAABBInView);
	REGISTER_LUA_CFUNC(IsSphereInView);

	REGISTER_LUA_CFUNC(IsUnitAllied);
	REGISTER_LUA_CFUNC(IsUnitInView);
	REGISTER_LUA_CFUNC(IsUnitVisible);
	REGISTER_LUA_CFUNC(IsUnitIcon);
	REGISTER_LUA_CFUNC(IsUnitSelected);

	REGISTER_LUA_CFUNC(GetUnitLuaDraw);
	REGISTER_LUA_CFUNC(GetUnitNoDraw);
	REGISTER_LUA_CFUNC(GetUnitEngineDrawMask);
	REGISTER_LUA_CFUNC(GetUnitNoMinimap);
	REGISTER_LUA_CFUNC(GetUnitNoGroup);
	REGISTER_LUA_CFUNC(GetUnitNoSelect);
	REGISTER_LUA_CFUNC(GetUnitAlwaysUpdateMatrix);
	REGISTER_LUA_CFUNC(GetUnitDrawFlag);
	REGISTER_LUA_CFUNC(GetUnitSelectionVolumeData);
	REGISTER_LUA_CFUNC(GetFeatureLuaDraw);
	REGISTER_LUA_CFUNC(GetFeatureNoDraw);
	REGISTER_LUA_CFUNC(GetFeatureEngineDrawMask);
	REGISTER_LUA_CFUNC(GetFeatureAlwaysUpdateMatrix);
	REGISTER_LUA_CFUNC(GetFeatureDrawFlag);
	REGISTER_LUA_CFUNC(GetFeatureSelectionVolumeData);

	REGISTER_LUA_CFUNC(GetUnitTransformMatrix);
	REGISTER_LUA_CFUNC(GetFeatureTransformMatrix);

	REGISTER_LUA_CFUNC(GetUnitViewPosition);

	REGISTER_LUA_CFUNC(GetVisibleUnits);
	REGISTER_LUA_CFUNC(GetVisibleFeatures);
	REGISTER_LUA_CFUNC(GetVisibleProjectiles);

	REGISTER_LUA_CFUNC(GetRenderUnits);
	REGISTER_LUA_CFUNC(GetRenderUnitsDrawFlagChanged);
	REGISTER_LUA_CFUNC(GetRenderFeatures);
	REGISTER_LUA_CFUNC(GetRenderFeaturesDrawFlagChanged);

	REGISTER_LUA_CFUNC(ClearUnitsPreviousDrawFlag);
	REGISTER_LUA_CFUNC(ClearFeaturesPreviousDrawFlag);

	REGISTER_LUA_CFUNC(GetUnitsInScreenRectangle);
	REGISTER_LUA_CFUNC(GetFeaturesInScreenRectangle);

	REGISTER_LUA_CFUNC(GetTeamColor);
	REGISTER_LUA_CFUNC(GetTeamOrigColor);

	REGISTER_LUA_CFUNC(GetLocalPlayerID);
	REGISTER_LUA_CFUNC(GetLocalTeamID);
	REGISTER_LUA_CFUNC(GetLocalAllyTeamID);

	// aliases
	REGISTER_NAMED_LUA_CFUNC("GetMyPlayerID", GetLocalPlayerID);
	REGISTER_NAMED_LUA_CFUNC("GetMyTeamID", GetLocalTeamID);
	REGISTER_NAMED_LUA_CFUNC("GetMyAllyTeamID", GetLocalAllyTeamID);

	REGISTER_LUA_CFUNC(GetSpectatingState);

	REGISTER_LUA_CFUNC(GetSelectedUnits);
	REGISTER_LUA_CFUNC(GetSelectedUnitsSorted);
	REGISTER_LUA_CFUNC(GetSelectedUnitsCounts);
	REGISTER_LUA_CFUNC(GetSelectedUnitsCount);
	REGISTER_LUA_CFUNC(GetBoxSelectionByEngine);

	REGISTER_LUA_CFUNC(HaveShadows);
	REGISTER_LUA_CFUNC(HaveAdvShading);
	REGISTER_LUA_CFUNC(GetWaterMode);
	REGISTER_LUA_CFUNC(GetMapDrawMode);
	REGISTER_LUA_CFUNC(GetMapSquareTexture);

	REGISTER_LUA_CFUNC(GetLosViewColors);

	REGISTER_LUA_CFUNC(GetNanoProjectileParams);

	REGISTER_LUA_CFUNC(GetCameraNames);
	REGISTER_LUA_CFUNC(GetCameraState);
	REGISTER_LUA_CFUNC(GetCameraPosition);
	REGISTER_LUA_CFUNC(GetCameraDirection);
	REGISTER_LUA_CFUNC(GetCameraRotation);
	REGISTER_LUA_CFUNC(GetCameraFOV);
	REGISTER_LUA_CFUNC(GetCameraVectors);
	REGISTER_LUA_CFUNC(WorldToScreenCoords);
	REGISTER_LUA_CFUNC(TraceScreenRay);
	REGISTER_LUA_CFUNC(GetPixelDir);

	REGISTER_LUA_CFUNC(GetTimer);
	REGISTER_LUA_CFUNC(GetTimerMicros);
	REGISTER_LUA_CFUNC(GetFrameTimer);
	REGISTER_LUA_CFUNC(DiffTimers);

	REGISTER_LUA_CFUNC(GetDrawSeconds);

	REGISTER_LUA_CFUNC(GetSoundDevices);
	REGISTER_LUA_CFUNC(GetSoundStreamTime);
	REGISTER_LUA_CFUNC(GetSoundEffectParams);

	REGISTER_LUA_CFUNC(GetFPS);
	REGISTER_LUA_CFUNC(GetGameSpeed);
	REGISTER_LUA_CFUNC(GetGameState);

	REGISTER_LUA_CFUNC(GetActiveCommand);
	REGISTER_LUA_CFUNC(GetDefaultCommand);
	REGISTER_LUA_CFUNC(GetActiveCmdDescs);
	REGISTER_LUA_CFUNC(GetActiveCmdDesc);
	REGISTER_LUA_CFUNC(GetCmdDescIndex);
	REGISTER_LUA_CFUNC(GetBuildFacing);
	REGISTER_LUA_CFUNC(GetBuildSpacing);
	REGISTER_LUA_CFUNC(GetGatherMode);
	REGISTER_LUA_CFUNC(GetActivePage);

	REGISTER_LUA_CFUNC(GetMouseButtonsPressed);
	REGISTER_LUA_CFUNC(GetMouseState);
	REGISTER_LUA_CFUNC(GetMouseCursor);
	REGISTER_LUA_CFUNC(GetMouseStartPosition);

	REGISTER_LUA_CFUNC(GetKeyFromScanSymbol);
	REGISTER_LUA_CFUNC(GetKeyState);
	REGISTER_LUA_CFUNC(GetModKeyState);
	REGISTER_LUA_CFUNC(GetPressedKeys);
	REGISTER_LUA_CFUNC(GetPressedScans);
	REGISTER_LUA_CFUNC(GetInvertQueueKey);

	REGISTER_LUA_CFUNC(GetClipboard);

	REGISTER_LUA_CFUNC(GetKeyCode);
	REGISTER_LUA_CFUNC(GetKeySymbol);
	REGISTER_LUA_CFUNC(GetScanSymbol);
	REGISTER_LUA_CFUNC(GetKeyBindings);
	REGISTER_LUA_CFUNC(GetActionHotKeys);

	REGISTER_LUA_CFUNC(GetLastMessagePositions);
	REGISTER_LUA_CFUNC(GetConsoleBuffer);
	REGISTER_LUA_CFUNC(GetCurrentTooltip);
	REGISTER_LUA_CFUNC(IsUserWriting);

	REGISTER_LUA_CFUNC(GetUnitGroup);
	REGISTER_LUA_CFUNC(GetGroupList);
	REGISTER_LUA_CFUNC(GetSelectedGroup);
	REGISTER_LUA_CFUNC(GetGroupUnits);
	REGISTER_LUA_CFUNC(GetGroupUnitsSorted);
	REGISTER_LUA_CFUNC(GetGroupUnitsCounts);
	REGISTER_LUA_CFUNC(GetGroupUnitsCount);

	REGISTER_LUA_CFUNC(GetPlayerRoster);
	REGISTER_LUA_CFUNC(GetPlayerTraffic);
	REGISTER_LUA_CFUNC(GetPlayerStatistics);

	REGISTER_LUA_CFUNC(GetDrawSelectionInfo);

	REGISTER_LUA_CFUNC(GetConfigParams);
	REGISTER_LUA_CFUNC(GetConfigInt);
	REGISTER_LUA_CFUNC(GetConfigFloat);
	REGISTER_LUA_CFUNC(GetConfigString);
	REGISTER_LUA_CFUNC(GetLogSections);

	REGISTER_LUA_CFUNC(GetAllGroundDecals);
	REGISTER_LUA_CFUNC(GetGroundDecalMiddlePos);
	REGISTER_LUA_CFUNC(GetGroundDecalQuadPos);
	REGISTER_LUA_CFUNC(GetGroundDecalSizeAndHeight);
	REGISTER_LUA_CFUNC(GetGroundDecalRotation);
	REGISTER_LUA_CFUNC(GetGroundDecalTexture);
	REGISTER_LUA_CFUNC(GetGroundDecalTextures);
	REGISTER_LUA_CFUNC(GetGroundDecalTextureParams);
	REGISTER_LUA_CFUNC(GetGroundDecalAlpha);
	REGISTER_LUA_CFUNC(GetGroundDecalNormal);
	REGISTER_LUA_CFUNC(GetGroundDecalTint);
	REGISTER_LUA_CFUNC(GetGroundDecalMisc);
	REGISTER_LUA_CFUNC(GetGroundDecalCreationFrame);
	REGISTER_LUA_CFUNC(GetGroundDecalOwner);
	REGISTER_LUA_CFUNC(GetGroundDecalType);

	REGISTER_LUA_CFUNC(UnitIconGetDraw);

	REGISTER_LUA_CFUNC(GetSyncedGCInfo);
	REGISTER_LUA_CFUNC(SolveNURBSCurve);

	return true;
}




/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): unitID not a number", caller);
		return nullptr;
	}

	CUnit* unit = unitHandler.GetUnit(lua_toint(L, index));

	if (unit == nullptr)
		return nullptr;

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0)
		return CLuaHandle::GetHandleFullRead(L) ? unit : nullptr;

	if ((unit->losStatus[readAllyTeam] & (LOS_INLOS | LOS_INRADAR)) == 0)
		return nullptr;

	return unit;
}

static inline CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad featureID", caller);
		return nullptr;
	}

	CFeature* feature = featureHandler.GetFeature(lua_toint(L, index));

	if (CLuaHandle::GetHandleFullRead(L))
		return feature;

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0)
		return nullptr;
	if (feature == nullptr)
		return nullptr;
	if (feature->IsInLosForAllyTeam(readAllyTeam))
		return feature;

	return nullptr;
}




static int GetSolidObjectLuaDraw(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return 0;

	lua_pushboolean(L, obj->luaDraw);
	return 1;
}

static int GetSolidObjectNoDraw(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return 0;

	lua_pushboolean(L, obj->noDraw);
	return 1;
}

static int GetSolidObjectEngineDrawMask(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return 0;

	lua_pushinteger(L, obj->engineDrawMask);
	return 1;
}

static int GetSolidObjectSelectionVolume(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return 0;

	return LuaUtils::PushColVolData(L, &obj->selectionVolume);
}



template <typename T>
static void PushNumberContainerAsArray(lua_State* const L, const T &v)
{
	lua_createtable(L, v.size(), 0);
	for (size_t i = 0; const auto &x : v) {
		lua_pushnumber(L, x);
		lua_rawseti(L, -2, ++i);
	}
}

template <typename T>
static size_t PushUnitListSortedByDef(lua_State *const L, const T &units)
{
	using unitDefID_t = int;
	using unitID_t = int;

	std::map <unitDefID_t, std::vector <unitID_t>> unitsByDef;

	for (const auto unitID : units)
		unitsByDef[unitHandler.GetUnit(unitID)->unitDef->id].push_back(unitID);

	lua_createtable(L, 0, unitsByDef.size());

	for (const auto & [unitDefID, unitIDs] : unitsByDef) {
		assert(!unitIDs.empty());

		PushNumberContainerAsArray(L, unitIDs);
		lua_rawseti(L, -2, unitDefID);
	}

	return unitsByDef.size();
}

template <typename T>
static size_t PushSparseUnitTallyByDef(lua_State *const L, const T &v)
{
	std::vector <size_t> counts (unitDefHandler->NumUnitDefs() + 1, 0);
	size_t numDefKeys = 0;
	for (const int unitID: v)
		if (!counts[unitHandler.GetUnit(unitID)->unitDef->id]++)
			numDefKeys++;

	lua_createtable(L, 0, numDefKeys);
	for (size_t i = 0; i < counts.size(); ++i) {
		if (counts[i] == 0)
			continue;

		lua_pushnumber(L, counts[i]);
		lua_rawseti(L, -2, i);
	}

	return numDefKeys;
}


/******************************************************************************
 * Replay
 * @section replay
******************************************************************************/

/***
 *
 * @function Spring.IsReplay
 *
 * @return boolean? isReplay
 */
int LuaUnsyncedRead::IsReplay(lua_State* L)
{
	lua_pushboolean(L, gameSetup->hostDemo);
	return 1;
}


/***
 *
 * @function Spring.GetReplayLength
 *
 * @return number? timeInSeconds
 */
int LuaUnsyncedRead::GetReplayLength(lua_State* L)
{
	if (gameServer != nullptr && gameServer->GetDemoReader()) {
		lua_pushnumber(L, gameServer->GetDemoReader()->GetFileHeader().gameTime);
		return 1;
	}
	return 0;
}

/******************************************************************************
 * Game/Menu Name
 * @section gamename
******************************************************************************/

/***
 *
 * @function Spring.GetGameName
 *
 * @return string name
 */
int LuaUnsyncedRead::GetGameName(lua_State* L)
{
	lua_pushstring(L, modInfo.humanNameVersioned.c_str());
	return 1;
}

/***
 *
 * @function Spring.GetMenuName
 *
 * @return string name name .. version from Modinfo.lua. E.g. "Spring: 1944 test-5640-ac2d15b".
 */
int LuaUnsyncedRead::GetMenuName(lua_State* L)
{
	lua_pushstring(L, luaMenuController->GetMenuName().c_str());
	return 1;
}


/******************************************************************************
 * Profiling
 * @section profiling
******************************************************************************/


/***
 *
 * @function Spring.GetProfilerTimeRecord
 *
 * @param profilerName string
 * @param frameData boolean? (Default: `false`)
 *
 * @return number total in ms
 * @return number current in ms
 * @return number max_dt
 * @return number time_pct
 * @return number peak_pct
 * @return table<number,number>? frameData Table where key is the frame index and value is duration.
 */
int LuaUnsyncedRead::GetProfilerTimeRecord(lua_State* L)
{
	const CTimeProfiler::TimeRecord& record = CTimeProfiler::GetInstance().GetTimeRecord(lua_tostring(L, 1));

	int numRet = 5;
	lua_pushnumber(L, record.total.toMilliSecsf());
	lua_pushnumber(L, record.current.toMilliSecsf());
	lua_pushnumber(L, record.stats.x); // max-dt
	lua_pushnumber(L, record.stats.y); // time-%
	lua_pushnumber(L, record.stats.z); // peak-%

	if (luaL_optboolean(L, 2, false)) {
		for (size_t i = 0; i < record.frames.size(); i++) {
			lua_pushnumber(L, i + 1); // key
			lua_pushnumber(L, record.frames[i].toMilliSecsf()); // val
			lua_rawset(L, -3);
		}
		++numRet;
	}

	return numRet;
}

/***
 *
 * @function Spring.GetProfilerRecordNames
 *
 * @return string[] profilerNames
 */
int LuaUnsyncedRead::GetProfilerRecordNames(lua_State* L)
{
	const auto& sortedProfiles = CTimeProfiler::GetInstance().GetSortedProfiles();

	lua_createtable(L, sortedProfiles.size(), 0);

	for (size_t i = 0; i < sortedProfiles.size(); i++) {
		lua_pushnumber(L, i + 1); // key
		lua_pushsstring(L, sortedProfiles[i].first); // val
		lua_rawset(L, -3);
	}

	return 1;
}


/***
 *
 * @function Spring.GetLuaMemUsage
 *
 * @return number luaHandleAllocedMem in kilobytes
 * @return number luaHandleNumAllocs divided by 1000
 * @return number luaGlobalAllocedMem in kilobytes
 * @return number luaGlobalNumAllocs divided by 1000
 * @return number luaUnsyncedGlobalAllocedMem in kilobytes
 * @return number luaUnsyncedGlobalNumAllocs divided by 1000
 * @return number luaSyncedGlobalAllocedMem in kilobytes
 * @return number luaSyncedGlobalNumAllocs divided by 1000
 */
int LuaUnsyncedRead::GetLuaMemUsage(lua_State* L)
{
	const luaContextData* lcd = GetLuaContextData(L);

	// handle and global stats
	const SLuaAllocState* lhs = &lcd->allocState;
	      SLuaAllocState  lgs;

	spring_lua_alloc_get_stats(&lgs);

	lua_pushnumber(L, lhs->allocedBytes / 1024.0f); // (kilo)bytes, can exceed 1<<24 otherwise
	lua_pushnumber(L, lhs->numLuaAllocs / 1000.0f); // (kilo)allocs, ditto
	lua_pushnumber(L, lgs.allocedBytes / 1024.0f);
	lua_pushnumber(L, lgs.numLuaAllocs / 1000.0f);

	// [0] := unsynced, [1] := synced
	extern const spring::unsynced_set<const luaContextData*>* LUAHANDLE_CONTEXTS[2];

	// sum up the individual (unsynced and synced) state footprints
	for (bool synced: {false, true}) {
		lgs.allocedBytes = {0};
		lgs.numLuaAllocs = {0};

		for (const luaContextData* lcd: *LUAHANDLE_CONTEXTS[synced]) {
			lhs = &lcd->allocState;

			lgs.allocedBytes += lhs->allocedBytes;
			lgs.numLuaAllocs += lhs->numLuaAllocs;
		}

		lua_pushnumber(L, lgs.allocedBytes / 1024.0f);
		lua_pushnumber(L, lgs.numLuaAllocs / 1000.0f);
	}

	return 8;
}


/***
 *
 * @function Spring.GetVidMemUsage
 *
 * @return number usedMem in MB
 * @return number availableMem in MB
 */
int LuaUnsyncedRead::GetVidMemUsage(lua_State* L)
{
	int2 vidMemInfo;

	GetAvailableVideoRAM(&vidMemInfo.x, globalRenderingInfo.glVendor);

	lua_pushnumber(L, (vidMemInfo.x - vidMemInfo.y) / 1024.0f); // MB (used = total - free)
	lua_pushnumber(L, (vidMemInfo.x               ) / 1024.0f); // MB
	return 2;
}


static void PushTimer(lua_State* L, const spring_time& time, bool microseconds)
{
	// use time since Spring's epoch in MILLIseconds because that
	// is more likely to fit in a 32-bit pointer (on any platforms
	// where sizeof(void*) == 4) than time since ::chrono's epoch
	// (which can be arbitrarily large) and can be represented by
	// single-precision floats better
	//
	// 4e9millis == 4e6s == 46.3 days until overflow
	ptrdiff_t p = 0;

	if (microseconds) {
		const std::uint64_t micros = time.toMicroSecs<std::uint64_t>();

		if (sizeof(void*) == 8) {
			p = spring::SafeCast<std::uint64_t>(micros);
		}
		else {
			p = spring::SafeCast<std::uint32_t>(micros);
		}
	}
	else {
		const std::uint64_t millis = time.toMilliSecs<std::uint64_t>();

		if (sizeof(void*) == 8) {
			p = spring::SafeCast<std::uint64_t>(millis);
		}
		else {
			p = spring::SafeCast<std::uint32_t>(millis);
		}
	}

	lua_pushlightuserdata(L, reinterpret_cast<void*>(p));
}

/*** Get a timer with millisecond resolution
 *
 * @function Spring.GetTimer
 * @return integer
 */
int LuaUnsyncedRead::GetTimer(lua_State* L)
{
	PushTimer(L, spring_now(), false);
	return 1;
}


/*** Get a timer with microsecond resolution
 *
 * @function Spring.GetTimerMicros
 * @return integer
 */
int LuaUnsyncedRead::GetTimerMicros(lua_State* L)
{
	PushTimer(L, spring_now(), true);
	return 1;
}


/*** Get a timer for the start of the frame
 *
 * @function Spring.GetFrameTimer
 *
 * This should give better results for camera interpolations
 *
 * @param lastFrameTime boolean? (Default: `false`) whether to use last frame time instead of last frame start
 * @return integer
 */
int LuaUnsyncedRead::GetFrameTimer(lua_State* L)
{
	if (luaL_optboolean(L, 1, false)) {
		PushTimer(L, game->lastFrameTime, false);
	} else {
		PushTimer(L, globalRendering->lastFrameStart, false);
	}
	return 1;
}


/***
 *
 * @function Spring.DiffTimers
 * @param endTimer integer
 * @param startTimer integer
 * @param returnMs boolean? (Default: `false`) whether to return `timeAmount` in milliseconds as opposed to seconds
 * @param fromMicroSecs boolean? (Default: `false`) whether timers are in microseconds instead of milliseconds
 * @return number timeAmount
 */
int LuaUnsyncedRead::DiffTimers(lua_State* L)
{
	if (!lua_islightuserdata(L, 1) || !lua_islightuserdata(L, 2)) {
		luaL_error(L, "Incorrect arguments to DiffTimers()");
	}

	const void* p1 = lua_touserdata(L, 1);
	const void* p2 = lua_touserdata(L, 2);

	const std::uint64_t t1 = (sizeof(void*) == 8) ?
		*reinterpret_cast<std::uint64_t*>(&p1) :
		*reinterpret_cast<std::uint32_t*>(&p1);
	const std::uint64_t t2 = (sizeof(void*) == 8) ?
		*reinterpret_cast<std::uint64_t*>(&p2) :
		*reinterpret_cast<std::uint32_t*>(&p2);

	// t1 is supposed to be the most recent time-point
	assert(t1 >= t2);

	if (luaL_optboolean(L, 4, false)) {
		const spring_time dt = spring_time::fromMicroSecs(t1 - t2);

		if (luaL_optboolean(L, 3, false)) {
			lua_pushnumber(L, dt.toMilliSecsf());
		}
		else {
			lua_pushnumber(L, dt.toSecsf());
		}
	}
	else {
		const spring_time dt = spring_time::fromMilliSecs(t1 - t2);

		if (luaL_optboolean(L, 3, false)) {
			lua_pushnumber(L, dt.toMilliSecsf());
		}
		else {
			lua_pushnumber(L, dt.toSecsf());
		}
	}
	return 1;
}


/******************************************************************************
 * Screen/Rendering Info
 * @section screeninfo
******************************************************************************/


/***
 *
 * @function Spring.GetNumDisplays
 *
 * @return number numDisplays as returned by `SDL_GetNumVideoDisplays`
 */
int LuaUnsyncedRead::GetNumDisplays(lua_State* L)
{
	lua_pushnumber(L, SDL_GetNumVideoDisplays());
	return 1;
}


/*** Get main view geometry (map and game rendering)
 *
 * @function Spring.GetViewGeometry
 *
 * @return number viewSizeX in px
 * @return number viewSizeY in px
 * @return number viewPosX offset from leftmost screen left border in px
 * @return number viewPosY offset from bottommost screen bottom border in px
 */
int LuaUnsyncedRead::GetViewGeometry(lua_State* L)
{
	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);
	lua_pushnumber(L, globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewPosY);
	return 4;
}


/*** Get dual view geometry (minimap when enabled)
 *
 * @function Spring.GetDualViewGeometry
 *
 * @return number dualViewSizeX in px
 * @return number dualViewSizeY in px
 * @return number dualViewPosX offset from leftmost screen left border in px
 * @return number dualViewPosY offset from bottommost screen bottom border in px
 */
int LuaUnsyncedRead::GetDualViewGeometry(lua_State* L)
{
	lua_pushnumber(L, globalRendering->dualViewSizeX);
	lua_pushnumber(L, globalRendering->dualViewSizeY);
	lua_pushnumber(L, globalRendering->dualViewPosX);
	lua_pushnumber(L, globalRendering->dualViewPosY);
	return 4;
}


/*** Get main window geometry
 *
 * @function Spring.GetWindowGeometry
 *
 * @return number winSizeX in px
 * @return number winSizeY in px
 * @return number winPosX in px
 * @return number winPosY in px
 * @return number windowBorderTop in px
 * @return number windowBorderLeft in px
 * @return number windowBorderBottom in px
 * @return number windowBorderRight in px
 */
int LuaUnsyncedRead::GetWindowGeometry(lua_State* L)
{
	// origin BOTTOMLEFT
	const int winPosY_bl = globalRendering->screenSizeY - globalRendering->winSizeY - globalRendering->winPosY;

	lua_pushnumber(L, globalRendering->winSizeX);
	lua_pushnumber(L, globalRendering->winSizeY);
	lua_pushnumber(L, globalRendering->winPosX);
	lua_pushnumber(L, winPosY_bl);

	lua_pushnumber(L, globalRendering->winBorder[0]);
	lua_pushnumber(L, globalRendering->winBorder[1]);
	lua_pushnumber(L, globalRendering->winBorder[2]);
	lua_pushnumber(L, globalRendering->winBorder[3]);
	return 4 + 4;
}

/*** Get main window display mode
 *
 * @function Spring.GetWindowDisplayMode
 * @return number width in px
 * @return number height in px
 * @return number bits per pixel
 * @return number refresh rate in Hz
 */
int LuaUnsyncedRead::GetWindowDisplayMode(lua_State* L)
{
	SDL_DisplayMode dmode;
	if (!SDL_GetWindowDisplayMode(globalRendering->GetWindow(), &dmode)) {
		lua_pushnumber(L, dmode.w);
		lua_pushnumber(L, dmode.h);
		lua_pushnumber(L, SDL_BITSPERPIXEL(dmode.format));
		lua_pushnumber(L, dmode.refresh_rate);
		lua_pushstring(L, SDL_GetPixelFormatName(dmode.format));
		return 5;
	}
	return 0;
}


/*** Get screen geometry
 *
 * @function Spring.GetScreenGeometry
 *
 * @param displayIndex number? (Default: `-1`)
 * @param queryUsable boolean? (Default: `false`)
 *
 * @return number screenSizeX in px
 * @return number screenSizeY in px
 * @return number screenPosX in px
 * @return number screenPosY in px
 * @return number windowBorderTop in px
 * @return number windowBorderLeft in px
 * @return number windowBorderBottom in px
 * @return number windowBorderRight in px
 * @return number? screenUsableSizeX in px
 * @return number? screenUsableSizeY in px
 * @return number? screenUsablePosX in px
 * @return number? screenUsablePosY in px
 */
int LuaUnsyncedRead::GetScreenGeometry(lua_State* L)
{
	const int displayIndex = luaL_optint(L, 1, -1);
	const int* displayIndexPtr = (displayIndex != -1) ? &displayIndex : nullptr;
	const bool queryUsable = luaL_optboolean(L, 2, false); // whether to query usable screen size

	if (displayIndexPtr == nullptr) {
		lua_pushnumber(L, globalRendering->screenSizeX);
		lua_pushnumber(L, globalRendering->screenSizeY);
		lua_pushnumber(L, globalRendering->screenPosX);
		lua_pushnumber(L, globalRendering->screenPosY);
	}
	else {
		SDL_Rect r{};
		globalRendering->GetDisplayBounds(r, displayIndexPtr);
		lua_pushnumber(L, r.w);
		lua_pushnumber(L, r.h);
		lua_pushnumber(L, r.x);
		lua_pushnumber(L, r.y);
	}

	if (!queryUsable)
		return 4;

	{
		SDL_Rect r{};
		globalRendering->GetUsableDisplayBounds(r, displayIndexPtr);
		lua_pushnumber(L, r.w);
		lua_pushnumber(L, r.h);
		lua_pushnumber(L, r.x);
		lua_pushnumber(L, r.y);

		return 4 + 4;
	}
}


/*** Get minimap geometry
 *
 * @function Spring.GetMiniMapGeometry
 *
 * @return number minimapPosX in px
 * @return number minimapPosY in px
 * @return number minimapSizeX in px
 * @return number minimapSizeY in px
 * @return boolean minimized
 * @return boolean maximized
 */
int LuaUnsyncedRead::GetMiniMapGeometry(lua_State* L)
{
	if (minimap == nullptr)
		return 0;

	lua_pushnumber(L, minimap->GetPosX());
	lua_pushnumber(L, minimap->GetPosY());
	lua_pushnumber(L, minimap->GetSizeX());
	lua_pushnumber(L, minimap->GetSizeY());
	lua_pushboolean(L, minimap->GetMinimized());
	lua_pushboolean(L, minimap->GetMaximized());

	return 6;
}


/*** Get minimap rotation
 *
 * @function Spring.GetMiniMapRotation
 * @return number amount in radians
 */
int LuaUnsyncedRead::GetMiniMapRotation(lua_State* L)
{
	if (minimap == nullptr)
		return 0;

	lua_pushnumber(L, minimap->GetRotation());

	return 1;
}


/***
 * @function Spring.GetMiniMapDualScreen
 * @return "left"|"right"|false position `"left"` or `"right"` when dual screen is enabled, otherwise `false`.
 */
int LuaUnsyncedRead::GetMiniMapDualScreen(lua_State* L)
{
	if (minimap == nullptr)
		return 0;

	if (!globalRendering->dualScreenMode) {
		lua_pushboolean(L, false);
	} else {
		if (globalRendering->dualScreenMiniMapOnLeft) {
			lua_pushliteral(L, "left");
		} else {
			lua_pushliteral(L, "right");
		}
	}
	return 1;
}


/*** Get vertices from currently active selection box
 *
 * @function Spring.GetSelectionBox
 *
 * Returns nil when selection box is inactive
 *
 * @return number? left
 * @return number? top
 * @return number? right
 * @return number? bottom
 *
 * @see Spring.GetUnitsInScreenRectangle
 */
int LuaUnsyncedRead::GetSelectionBox(lua_State* L)
{
	float3 bl, br, tl, tr;
	if (!mouse->GetSelectionBoxVertices(bl, br, tl, tr)) {
		lua_pushnil(L);

		return 1;
	}

	const auto bottomLeft = camera->CalcViewPortCoordinates(bl);
	const auto topRight   = camera->CalcViewPortCoordinates(tr);

	lua_pushnumber(L, bottomLeft.x);
	lua_pushnumber(L, topRight.y);
	lua_pushnumber(L, topRight.x);
	lua_pushnumber(L, bottomLeft.y);

	return 4;
}


/***
 *
 * @function Spring.GetDrawSelectionInfo
 * @return boolean
 */
int LuaUnsyncedRead::GetDrawSelectionInfo(lua_State* L)
{
	lua_pushboolean(L, guihandler ? guihandler->GetDrawSelectionInfo() : 0);
	return 1;
}


/***
 *
 * @function Spring.IsAboveMiniMap
 *
 * @param x number
 * @param y number
 *
 * @return boolean isAbove
 */
int LuaUnsyncedRead::IsAboveMiniMap(lua_State* L)
{
	if (minimap == nullptr)
		return 0;

	if (minimap->GetMinimized() || game->hideInterface)
		return false;

	const int x = luaL_checkint(L, 1) + globalRendering->viewPosX;
	const int y = luaL_checkint(L, 2) + globalRendering->viewPosY;

	const int x0 = minimap->GetPosX();
	const int y0 = minimap->GetPosY();
	const int x1 = x0 + minimap->GetSizeX();
	const int y1 = y0 + minimap->GetSizeY();

	lua_pushboolean(L, (x >= x0) && (x < x1) &&
	                   (y >= y0) && (y < y1));

	return 1;
}


/***
 *
 * @function Spring.GetDrawFrame
 *
 * @return number low_16bit
 * @return number high_16bit
 */
int LuaUnsyncedRead::GetDrawFrame(lua_State* L)
{
	lua_pushnumber(L, globalRendering->drawFrame & 0xFFFF);
	lua_pushnumber(L, (globalRendering->drawFrame >> 16) & 0xFFFF);
	return 2;
}


/***
 *
 * @function Spring.GetFrameTimeOffset
 *
 * Ideally, when running 30hz sim, and 60hz rendering, the draw frames should
 * have and offset of either 0.0 frames, or 0.5 frames.
 *
 * When draw frames are not integer multiples of sim frames, some interpolation
 * happens, and this timeoffset shows how far along it is.
 *
 * @return number? offset of the current draw frame from the last sim frame, expressed in fractions of a frame
 */
int LuaUnsyncedRead::GetFrameTimeOffset(lua_State* L)
{
	lua_pushnumber(L, globalRendering->timeOffset);
	return 1;
}

/*** Gets game time for drawing purposes
 *
 * Returns the game time, taking the interpolated draw frame into account.
 *
 * @return number game time in seconds
 */
int LuaUnsyncedRead::GetGameSecondsInterpolated(lua_State* L)
{
	lua_pushnumber(L, (gs->GetLuaSimFrame() + globalRendering->timeOffset) / GAME_SPEED);
	return 1;
}

/***
 *
 * @function Spring.GetLastUpdateSeconds
 *
 * @return number? lastUpdateSeconds
 */
int LuaUnsyncedRead::GetLastUpdateSeconds(lua_State* L)
{
	lua_pushnumber(L, game->updateDeltaSeconds);
	return 1;
}


/***
 *
 * @function Spring.GetVideoCapturingMode
 *
 * @return boolean allowRecord
 */
int LuaUnsyncedRead::GetVideoCapturingMode(lua_State* L)
{
	lua_pushboolean(L, videoCapturing->AllowRecord());
	return 1;
}


/******************************************************************************
 * Unit attributes
 * @section unitattributes
******************************************************************************/


/***
 *
 * @function Spring.IsUnitAllied
 * @param unitID integer
 * @return boolean? isAllied nil with unitID cannot be parsed
 */
int LuaUnsyncedRead::IsUnitAllied(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		// in this case handle has full-read access since unit != nullptr
		lua_pushboolean(L, unit->allyteam == gu->myAllyTeam);
	} else {
		lua_pushboolean(L, unit->allyteam == CLuaHandle::GetHandleReadAllyTeam(L));
	}

	return 1;
}


/***
 *
 * @function Spring.IsUnitSelected
 * @param unitID integer
 * @return boolean? isSelected nil when unitID cannot be parsed
 */
int LuaUnsyncedRead::IsUnitSelected(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const auto& selUnits = selectedUnitsHandler.selectedUnits;
	lua_pushboolean(L, selUnits.find(unit->id) != selUnits.end());
	return 1;
}


/***
 *
 * @function Spring.GetUnitLuaDraw
 * @param unitID integer
 * @return boolean? draw nil when unitID cannot be parsed
 */
int LuaUnsyncedRead::GetUnitLuaDraw(lua_State* L)
{
	return (GetSolidObjectLuaDraw(L, ParseUnit(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetUnitNoDraw
 * @param unitID integer
 * @return boolean? nil when unitID cannot be parsed
 */
int LuaUnsyncedRead::GetUnitNoDraw(lua_State* L)
{
	return (GetSolidObjectNoDraw(L, ParseUnit(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetUnitEngineDrawMask
 * @param unitID integer
 * @return boolean? nil when unitID cannot be parsed
 */
int LuaUnsyncedRead::GetUnitEngineDrawMask(lua_State* L)
{
	return (GetSolidObjectEngineDrawMask(L, ParseUnit(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetUnitAlwaysUpdateMatrix
 * @param unitID integer
 * @return boolean? nil when unitID cannot be parsed
 */
int LuaUnsyncedRead::GetUnitAlwaysUpdateMatrix(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->alwaysUpdateMat);
	return 1;
}

/***
 *
 * @function Spring.GetUnitDrawFlag
 * @param unitID integer
 * @return number? nil when unitID cannot be parsed
 */
int LuaUnsyncedRead::GetUnitDrawFlag(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushinteger(L, unit->drawFlag);
	return 1;
}

/***
 *
 * @function Spring.GetUnitNoMinimap
 * @param unitID integer
 * @return boolean? nil when unitID cannot be parsed
 */
int LuaUnsyncedRead::GetUnitNoMinimap(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->noMinimap);
	return 1;
}

/***
 * Check if a unit is not allowed to be added to a group by a player.
 *
 * @function Spring.GetUnitNoGroup
 * @param unitID integer
 * @return boolean? noGroup `true` if the unit is not allowed to be added to a group, `false` if it is allowed to be added to a group, or `nil` when `unitID` is not valid.
 */
int LuaUnsyncedRead::GetUnitNoGroup(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->noGroup);
	return 1;
}

/***
 *
 * @function Spring.GetUnitNoSelect
 * @param unitID integer
 * @return boolean? noSelect `nil` when `unitID` cannot be parsed.
 */
int LuaUnsyncedRead::GetUnitNoSelect(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->noSelect);
	return 1;
}


/***
 *
 * @function Spring.UnitIconGetDraw
 * @param unitID integer
 * @return boolean? drawIcon
 * `true` if icon is being drawn, `nil` when unitID is invalid, otherwise `false`.
 */
int LuaUnsyncedRead::UnitIconGetDraw(lua_State* L) {
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->drawIcon);
	return 1;
}


/***
 *
 * @function Spring.GetUnitSelectionVolumeData
 * @param unitID integer
 * @return number? scaleX nil when unitID cannot be parsed
 * @return number scaleY
 * @return number scaleZ
 * @return number offsetX
 * @return number offsetY
 * @return number offsetZ
 * @return number volumeType
 * @return number useContHitTest
 * @return number getPrimaryAxis
 * @return boolean ignoreHits
 */
int LuaUnsyncedRead::GetUnitSelectionVolumeData(lua_State* L)
{
	return GetSolidObjectSelectionVolume(L, ParseUnit(L, __func__, 1));
}


/******************************************************************************
 * Feature attributes
 * @section featureattributes
******************************************************************************/


/***
 *
 * @function Spring.GetFeatureLuaDraw
 * @param featureID integer
 * @return boolean? nil when featureID cannot be parsed
 */
int LuaUnsyncedRead::GetFeatureLuaDraw(lua_State* L)
{
	return (GetSolidObjectLuaDraw(L, ParseFeature(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetFeatureNoDraw
 * @param featureID integer
 * @return boolean? nil when featureID cannot be parsed
 */
int LuaUnsyncedRead::GetFeatureNoDraw(lua_State* L)
{
	return (GetSolidObjectNoDraw(L, ParseFeature(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetFeatureEngineDrawMask
 * @param featureID integer
 * @return boolean? nil when featureID cannot be parsed
 */
int LuaUnsyncedRead::GetFeatureEngineDrawMask(lua_State* L)
{
	return (GetSolidObjectEngineDrawMask(L, ParseFeature(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetFeatureAlwaysUpdateMatrix
 * @param featureID integer
 * @return boolean? nil when featureID cannot be parsed
 */
int LuaUnsyncedRead::GetFeatureAlwaysUpdateMatrix(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	lua_pushboolean(L, feature->alwaysUpdateMat);
	return 1;
}

/***
 *
 * @function Spring.GetFeatureDrawFlag
 * @param featureID integer
 * @return number? nil when featureID cannot be parsed
 */
int LuaUnsyncedRead::GetFeatureDrawFlag(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	lua_pushinteger(L, feature->drawFlag);
	return 1;
}

/***
 *
 * @function Spring.GetFeatureSelectionVolumeData
 * @param featureID integer
 * @return number? scaleX nil when unitID cannot be parsed
 * @return number scaleY
 * @return number scaleZ
 * @return number offsetX
 * @return number offsetY
 * @return number offsetZ
 * @return number volumeType
 * @return number useContHitTest
 * @return number getPrimaryAxis
 * @return boolean ignoreHits
 */
int LuaUnsyncedRead::GetFeatureSelectionVolumeData(lua_State* L)
{
	return GetSolidObjectSelectionVolume(L, ParseFeature(L, __func__, 1));
}



static int GetObjectTransformMatrix(const CSolidObject* o, lua_State* L)
{
	if (o == nullptr)
		return 0;

	CMatrix44f m = o->GetTransformMatrix(false);

	if (luaL_optboolean(L, 2, false))
		m = m.InvertAffine();

	for (int i = 0; i < 16; i += 4) {
		lua_pushnumber(L, m[i + 0]);
		lua_pushnumber(L, m[i + 1]);
		lua_pushnumber(L, m[i + 2]);
		lua_pushnumber(L, m[i + 3]);
	}

	return 16;
}


/******************************************************************************
 * Misc
 * @section misc
******************************************************************************/


/***
 *
 * @function Spring.GetUnitTransformMatrix
 * @param unitID integer
 * @return number? m11 nil when unitID cannot be parsed
 * @return number m12
 * @return number m13
 * @return number m14
 * @return number m21
 * @return number m22
 * @return number m23
 * @return number m24
 * @return number m31
 * @return number m32
 * @return number m33
 * @return number m34
 * @return number m41
 * @return number m42
 * @return number m43
 * @return number m44
 */
int LuaUnsyncedRead::GetUnitTransformMatrix(lua_State* L) { return (GetObjectTransformMatrix(ParseUnit(L, __func__, 1), L)); }


/***
 *
 * @function Spring.GetFeatureTransformMatrix
 * @param featureID integer
 * @return number? m11 nil when featureID cannot be parsed
 * @return number m12
 * @return number m13
 * @return number m14
 * @return number m21
 * @return number m22
 * @return number m23
 * @return number m24
 * @return number m31
 * @return number m32
 * @return number m33
 * @return number m34
 * @return number m41
 * @return number m42
 * @return number m43
 * @return number m44
 */
int LuaUnsyncedRead::GetFeatureTransformMatrix(lua_State* L) { return (GetObjectTransformMatrix(ParseFeature(L, __func__, 1), L)); }


/******************************************************************************
 * Inview
 * @section inview
******************************************************************************/


/***
 *
 * @function Spring.IsUnitInView
 * @param unitID integer
 * @return boolean? inView nil when unitID cannot be parsed
 */
int LuaUnsyncedRead::IsUnitInView(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, camera->InView(unit->midPos, unit->radius));
	return 1;
}


/***
 *
 * @function Spring.IsUnitVisible
 * @param unitID integer
 * @param radius number? unitRadius when not specified
 * @param checkIcon boolean
 * @return boolean? isVisible nil when unitID cannot be parsed
 */
int LuaUnsyncedRead::IsUnitVisible(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const float radius = luaL_optnumber(L, 2, unit->radius);
	const bool checkIcon = lua_toboolean(L, 3);

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0) {
		if (!CLuaHandle::GetHandleFullRead(L)) {
			lua_pushboolean(L, false);
		} else {
			lua_pushboolean(L,
				(!checkIcon || !unit->GetIsIcon()) &&
				camera->InView(unit->midPos, radius));
		}
	}
	else {
		if ((unit->losStatus[readAllyTeam] & LOS_INLOS) == 0) {
			lua_pushboolean(L, false);
		} else {
			lua_pushboolean(L,
				(!checkIcon || !unit->GetIsIcon()) &&
				camera->InView(unit->midPos, radius));
		}
	}
	return 1;
}


/***
 *
 * @function Spring.IsUnitIcon
 * @param unitID integer
 * @return boolean? isUnitIcon nil when unitID cannot be parsed
 */
int LuaUnsyncedRead::IsUnitIcon(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->GetIsIcon());
	return 1;
}


/***
 *
 * @function Spring.IsAABBInView
 * @param minX number
 * @param minY number
 * @param minZ number
 * @param maxX number
 * @param maxY number
 * @param maxZ number
 * @return boolean inView
 */
int LuaUnsyncedRead::IsAABBInView(lua_State* L)
{
	float3 mins = float3(luaL_checkfloat(L, 1),
	                     luaL_checkfloat(L, 2),
	                     luaL_checkfloat(L, 3));
	float3 maxs = float3(luaL_checkfloat(L, 4),
	                     luaL_checkfloat(L, 5),
	                     luaL_checkfloat(L, 6));

	if (mins.x > maxs.x) std::swap(mins.x, maxs.x);
	if (mins.y > maxs.y) std::swap(mins.y, maxs.y);
	if (mins.z > maxs.z) std::swap(mins.z, maxs.z);

	lua_pushboolean(L, camera->InView(mins, maxs));
	return 1;
}


/***
 *
 * @function Spring.IsSphereInView
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param radius number? (Default: `0`)
 * @return boolean inView
 */
int LuaUnsyncedRead::IsSphereInView(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));
	const float radius = lua_israwnumber(L, 4) ? lua_tofloat(L, 4) : 0.0f;

	lua_pushboolean(L, camera->InView(pos, radius));
	return 1;
}


/***
 *
 * @function Spring.GetUnitViewPosition
 * @param unitID integer
 * @param midPos boolean? (Default: `false`)
 * @return number? x nil when unitID cannot be parsed
 * @return number y
 * @return number z
 */
int LuaUnsyncedRead::GetUnitViewPosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const float3 unitPos = (luaL_optboolean(L, 2, false)) ? unit->GetObjDrawMidPos() : unit->drawPos;
	const float3 errorVec = unit->GetLuaErrorVector(CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleFullRead(L));

	lua_pushnumber(L, unitPos.x + errorVec.x);
	lua_pushnumber(L, unitPos.y + errorVec.y);
	lua_pushnumber(L, unitPos.z + errorVec.z);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

// never instantiated directly
template<class T> class CWorldObjectQuadDrawer: public CReadMap::IQuadDrawer {
public:
	typedef std::vector<T*> ObjectList;
	typedef std::vector< const ObjectList* > ObjectVector;

	void ResetState() override {
		objectLists.clear();
		objectLists.reserve(64);

		objectCount = 0;
	}

	unsigned int GetQuadCount() const { return (objectLists.size()); }
	unsigned int GetObjectCount() const { return objectCount; }

	const ObjectVector& GetObjectLists() { return objectLists; }

	void AddObjectList(const ObjectList* objects) {
		if (objects->empty())
			return;

		objectLists.push_back(objects);
		objectCount += objects->size();
	}

protected:
	// note: stores pointers to lists, not copies
	// its size equals the number of visible quads
	ObjectVector objectLists;

	unsigned int objectCount;
};


class CVisUnitQuadDrawer: public CWorldObjectQuadDrawer<CUnit> {
public:
	void DrawQuad(int x, int y) override {
		const CQuadField::Quad& q = quadField.GetQuadAt(x, y);
		const ObjectList* o = &q.units;

		AddObjectList(o);
	}
};

class CVisFeatureQuadDrawer: public CWorldObjectQuadDrawer<CFeature> {
public:
	void DrawQuad(int x, int y) override {
		const CQuadField::Quad& q = quadField.GetQuadAt(x, y);
		const ObjectList* o = &q.features;

		AddObjectList(o);
	}
};

class CVisProjectileQuadDrawer: public CWorldObjectQuadDrawer<CProjectile> {
public:
	void DrawQuad(int x, int y) override {
		const CQuadField::Quad& q = quadField.GetQuadAt(x, y);
		const ObjectList* o = &q.projectiles;

		AddObjectList(o);
	}
};


/***
 *
 * @function Spring.GetVisibleUnits
 * @param teamID integer? (Default: `-1`)
 * @param radius number? (Default: `30`)
 * @param icons boolean? (Default: `true`)
 * @return nil|number[] unitIDs
 */
int LuaUnsyncedRead::GetVisibleUnits(lua_State* L)
{
	// arg 1 - teamID
	int teamID = luaL_optint(L, 1, -1);
	int allyTeamID = CLuaHandle::GetHandleReadAllyTeam(L);

	if (teamID == LuaUtils::MyUnits) {
		const int scriptTeamID = CLuaHandle::GetHandleReadTeam(L);

		if (scriptTeamID >= 0) {
			teamID = scriptTeamID;
		} else {
			teamID = LuaUtils::AllUnits;
		}
	}

	if (teamID >= 0) {
		if (!teamHandler.IsValidTeam(teamID))
			return 0;

		allyTeamID = teamHandler.AllyTeam(teamID);
	}
	if (allyTeamID < 0) {
		if (!CLuaHandle::GetHandleFullRead(L)) {
			return 0;
		}
	}

	// arg 3 - noIcons
	const bool noIcons = !luaL_optboolean(L, 3, true);

	float radiusMult = 1.0f;
	float testRadius = 0.0f;

	// arg 2 - use fixed test-value or add unit radii to it
	if (lua_israwnumber(L, 2)) {
		radiusMult = float((testRadius = lua_tofloat(L, 2)) >= 0.0f);
		testRadius = std::max(testRadius, -testRadius);
	}

	static CVisUnitQuadDrawer unitQuadIter;

	unitQuadIter.ResetState();
	readMap->GridVisibility(nullptr, &unitQuadIter, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);

	// Even though we're in unsynced it's ok to use gs->tempNum since its exact value
	// doesn't matter
	const int tempNum = gs->GetTempNum();
	lua_createtable(L, unitQuadIter.GetObjectCount(), 0);

	unsigned int count = 0;
	for (auto visUnitList: unitQuadIter.GetObjectLists()) {
		for (CUnit* u: *visUnitList) {
			if (u->tempNum == tempNum)
				continue;

			u->tempNum = tempNum;

			if (u->noDraw)
				continue;

			if (allyTeamID >= 0 && !(u->losStatus[allyTeamID] & LOS_INLOS))
				continue;

			if (noIcons && u->GetIsIcon())
				continue;

			if ((teamID == LuaUtils::AllyUnits)  && (allyTeamID != u->allyteam))
				continue;

			if ((teamID == LuaUtils::EnemyUnits) && (allyTeamID == u->allyteam))
				continue;

			if ((teamID >= 0) && (teamID != u->team))
				continue;

			//No check for AllUnits, since there's no need.

			if (!camera->InView(u->drawMidPos, testRadius + (u->GetDrawRadius() * radiusMult)))
				continue;

			lua_pushnumber(L, u->id);
			lua_rawseti(L, -2, ++count);
		}
	}

	return 1;
}


/***
 *
 * @function Spring.GetVisibleFeatures
 * @param teamID integer? (Default: `-1`)
 * @param radius number? (Default: `30`)
 * @param icons boolean? (Default: `true`)
 * @param geos boolean? (Default: `true`)
 * @return nil|number[] featureIDs
 */
int LuaUnsyncedRead::GetVisibleFeatures(lua_State* L)
{
	// arg 1 - allyTeamID
	int allyTeamID = luaL_optint(L, 1, -1);

	if (allyTeamID >= 0) {
		if (!teamHandler.ValidAllyTeam(allyTeamID)) {
			return 0;
		}
	} else {
		allyTeamID = -1;

		if (!CLuaHandle::GetHandleFullRead(L)) {
			allyTeamID = CLuaHandle::GetHandleReadAllyTeam(L);
		}
	}

	const bool noIcons = !luaL_optboolean(L, 3, true);
	const bool noGeos = !luaL_optboolean(L, 4, true);

	float radiusMult = 0.0f; // 0 or 1
	float testRadius = 0.0f;

	// arg 2 - use fixed test-value or add feature radii to it
	if (lua_israwnumber(L, 2)) {
		radiusMult = float((testRadius = lua_tofloat(L, 2)) >= 0.0f);
		testRadius = std::max(testRadius, -testRadius);
	}

	static CVisFeatureQuadDrawer featureQuadIter;

	featureQuadIter.ResetState();
	readMap->GridVisibility(nullptr, &featureQuadIter, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);

	// Even though we're in unsynced it's ok to use gs->tempNum since its exact value
	// doesn't matter
	const int tempNum = gs->GetTempNum();
	lua_createtable(L, featureQuadIter.GetObjectCount(), 0);

	unsigned int count = 0;
	for (auto visFeatureList: featureQuadIter.GetObjectLists()) {
		for (CFeature* f: *visFeatureList) {
			if (f->tempNum == tempNum)
				continue;

			f->tempNum = tempNum;

			if (f->noDraw)
				continue;

			if (noIcons && f->drawFlag == DrawFlags::SO_DRICON_FLAG)
				continue;

			if (noGeos && f->def->geoThermal)
				continue;

			if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(allyTeamID))
				continue;

			if (!camera->InView(f->drawMidPos, testRadius + (f->GetDrawRadius() * radiusMult)))
				continue;

			lua_pushnumber(L, f->id);
			lua_rawseti(L, -2, ++count);
		}
	}


	return 1;
}


/***
 *
 * @function Spring.GetVisibleProjectiles
 * @param allyTeamID integer? (Default: `-1`)
 * @param addSyncedProjectiles boolean? (Default: `true`)
 * @param addWeaponProjectiles boolean? (Default: `true`)
 * @param addPieceProjectiles boolean? (Default: `true`)
 * @return nil|number[] projectileIDs
 */
int LuaUnsyncedRead::GetVisibleProjectiles(lua_State* L)
{
	int allyTeamID = luaL_optint(L, 1, -1);

	if (allyTeamID >= 0) {
		if (!teamHandler.ValidAllyTeam(allyTeamID)) {
			return 0;
		}
	} else {
		allyTeamID = -1;

		if (!CLuaHandle::GetHandleFullRead(L)) {
			allyTeamID = CLuaHandle::GetHandleReadAllyTeam(L);
		}
	}

	static CVisProjectileQuadDrawer projQuadIter;

	/*const bool addSyncedProjectiles =*/ luaL_optboolean(L, 2, true);
	const bool addWeaponProjectiles = luaL_optboolean(L, 3, true);
	const bool addPieceProjectiles = luaL_optboolean(L, 4, true);


	projQuadIter.ResetState();
	readMap->GridVisibility(nullptr, &projQuadIter, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);

	// Even though we're in unsynced it's ok to use gs->tempNum since its exact value
	// doesn't matter
	const int tempNum = gs->GetTempNum();
	lua_createtable(L, projQuadIter.GetObjectCount(), 0);

	unsigned int count = 0;
	for (auto visProjectileList: projQuadIter.GetObjectLists()) {
		for (CProjectile* p: *visProjectileList) {
			if (p->tempNum == tempNum)
				continue;

			p->tempNum = tempNum;


			if (allyTeamID >= 0 && !losHandler->InLos(p, allyTeamID))
				continue;

			if (!camera->InView(p->pos, p->GetDrawRadius()))
				continue;

			#if 1
			// filter out unsynced projectiles, the SyncedRead
			// projecile Get* functions accept only synced ID's
			// (specifically they interpret all ID's as synced)
			if (!p->synced)
				continue;
			#else
			if (!addSyncedProjectiles && p->synced)
				continue;
			#endif
			if (!addWeaponProjectiles && p->weapon)
				continue;
			if (!addPieceProjectiles && p->piece)
				continue;

			lua_pushnumber(L, p->id);
			lua_rawseti(L, -2, ++count);
		}
	}

	return 1;
}

namespace {
	template<typename V>
	static int GetRenderObjects(lua_State* L, const V& renderObjects, const char* func) {
		const int  drawMask = luaL_optint(L, 1, 0);
		const bool sendMask = luaL_optboolean(L, 2, false);

		lua_createtable(L, renderObjects.size(), 0);
		uint32_t count = 0;
		for (const auto renderObject : renderObjects)
		{
			if ((renderObject->drawFlag & drawMask) == 0)
				continue;

			lua_pushnumber(L, renderObject->id);
			lua_rawseti(L, -2, ++count);
		}

		if 	(!sendMask)
			return 1;

		lua_createtable(L, count, 0);
		count = 0;
		for (const auto renderObject : renderObjects)
		{
			if ((renderObject->drawFlag & drawMask) == 0)
				continue;

			lua_pushnumber(L, renderObject->drawFlag);
			lua_rawseti(L, -2, ++count);
		}

		return 2;
	}

	template<typename V>
	static int GetRenderObjectsDrawFlagChanged(lua_State* L, const V& renderObjects, const char* func) {
		const bool sendMask = luaL_optboolean(L, 1, false);

		std::vector<int> changedIds;
		changedIds.reserve(renderObjects.size());

		std::vector<uint8_t> changedDrawFlags;
		changedDrawFlags.reserve(renderObjects.size());

		for (const auto renderObject : renderObjects)
		{
			if (renderObject->previousDrawFlag == renderObject->drawFlag)
				continue;
			changedIds.push_back(renderObject->id);
			changedDrawFlags.push_back(renderObject->drawFlag);
		}

		lua_createtable(L, changedIds.size(), 0);
		uint32_t count = 0;
		for (const auto id : changedIds)
		{
			lua_pushnumber(L, id);
			lua_rawseti(L, -2, ++count);
		}

		if 	(!sendMask)
			return 1;

		lua_createtable(L, changedDrawFlags.size(), 0);
		count = 0;
		for (const auto drawFlag : changedDrawFlags)
		{
			lua_pushnumber(L, drawFlag);
			lua_rawseti(L, -2, ++count);
		}

		return 2;
	}
}


/***
 *
 * @function Spring.GetRenderUnits
 */
int LuaUnsyncedRead::GetRenderUnits(lua_State* L)
{
	return GetRenderObjects(L, unitDrawer->GetUnsortedUnits(), __func__);
}

/***
 *
 * @function Spring.GetRenderUnitsDrawFlagChanged
 */
int LuaUnsyncedRead::GetRenderUnitsDrawFlagChanged(lua_State* L)
{
	return GetRenderObjectsDrawFlagChanged(L, unitDrawer->GetUnsortedUnits(), __func__);
}

/***
 *
 * @function Spring.GetRenderFeatures
 */
int LuaUnsyncedRead::GetRenderFeatures(lua_State* L)
{
	return GetRenderObjects(L, featureDrawer->GetUnsortedFeatures(), __func__);
}

/***
 *
 * @function Spring.GetRenderFeaturesDrawFlagChanged
 */
int LuaUnsyncedRead::GetRenderFeaturesDrawFlagChanged(lua_State* L)
{
	return GetRenderObjectsDrawFlagChanged(L, featureDrawer->GetUnsortedFeatures(), __func__);
}

/***
 *
 * @function Spring.ClearUnitsPreviousDrawFlag
 * @return nil
 */
int LuaUnsyncedRead::ClearUnitsPreviousDrawFlag(lua_State* L)
{
	unitDrawer->ClearPreviousDrawFlags();
	return 0;
}

/***
 *
 * @function Spring.ClearFeaturesPreviousDrawFlag
 * @return nil
 */
int LuaUnsyncedRead::ClearFeaturesPreviousDrawFlag(lua_State* L)
{
	featureDrawer->ClearPreviousDrawFlags();
	return 0;
}

/*** Get units inside a rectangle area on the map
 *
 * @function Spring.GetUnitsInScreenRectangle
 * @param left number
 * @param top number
 * @param right number
 * @param bottom number
 * @param allegiance number? (Default: `-1`) teamID when > 0, when < 0 one of AllUnits = -1, MyUnits = -2, AllyUnits = -3, EnemyUnits = -4
 * @return nil|number[] unitIDs
 */
int LuaUnsyncedRead::GetUnitsInScreenRectangle(lua_State* L)
{
	float l = luaL_checkfloat(L, 1);
	float t = luaL_checkfloat(L, 2);
	float r = luaL_checkfloat(L, 3);
	float b = luaL_checkfloat(L, 4);

	if (l > r) std::swap(l, r);
	if (t > b) std::swap(t, b);

	static CVisUnitQuadDrawer unitQuadIter;

	unitQuadIter.ResetState();
	readMap->GridVisibility(nullptr, &unitQuadIter, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);

	const int readTeam = CLuaHandle::GetHandleReadTeam(L);
	const int readATeam = CLuaHandle::GetHandleReadAllyTeam(L);

	const int allegiance = LuaUtils::ParseAllegiance(L, __func__, 5);

	std::function<bool(const CUnit*)> disqualifierFunc;

	switch (allegiance)
	{
	case LuaUtils::AllUnits:
		disqualifierFunc = [L](const CUnit* unit) -> bool { return !LuaUtils::IsUnitVisible(L, unit); };
		break;
	case LuaUtils::MyUnits:
		disqualifierFunc = [readTeam](const CUnit* unit) -> bool { return unit->team != readTeam; };
		break;
	case LuaUtils::AllyUnits:
		disqualifierFunc = [readATeam](const CUnit* unit) -> bool { return unit->allyteam != readATeam; };
		break;
	case LuaUtils::EnemyUnits:
		disqualifierFunc = [readATeam](const CUnit* unit) -> bool { return unit->allyteam == readATeam; };
		break;
	default: {
		if (LuaUtils::IsAlliedTeam(L, allegiance)) {
			disqualifierFunc = [allegiance](const CUnit* unit) -> bool { return unit->team != allegiance; };
		}
		else {
			disqualifierFunc = [allegiance, L](const CUnit* unit) -> bool {
				if (unit->team != allegiance)
					return true;

				if (!LuaUtils::IsUnitVisible(L, unit))
					return true;

				return false;
			};
		}
	} break;
	}

	// Even though we're in unsynced it's ok to use gs->tempNum since its exact value
	// doesn't matter
	const int tempNum = gs->GetTempNum();
	lua_createtable(L, unitQuadIter.GetObjectCount(), 0);

	uint32_t count = 0;
	for (auto visUnitList : unitQuadIter.GetObjectLists()) {
		for (CUnit* unit : *visUnitList) {
			if (disqualifierFunc(unit))
				continue;

			if (unit->tempNum == tempNum)
				continue;

			unit->tempNum = tempNum;

			const float3 vpPos = camera->CalcViewPortCoordinates(unit->drawPos);

			if (vpPos.x > r || vpPos.x < l)
				continue;

			if (vpPos.y > b || vpPos.y < t)
				continue;

			if (vpPos.z > 1.0f || vpPos.z < 0.0f)
				continue;

			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, ++count);
		}
	}

	return 1;
}

/*** Get features inside a rectangle area on the map
	*
	* @function Spring.GetFeaturesInScreenRectangle
	* @param left number
	* @param top number
	* @param right number
	* @param bottom number
	* @return nil|number[] featureIDs
	*/
int LuaUnsyncedRead::GetFeaturesInScreenRectangle(lua_State* L)
{
	float l = luaL_checkfloat(L, 1);
	float t = luaL_checkfloat(L, 2);
	float r = luaL_checkfloat(L, 3);
	float b = luaL_checkfloat(L, 4);

	if (l > r) std::swap(l, r);
	if (t > b) std::swap(t, b);

	static CVisFeatureQuadDrawer featureQuadIter;

	featureQuadIter.ResetState();
	readMap->GridVisibility(nullptr, &featureQuadIter, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);

	const int tempNum = gs->GetTempNum();
	lua_createtable(L, featureQuadIter.GetObjectCount(), 0);

	uint32_t count = 0;
	for (auto visFeatureList : featureQuadIter.GetObjectLists()) {
		for ( CFeature* feature : *visFeatureList ) {
			if (feature->tempNum == tempNum)
				continue;

			feature->tempNum = tempNum;
			const float3 vpPos = camera->CalcViewPortCoordinates(feature->drawPos);

			if (vpPos.x > r || vpPos.x < l)
				continue;

			if (vpPos.y > b || vpPos.y < t)
				continue;

			if (vpPos.z > 1.0f || vpPos.z < 0.0f)
				continue;

			lua_pushnumber(L, feature->id);
			lua_rawseti(L, -2, ++count);
		}
	}

	return 1;
}

/******************************************************************************/
/******************************************************************************/

/***
 *
 * @function Spring.GetLocalPlayerID
 * @return integer playerID
 */
int LuaUnsyncedRead::GetLocalPlayerID(lua_State* L)
{
	lua_pushnumber(L, gu->myPlayerNum);
	return 1;
}


/***
 *
 * @function Spring.GetLocalTeamID
 * @return integer teamID
 */
int LuaUnsyncedRead::GetLocalTeamID(lua_State* L)
{
	lua_pushnumber(L, gu->myTeam);
	return 1;
}


/***
 *
 * @function Spring.GetLocalAllyTeamID
 * @return integer allyTeamID
 */
int LuaUnsyncedRead::GetLocalAllyTeamID(lua_State* L)
{
	lua_pushnumber(L, gu->myAllyTeam);
	return 1;
}


/***
 *
 * @function Spring.GetSpectatingState
 * @return boolean spectating
 * @return boolean spectatingFullView
 * @return boolean spectatingFullSelect
 */
int LuaUnsyncedRead::GetSpectatingState(lua_State* L)
{
	lua_pushboolean(L, gu->spectating);
	lua_pushboolean(L, gu->spectatingFullView);
	lua_pushboolean(L, gu->spectatingFullSelect);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

/***
 *
 * @function Spring.GetSelectedUnits
 * @return number[] unitIDs
 */
int LuaUnsyncedRead::GetSelectedUnits(lua_State* L)
{
	PushNumberContainerAsArray(L, selectedUnitsHandler.selectedUnits);
	return 1;
}

/*** Get selected units aggregated by unitDefID
 *
 * @function Spring.GetSelectedUnitsSorted
 * @return table<number,number[]> where keys are unitDefIDs and values are unitIDs
 * @return integer the number of unitDefIDs
 */
int LuaUnsyncedRead::GetSelectedUnitsSorted(lua_State* L)
{
	const auto numDefKeys = PushUnitListSortedByDef(L, selectedUnitsHandler.selectedUnits);
	lua_pushnumber(L, numDefKeys);

	return 2;
}


/*** Get an aggregate count of selected units per unitDefID
 *
 * @function Spring.GetSelectedUnitsCounts
 *
 * @return table<number,number> unitsCounts where keys are unitDefIDs and values are counts
 * @return integer the number of unitDefIDs
 */
int LuaUnsyncedRead::GetSelectedUnitsCounts(lua_State* L)
{
	const auto numDefKeys = PushSparseUnitTallyByDef(L, selectedUnitsHandler.selectedUnits);
	lua_pushnumber(L, numDefKeys);

	return 2;
}


/*** Returns the amount of selected units
 *
 * @function Spring.GetSelectedUnitsCount
 * @return number selectedUnitsCount
 */
int LuaUnsyncedRead::GetSelectedUnitsCount(lua_State* L)
{
	lua_pushnumber(L, selectedUnitsHandler.selectedUnits.size());
	return 1;
}

/*** Get if selection box is handled by engine.
 *
 * @function Spring.GetBoxSelectionByEngine
 *
 * @return boolean isHandledByEngine `true` if the engine will select units inside selection box on release, otherwise `false`.
 *
 * @see Spring.GetSelectionBox
 * @see Spring.SetBoxSelectionByEngine
 */
int LuaUnsyncedRead::GetBoxSelectionByEngine(lua_State* L)
{
	lua_pushboolean(L, selectedUnitsHandler.GetBoxSelectionHandledByEngine());
	return 1;
}


/******************************************************************************/
/******************************************************************************/

/***
 *
 * @function Spring.IsGUIHidden
 * @return boolean
 */
int LuaUnsyncedRead::IsGUIHidden(lua_State* L)
{
	lua_pushboolean(L, game != nullptr && game->hideInterface);
	return 1;
}


/***
 *
 * @function Spring.HaveShadows
 * @return boolean shadowsLoaded
 */
int LuaUnsyncedRead::HaveShadows(lua_State* L)
{
	lua_pushboolean(L, shadowHandler.ShadowsLoaded());
	return 1;
}


/***
 *
 * @function Spring.HaveAdvShading
 * @return boolean useAdvShading
 * @return boolean groundUseAdvShading
 */
int LuaUnsyncedRead::HaveAdvShading(lua_State* L)
{
	lua_pushboolean(L, true);
	lua_pushboolean(L, readMap->GetGroundDrawer()->UseAdvShading());
	return 2;
}


/***
 *
 * @function Spring.GetWaterMode
 * @return integer waterRendererID
 * @return string waterRendererName
 * @see rts/Rendering/Env/IWater.h
 */
int LuaUnsyncedRead::GetWaterMode(lua_State* L)
{
	const auto& water = IWater::GetWater();
	lua_pushnumber(L, water->GetID());
	lua_pushstring(L, IWater::GetWaterName(water->GetID()));
	return 2;
}


/***
 *
 * @function Spring.GetMapDrawMode
 * @return "normal"|"height"|"metal"|"pathTraversability"|"los"
 */
int LuaUnsyncedRead::GetMapDrawMode(lua_State* L)
{
	using P = std::pair<const char*, const char*>;
	constexpr std::array<P, 5> modes = {
		P(        "", "normal"            ),
		P(    "path", "pathTraversability"),
		P(    "heat", "pathHeat"          ),
		P(    "flow", "pathFlow"          ),
		P("pathcost", "pathCost"          ),
	};

	const auto& mode = infoTextureHandler->GetMode();
	const auto  iter = std::find_if(modes.begin(), modes.end(), [&mode](const P& p) { return (strcmp(p.first, mode.c_str()) == 0); });

	if (iter != modes.end()) {
		lua_pushstring(L, iter->second);
	} else {
		lua_pushstring(L, mode.c_str());
	}

	return 1;
}


/***
 *
 * @function Spring.GetMapSquareTexture
 * @param texSquareX number
 * @param texSquareY number
 * @param lodMin number
 * @param luaTexName string
 * @param lodMax number? (Default: lodMin)
 * @return boolean? success
 */
int LuaUnsyncedRead::GetMapSquareTexture(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L)) {
		return 0;
	}

	const int texSquareX = luaL_checkint(L, 1);
	const int texSquareY = luaL_checkint(L, 2);
	const int lodMin = luaL_checkint(L, 3);
	const std::string& texName = luaL_checkstring(L, 4);
	const int lodMax = luaL_optinteger(L, 5, lodMin); // should have been in pos 4 instead, but the compatibility is the king

	CBaseGroundDrawer* groundDrawer = readMap->GetGroundDrawer();
	CBaseGroundTextures* groundTextures = groundDrawer->GetGroundTextures();

	if (groundTextures == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}
	if (texName.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	const LuaTextures& luaTextures = CLuaHandle::GetActiveTextures(L);
	const LuaTextures::Texture* luaTexture = luaTextures.GetInfo(texName);

	if (luaTexture == nullptr) {
		// not a valid texture (name)
		lua_pushboolean(L, false);
		return 1;
	}

	const int tid = luaTexture->id;
	const int txs = luaTexture->xsize;
	const int tys = luaTexture->ysize;

	if (txs != tys) {
		// square textures only
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, groundTextures->GetSquareLuaTexture(texSquareX, texSquareY, tid, txs, tys, lodMin, lodMax));
	return 1;
}

/***
 * @function Spring.GetLosViewColors
 * @return rgb always
 * @return rgb LOS
 * @return rgb radar
 * @return rgb jam
 * @return rgb radar2
 */
int LuaUnsyncedRead::GetLosViewColors(lua_State* L)
{
#define PACK_COLOR_VECTOR(color) \
	lua_createtable(L, 3, 0); \
	lua_pushnumber(L, color[0] / scale); lua_rawseti(L, -2, 1); \
	lua_pushnumber(L, color[1] / scale); lua_rawseti(L, -2, 2); \
	lua_pushnumber(L, color[2] / scale); lua_rawseti(L, -2, 3);

	const float scale = (float)CBaseGroundDrawer::losColorScale;
	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	PACK_COLOR_VECTOR(gd->alwaysColor);
	PACK_COLOR_VECTOR(gd->losColor);
	PACK_COLOR_VECTOR(gd->radarColor);
	PACK_COLOR_VECTOR(gd->jamColor);
	PACK_COLOR_VECTOR(gd->radarColor2);
	return 5;
}


/***
 *
 * @function Spring.GetNanoProjectileParams
 * @return number rotVal in degrees
 * @return number rotVel in degrees
 * @return number rotAcc in degrees
 * @return number rotValRng in degrees
 * @return number rotVelRng in degrees
 * @return number rotAccRng in degrees
 */
int LuaUnsyncedRead::GetNanoProjectileParams(lua_State* L)
{
	lua_pushnumber(L, CNanoProjectile::rotVal0 * (math::RAD_TO_DEG                            ));
	lua_pushnumber(L, CNanoProjectile::rotVel0 * (math::RAD_TO_DEG * GAME_SPEED               ));
	lua_pushnumber(L, CNanoProjectile::rotAcc0 * (math::RAD_TO_DEG * (GAME_SPEED * GAME_SPEED)));

	lua_pushnumber(L, CNanoProjectile::rotValRng0 * (math::RAD_TO_DEG                            ));
	lua_pushnumber(L, CNanoProjectile::rotVelRng0 * (math::RAD_TO_DEG * GAME_SPEED               ));
	lua_pushnumber(L, CNanoProjectile::rotAccRng0 * (math::RAD_TO_DEG * (GAME_SPEED * GAME_SPEED)));

	return 6;
}


/***
 * Get available cameras.
 *
 * @function Spring.GetCameraNames
 * @return table<string, integer> indexByName Table where where keys are names and values are indices.
 */
int LuaUnsyncedRead::GetCameraNames(lua_State* L)
{
	const std::array<CCameraController*, CCameraHandler::CAMERA_MODE_LAST>& cc = camHandler->GetControllers();

	lua_createtable(L, 0, cc.size());

	for (size_t i = 0; i < cc.size(); ++i) {
		lua_pushsstring(L, cc[i]->GetName()); // key
		lua_pushnumber(L, i); // val
		lua_rawset(L, -3);
	}

	return 1;
}

/***
 * @function Spring.GetCameraState
 * @param useReturns false
 * @return CameraState cameraState
 */
/***
 * @function Spring.GetCameraState
 * @param useReturns true? (Default: `true`) Return multiple values instead of a table.
 * @return CameraName name
 * @return any Fields depending on current controller mode.
 */
int LuaUnsyncedRead::GetCameraState(lua_State* L)
{
	CCameraController::StateMap camState;
	camHandler->GetState(camState);

	if (!luaL_optboolean(L, 1, true)) {
		// table-less version; pushes just the name and the cam-specific values
		// use GetCamera{Position,Direction,FOV} for the common fields from the base class
		lua_pushsstring(L, camHandler->GetCurrentController().GetName());

		switch (camHandler->GetCurrentControllerNum()) {
		case CCameraHandler::CAMERA_MODE_FIRSTPERSON:
		case CCameraHandler::CAMERA_MODE_ROTOVERHEAD: // happens to have the same set of values as FPS
			lua_pushnumber(L, camState["oldHeight"]);
			return 1 + 1;

		case CCameraHandler::CAMERA_MODE_OVERHEAD:
			lua_pushnumber(L, camState["height"]);
			lua_pushnumber(L, camState["angle"]);
			lua_pushnumber(L, camState["flipped"]);
			return 1 + 3;

		case CCameraHandler::CAMERA_MODE_SPRING:
			lua_pushnumber(L, camState["rx"]);
			lua_pushnumber(L, camState["ry"]);
			lua_pushnumber(L, camState["rz"]);
			lua_pushnumber(L, camState["dist"]);
			return 1 + 4;

		case CCameraHandler::CAMERA_MODE_FREE:
			lua_pushnumber(L, camState["rx"]);
			lua_pushnumber(L, camState["ry"]);
			lua_pushnumber(L, camState["rz"]);
			lua_pushnumber(L, camState["vx"]);
			lua_pushnumber(L, camState["vy"]);
			lua_pushnumber(L, camState["vz"]);
			lua_pushnumber(L, camState["avx"]);
			lua_pushnumber(L, camState["avy"]);
			lua_pushnumber(L, camState["avz"]);
			return 1 + 9;

		case CCameraHandler::CAMERA_MODE_OVERVIEW:
		default:
			// overview has no extra values
			return 1 + 0;
		}
	}

	lua_createtable(L, 0,
			std::tuple_size<CCameraController::StateMap::ArrayMap>{});

	lua_pushliteral(L, "name");
	lua_pushsstring(L, (camHandler->GetCurrentController()).GetName());
	lua_rawset(L, -3);

	for (const auto& s: camState) {
		lua_pushsstring(L, s.first);
		lua_pushnumber(L, s.second);
		lua_rawset(L, -3);
	}

	return 1;
}


/***
 * @function Spring.GetCameraPosition
 * @return number posX
 * @return number posY
 * @return number posZ
 */
int LuaUnsyncedRead::GetCameraPosition(lua_State* L)
{
	lua_pushnumber(L, camera->GetPos().x);
	lua_pushnumber(L, camera->GetPos().y);
	lua_pushnumber(L, camera->GetPos().z);
	return 3;
}

/***
 * @function Spring.GetCameraDirection
 * @return number dirX
 * @return number dirY
 * @return number dirZ
 */
int LuaUnsyncedRead::GetCameraDirection(lua_State* L)
{
	lua_pushnumber(L, camera->GetDir().x);
	lua_pushnumber(L, camera->GetDir().y);
	lua_pushnumber(L, camera->GetDir().z);
	return 3;
}

/*** Get camera rotation in radians.
 * 
 * @function Spring.GetCameraRotation
 * @return number rotX Rotation around X axis in radians.
 * @return number rotY Rotation around Y axis in radians.
 * @return number rotZ Rotation around Z axis in radians.
 */
int LuaUnsyncedRead::GetCameraRotation(lua_State* L)
{
	lua_pushnumber(L, camera->GetRot().x);
	lua_pushnumber(L, camera->GetRot().y);
	lua_pushnumber(L, camera->GetRot().z);
	return 3;
}

/***
 * @function Spring.GetCameraFOV
 * @return number vFOV
 * @return number hFOV
 */
int LuaUnsyncedRead::GetCameraFOV(lua_State* L)
{
	lua_pushnumber(L, camera->GetVFOV());
	lua_pushnumber(L, camera->GetHFOV());
	return 2;
}

/***
 * @class CameraVectors
 * @field forward xyz
 * @field up xyz
 * @field right xyz
 * @field topFrustumPlane xyz
 * @field botFrustumPlane xyz
 * @field lftFrustumPlane xyz
 * @field rgtFrustumPlane xyz
 */

/***
 * @function Spring.GetCameraVectors
 * @return CameraVectors
 */
int LuaUnsyncedRead::GetCameraVectors(lua_State* L)
{
#define PACK_CAMERA_VECTOR(s,n) \
	HSTR_PUSH(L, #s);           \
	lua_createtable(L, 3, 0);            \
	lua_pushnumber(L, camera-> n .x); lua_rawseti(L, -2, 1); \
	lua_pushnumber(L, camera-> n .y); lua_rawseti(L, -2, 2); \
	lua_pushnumber(L, camera-> n .z); lua_rawseti(L, -2, 3); \
	lua_rawset(L, -3)

	lua_createtable(L, 0, 7);
	PACK_CAMERA_VECTOR(forward, GetDir());
	PACK_CAMERA_VECTOR(up, GetUp());
	PACK_CAMERA_VECTOR(right, GetRight());
	PACK_CAMERA_VECTOR(topFrustumPlane, GetFrustumPlane(CCamera::FRUSTUM_PLANE_TOP));
	PACK_CAMERA_VECTOR(botFrustumPlane, GetFrustumPlane(CCamera::FRUSTUM_PLANE_BOT));
	PACK_CAMERA_VECTOR(lftFrustumPlane, GetFrustumPlane(CCamera::FRUSTUM_PLANE_LFT));
	PACK_CAMERA_VECTOR(rgtFrustumPlane, GetFrustumPlane(CCamera::FRUSTUM_PLANE_RGT));

	return 1;
}


/***
 *
 * @function Spring.WorldToScreenCoords
 * @param x number
 * @param y number
 * @param z number
 * @return number viewPortX
 * @return number viewPortY
 * @return number viewPortZ
 */
int LuaUnsyncedRead::WorldToScreenCoords(lua_State* L)
{
	const float3 worldPos(luaL_checkfloat(L, 1),
	                      luaL_checkfloat(L, 2),
	                      luaL_checkfloat(L, 3));
	const float3 vpPos = camera->CalcViewPortCoordinates(worldPos);
	lua_pushnumber(L, vpPos.x);
	lua_pushnumber(L, vpPos.y);
	lua_pushnumber(L, vpPos.z);
	return 3;
}


/*** Get information about a ray traced from screen to world position
 *
 * @function Spring.TraceScreenRay
 *
 * Extended to allow a custom plane, parameters are (0, 1, 0, D=0) where D is the offset D can be specified in the third argument (if all the bools are false) or in the seventh (as shown).
 *
 * Intersection coordinates are returned in t[4],t[5],t[6] when the ray goes offmap and includeSky is true), or when no unit or feature is hit (or onlyCoords is true).
 *
 * This will only work for units & objects with the default collision sphere. Per Piece collision and custom collision objects are not supported.
 *
 * The unit must be selectable, to appear to a screen trace ray.
 *
 * @param screenX number position on x axis in mouse coordinates (origin on left border of view)
 * @param screenY number position on y axis in mouse coordinates (origin on top border of view)
 * @param onlyCoords boolean? (Default: `false`) return only description (1st return value) and coordinates (2nd return value)
 * @param useMinimap boolean? (Default: `false`) if position arguments are contained by minimap, use the minimap corresponding world position
 * @param includeSky boolean? (Default: `false`)
 * @param ignoreWater boolean? (Default: `false`)
 * @param heightOffset number? (Default: `0`)
 * @return nil|string description of traced position
 * @return nil|number|string|xyz unitID or feature, position triple when onlyCoords=true
 * @return nil|number|string featureID or ground
 * @return nil|xyz coords
 */
int LuaUnsyncedRead::TraceScreenRay(lua_State* L)
{
	// window coordinates
	const int mx = luaL_checkint(L, 1);
	const int my = luaL_checkint(L, 2);

	const int wx = mx + globalRendering->viewPosX;
	const int wy = globalRendering->viewSizeY - 1 - my;

	const int optArgIdx = 3 + lua_isnumber(L, 3); // 3 or 4
	const int newArgIdx = 3 + 4 * (optArgIdx == 3); // 7 or 3

	const bool onlyCoords  = luaL_optboolean(L, optArgIdx + 0, false);
	const bool useMiniMap  = luaL_optboolean(L, optArgIdx + 1, false);
	const bool includeSky  = luaL_optboolean(L, optArgIdx + 2, false);
	const bool ignoreWater = luaL_optboolean(L, optArgIdx + 3, false);

	if (useMiniMap && (minimap != nullptr) && !minimap->GetMinimized()) {
		const int px = minimap->GetPosX() - globalRendering->viewPosX;
		const int py = minimap->GetPosY() - globalRendering->viewPosY;
		const int sx = minimap->GetSizeX();
		const int sy = minimap->GetSizeY();

		if ((mx >= px) && (mx < (px + sx))  &&  (my >= py) && (my < (py + sy))) {
			const float3 pos = minimap->GetMapPosition(wx, wy);

			if (!onlyCoords) {
				const CUnit* unit = minimap->GetSelectUnit(pos);

				if (unit != nullptr) {
					lua_pushliteral(L, "unit");
					lua_pushnumber(L, unit->id);
					return 2;
				}
			}

			lua_pushliteral(L, "ground");
			lua_createtable(L, 3, 0);
			lua_pushnumber(L,                                      pos.x ); lua_rawseti(L, -2, 1);
			lua_pushnumber(L, CGround::GetHeightReal(pos.x, pos.z, false)); lua_rawseti(L, -2, 2);
			lua_pushnumber(L,                                      pos.z ); lua_rawseti(L, -2, 3);
			return 2;
		}
	}

	if ((mx < 0) || (mx >= globalRendering->viewSizeX))
		return 0;
	if ((my < 0) || (my >= globalRendering->viewSizeY))
		return 0;

	const CUnit* unit = nullptr;
	const CFeature* feature = nullptr;

	const float rawRange = camera->GetFarPlaneDist() * 1.4f;
	const float badRange = rawRange - 300.0f;

	const float3 camPos = camera->GetPos();
	const float3 pxlDir = camera->CalcPixelDir(wx, wy);

	// trace for player's allyteam
	const float traceDist = TraceRay::GuiTraceRay(camPos, pxlDir, rawRange, nullptr, unit, feature, true, onlyCoords, ignoreWater);
	const float planeDist = CGround::LinePlaneCol(camPos, pxlDir, rawRange, luaL_optnumber(L, newArgIdx, 0.0f));

	const float3 tracePos = camPos + (pxlDir * traceDist);
	const float3 planePos = camPos + (pxlDir * planeDist); // backup (for includeSky and onlyCoords)

	if ((traceDist < 0.0f || traceDist > badRange) && unit == nullptr && feature == nullptr) {
		// ray went into the void (or started too far above terrain)
		if (!includeSky)
			return 0;

		lua_pushliteral(L, "sky");
	} else {
		if (!onlyCoords) {
			if (unit != nullptr) {
				lua_pushliteral(L, "unit");
				lua_pushnumber(L, unit->id);
				return 2;
			}

			if (feature != nullptr) {
				lua_pushliteral(L, "feature");
				lua_pushnumber(L, feature->id);
				return 2;
			}
		}

		lua_pushliteral(L, "ground");
	}

	lua_createtable(L, 6, 0);
	lua_pushnumber(L, tracePos.x); lua_rawseti(L, -2, 1);
	lua_pushnumber(L, tracePos.y); lua_rawseti(L, -2, 2);
	lua_pushnumber(L, tracePos.z); lua_rawseti(L, -2, 3);
	lua_pushnumber(L, planePos.x); lua_rawseti(L, -2, 4);
	lua_pushnumber(L, planePos.y); lua_rawseti(L, -2, 5);
	lua_pushnumber(L, planePos.z); lua_rawseti(L, -2, 6);

	return 2;
}


/***
 *
 * @function Spring.GetPixelDir
 * @param x number
 * @param y number
 * @return number dirX
 * @return number dirY
 * @return number dirZ
 */
int LuaUnsyncedRead::GetPixelDir(lua_State* L)
{
	const int x = luaL_checkint(L, 1);
	const int y = luaL_checkint(L, 2);
	const float3 dir = camera->CalcPixelDir(x, y);
	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 3;
}



/******************************************************************************/

static bool AddPlayerToRoster(lua_State* L, int playerID, bool onlyActivePlayers, bool includePathingFlag)
{
#define PUSH_ROSTER_ENTRY(type, val) \
	lua_push ## type(L, val); lua_rawseti(L, -2, index++);

	const CPlayer* p = playerHandler.Player(playerID);

	if (onlyActivePlayers && !p->active)
		return false;

	int index = 1;
	lua_createtable(L, 7, 0);
	PUSH_ROSTER_ENTRY(string, p->name.c_str());
	PUSH_ROSTER_ENTRY(number, playerID);
	PUSH_ROSTER_ENTRY(number, p->team);
	PUSH_ROSTER_ENTRY(number, teamHandler.AllyTeam(p->team));
	PUSH_ROSTER_ENTRY(boolean, p->spectator);
	PUSH_ROSTER_ENTRY(number, p->cpuUsage);

	if (!includePathingFlag || p->ping != PATHING_FLAG) {
		const float pingSecs = p->ping * 0.001f;
		PUSH_ROSTER_ENTRY(number, pingSecs);
	} else {
		const float pingSecs = float(p->ping);
		PUSH_ROSTER_ENTRY(number, pingSecs);
	}

	return true;
}


/***
 *
 * @function Spring.GetTeamColor
 * @param teamID integer
 * @return number? r factor from 0 to 1
 * @return number? g factor from 0 to 1
 * @return number? b factor from 0 to 1
 * @return number? a factor from 0 to 1
 */
int LuaUnsyncedRead::GetTeamColor(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler.ActiveTeams()))
		return 0;

	const CTeam* team = teamHandler.Team(teamID);
	if (team == nullptr)
		return 0;

	lua_pushnumber(L, team->color[0] / 255.0f);
	lua_pushnumber(L, team->color[1] / 255.0f);
	lua_pushnumber(L, team->color[2] / 255.0f);
	lua_pushnumber(L, team->color[3] / 255.0f);
	return 4;
}


/***
 *
 * @function Spring.GetTeamOrigColor
 * @param teamID integer
 * @return number? r factor from 0 to 1
 * @return number? g factor from 0 to 1
 * @return number? b factor from 0 to 1
 * @return number? a factor from 0 to 1
 */
int LuaUnsyncedRead::GetTeamOrigColor(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler.ActiveTeams()))
		return 0;

	const CTeam* team = teamHandler.Team(teamID);
	if (team == nullptr)
		return 0;

	lua_pushnumber(L, team->origColor[0] / 255.0f);
	lua_pushnumber(L, team->origColor[1] / 255.0f);
	lua_pushnumber(L, team->origColor[2] / 255.0f);
	lua_pushnumber(L, team->origColor[3] / 255.0f);
	return 4;
}


/***
 *
 * @function Spring.GetDrawSeconds
 * @return integer time Time in seconds.
 */
int LuaUnsyncedRead::GetDrawSeconds(lua_State* L)
{
	lua_pushnumber(L, spring_tomsecs(globalRendering->grTime) * 0.001f);
	return 1;
}


/******************************************************************************
 * Sound
 * @section sound
******************************************************************************/

/***
 * @class SoundDeviceSpec
 *
 * Contains data about a sound device.
 *
 * @field name string
 */


/***
 *
 * @function Spring.GetSoundDevices
 * @return SoundDeviceSpec[] devices Sound devices.
 */
int LuaUnsyncedRead::GetSoundDevices(lua_State* L)
{
	std::vector<std::string> devices = sound->GetSoundDevices();

	lua_createtable(L, 0, devices.size());

	for (size_t i = 0; i < devices.size(); ++i) {
		lua_createtable(L, 0, 1); // create a table for below {name, device} KV
		LuaPushNamedString(L, "name", devices[i]);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

/***
 *
 * @function Spring.GetSoundStreamTime
 * @return number playTime
 * @return number time
 */
int LuaUnsyncedRead::GetSoundStreamTime(lua_State* L)
{
	lua_pushnumber(L, Channels::BGMusic->StreamGetPlayTime());
	lua_pushnumber(L, Channels::BGMusic->StreamGetTime());
	return 2;
}


/***
 *
 * @function Spring.GetSoundEffectParams
 */
int LuaUnsyncedRead::GetSoundEffectParams(lua_State* L)
{
#if defined(HEADLESS) || defined(NO_SOUND)
	return 0;
#else
	if (!efx.Supported())
		return 0;

	EAXSfxProps& efxprops = efx.sfxProperties;

	lua_createtable(L, 0, 2);

	size_t n = efxprops.filter_props_f.size();

	lua_pushliteral(L, "passfilter");
	lua_createtable(L, 0, n);
	lua_rawset(L, -3);

	for (const auto& filterProp: efxprops.filter_props_f) {
		const ALuint param = filterProp.first;
		const auto fit = alFilterParamToName.find(param);

		if (fit != alFilterParamToName.end()) {
			lua_pushsstring(L, fit->second); // name
			lua_pushnumber(L, filterProp.second);
			lua_rawset(L, -3);
		}
	}


	n = efxprops.reverb_props_v.size() + efxprops.reverb_props_f.size() + efxprops.reverb_props_i.size();

	lua_pushliteral(L, "reverb");
	lua_createtable(L, 0, n);
	lua_rawset(L, -3);

	for (const auto& reverbProp: efxprops.reverb_props_f) {
		const ALuint param = reverbProp.first;
		const auto fit = alParamToName.find(param);

		if (fit != alParamToName.end()) {
			lua_pushsstring(L, fit->second); // name
			lua_pushnumber(L, reverbProp.second);
			lua_rawset(L, -3);
		}
	}
	for (const auto& reverbProp: efxprops.reverb_props_v) {
		const ALuint param = reverbProp.first;
		const auto fit = alParamToName.find(param);

		if (fit != alParamToName.end()) {
			const float3& v = reverbProp.second;

			lua_pushsstring(L, fit->second); // name
			lua_createtable(L, 3, 0);
				lua_pushnumber(L, v.x);
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, v.y);
				lua_rawseti(L, -2, 2);
				lua_pushnumber(L, v.z);
				lua_rawseti(L, -2, 3);
			lua_rawset(L, -3);
		}
	}
	for (const auto& reverbProp: efxprops.reverb_props_i) {
		const ALuint param = reverbProp.first;
		const auto fit = alParamToName.find(param);

		if (fit != alParamToName.end()) {
			lua_pushsstring(L, fit->second); // name
			lua_pushboolean(L, reverbProp.second);
			lua_rawset(L, -3);
		}
	}

	return 1;
#endif // defined(HEADLESS) || defined(NO_SOUND)
}


/******************************************************************************
 * Game Speed
 * @section gamespeed
******************************************************************************/


/***
 *
 * @function Spring.GetFPS
 * @return number fps
 */
int LuaUnsyncedRead::GetFPS(lua_State* L)
{
	assert(globalRendering != nullptr);
	// true FPS is never fractional, but the calculation averages over time
	lua_pushnumber(L, int(globalRendering->FPS));
	return 1;
}


/***
 *
 * @function Spring.GetGameSpeed
 * @return number wantedSpeedFactor
 * @return number speedFactor
 * @return boolean paused
 */
int LuaUnsyncedRead::GetGameSpeed(lua_State* L)
{
	lua_pushnumber(L, gs->wantedSpeedFactor);
	lua_pushnumber(L, gs->speedFactor);
	lua_pushboolean(L, gs->paused);
	return 3;
}

/***
 *
 * @function Spring.GetGameState
 * @param maxLatency number? (Default: `500`) used for `isSimLagging` return parameter
 * @return boolean doneLoading
 * @return boolean isSavedGame
 * @return boolean isClientPaused
 * @return boolean isSimLagging
 */
int LuaUnsyncedRead::GetGameState(lua_State* L)
{
  const float maxLatency = luaL_optfloat(L, 1, 500.0f);

	lua_pushboolean(L, game->IsDoneLoading());
	lua_pushboolean(L, game->IsSavedGame());
	lua_pushboolean(L, game->IsClientPaused()); // local state; included for demos
	lua_pushboolean(L, game->IsSimLagging(maxLatency));
	return 4;
}


/******************************************************************************
 * Commands
 * @section commands
******************************************************************************/


/***
 *
 * @function Spring.GetActiveCommand
 * @return number? cmdIndex
 * @return integer? cmdID
 * @return number? cmdType
 * @return nil|string cmdName
 */
int LuaUnsyncedRead::GetActiveCommand(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const vector<SCommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	const int inCommand = guihandler->inCommand;
	lua_pushnumber(L, inCommand + CMD_INDEX_OFFSET);

	if ((inCommand < 0) || (inCommand >= cmdDescCount))
		return 1;

	lua_pushnumber(L, cmdDescs[inCommand].id);
	lua_pushnumber(L, cmdDescs[inCommand].type);
	lua_pushsstring(L, cmdDescs[inCommand].name);
	return 4;
}


/***
 *
 * @function Spring.GetDefaultCommand
 * @return integer? cmdIndex
 * @return integer? cmdID
 * @return integer? cmdType
 * @return string? cmdName
 */
int LuaUnsyncedRead::GetDefaultCommand(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const int defCmd = guihandler->GetDefaultCommand(mouse->lastx, mouse->lasty);

	const vector<SCommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	lua_pushnumber(L, defCmd + CMD_INDEX_OFFSET);

	if ((defCmd < 0) || (defCmd >= cmdDescCount))
		return 1;

	lua_pushnumber(L, cmdDescs[defCmd].id);
	lua_pushnumber(L, cmdDescs[defCmd].type);
	lua_pushsstring(L, cmdDescs[defCmd].name);
	return 4;
}

/***
 *
 * @function Spring.GetActiveCmdDescs
 * @return CommandDescription[] cmdDescs
 */
int LuaUnsyncedRead::GetActiveCmdDescs(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const vector<SCommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	lua_checkstack(L, 1 + 2);
	// When CMD_INDEX_OFFSET is not 1, lua will resort to using the hash
	// part to index table keys as we're no longer adding keys to the table
	// following the sequence 1 to N for any N.
	lua_createtable(L,
			CMD_INDEX_OFFSET == 1 ? cmdDescCount : 0,
			CMD_INDEX_OFFSET == 1 ? 0 : cmdDescCount);

	for (int i = 0; i < cmdDescCount; i++) {
		LuaUtils::PushCommandDesc(L, cmdDescs[i]);
		lua_rawseti(L, -2, i + CMD_INDEX_OFFSET);
	}
	return 1;
}


/***
 *
 * @function Spring.GetActiveCmdDesc
 * @param cmdIndex integer
 * @return CommandDescription?
 */
int LuaUnsyncedRead::GetActiveCmdDesc(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const int cmdIndex = luaL_checkint(L, 1) - CMD_INDEX_OFFSET;

	const vector<SCommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	if ((cmdIndex < 0) || (cmdIndex >= cmdDescCount))
		return 0;

	LuaUtils::PushCommandDesc(L, cmdDescs[cmdIndex]);
	return 1;
}


/***
 *
 * @function Spring.GetCmdDescIndex
 * @param cmdID integer
 * @return integer? cmdDescIndex
 */
int LuaUnsyncedRead::GetCmdDescIndex(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const int cmdId = luaL_checkint(L, 1);

	const vector<SCommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();
	for (int i = 0; i < cmdDescCount; i++) {
		if (cmdId == cmdDescs[i].id) {
			lua_pushnumber(L, i + CMD_INDEX_OFFSET);
			return 1;
		}
	}
	return 0;
}


/******************************************************************************/

/***
 * Facing direction represented as an integer only.
 * 
 * @see Facing
 * 
 * @alias FacingInteger
 * | 0 # South
 * | 1 # East
 * | 2 # North
 * | 3 # West
 */

/***
 *
 * @function Spring.GetBuildFacing
 * @return FacingInteger buildFacing
 */
int LuaUnsyncedRead::GetBuildFacing(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	lua_pushnumber(L, guihandler->buildFacing);
	return 1;
}


/***
 *
 * @function Spring.GetBuildSpacing
 * @return number buildSpacing
 */
int LuaUnsyncedRead::GetBuildSpacing(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	lua_pushnumber(L, guihandler->buildSpacing);
	return 1;
}


/***
 *
 * @function Spring.GetGatherMode
 * @return number gatherMode
 */
int LuaUnsyncedRead::GetGatherMode(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	lua_pushnumber(L, guihandler->GetGatherMode());
	return 1;
}


/******************************************************************************/

/***
 *
 * @function Spring.GetActivePage
 * @return number activePage
 * @return number maxPage
 */
int LuaUnsyncedRead::GetActivePage(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	lua_pushnumber(L, guihandler->GetActivePage());
	lua_pushnumber(L, guihandler->GetMaxPage());
	return 2;
}


/******************************************************************************
 * Mouse State
 * @section mousestate
******************************************************************************/


/***
 *
 * @function Spring.GetMouseState
 * @return number x
 * @return number y
 * @return number lmbPressed left mouse button pressed
 * @return number mmbPressed middle mouse button pressed
 * @return number rmbPressed right mouse button pressed
 * @return boolean offscreen
 * @return boolean mmbScroll
 */
int LuaUnsyncedRead::GetMouseState(lua_State* L)
{
	assert(mouse != nullptr);
	assert(globalRendering != nullptr);

	lua_pushnumber(L, mouse->lastx - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - mouse->lasty - 1);

	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_LEFT  ].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_MIDDLE].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_RIGHT ].pressed);
	lua_pushboolean(L, mouse->offscreen);
	lua_pushboolean(L, mouse->mmbScroll);
	return 7;
}


/***
 *
 * @function Spring.GetMouseCursor
 * @return string cursorName
 * @return number cursorScale
 */
int LuaUnsyncedRead::GetMouseCursor(lua_State* L)
{
	assert(mouse != nullptr);

	lua_pushsstring(L, mouse->GetCurrentCursor());
	lua_pushnumber(L, mouse->GetCurrentCursorScale());
	return 2;
}


/***
 *
 * @function Spring.GetMouseStartPosition
 * @param button number
 * @return number x
 * @return number y
 * @return number camPosX
 * @return number camPosY
 * @return number camPosZ
 * @return number dirX
 * @return number dirY
 * @return number dirZ
 */
int LuaUnsyncedRead::GetMouseStartPosition(lua_State* L)
{
	assert(mouse != nullptr);

	const int button = luaL_checkint(L, 1);

	if ((button <= 0) || (button > NUM_BUTTONS))
		return 0;

	const CMouseHandler::ButtonPressEvt& bp = mouse->buttons[button];
	lua_pushnumber(L, bp.x);
	lua_pushnumber(L, bp.y);
	lua_pushnumber(L, bp.camPos.x);
	lua_pushnumber(L, bp.camPos.y);
	lua_pushnumber(L, bp.camPos.z);
	lua_pushnumber(L, bp.dir.x);
	lua_pushnumber(L, bp.dir.y);
	lua_pushnumber(L, bp.dir.z);
	return 8;
}


/***
 *
 * @function Spring.GetMouseButtonsPressed
 *
 * Get pressed status for specific buttons.
 *
 * @param button1 integer Index of the first button.
 * @param ... integer Indices for more buttons.
 * @return boolean ... Pressed status for the buttons.
 */
int LuaUnsyncedRead::GetMouseButtonsPressed(lua_State* L)
{
	const int numArgs = lua_gettop(L);

	if (numArgs == 0) {
		luaL_error(L, "Need to pass some button indexes.");
	}

	for (int i = 1; i <= numArgs ; ++i) {
		const int button = luaL_checkint(L, i);

		if (button <= 0)
			luaL_error(L, "%d: bad button index", button);
		if (button > NUM_BUTTONS)
			lua_pushboolean(L, false);
		else
			lua_pushboolean(L, mouse->buttons[button].pressed);
	}

	return numArgs;
}


/******************************************************************************
 * Text
 * @section text
******************************************************************************/


/***
 *
 * @function Spring.GetClipboard
 * @return string text
 */
int LuaUnsyncedRead::GetClipboard(lua_State* L)
{
	char* text = SDL_GetClipboardText();
	if (text == nullptr)
		return 0;
	lua_pushstring(L, text);
	SDL_free(text);
	return 1;
}


/***
 *
 * @function Spring.IsUserWriting
 * @return boolean
 */
int LuaUnsyncedRead::IsUserWriting(lua_State* L)
{
	lua_pushboolean(L, gameTextInput.userWriting);
	return 1;
}


/******************************************************************************
 * Console
 * @section console
******************************************************************************/


/***
 *
 * @function Spring.GetLastMessagePositions
 * @return xyz[] message positions
 */
int LuaUnsyncedRead::GetLastMessagePositions(lua_State* L)
{
	lua_createtable(L, infoConsole->GetMsgPosCount(), 0);

	for (unsigned int i = 1; i <= infoConsole->GetMsgPosCount(); i++) {
		lua_createtable(L, 3, 0); {
			const float3 msgpos = infoConsole->GetMsgPos();
			lua_pushnumber(L, msgpos.x); lua_rawseti(L, -2, 1);
			lua_pushnumber(L, msgpos.y); lua_rawseti(L, -2, 2);
			lua_pushnumber(L, msgpos.z); lua_rawseti(L, -2, 3);
		}
		lua_rawseti(L, -2, i);
	}

	return 1;
}


/***
 * @function Spring.GetConsoleBuffer
 * @param maxLines number
 * @return { text: string, priority: integer }[] buffer
 */
int LuaUnsyncedRead::GetConsoleBuffer(lua_State* L)
{
	std::vector<CInfoConsole::RawLine> lines;
	infoConsole->GetRawLines(lines);

	const size_t lineCount = lines.size();
	      size_t startLine = 0;

	if (lua_gettop(L) >= 1)
		startLine = lineCount - std::min(lineCount, size_t(luaL_checkint(L, 1)));

	// table = { [1] = { text = string, zone = number}, etc... }
	lua_createtable(L, lineCount - startLine, 0);

	for (size_t i = startLine, n = 0; i < lineCount; i++) {
		lua_pushnumber(L, ++n);
		lua_createtable(L, 0, 2); {
			lua_pushliteral(L, "text");
			lua_pushsstring(L, lines[i].text);
			lua_rawset(L, -3);
			lua_pushliteral(L, "priority");
			lua_pushnumber(L, lines[i].level);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}

	return 1;
}


/***
 * @function Spring.GetCurrentTooltip
 * @return string tooltip
 */
int LuaUnsyncedRead::GetCurrentTooltip(lua_State* L)
{
	lua_pushsstring(L, mouse->GetCurrentTooltip());
	return 1;
}


/******************************************************************************
 * Key Input
 * @section keyinput
******************************************************************************/


/***
 * @function Spring.GetKeyFromScanSymbol
 * @param scanSymbol string
 * @return string keyName
 */
int LuaUnsyncedRead::GetKeyFromScanSymbol(lua_State* L)
{
	const std::string& symbol = StringToLower(luaL_optstring(L, 1, ""));

	std::string result = "";

	if (symbol.empty()) {
		lua_pushstring(L, result.c_str());
		return 1;
	}

	SDL_Scancode scanCode = (SDL_Scancode)scanCodes.GetCode(symbol);
	if (scanCode <= 0) {
		lua_pushstring(L, result.c_str());
		return 1;
	}

	SDL_Keycode keyCode = (SDL_Keycode)SDL_GetKeyFromScancode(scanCode);
	if (keyCode <= 0 || keyCode == 0x40000000) {
		lua_pushstring(L, result.c_str());
		return 1;
	}

	result = keyCodes.GetDefaultName(keyCode);

	lua_pushstring(L, result.c_str());

	return 1;
}

/***
 *
 * @function Spring.GetKeyState
 * @param keyCode number
 * @return boolean pressed
 */
int LuaUnsyncedRead::GetKeyState(lua_State* L)
{
	const int key = SDL12_keysyms(luaL_checkint(L, 1));
	lua_pushboolean(L, KeyInput::IsKeyPressed(key));
	return 1;
}


/***
 *
 * @function Spring.GetModKeyState
 * @return boolean alt
 * @return boolean ctrl
 * @return boolean meta
 * @return boolean shift
 */
int LuaUnsyncedRead::GetModKeyState(lua_State* L)
{
	lua_pushboolean(L, KeyInput::GetKeyModState(KMOD_ALT));
	lua_pushboolean(L, KeyInput::GetKeyModState(KMOD_CTRL));
	lua_pushboolean(L, KeyInput::GetKeyModState(KMOD_GUI));
	lua_pushboolean(L, KeyInput::GetKeyModState(KMOD_SHIFT));
	return 4;
}


/***
 *
 * @function Spring.GetPressedKeys
 * @return table<number|string,true> where keys are keyCodes or key names
 */
int LuaUnsyncedRead::GetPressedKeys(lua_State* L)
{
	const auto& keys = KeyInput::GetPressedKeys();

	lua_createtable(L, keys.size(), 0);

	for (auto key: keys) {
		if (!key.second)
			continue;

		const int keyCode = SDL21_keysyms(key.first);

		// [keyCode] = true
		lua_pushboolean(L, true);
		lua_rawseti(L, -2, keyCode);

		// ["keyName"] = true
		lua_pushsstring(L, keyCodes.GetName(keyCode));
		lua_pushboolean(L, true);
		lua_rawset(L, -3);
	}

	return 1;
}


/***
 *
 * @function Spring.GetPressedScans
 * @return table<number|string,true> where keys are scanCodes or scan names
 */
int LuaUnsyncedRead::GetPressedScans(lua_State* L)
{
	const auto& scans = KeyInput::GetPressedScans();

	lua_createtable(L, scans.size(), 0);

	for (auto scan: scans) {
		if (!scan.second)
			continue;

		const int scanCode = scan.first;

		// [scanCode] = true
		lua_pushboolean(L, true);
		lua_rawseti(L, -2, scanCode);

		// ["scanName"] = true
		lua_pushsstring(L, scanCodes.GetName(scanCode));
		lua_pushboolean(L, true);
		lua_rawset(L, -3);
	}

	return 1;
}


/***
 *
 * @function Spring.GetInvertQueueKey
 * @return number? queueKey
 */
int LuaUnsyncedRead::GetInvertQueueKey(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	lua_pushboolean(L, guihandler->GetInvertQueueKey());
	return 1;
}


/***
 *
 * @function Spring.GetKeyCode
 * @param keySym string
 * @return number keyCode
 */
int LuaUnsyncedRead::GetKeyCode(lua_State* L)
{
	lua_pushnumber(L, SDL21_keysyms(keyCodes.GetCode(luaL_checksstring(L, 1))));
	return 1;
}


/***
 *
 * @function Spring.GetKeySymbol
 * @param keyCode number
 * @return string keyCodeName
 * @return string keyCodeDefaultName name when there are not aliases
 */
int LuaUnsyncedRead::GetKeySymbol(lua_State* L)
{
	const int keycode = SDL12_keysyms(luaL_checkint(L, 1));
	lua_pushsstring(L, keyCodes.GetName(keycode));
	lua_pushsstring(L, keyCodes.GetDefaultName(keycode));
	return 2;
}


/***
 *
 * @function Spring.GetScanSymbol
 * @param scanCode number
 * @return string scanCodeName
 * @return string scanCodeDefaultName name when there are not aliases
 */
int LuaUnsyncedRead::GetScanSymbol(lua_State* L)
{
	const int scanCode = luaL_checkint(L, 1);
	lua_pushsstring(L, scanCodes.GetName(scanCode));
	lua_pushsstring(L, scanCodes.GetDefaultName(scanCode));
	return 2;
}


/***
 * Keybinding
 *
 * Contains data about a keybinding
 *
 * @class KeyBinding
 * @field command string
 * @field extra string
 * @field boundWith string
 */


/***
 * @function Spring.GetKeyBindings
 * @param keySet1 string? filters keybindings bound to this keyset
 * @param keySet2 string? OR bound to this keyset
 * @return KeyBinding[]
 */
int LuaUnsyncedRead::GetKeyBindings(lua_State* L)
{
	ActionList actions;
	const std::string& argument = luaL_optstring(L, 1, "");

	if (argument.empty()) {
		actions = keyBindings.GetActionList();
	} else {
		CKeySet ks;

		if (!ks.Parse(luaL_checksstring(L, 1)))
			return 0;

		CKeyChain keyChain;
		keyChain.emplace_back(ks);

		const std::string& arg2 = luaL_optstring(L, 2, "");

		if (arg2.empty()) {
			actions = keyBindings.GetActionList(keyChain);
		} else {
			if (!ks.Parse(luaL_checksstring(L, 2)))
				return 0;

			CKeyChain keyChain2;
			keyChain2.emplace_back(ks);

			actions = keyBindings.GetActionList(keyChain, keyChain2);
		}
	}

	int i = 1;
	lua_createtable(L, actions.size(), 0);
	for (const Action& action: actions) {
		lua_createtable(L, 0, 4);
			lua_pushsstring(L, action.command);
			lua_pushsstring(L, action.extra);
			lua_rawset(L, -3);
			LuaPushNamedString(L, "command",   action.command);
			LuaPushNamedString(L, "extra",     action.extra);
			LuaPushNamedString(L, "boundWith", action.boundWith);
		lua_rawseti(L, -2, i++);
	}
	return 1;
}


/***
 *
 * @function Spring.GetActionHotKeys
 * @param actionName string
 * @return string[]? hotkeys
 */
int LuaUnsyncedRead::GetActionHotKeys(lua_State* L)
{
	const CKeyBindings::HotkeyList& hotkeys = keyBindings.GetHotkeys(luaL_checksstring(L, 1));

	lua_createtable(L, 0, hotkeys.size());
	int i = 1;
	for (const std::string& hotkey: hotkeys) {
		lua_pushsstring(L, hotkey);
		lua_rawseti(L, -2, i++);
	}
	return 1;
}


/******************************************************************************
 * Unit Groups
 * @section unitgroups
******************************************************************************/


/***
 *
 * @function Spring.GetGroupList
 * @return nil|table<number,number> where keys are groupIDs and values are counts
 */
int LuaUnsyncedRead::GetGroupList(lua_State* L)
{
	unsigned int count = 0;

	const std::vector<CGroup>& groups = uiGroupHandlers[gu->myTeam].GetGroups();

	// not an array-table
	lua_createtable(L, 0, groups.size());

	for (const CGroup& group: groups) {
		if (group.units.empty())
			continue;

		lua_pushnumber(L, group.units.size());
		lua_rawseti(L, -2, group.id);
		count++;
	}

	lua_pushnumber(L, count);
	return 2;
}


/***
 *
 * @function Spring.GetSelectedGroup
 * @return integer groupID -1 when no group selected
 */
int LuaUnsyncedRead::GetSelectedGroup(lua_State* L)
{
	lua_pushnumber(L, selectedUnitsHandler.GetSelectedGroup());
	return 1;
}


/***
 *
 * @function Spring.GetUnitGroup
 * @param unitID integer
 * @return integer? groupID
 */
int LuaUnsyncedRead::GetUnitGroup(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;
	if (unit->team != gu->myTeam)
		return 0;

	const CGroup* group = unit->GetGroup();

	if (group == nullptr)
		return 0;

	lua_pushnumber(L, group->id);
	return 1;
}

static inline const CGroup* GetGroupFromArg(lua_State* L, int arg)
{
	const int groupID = luaL_checkint(L, arg);
	const auto& groupHandler = uiGroupHandlers[gu->myTeam];

	if (!groupHandler.HasGroup(groupID))
		return nullptr;

	return groupHandler.GetGroup(groupID);
}

/***
 *
 * @function Spring.GetGroupUnits
 * @param groupID integer
 * @return nil|number[] unitIDs
 */
int LuaUnsyncedRead::GetGroupUnits(lua_State* L)
{
	const auto group = GetGroupFromArg(L, 1);
	if (!group)
		return 0;

	PushNumberContainerAsArray(L, group->units);
	return 1;
}


/***
 *
 * @function Spring.GetGroupUnitsSorted
 * @param groupID integer
 * @return nil|table<number,number[]> where keys are unitDefIDs and values are unitIDs
 */
int LuaUnsyncedRead::GetGroupUnitsSorted(lua_State* L)
{
	const auto group = GetGroupFromArg(L, 1);
	if (!group)
		return 0;

	PushUnitListSortedByDef(L, group->units);
	return 1;
}


/***
 *
 * @function Spring.GetGroupUnitsCounts
 * @param groupID integer
 * @return nil|table<number,number> where keys are unitDefIDs and values are counts
 */
int LuaUnsyncedRead::GetGroupUnitsCounts(lua_State* L)
{
	const auto group = GetGroupFromArg(L, 1);
	if (!group)
		return 0;

	PushSparseUnitTallyByDef(L, group->units);
	return 1;
}


/***
 *
 * @function Spring.GetGroupUnitsCount
 * @param groupID integer
 * @return number? groupSize
 */
int LuaUnsyncedRead::GetGroupUnitsCount(lua_State* L)
{
	const auto group = GetGroupFromArg(L, 1);
	if (!group)
		return 0;

	lua_pushnumber(L, group->units.size());
	return 1;
}


/******************************************************************************
 * Team/Player Info
 * @section teamplayerinfo
******************************************************************************/


/*** Roster
 *
 * Contains data about a player
 *
 * @class Roster
 * @field name string
 * @field playerID integer
 * @field teamID integer
 * @field allyTeamID integer
 * @field spectator boolean
 * @field cpuUsage number in order to find the progress, use: cpuUsage&0x1 if it's PC or BO, cpuUsage& 0xFE to get path res, (cpuUsage>>8)*1000 for the progress
 * @field pingTime number if -1, the player is pathfinding
 */


/***
 *
 * @function Spring.GetPlayerRoster
 * @param sortType number? return unsorted if unspecified. Disabled = 0, Allies = 1, TeamID = 2, PlayerName = 3, PlayerCPU = 4, PlayerPing = 5
 * @param showPathingPlayers boolean? (Default: `false`)
 * @return Roster[]? playerTable
 */
int LuaUnsyncedRead::GetPlayerRoster(lua_State* L)
{
	const PlayerRoster::SortType oldSortType = playerRoster.GetSortType();

	if (!lua_isnone(L, 1))
		playerRoster.SetSortTypeByCode((PlayerRoster::SortType) luaL_checkint(L, 1));

	const bool includePathingFlag = luaL_optboolean(L, 2, false);

	// get the sorted indices of *all* (including inactive) players
	const std::vector<int>& playerIndices = playerRoster.GetIndices(includePathingFlag);

	// revert
	playerRoster.SetSortTypeByCode(oldSortType);

	// push the active players
	lua_createtable(L, playerIndices.size(), 0);
	for (size_t i = 0, j = 1, s = playerIndices.size(); i < s; i++) {
		if (!AddPlayerToRoster(L, playerIndices[i], true, includePathingFlag))
			continue;

		// t[j] = {...}
		lua_rawseti(L, -2, j++);
	}

	return 1;
}


/***
 *
 * @function Spring.GetPlayerTraffic
 * @param playerID integer
 * @param packetID integer?
 * @return number traffic
 */
int LuaUnsyncedRead::GetPlayerTraffic(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	const int packetID = (int)luaL_optnumber(L, 2, -1);

	const auto& traffic = game->GetPlayerTraffic();
	const auto it = traffic.find(playerID);

	if (it == traffic.end()) {
		lua_pushnumber(L, -1);
		return 1;
	}

	// only allow viewing stats for specific packet types
	if (
		(playerID != -1) &&              // all system counts can be read
		(playerID != gu->myPlayerNum) && // all  self  counts can be read
		(packetID != -1) &&
		(packetID != NETMSG_CHAT)     &&
		(packetID != NETMSG_PAUSE)    &&
		(packetID != NETMSG_LUAMSG)   &&
		(packetID != NETMSG_STARTPOS) &&
		(packetID != NETMSG_USER_SPEED)
	) {
		lua_pushnumber(L, -1);
		return 1;
	}

	const CGame::PlayerTrafficInfo& pti = it->second;
	if (packetID == -1) {
		lua_pushnumber(L, pti.total);
		return 1;
	}

	const auto pit = pti.packets.find(packetID);
	if (pit == pti.packets.end()) {
		lua_pushnumber(L, -1);
		return 1;
	}

	lua_pushnumber(L, pit->second);
	return 1;
}


/***
 *
 * @function Spring.GetPlayerStatistics
 * @param playerID integer
 * @return number? mousePixels nil when invalid playerID
 * @return number mouseClicks
 * @return number keyPresses
 * @return number numCommands
 * @return number unitCommands
 */
int LuaUnsyncedRead::GetPlayerStatistics(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	if (!playerHandler.IsValidPlayer(playerID))
		return 0;

	const CPlayer* player = playerHandler.Player(playerID);
	if (player == nullptr)
		return 0;

	const PlayerStatistics& pStats = player->currentStats;

	lua_pushnumber(L, pStats.mousePixels);
	lua_pushnumber(L, pStats.mouseClicks);
	lua_pushnumber(L, pStats.keyPresses);
	lua_pushnumber(L, pStats.numCommands);
	lua_pushnumber(L, pStats.unitCommands);

	return 5;
}


/******************************************************************************
 * Configuration
 * @section configuration
******************************************************************************/


/*** Configuration
 *
 * Contains data about a configuration, only name and type are guaranteed
 *
 * @class Configuration
 * @field name string
 * @field type string
 * @field description string
 * @field defaultValue string
 * @field minimumValue string
 * @field maximumValue string
 * @field safemodeValue string
 * @field declarationFile string
 * @field declarationLine string
 * @field readOnly boolean
 */


/***
 *
 * @function Spring.GetConfigParams
 * @return Configuration[]
 */
int LuaUnsyncedRead::GetConfigParams(lua_State* L)
{
	ConfigVariable::MetaDataMap cfgmap = ConfigVariable::GetMetaDataMap();
	lua_createtable(L, cfgmap.size(), 0);

	int i = 1;
	for (ConfigVariable::MetaDataMap::const_iterator it = cfgmap.begin(); it != cfgmap.end(); ++it) {
		const ConfigVariableMetaData* meta = it->second;

		lua_createtable(L, 0, 11);

			lua_pushliteral(L, "name");
			lua_pushsstring(L, meta->GetKey());
			lua_rawset(L, -3);
			if (meta->GetDescription().IsSet()) {
				lua_pushliteral(L, "description");
				lua_pushsstring(L, meta->GetDescription().ToString());
				lua_rawset(L, -3);
			}
			lua_pushliteral(L, "type");
			lua_pushsstring(L, meta->GetType());
			lua_rawset(L, -3);
			if (meta->GetDefaultValue().IsSet()) {
				lua_pushliteral(L, "defaultValue");
				lua_pushsstring(L, meta->GetDefaultValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetMinimumValue().IsSet()) {
				lua_pushliteral(L, "minimumValue");
				lua_pushsstring(L, meta->GetMinimumValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetMaximumValue().IsSet()) {
				lua_pushliteral(L, "maximumValue");
				lua_pushsstring(L, meta->GetMaximumValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetSafemodeValue().IsSet()) {
				lua_pushliteral(L, "safemodeValue");
				lua_pushsstring(L, meta->GetSafemodeValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetDeclarationFile().IsSet()) {
				lua_pushliteral(L, "declarationFile");
				lua_pushsstring(L, meta->GetDeclarationFile().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetDeclarationLine().IsSet()) {
				lua_pushliteral(L, "declarationLine");
				lua_pushnumber(L, meta->GetDeclarationLine().Get());
				lua_rawset(L, -3);
			}
			if (meta->GetReadOnly().IsSet()) {
				lua_pushliteral(L, "readOnly");
				lua_pushboolean(L, !!meta->GetReadOnly().Get());
				lua_rawset(L, -3);
			}
			if (meta->GetDeprecated().IsSet()) {
				lua_pushliteral(L, "readOnly");
				lua_pushboolean(L, !!meta->GetDeprecated().Get());
				lua_rawset(L, -3);
			}

		lua_rawseti(L, -2, i++);
	}
	return 1;
}


/***
 *
 * @function Spring.GetConfigInt
 * @param name string
 * @param default number? (Default: `0`)
 * @return number? configInt
 */
int LuaUnsyncedRead::GetConfigInt(lua_State* L)
{
	const auto key = luaL_checkstring(L, 1);
	if (!lua_isnoneornil(L, 2))
		lua_pushinteger(L, configHandler->GetIntSafe(key, luaL_optint(L, 2, 0)));
	else if (configHandler->IsSet(key))
		lua_pushinteger(L, configHandler->GetInt(key));
	else
		lua_pushnil(L);

	return 1;
}


/***
 *
 * @function Spring.GetConfigFloat
 * @param name string
 * @param default number? (Default: `0`)
 * @return number? configFloat
 */
int LuaUnsyncedRead::GetConfigFloat(lua_State* L)
{
	const auto key = luaL_checkstring(L, 1);
	if (!lua_isnoneornil(L, 2))
		lua_pushnumber(L, configHandler->GetFloatSafe(key, luaL_optfloat(L, 2, 0.0f)));
	else if (configHandler->IsSet(key))
		lua_pushnumber(L, configHandler->GetFloat(key));
	else
		lua_pushnil(L);

	return 1;
}


/***
 *
 * @function Spring.GetConfigString
 * @param name string
 * @param default string? (Default: `""`)
 * @return number? configString
 */
int LuaUnsyncedRead::GetConfigString(lua_State* L)
{
	const auto key = luaL_checkstring(L, 1);
	if (!lua_isnoneornil(L, 2))
		lua_pushsstring(L, configHandler->GetStringSafe(key, luaL_optstring(L, 2, "")));
	else if (configHandler->IsSet(key))
		lua_pushsstring(L, configHandler->GetString(key));
	else
		lua_pushnil(L);

	return 1;
}


/***
 *
 * @function Spring.GetLogSections
 * @return table<string,number> sections where keys are names and loglevel are values. E.g. `{ "KeyBindings" = LOG.INFO, "Font" = LOG.INFO, "Sound" = LOG.WARNING, ... }`
 */
int LuaUnsyncedRead::GetLogSections(lua_State* L) {
	const int numLogSections = log_filter_section_getNumRegisteredSections();

	lua_createtable(L, 0, numLogSections);
	for (int i = 0; i < numLogSections; ++i) {
		const char* sectionName = log_filter_section_getRegisteredIndex(i);
		const int logLevel = log_filter_section_getMinLevel(sectionName);

		lua_pushstring(L, sectionName);
		lua_pushnumber(L, logLevel);
		lua_rawset(L, -3);
	}

	return 1;
}


/******************************************************************************
 * Decals
 * @section decals
******************************************************************************/


/***
 *
 * @function Spring.GetAllGroundDecals
 *
 * @return number[] decalIDs
 */
int LuaUnsyncedRead::GetAllGroundDecals(lua_State* L)
{
	const auto& decals = groundDecals->GetAllDecals();

	int numValid = 0;
	for (const auto& d : decals) {
		numValid += d.IsValid();
	}

	if (numValid == 0) {
		lua_newtable(L);
		return 1;
	}

	int i = 1;
	lua_createtable(L, numValid, 0);
	for (const auto& d: decals) {
		if (!d.IsValid())
			continue;

		lua_pushnumber(L, d.info.id);
		lua_rawseti(L, -2, i++);
	}

	return 1;
}


/***
 *
 * @function Spring.GetGroundDecalMiddlePos
 * @param decalID integer
 * @return number? posX
 * @return number posZ
 */
int LuaUnsyncedRead::GetGroundDecalMiddlePos(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal) {
		return 0;
	}

	const float2 midPointCurr = (decal->posTL + decal->posTR + decal->posBR + decal->posBL) * 0.25f;
	lua_pushnumber(L, midPointCurr.x);
	lua_pushnumber(L, midPointCurr.y);

	return 2;
}

/***
 *
 * @function Spring.GetDecalQuadPos
 * @param decalID integer
 * @return number? posTL.x
 * @return number posTL.z
 * @return number posTR.x
 * @return number posTR.z
 * @return number posBR.x
 * @return number posBR.z
 * @return number posBL.x
 * @return number posBL.z
 */
int LuaUnsyncedRead::GetGroundDecalQuadPos(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal) {
		return 0;
	}

	lua_pushnumber(L, decal->posTL.x);
	lua_pushnumber(L, decal->posTL.y);
	lua_pushnumber(L, decal->posTR.x);
	lua_pushnumber(L, decal->posTR.y);
	lua_pushnumber(L, decal->posBR.x);
	lua_pushnumber(L, decal->posBR.y);
	lua_pushnumber(L, decal->posBL.x);
	lua_pushnumber(L, decal->posBL.y);

	return 8;
}


/***
 *
 * @function Spring.GetGroundDecalSizeAndHeight
 * @param decalID integer
 * @return number? sizeX
 * @return number sizeY
 * @return number projCubeHeight
 */
int LuaUnsyncedRead::GetGroundDecalSizeAndHeight(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal) {
		return 0;
	}

	// average and take half of it
	lua_pushnumber(L, (decal->posTL.Distance(decal->posTR) + decal->posBL.Distance(decal->posBR)) * 0.5f * 0.5f);
	lua_pushnumber(L, (decal->posTL.Distance(decal->posBL) + decal->posTR.Distance(decal->posBR)) * 0.5f * 0.5f);
	lua_pushnumber(L, decal->height);

	return 3;
}


/***
 *
 * @function Spring.GetGroundDecalRotation
 * @param decalID integer
 * @return number? rotation Rotation in radians.
 */
int LuaUnsyncedRead::GetGroundDecalRotation(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal) {
		return 0;
	}

	lua_pushnumber(L, decal->rot);

	return 1;
}


/***
 *
 * @function Spring.GetGroundDecalTexture
 * @param decalID integer
 * @param isMainTex boolean? (Default: `true`) If `false`, return the normal/glow map.
 * @return nil|string texture
 */
int LuaUnsyncedRead::GetGroundDecalTexture(lua_State* L)
{
	const auto& texName = groundDecals->GetDecalTexture(luaL_checkint(L, 1), luaL_optboolean(L, 2, true));
	lua_pushsstring(L, texName);

	return 1;
}

/***
 *
 * @function Spring.GetDecalTextures
 * @param isMainTex boolean? (Default: `true`) If `false`, return the texture for normal/glow maps.
 * @return string[] textureNames All textures on the atlas and available for use in `SetGroundDecalTexture`.
 * @see Spring.GetGroundDecalTexture
 */
int LuaUnsyncedRead::GetGroundDecalTextures(lua_State* L)
{
	const auto& texNames = groundDecals->GetDecalTextures(luaL_optboolean(L, 2, true));
	LuaUtils::PushStringVector(L, texNames);

	return 1;
}


/***
 *
 * @function Spring.SetGroundDecalTextureParams
 * @param decalID integer
 * @return number? texWrapDistance If non-zero, sets the mode to repeat the texture along the left-right direction of the decal every texWrapFactor elmos.
 * @return number texTraveledDistance Shifts the texture repetition defined by texWrapFactor so the texture of a next line in the continuous multiline can start where the previous finished. For that it should collect all elmo lengths of the previously set multiline segments.
 */
int LuaUnsyncedRead::GetGroundDecalTextureParams(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal) {
		return 0;
	}

	lua_pushnumber(L, decal->uvWrapDistance);
	lua_pushnumber(L, decal->uvTraveledDistance);

	return 2;
}


/***
 *
 * @function Spring.GetGroundDecalAlpha
 * @param decalID integer
 * @return number? alpha Between 0 and 1
 * @return number alphaFalloff Between 0 and 1, per second
 */
int LuaUnsyncedRead::GetGroundDecalAlpha(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal) {
		return 0;
	}

	lua_pushnumber(L, decal->alpha);
	lua_pushnumber(L, decal->alphaFalloff * GAME_SPEED);

	return 2;
}

/***
 *
 * @function Spring.GetGroundDecalNormal
 *
 * If all three equal 0, the decal follows the normals of ground at midpoint
 *
 * @param decalID integer
 * @return number? normal.x
 * @return number normal.y
 * @return number normal.z
 */
int LuaUnsyncedRead::GetGroundDecalNormal(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal) {
		return 0;
	}

	lua_pushnumber(L, decal->forcedNormal.x);
	lua_pushnumber(L, decal->forcedNormal.y);
	lua_pushnumber(L, decal->forcedNormal.z);

	return 3;
}

/***
 *
 * @function Spring.GetGroundDecalTint
 * Gets the tint of the ground decal.
 * A color of (0.5, 0.5, 0.5, 0.5) is effectively no tint
 * @param decalID integer
 * @return number? tintR
 * @return number tintG
 * @return number tintB
 * @return number tintA
 */
int LuaUnsyncedRead::GetGroundDecalTint(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal) {
		return 0;
	}

	float4 tintColor = decal->tintColor;
	lua_pushnumber(L, tintColor.r);
	lua_pushnumber(L, tintColor.g);
	lua_pushnumber(L, tintColor.b);
	lua_pushnumber(L, tintColor.a);

	return 4;
}

/***
 *
 * @function Spring.GetGroundDecalMisc
 * Returns less important parameters of a ground decal
 * @param decalID integer
 * @return number? dotElimExp
 * @return number refHeight
 * @return number minHeight
 * @return number maxHeight
 * @return number forceHeightMode
 */
int LuaUnsyncedRead::GetGroundDecalMisc(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal) {
		return 0;
	}

	lua_pushnumber(L, decal->dotElimExp);
	lua_pushnumber(L, decal->refHeight);
	lua_pushnumber(L, decal->minHeight);
	lua_pushnumber(L, decal->maxHeight);
	lua_pushnumber(L, decal->forceHeightMode);
	return 5;
}

/***
 *
 * @function Spring.GetGroundDecalCreationFrame
 *
 * Min can be not equal to max for "gradient" style decals, e.g. unit tracks
 *
 * @param decalID integer
 * @return number? creationFrameMin
 * @return number creationFrameMax
 */
int LuaUnsyncedRead::GetGroundDecalCreationFrame(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal) {
		return 0;
	}

	lua_pushnumber(L, decal->createFrameMin);
	lua_pushnumber(L, decal->createFrameMax);

	return 2;
}


/***
 * @function Spring.GetGroundDecalOwner
 * @param decalID integer
 * @return integer? value If owner is a unit, then this is `unitID`, if owner is
 * a feature it is `featureID + MAX_UNITS`. If there is no owner, then `nil`.
 */
int LuaUnsyncedRead::GetGroundDecalOwner(lua_State* L)
{
	const auto* so = groundDecals->GetDecalSolidObjectOwner(luaL_checkint(L, 1));
	if (so == nullptr)
		return 0;

	if (const auto* f = dynamic_cast<const CFeature*>(so); f != nullptr)
		lua_pushnumber(L, unitHandler.MaxUnits() + so->id);
	else
		lua_pushnumber(L,                          so->id);

	return 1;
}


/***
 *
 * @function Spring.GetGroundDecalType
 * @param decalID integer
 * @return nil|string type "explosion"|"plate"|"lua"|"track"|"unknown"
 */
int LuaUnsyncedRead::GetGroundDecalType(lua_State* L)
{
	const auto* decal = groundDecals->GetDecalById(luaL_checkint(L, 1));
	if (!decal || decal->info.type == static_cast<uint8_t>(GroundDecal::Type::DECAL_NONE)) {
		return 0;
	}

	switch (decal->info.type)
	{
	case static_cast<uint8_t>(GroundDecal::Type::DECAL_PLATE):
		lua_pushliteral(L, "plate");
		break;
	case static_cast<uint8_t>(GroundDecal::Type::DECAL_EXPLOSION):
		lua_pushliteral(L, "explosion");
		break;
	case static_cast<uint8_t>(GroundDecal::Type::DECAL_TRACK):
		lua_pushliteral(L, "track");
		break;
	case static_cast<uint8_t>(GroundDecal::Type::DECAL_LUA):
		lua_pushliteral(L, "lua");
		break;
	default:
		lua_pushliteral(L, "unknown");
		break;
	}

	return 1;
}


/******************************************************************************
 * Misc
 * @section misc
******************************************************************************/


/***
 *
 * @function Spring.GetSyncedGCInfo
 * @param collectGC boolean? (Default: `false`) collect before returning metric
 * @return number? GC values are expressed in Kbytes: #bytes/2^10
 */
int LuaUnsyncedRead::GetSyncedGCInfo(lua_State* L) {
	if (luaRules == nullptr)
		return 0;

	lua_State* syncedL = luaRules->syncedLuaHandle.GetLuaGCState();

	if (syncedL == nullptr)
		return 0;

	auto luaLock = spring::ScopedNullResource([syncedL]() { lua_lock(syncedL); }, [syncedL]() { lua_unlock(syncedL); });

	if (luaL_optboolean(L, 1, false)) { //perform collection before returning the use metric
		lua_gc(syncedL, LUA_GCCOLLECT, 0); //mark
		lua_gc(syncedL, LUA_GCCOLLECT, 0); //sweep
		lua_gc(syncedL, LUA_GCSTOP, 0); //just in case
	}

	lua_pushnumber(L, lua_gc(syncedL, LUA_GCCOUNT, 0));
	return 1;
}

/***
 *
 * @function Spring.SolveNURBSCurve
 * @param groupID integer
 * @return number[]? unitIDs
 */
int LuaUnsyncedRead::SolveNURBSCurve(lua_State* L)
{
	int degree = luaL_checkint(L, 1);

	std::vector<float4> cpoints{};
	std::vector<float> knots{};

	LuaUtils::ParseFloat4Vector(L, 2, cpoints);
	LuaUtils::ParseFloatVector(L, 3, knots);

	int segments = luaL_checkint(L, 4);

	const auto result = NURBS::SolveNURBSCurve(degree, cpoints, knots, segments);

	lua_createtable(L, result.size(), 0);
	for (size_t i = 0; const auto &x : result) {
		lua_pushnumber(L, x.x);
		lua_rawseti(L, -2, ++i);
		lua_pushnumber(L, x.y);
		lua_rawseti(L, -2, ++i);
		lua_pushnumber(L, x.z);
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}
