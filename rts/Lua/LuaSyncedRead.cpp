/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaSyncedRead.h"

#include "LuaInclude.h"

#include "LuaConfig.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaMetalMap.h"
#include "LuaPathFinder.h"
#include "LuaRules.h"
#include "LuaRulesParams.h"
#include "LuaUtils.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/MapParser.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/Models/3DModel.hpp"
#include "Rendering/Models/3DModelPiece.hpp"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/MoveTypes/StaticMoveType.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/QueuedBuildOverlap.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitToolTipMap.hpp"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/AABB.hpp"
#include "System/MainDefines.h"
#include "System/SpringMath.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/StringUtil.h"

#include <cctype>
#include <functional>
#include <type_traits>


using std::min;
using std::max;

static const LuaHashString hs_n("n");



/******************************************************************************
 * Synced Read
 *
 * @see rts/Lua/LuaSyncedRead.cpp
******************************************************************************/

bool LuaSyncedRead::PushEntries(lua_State* L)
{
	/*** @field Spring.ALL_UNITS integer */
	LuaPushNamedNumber(L, "ALL_UNITS", LuaUtils::AllUnits);
	/*** @field Spring.MY_UNITS integer */
	LuaPushNamedNumber(L, "MY_UNITS", LuaUtils::MyUnits);
	/*** @field Spring.ALLY_UNITS integer */
	LuaPushNamedNumber(L, "ALLY_UNITS", LuaUtils::AllyUnits);
	/*** @field Spring.ENEMY_UNITS integer */
	LuaPushNamedNumber(L, "ENEMY_UNITS", LuaUtils::EnemyUnits);

	// READ routines, sync safe
	REGISTER_LUA_CFUNC(IsCheatingEnabled);
	REGISTER_LUA_CFUNC(IsGodModeEnabled);
	REGISTER_LUA_CFUNC(IsDevLuaEnabled);
	REGISTER_LUA_CFUNC(IsEditDefsEnabled);
	REGISTER_LUA_CFUNC(IsNoCostEnabled);
	REGISTER_LUA_CFUNC(GetGlobalLos);
	REGISTER_LUA_CFUNC(AreHelperAIsEnabled);
	REGISTER_LUA_CFUNC(FixedAllies);

	REGISTER_LUA_CFUNC(IsGameOver);

	REGISTER_LUA_CFUNC(GetGaiaTeamID);

	REGISTER_LUA_CFUNC(GetGameFrame);
	REGISTER_LUA_CFUNC(GetGameSeconds);

	REGISTER_LUA_CFUNC(GetGameRulesParam);
	REGISTER_LUA_CFUNC(GetGameRulesParams);

	REGISTER_LUA_CFUNC(GetPlayerRulesParam);
	REGISTER_LUA_CFUNC(GetPlayerRulesParams);

	REGISTER_LUA_CFUNC(GetMapOption);
	REGISTER_LUA_CFUNC(GetMapOptions);
	REGISTER_LUA_CFUNC(GetModOption);
	REGISTER_LUA_CFUNC(GetModOptions);

	REGISTER_LUA_CFUNC(GetTidal);
	REGISTER_LUA_CFUNC(GetWind);

	REGISTER_LUA_CFUNC(GetHeadingFromVector);
	REGISTER_LUA_CFUNC(GetVectorFromHeading);
	REGISTER_LUA_CFUNC(GetHeadingFromFacing);
	REGISTER_LUA_CFUNC(GetFacingFromHeading);

	REGISTER_LUA_CFUNC(GetSideData);

	REGISTER_LUA_CFUNC(GetAllyTeamStartBox);
	REGISTER_LUA_CFUNC(GetTeamStartPosition);
	REGISTER_LUA_CFUNC(GetMapStartPositions);

	REGISTER_LUA_CFUNC(GetPlayerList);
	REGISTER_LUA_CFUNC(GetTeamList);
	REGISTER_LUA_CFUNC(GetAllyTeamList);

	REGISTER_LUA_CFUNC(GetPlayerInfo);
	REGISTER_LUA_CFUNC(GetPlayerControlledUnit);
	REGISTER_LUA_CFUNC(GetAIInfo);

	REGISTER_LUA_CFUNC(GetTeamInfo);
	REGISTER_LUA_CFUNC(GetTeamAllyTeamID);
	REGISTER_LUA_CFUNC(GetTeamResources);
	REGISTER_LUA_CFUNC(GetTeamUnitStats);
	REGISTER_LUA_CFUNC(GetTeamResourceStats);
	REGISTER_LUA_CFUNC(GetTeamDamageStats);
	REGISTER_LUA_CFUNC(GetTeamRulesParam);
	REGISTER_LUA_CFUNC(GetTeamRulesParams);
	REGISTER_LUA_CFUNC(GetTeamStatsHistory);
	REGISTER_LUA_CFUNC(GetTeamLuaAI);
	REGISTER_LUA_CFUNC(GetTeamMaxUnits);

	REGISTER_LUA_CFUNC(GetAllyTeamInfo);
	REGISTER_LUA_CFUNC(AreTeamsAllied);
	REGISTER_LUA_CFUNC(ArePlayersAllied);

	REGISTER_LUA_CFUNC(ValidUnitID);
	REGISTER_LUA_CFUNC(ValidFeatureID);

	REGISTER_LUA_CFUNC(GetAllUnits);
	REGISTER_LUA_CFUNC(GetTeamUnits);

	REGISTER_LUA_CFUNC(GetTeamUnitsSorted);
	REGISTER_LUA_CFUNC(GetTeamUnitsCounts);
	REGISTER_LUA_CFUNC(GetTeamUnitsByDefs);
	REGISTER_LUA_CFUNC(GetTeamUnitDefCount);
	REGISTER_LUA_CFUNC(GetTeamUnitCount);

	REGISTER_LUA_CFUNC(GetUnitsInRectangle);
	REGISTER_LUA_CFUNC(GetUnitsInBox);
	REGISTER_LUA_CFUNC(GetUnitsInPlanes);
	REGISTER_LUA_CFUNC(GetUnitsInSphere);
	REGISTER_LUA_CFUNC(GetUnitsInCylinder);

	REGISTER_LUA_CFUNC(GetUnitArrayCentroid);
	REGISTER_LUA_CFUNC(GetUnitMapCentroid);

	REGISTER_LUA_CFUNC(GetFeaturesInRectangle);
	REGISTER_LUA_CFUNC(GetFeaturesInSphere);
	REGISTER_LUA_CFUNC(GetFeaturesInCylinder);

	REGISTER_LUA_CFUNC(GetAllProjectiles);
	REGISTER_LUA_CFUNC(GetProjectilesInRectangle);
	REGISTER_LUA_CFUNC(GetProjectilesInSphere);

	REGISTER_LUA_CFUNC(GetUnitNearestAlly);
	REGISTER_LUA_CFUNC(GetUnitNearestEnemy);
	REGISTER_LUA_CFUNC(GetClosestEnemyUnit);

	REGISTER_LUA_CFUNC(GetUnitTooltip);
	REGISTER_LUA_CFUNC(GetUnitDefID);
	REGISTER_LUA_CFUNC(GetUnitMoveDefID);
	REGISTER_LUA_CFUNC(GetUnitTeam);
	REGISTER_LUA_CFUNC(GetUnitAllyTeam);
	REGISTER_LUA_CFUNC(GetUnitNeutral);
	REGISTER_LUA_CFUNC(GetUnitHealth);
	REGISTER_LUA_CFUNC(GetUnitIsDead);
	REGISTER_LUA_CFUNC(GetUnitIsStunned);
	REGISTER_LUA_CFUNC(GetUnitIsBeingBuilt);
	REGISTER_LUA_CFUNC(GetUnitResources);
	REGISTER_LUA_CFUNC(GetUnitStorage);
	REGISTER_LUA_CFUNC(GetUnitCosts);
	REGISTER_LUA_CFUNC(GetUnitCostTable);
	REGISTER_LUA_CFUNC(GetUnitMetalExtraction);
	REGISTER_LUA_CFUNC(GetUnitMaxRange);
	REGISTER_LUA_CFUNC(GetUnitExperience);
	REGISTER_LUA_CFUNC(GetUnitStates);
	REGISTER_LUA_CFUNC(GetUnitArmored);
	REGISTER_LUA_CFUNC(GetUnitIsActive);
	REGISTER_LUA_CFUNC(GetUnitIsCloaked);
	REGISTER_LUA_CFUNC(GetUnitSeismicSignature);
	REGISTER_LUA_CFUNC(GetUnitLeavesGhost);
	REGISTER_LUA_CFUNC(GetUnitSelfDTime);
	REGISTER_LUA_CFUNC(GetUnitStockpile);
	REGISTER_LUA_CFUNC(GetUnitSensorRadius);
	REGISTER_LUA_CFUNC(GetUnitPosErrorParams);
	REGISTER_LUA_CFUNC(GetUnitHeight);
	REGISTER_LUA_CFUNC(GetUnitRadius);
	REGISTER_LUA_CFUNC(GetUnitBuildeeRadius);
	REGISTER_LUA_CFUNC(GetUnitMass);
	REGISTER_LUA_CFUNC(GetUnitPosition);
	REGISTER_LUA_CFUNC(GetUnitBasePosition);
	REGISTER_LUA_CFUNC(GetUnitVectors);
	REGISTER_LUA_CFUNC(GetUnitRotation);
	REGISTER_LUA_CFUNC(GetUnitDirection);
	REGISTER_LUA_CFUNC(GetUnitHeading);
	REGISTER_LUA_CFUNC(GetUnitVelocity);
	REGISTER_LUA_CFUNC(GetUnitBuildFacing);
	REGISTER_LUA_CFUNC(GetUnitIsBuilding);
	REGISTER_LUA_CFUNC(GetUnitWorkerTask);
	REGISTER_LUA_CFUNC(GetUnitEffectiveBuildRange);
	REGISTER_LUA_CFUNC(GetUnitCurrentBuildPower);
	REGISTER_LUA_CFUNC(GetUnitHarvestStorage);
	REGISTER_LUA_CFUNC(GetUnitBuildParams);
	REGISTER_LUA_CFUNC(GetUnitInBuildStance);
	REGISTER_LUA_CFUNC(GetUnitNanoPieces);
	REGISTER_LUA_CFUNC(GetUnitTransporter);
	REGISTER_LUA_CFUNC(GetUnitIsTransporting);
	REGISTER_LUA_CFUNC(GetUnitShieldState);
	REGISTER_LUA_CFUNC(GetUnitFlanking);
	REGISTER_LUA_CFUNC(GetUnitWeaponState);
	REGISTER_LUA_CFUNC(GetUnitWeaponDamages);
	REGISTER_LUA_CFUNC(GetUnitWeaponVectors);
	REGISTER_LUA_CFUNC(GetUnitWeaponTryTarget);
	REGISTER_LUA_CFUNC(GetUnitWeaponTestTarget);
	REGISTER_LUA_CFUNC(GetUnitWeaponTestRange);
	REGISTER_LUA_CFUNC(GetUnitWeaponHaveFreeLineOfFire);
	REGISTER_LUA_CFUNC(GetUnitWeaponCanFire);
	REGISTER_LUA_CFUNC(GetUnitWeaponTarget);
	REGISTER_LUA_CFUNC(GetUnitTravel);
	REGISTER_LUA_CFUNC(GetUnitFuel);
	REGISTER_LUA_CFUNC(GetUnitEstimatedPath);
	REGISTER_LUA_CFUNC(GetUnitLastAttacker);
	REGISTER_LUA_CFUNC(GetUnitLastAttackedPiece);
	REGISTER_LUA_CFUNC(GetUnitLosState);
	REGISTER_LUA_CFUNC(GetUnitSeparation);
	REGISTER_LUA_CFUNC(GetUnitFeatureSeparation);
	REGISTER_LUA_CFUNC(GetUnitDefDimensions);
	REGISTER_LUA_CFUNC(GetUnitCollisionVolumeData);
	REGISTER_LUA_CFUNC(GetUnitPieceCollisionVolumeData);

	REGISTER_LUA_CFUNC(GetUnitBlocking);
	REGISTER_LUA_CFUNC(GetUnitMoveTypeData);

	REGISTER_LUA_CFUNC(GetUnitCommandCount);
	REGISTER_LUA_CFUNC(GetUnitCommands);
	REGISTER_LUA_CFUNC(GetUnitCurrentCommand);
	REGISTER_LUA_CFUNC(GetFactoryCounts);
	REGISTER_LUA_CFUNC(GetFactoryCommandCount);
	REGISTER_LUA_CFUNC(GetFactoryCommands);

	REGISTER_LUA_CFUNC(GetFactoryBuggerOff);

	REGISTER_LUA_CFUNC(GetCommandQueue);
	REGISTER_LUA_CFUNC(GetFullBuildQueue);
	REGISTER_LUA_CFUNC(GetRealBuildQueue);

	REGISTER_LUA_CFUNC(GetUnitCmdDescs);
	REGISTER_LUA_CFUNC(FindUnitCmdDesc);

	REGISTER_LUA_CFUNC(GetUnitRulesParam);
	REGISTER_LUA_CFUNC(GetUnitRulesParams);

	REGISTER_LUA_CFUNC(GetCEGID);

	REGISTER_LUA_CFUNC(GetAllFeatures);
	REGISTER_LUA_CFUNC(GetFeatureDefID);
	REGISTER_LUA_CFUNC(GetFeatureTeam);
	REGISTER_LUA_CFUNC(GetFeatureAllyTeam);
	REGISTER_LUA_CFUNC(GetFeatureHealth);
	REGISTER_LUA_CFUNC(GetFeatureHeight);
	REGISTER_LUA_CFUNC(GetFeatureRadius);
	REGISTER_LUA_CFUNC(GetFeaturePosition);
	REGISTER_LUA_CFUNC(GetFeatureMass);
	REGISTER_LUA_CFUNC(GetFeatureRotation);
	REGISTER_LUA_CFUNC(GetFeatureDirection);
	REGISTER_LUA_CFUNC(GetFeatureVelocity);
	REGISTER_LUA_CFUNC(GetFeatureHeading);
	REGISTER_LUA_CFUNC(GetFeatureResources);
	REGISTER_LUA_CFUNC(GetFeatureBlocking);
	REGISTER_LUA_CFUNC(GetFeatureNoSelect);
	REGISTER_LUA_CFUNC(GetFeatureResurrect);

	REGISTER_LUA_CFUNC(GetFeatureLastAttackedPiece);
	REGISTER_LUA_CFUNC(GetFeatureCollisionVolumeData);
	REGISTER_LUA_CFUNC(GetFeaturePieceCollisionVolumeData);
	REGISTER_LUA_CFUNC(GetFeatureSeparation);

	REGISTER_LUA_CFUNC(GetFeatureFireTime);
	REGISTER_LUA_CFUNC(GetFeatureSmokeTime);

	REGISTER_LUA_CFUNC(GetFeatureRulesParam);
	REGISTER_LUA_CFUNC(GetFeatureRulesParams);

	REGISTER_LUA_CFUNC(GetProjectilePosition);
	REGISTER_LUA_CFUNC(GetProjectileDirection);
	REGISTER_LUA_CFUNC(GetProjectileVelocity);
	REGISTER_LUA_CFUNC(GetProjectileGravity);
	REGISTER_LUA_CFUNC(GetProjectileTarget);
	REGISTER_LUA_CFUNC(GetProjectileIsIntercepted);
	REGISTER_LUA_CFUNC(GetProjectileTimeToLive);
	REGISTER_LUA_CFUNC(GetProjectileOwnerID);
	REGISTER_LUA_CFUNC(GetProjectileTeamID);
	REGISTER_LUA_CFUNC(GetProjectileAllyTeamID);
	REGISTER_LUA_CFUNC(GetProjectileType);
	REGISTER_LUA_CFUNC(GetProjectileDefID);
	REGISTER_LUA_CFUNC(GetProjectileDamages);
	REGISTER_LUA_CFUNC(GetPieceProjectileParams);
	REGISTER_LUA_CFUNC(GetPieceProjectileName);

	REGISTER_LUA_CFUNC(IsPosInMap);
	REGISTER_LUA_CFUNC(GetWaterPlaneLevel);
	REGISTER_LUA_CFUNC(GetWaterLevel);
	REGISTER_LUA_CFUNC(GetGroundHeight);
	REGISTER_LUA_CFUNC(GetGroundOrigHeight);
	REGISTER_LUA_CFUNC(GetGroundNormal);
	REGISTER_LUA_CFUNC(GetGroundInfo);
	REGISTER_LUA_CFUNC(GetGroundBlocked);
	REGISTER_LUA_CFUNC(GetGroundExtremes);
	REGISTER_LUA_CFUNC(GetTerrainTypeData);
	REGISTER_LUA_CFUNC(GetGrass);

	REGISTER_LUA_CFUNC(GetSmoothMeshHeight);

	REGISTER_LUA_CFUNC(TestMoveOrder);
	REGISTER_LUA_CFUNC(TestBuildOrder);
	REGISTER_LUA_CFUNC(TestBuildOrderOverlap);
	REGISTER_LUA_CFUNC(Pos2BuildPos);
	REGISTER_LUA_CFUNC(ClosestBuildPos);

	REGISTER_LUA_CFUNC(GetPositionLosState);
	REGISTER_LUA_CFUNC(IsPosInLos);
	REGISTER_LUA_CFUNC(IsPosInRadar);
	REGISTER_LUA_CFUNC(IsPosInAirLos);
	REGISTER_LUA_CFUNC(IsUnitInLos);
	REGISTER_LUA_CFUNC(IsUnitInAirLos);
	REGISTER_LUA_CFUNC(IsUnitInRadar);
	REGISTER_LUA_CFUNC(IsUnitInJammer);
	REGISTER_LUA_CFUNC(GetClosestValidPosition);

	REGISTER_LUA_CFUNC(GetModelRootPiece);
	REGISTER_LUA_CFUNC(GetModelPieceList);
	REGISTER_LUA_CFUNC(GetModelPieceMap);
	REGISTER_LUA_CFUNC(GetUnitRootPiece);
	REGISTER_LUA_CFUNC(GetUnitPieceMap);
	REGISTER_LUA_CFUNC(GetUnitPieceList);
	REGISTER_LUA_CFUNC(GetUnitPieceInfo);
	REGISTER_LUA_CFUNC(GetUnitPiecePosition);
	REGISTER_LUA_CFUNC(GetUnitPieceDirection);
	REGISTER_LUA_CFUNC(GetUnitPiecePosDir);
	REGISTER_LUA_CFUNC(GetUnitPieceMatrix);
	REGISTER_LUA_CFUNC(GetUnitScriptPiece);
	REGISTER_LUA_CFUNC(GetUnitScriptNames);

	REGISTER_LUA_CFUNC(GetFeatureRootPiece);
	REGISTER_LUA_CFUNC(GetFeaturePieceMap);
	REGISTER_LUA_CFUNC(GetFeaturePieceList);
	REGISTER_LUA_CFUNC(GetFeaturePieceInfo);
	REGISTER_LUA_CFUNC(GetFeaturePiecePosition);
	REGISTER_LUA_CFUNC(GetFeaturePieceDirection);
	REGISTER_LUA_CFUNC(GetFeaturePiecePosDir);
	REGISTER_LUA_CFUNC(GetFeaturePieceMatrix);

	REGISTER_LUA_CFUNC(TraceRayGroundInDirection);
	REGISTER_LUA_CFUNC(TraceRayGroundBetweenPositions);
	REGISTER_LUA_CFUNC(TraceRayInDirection);
	REGISTER_LUA_CFUNC(TraceRayBetweenPositions);

	REGISTER_LUA_CFUNC(GetRadarErrorParams);

	if (!LuaMetalMap::PushReadEntries(L))
		return false;

	if (!LuaPathFinder::PushEntries(L))
		return false;

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Access helpers
//

#if 0
static inline bool IsPlayerSynced(lua_State* L, const CPlayer* player)
{
	const bool syncedHandle = CLuaHandle::GetHandleSynced(L);
	const bool onlyFromDemo = syncedHandle && gameSetup->hostDemo;
	return (!onlyFromDemo || player->isFromDemo);
}
#endif

static inline bool IsPlayerUnsynced(lua_State* L, const CPlayer* player)
{
	const bool syncedHandle = CLuaHandle::GetHandleSynced(L);
	const bool onlyFromDemo = syncedHandle && gameSetup->hostDemo;

	return (onlyFromDemo && !player->isFromDemo);
}



/******************************************************************************/
/******************************************************************************/

static int GetSolidObjectLastHitPiece(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;
	if (o->hitModelPieces[true] == nullptr)
		return 0;

	const LocalModelPiece* lmp = o->hitModelPieces[true];
	const S3DModelPiece* omp = lmp->original;

	if (lua_isboolean(L, 1) && lua_toboolean(L, 1)) {
		lua_pushnumber(L, lmp->GetLModelPieceIndex() + 1);
	} else {
		lua_pushsstring(L, omp->name);
	}

	lua_pushnumber(L, o->pieceHitFrames[true]);
	return 2;
}

static int PushPieceCollisionVolumeData(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	return LuaUtils::PushColVolData(L, lmp->GetCollisionVolume());
}


static int PushTerrainTypeData(lua_State* L, const CMapInfo::TerrainType* tt, bool groundInfo) {
	lua_pushinteger(L, tt - &mapInfo->terrainTypes[0]); // index
	lua_pushsstring(L, tt->name);

	if (groundInfo) {
		assert(lua_isnumber(L, 1));
		assert(lua_isnumber(L, 2));
		// WTF is this still doing here?
		LuaMetalMap::GetMetalAmount(L);
	}

	lua_pushnumber(L, tt->hardness);
	lua_pushnumber(L, tt->tankSpeed);
	lua_pushnumber(L, tt->kbotSpeed);
	lua_pushnumber(L, tt->hoverSpeed);
	lua_pushnumber(L, tt->shipSpeed);
	lua_pushboolean(L, tt->receiveTracks);
	return (8 + groundInfo);
}

static int GetWorldObjectVelocity(lua_State* L, const CWorldObject* o)
{
	if (o == nullptr)
		return 0;

	lua_pushnumber(L, o->speed.x);
	lua_pushnumber(L, o->speed.y);
	lua_pushnumber(L, o->speed.z);
	lua_pushnumber(L, o->speed.w);
	return 4;
}

static int GetSolidObjectMass(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	lua_pushnumber(L, o->mass);

	return 1;
}

static int GetSolidObjectPosition(lua_State* L, const CSolidObject* o, bool isFeature)
{
	if (o == nullptr)
		return 0;

	float3 errorVec;

	// no error for features
	if (!isFeature && !LuaUtils::IsAllyUnit(L, static_cast<const CUnit*>(o)))
		errorVec = static_cast<const CUnit*>(o)->GetLuaErrorVector(CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleFullRead(L));

	// NOTE:
	//   must be called before any pushing to the stack, else
	//   in case of noneornil it will read the pushed items.
	const bool returnMidPos = luaL_optboolean(L, 2, false);
	const bool returnAimPos = luaL_optboolean(L, 3, false);

	// base-position
	lua_pushnumber(L, o->pos.x + errorVec.x);
	lua_pushnumber(L, o->pos.y + errorVec.y);
	lua_pushnumber(L, o->pos.z + errorVec.z);

	if (returnMidPos) {
		lua_pushnumber(L, o->midPos.x + errorVec.x);
		lua_pushnumber(L, o->midPos.y + errorVec.y);
		lua_pushnumber(L, o->midPos.z + errorVec.z);
	}
	if (returnAimPos) {
		lua_pushnumber(L, o->aimPos.x + errorVec.x);
		lua_pushnumber(L, o->aimPos.y + errorVec.y);
		lua_pushnumber(L, o->aimPos.z + errorVec.z);
	}

	return (3 + (3 * returnMidPos) + (3 * returnAimPos));
}

static int GetSolidObjectRotation(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const CMatrix44f& matrix = o->GetTransformMatrix(CLuaHandle::GetHandleSynced(L));
	const float3 angles = matrix.GetEulerAnglesLftHand();

	assert(matrix.IsOrthoNormal());

	lua_pushnumber(L, angles[CMatrix44f::ANGLE_P]);
	lua_pushnumber(L, angles[CMatrix44f::ANGLE_Y]);
	lua_pushnumber(L, angles[CMatrix44f::ANGLE_R]);
	return 3;
}

static int GetSolidObjectBlocking(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	lua_pushboolean(L, o->HasPhysicalStateBit(CSolidObject::PSTATE_BIT_BLOCKING));
	lua_pushboolean(L, o->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS));
	lua_pushboolean(L, o->HasCollidableStateBit(CSolidObject::CSTATE_BIT_PROJECTILES ));
	lua_pushboolean(L, o->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS ));

	lua_pushboolean(L, o->crushable);
	lua_pushboolean(L, o->blockEnemyPushing);
	lua_pushboolean(L, o->blockHeightChanges);

	return 7;
}




/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "[%s] unitID (arg #%d) not a number\n", caller, index);
		return nullptr;
	}

	return (unitHandler.GetUnit(lua_toint(L, index)));
}

static inline const CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	const CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	// include the vistest for LuaUnsyncedRead
	if (!LuaUtils::IsUnitVisible(L, unit))
		return nullptr;

	return unit;
}

static inline const CUnit* ParseAllyUnit(lua_State* L, const char* caller, int index)
{
	const CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!LuaUtils::IsAllyUnit(L, unit))
		return nullptr;

	return unit;
}

static inline const CUnit* ParseInLosUnit(lua_State* L, const char* caller, int index)
{
	const CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!LuaUtils::IsUnitInLos(L, unit))
		return nullptr;

	return unit;
}


static inline const CUnit* ParseTypedUnit(lua_State* L, const char* caller, int index)
{
	const CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!LuaUtils::IsUnitTyped(L, unit))
		return nullptr;

	return unit;
}


static const CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "[%s] featureID (arg #%d) not a number\n", caller, index);
		return nullptr;
	}

	const CFeature* feature = featureHandler.GetFeature(lua_toint(L, index));

	if (feature == nullptr)
		return nullptr;

	// include the vistest for LuaUnsyncedRead
	if (!LuaUtils::IsFeatureVisible(L, feature))
		return nullptr;

	return feature;
}


static const CProjectile* ParseProjectile(lua_State* L, const char* caller, int index)
{
	const CProjectile* p = projectileHandler.GetProjectileBySyncedID(luaL_checkint(L, index));

	if (p == nullptr)
		return nullptr;

	if (!LuaUtils::IsProjectileVisible(L, p))
		return nullptr;

	return p;
}


static inline const CTeam* ParseTeam(lua_State* L, const char* caller, int index)
{
	const int teamID = luaL_checkint(L, index);

	if (!teamHandler.IsValidTeam(teamID))
		luaL_error(L, "Bad teamID in %s\n", caller);

	return teamHandler.Team(teamID);
}


/******************************************************************************/

static int PushRulesParams(lua_State* L, const char* caller,
                          const LuaRulesParams::Params& params,
                          const int losStatus)
{
	lua_createtable(L, 0, params.size());

	for (const auto& it: params) {
		const std::string& name = it.first;
		const LuaRulesParams::Param& param = it.second;
		if (!(param.los & losStatus))
			continue;

		std::visit ([L, &name](auto&& value) {
			using T = std::decay_t <decltype(value)>;
			if constexpr (std::is_same_v <T, float>)
				LuaPushNamedNumber(L, name, value);
			else if constexpr (std::is_same_v <T, bool>)
				LuaPushNamedBool(L, name, value);
			else if constexpr (std::is_same_v <T, std::string>)
				LuaPushNamedString(L, name, value);
		}, param.value);
	}

	return 1;
}


static int GetRulesParam(lua_State* L, const char* caller, int index,
                          const LuaRulesParams::Params& params,
                          const int& losStatus)
{
	const std::string& key = luaL_checkstring(L, index);
	const auto it = params.find(key);
	if (it == params.end())
		return 0;

	const LuaRulesParams::Param& param = it->second;
	if (!(param.los & losStatus))
		return 0;

	std::visit ([L](auto&& value) {
		using T = std::decay_t <decltype(value)>;
		if constexpr (std::is_same_v <T, float>)
			lua_pushnumber(L, value);
		else if constexpr (std::is_same_v <T, bool>)
			lua_pushboolean(L, value);
		else if constexpr (std::is_same_v <T, std::string>)
			lua_pushsstring(L, value);
	}, param.value);

	return 1;
}


/******************************************************************************
 * Game States
 * @section gamestates
******************************************************************************/

/***
 *
 * @function Spring.IsCheatingEnabled
 *
 * @return boolean enabled
 */
int LuaSyncedRead::IsCheatingEnabled(lua_State* L)
{
	lua_pushboolean(L, gs->cheatEnabled);
	return 1;
}


/***
 *
 * @function Spring.IsGodModeEnabled
 *
 * @return boolean enabled
 */
int LuaSyncedRead::IsGodModeEnabled(lua_State* L)
{
	lua_pushboolean(L, gs->godMode != 0);
	lua_pushboolean(L, (gs->godMode & GODMODE_ATC_BIT) != 0);
	lua_pushboolean(L, (gs->godMode & GODMODE_ETC_BIT) != 0);
	return 3;
}


/***
 *
 * @function Spring.IsDevLuaEnabled
 *
 * @return boolean enabled
 */
int LuaSyncedRead::IsDevLuaEnabled(lua_State* L)
{
	lua_pushboolean(L, CLuaHandle::GetDevMode());
	return 1;
}


/***
 *
 * @function Spring.IsEditDefsEnabled
 *
 * @return boolean enabled
 */
int LuaSyncedRead::IsEditDefsEnabled(lua_State* L)
{
	lua_pushboolean(L, gs->editDefsEnabled);
	return 1;
}


/***
 *
 * @function Spring.IsNoCostEnabled
 *
 * @return boolean enabled
 */
int LuaSyncedRead::IsNoCostEnabled(lua_State* L)
{
	lua_pushboolean(L, unitDefHandler->GetNoCost());
	return 1;
}


/***
 *
 * @function Spring.GetGlobalLos
 *
 * @param teamID TeamID?
 *
 * @return boolean enabled
 */
int LuaSyncedRead::GetGlobalLos(lua_State* L)
{
	const int allyTeam = luaL_optint(L, 1, CLuaHandle::GetHandleReadAllyTeam(L));
	if (!teamHandler.IsValidAllyTeam(allyTeam))
		return 0;

	lua_pushboolean(L, losHandler->GetGlobalLOS(allyTeam));
	return 1;
}


/***
 *
 * @function Spring.AreHelperAIsEnabled
 *
 * @return boolean enabled
 */
int LuaSyncedRead::AreHelperAIsEnabled(lua_State* L)
{
	lua_pushboolean(L, !gs->noHelperAIs);
	return 1;
}


/***
 *
 * @function Spring.FixedAllies
 *
 * @return boolean? enabled
 */
int LuaSyncedRead::FixedAllies(lua_State* L)
{
	lua_pushboolean(L, gameSetup->fixedAllies);
	return 1;
}


/***
 *
 * @function Spring.IsGameOver
 *
 * @return boolean isGameOver
 */
int LuaSyncedRead::IsGameOver(lua_State* L)
{
	if (game == nullptr)
		return 0;

	lua_pushboolean(L, game->IsGameOver());
	return 1;
}


/******************************************************************************
 * Speed/Time
 * @section speedtime
******************************************************************************/


/***
 *
 * @function Spring.GetGameFrame
 *
 * @return integer t1 frameNum % dayFrames
 * @return integer t2 frameNum / dayFrames
 */
int LuaSyncedRead::GetGameFrame(lua_State* L)
{
	const int simFrames = gs->GetLuaSimFrame();
	const int dayFrames = GAME_SPEED * (24 * 60 * 60);

	lua_pushnumber(L, simFrames % dayFrames);
	lua_pushnumber(L, simFrames / dayFrames);
	return 2;
}


/***
 *
 * @function Spring.GetGameSeconds
 *
 * @return number seconds
 */
int LuaSyncedRead::GetGameSeconds(lua_State* L)
{
	lua_pushnumber(L, gs->GetLuaSimFrame() * INV_GAME_SPEED);
	return 1;
}


/******************************************************************************
 * Environment
 * @section environment
******************************************************************************/


/***
 *
 * @function Spring.GetTidal
 *
 * @return number tidalStrength
 */
int LuaSyncedRead::GetTidal(lua_State* L)
{
	lua_pushnumber(L, envResHandler.GetCurrentTidalStrength());
	return 1;
}


/***
 *
 * @function Spring.GetWind
 *
 * @return number windSpeedX
 * @return number windSpeedY (Always 0)
 * @return number windSpeedZ
 * @return number windStrength (Length of windSpeed)
 * @return number windDirX (Normalized windSpeed)
 * @return number windDirY (Always 0)
 * @return number windDirZ
 */
int LuaSyncedRead::GetWind(lua_State* L)
{
	lua_pushnumber(L, envResHandler.GetCurrentWindVec().x);
	lua_pushnumber(L, envResHandler.GetCurrentWindVec().y);
	lua_pushnumber(L, envResHandler.GetCurrentWindVec().z);
	lua_pushnumber(L, envResHandler.GetCurrentWindStrength());
	lua_pushnumber(L, envResHandler.GetCurrentWindDir().x);
	lua_pushnumber(L, envResHandler.GetCurrentWindDir().y);
	lua_pushnumber(L, envResHandler.GetCurrentWindDir().z);
	return 7;
}


/******************************************************************************
 * Rules/Params
 *
 * @section environment
 *
 * The following functions allow to save data per game, team and unit.
 * The advantage of it is that it can be read from anywhere (even from LuaUI and AIs!)
******************************************************************************/

/***
 * @class RulesParams : table<string, integer>
 */

/***
 *
 * @function Spring.GetGameRulesParams
 *
 * @return RulesParams rulesParams map with rules names as key and values as values
 */
int LuaSyncedRead::GetGameRulesParams(lua_State* L)
{
	// always readable for all
	return PushRulesParams(L, __func__, CSplitLuaHandle::GetGameParams(), LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK);
}


/***
 *
 * @function Spring.GetTeamRulesParams
 *
 * @param teamID TeamID
 *
 * @return RulesParams rulesParams map with rules names as key and values as values
 */
int LuaSyncedRead::GetTeamRulesParams(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr || game == nullptr)
		return 0;

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC;

	if (LuaUtils::IsAlliedTeam(L, team->teamNum) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler.AlliedTeams(team->teamNum, CLuaHandle::GetHandleReadTeam(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}

	return PushRulesParams(L, __func__, team->modParams, losMask);
}

/***
 *
 * @function Spring.GetPlayerRulesParams
 *
 * @param playerID PlayerID
 *
 * @return RulesParams rulesParams map with rules names as key and values as values
 */
int LuaSyncedRead::GetPlayerRulesParams(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	if (!playerHandler.IsValidPlayer(playerID))
		return 0;

	const auto player = playerHandler.Player(playerID);
	if (player == nullptr || IsPlayerUnsynced(L, player))
		return 0;

	int losMask;
	if (CLuaHandle::GetHandleSynced(L)) {
		/* We're using GetHandleSynced even though other RulesParams don't,
		 * because handles don't have the concept of "being a player" while
		 * they do have the concept of "being a team" via `Script.CallAsTeam`.
		 * So there is no way to limit their perspective in a good way yet. */
		losMask = LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;

	} else if (playerID == gu->myPlayerNum || CLuaHandle::GetHandleFullRead(L) || game->IsGameOver()) {
		/* The FullRead check is not redundant, for example
		 * `/specfullview 1` is not synced but has full read. */
		losMask = LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;

	} else {
		/* Currently private rulesparams can only be read by that player, not
		 * even the other players on their team (commsharing, not allyteam).
		 * This is purposefully different from how other rules params work as
		 * perhaps games where you switch teams often enough to warrant Player
		 * rules params instead of Team may also want some secrecy.
		 *
		 * Also, perhaps the 'allied' visibility level could be made to grant
		 * visibility to the team/allyteam, but that would require some thought
		 * since normally it means 'different allyteam with dynamic alliance'. */
		losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;
	}

	return PushRulesParams(L, __func__, player->modParams, losMask);
}


static int GetUnitRulesParamLosMask(lua_State* L, const CUnit* unit)
{
	if (LuaUtils::IsAllyUnit(L, unit) || game->IsGameOver())
		return LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	if (teamHandler.AlliedTeams(unit->team, CLuaHandle::GetHandleReadTeam(L)))
		return LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0)
		return LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	const auto losStatus = unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)];
	if (losStatus & LOS_INLOS)
		return LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
	if (losStatus & (LOS_PREVLOS | LOS_CONTRADAR))
		return LuaRulesParams::RULESPARAMLOS_TYPED_MASK;
	if (losStatus & LOS_INRADAR)
		return LuaRulesParams::RULESPARAMLOS_INRADAR_MASK;

	return LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;
}


/***
 *
 * @function Spring.GetUnitRulesParams
 *
 * @param unitID UnitID
 *
 * @return RulesParams rulesParams map with rules names as key and values as values
 */
int LuaSyncedRead::GetUnitRulesParams(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr || game == nullptr)
		return 0;

	return PushRulesParams(L, __func__, unit->modParams, GetUnitRulesParamLosMask(L, unit));
}


/***
 *
 * @function Spring.GetFeatureRulesParams
 *
 * @param featureID FeatureID
 *
 * @return RulesParams rulesParams map with rules names as key and values as values
 */
int LuaSyncedRead::GetFeatureRulesParams(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (LuaUtils::IsAlliedAllyTeam(L, feature->allyteam) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler.AlliedTeams(feature->team, CLuaHandle::GetHandleReadTeam(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}
	else if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		//! NoAccessTeam
	}
	else if (LuaUtils::IsFeatureVisible(L, feature)) {
		losMask |= LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
	}

	const LuaRulesParams::Params&  params = feature->modParams;

	return PushRulesParams(L, __func__, params, losMask);
}


/***
 *
 * @function Spring.GetGameRulesParam
 *
 * @param name string rules-parameter key (only this argument is read)
 *
 * @return number|boolean|string|nil value
 */
int LuaSyncedRead::GetGameRulesParam(lua_State* L)
{
	// always readable for all
	return GetRulesParam(L, __func__, 1, CSplitLuaHandle::GetGameParams(), LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK);
}


/***
 *
 * @function Spring.GetTeamRulesParam
 *
 * @param teamID TeamID
 * @param ruleRef number|string the rule index or name
 *
 * @return number|string|nil value
 */
int LuaSyncedRead::GetTeamRulesParam(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr || game == nullptr)
		return 0;

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC;

	if (LuaUtils::IsAlliedTeam(L, team->teamNum) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler.AlliedTeams(team->teamNum, CLuaHandle::GetHandleReadTeam(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}

	return GetRulesParam(L, __func__, 2, team->modParams, losMask);
}


/***
 *
 * @function Spring.GetPlayerRulesParam
 *
 * @param playerID PlayerID
 * @param ruleRef number|string the rule index or name
 *
 * @return number|string|nil value
 */
int LuaSyncedRead::GetPlayerRulesParam(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	if (!playerHandler.IsValidPlayer(playerID))
		return 0;

	const auto player = playerHandler.Player(playerID);
	if (player == nullptr || IsPlayerUnsynced(L, player))
		return 0;

	int losMask; // see `GetPlayerRulesParams` (plural) above for commentary
	if (CLuaHandle::GetHandleSynced(L))
		losMask = LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	else if (playerID == gu->myPlayerNum || CLuaHandle::GetHandleFullRead(L) || game->IsGameOver())
		losMask = LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	else
		losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	return GetRulesParam(L, __func__, 2, player->modParams, losMask);
}


/***
 *
 * @function Spring.GetUnitRulesParam
 *
 * @param unitID UnitID
 * @param ruleRef number|string the rule index or name
 *
 * @return number|string|nil value
 */
int LuaSyncedRead::GetUnitRulesParam(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr || game == nullptr)
		return 0;

	return GetRulesParam(L, __func__, 2, unit->modParams, GetUnitRulesParamLosMask(L, unit));
}


/***
 *
 * @function Spring.GetFeatureRulesParam
 *
 * @param featureID FeatureID
 * @param ruleRef number|string the rule index or name
 *
 * @return number|string|nil value
 */
int LuaSyncedRead::GetFeatureRulesParam(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (LuaUtils::IsAlliedAllyTeam(L, feature->allyteam) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler.AlliedTeams(feature->team, CLuaHandle::GetHandleReadTeam(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}
	else if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		//! NoAccessTeam
	}
	else if (LuaUtils::IsFeatureVisible(L, feature)) {
		losMask |= LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
	}

	return GetRulesParam(L, __func__, 2, feature->modParams, losMask);
}


/******************************************************************************
 * Mod and Map options
 *
 * @section modmapoptions
 *
 * *Warning*: boolean values are not transferred from C to Lua correctly.
 * For this reason the respective option has to be converted to a number
 * and checked accordingly via an IF statement as shown below:
 *
 *     if (tonumber(Spring.GetModOptions.exampleOption) == 1) then...end
 *
 * The following check therefore is insufficient!
 *
 *     if (Spring.GetModOptions.exampleOption) then...end
******************************************************************************/

static int PushSingleOption(lua_State* L, const auto &options)
{
	const std::string& key = luaL_checkstring(L, 1);

	const std::string* value = Recoil::map_try_get(options, key);
	if (value == nullptr)
		return 0;

	lua_pushsstring(L, *value);
	return 1;
}

static int PushAllOptions(lua_State* L, const auto &options)
{
	lua_createtable(L, 0, options.size());

	for (const auto& [key, value] : options) {
		lua_pushsstring(L, key);
		lua_pushsstring(L, value);
		lua_rawset(L, -3);
	}

	return 1;
}

/***
 *
 * @function Spring.GetMapOption
 *
 * @param mapOption string
 *
 * @return string value Value of `modOption`.
 * */
int LuaSyncedRead::GetMapOption(lua_State* L)
{
	return PushSingleOption(L, CGameSetup::GetMapOptions());
}
/***
 *
 * @function Spring.GetMapOptions
 *
 * @return table<string, string> mapOptions Table with options names as keys and values as values.
 */
int LuaSyncedRead::GetMapOptions(lua_State* L)
{
	return PushAllOptions(L, CGameSetup::GetMapOptions());
}


/***
 *
 * @function Spring.GetModOption
 *
 * @param modOption string
 *
 * @return string value Value of `modOption`.
 */
int LuaSyncedRead::GetModOption(lua_State* L)
{
	return PushSingleOption(L, CGameSetup::GetModOptions());
}


/***
 *
 * @function Spring.GetModOptions
 *
 * @return table<string, string> modOptions Table with options names as keys and values as values.
 */
int LuaSyncedRead::GetModOptions(lua_State* L)
{
	return PushAllOptions(L, CGameSetup::GetModOptions());
}


/******************************************************************************
 * Vectors
 *
 * @section vectors
******************************************************************************/


/***
 *
 * @function Spring.GetHeadingFromVector
 *
 * @param x number
 * @param z number
 *
 * @return integer heading
 */
int LuaSyncedRead::GetHeadingFromVector(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	const short int heading = ::GetHeadingFromVector(x, z);
	lua_pushnumber(L, heading);
	return 1;
}


/***
 *
 * @function Spring.GetVectorFromHeading
 *
 * @param heading integer
 *
 * @return number x
 * @return number z
 */
int LuaSyncedRead::GetVectorFromHeading(lua_State* L)
{
	const short int h = (short int)luaL_checknumber(L, 1);
	const float3& vec = ::GetVectorFromHeading(h);
	lua_pushnumber(L, vec.x);
	lua_pushnumber(L, vec.z);
	return 2;
}

/***
 * @function Spring.GetFacingFromHeading
 * @param heading integer
 * @return FacingInteger facing
 */
int LuaSyncedRead::GetFacingFromHeading(lua_State* L)
{
	lua_pushnumber(L, ::GetFacingFromHeading(luaL_checknumber(L, 1)));
	return 1;
}

/***
 * @function Spring.GetHeadingFromFacing
 * @param facing FacingInteger
 * @return integer heading
 */
int LuaSyncedRead::GetHeadingFromFacing(lua_State* L)
{
	lua_pushnumber(L, ::GetHeadingFromFacing(luaL_checknumber(L, 1)));
	return 1;
}


/******************************************************************************
 * Sides and Factions
 *
 * @section sidesfactions
******************************************************************************/


/***
 * Side spec
 *
 * @class SideSpec
 * @x_helper
 *
 * Returned when getting arrays of side specifications.
 *
 * @field sideName string Lowercase side name.
 * @field caseName string Original case side name.
 * @field startUnit string
 * @see Spring.GetSideData
 */

/***
 * @function Spring.GetSideData
 *
 * @param sideName string Case-insensitive side name.
 * @return string? startUnit
 * @return string? caseName Original case side name.
 */

/***
 * @function Spring.GetSideData
 * @param sideID integer
 * @return string? sideName Lowercase side name.
 * @return string? startUnit
 * @return string? caseName Original case side name.
 */

/***
 * @function Spring.GetSideData
 * @return SideSpec[] sideArray
 */
int LuaSyncedRead::GetSideData(lua_State* L)
{
	if (lua_israwstring(L, 1)) {
		const string sideName = lua_tostring(L, 1);
		const string& startUnit = sideParser.GetStartUnit(sideName);
		const string& caseName  = sideParser.GetCaseName(sideName);
		if (startUnit.empty())
			return 0;

		lua_pushsstring(L, startUnit);
		lua_pushsstring(L, caseName);
		return 2;
	}
	if (lua_israwnumber(L, 1)) {
		const unsigned int index = lua_toint(L, 1) - 1;
		if (!sideParser.ValidSide(index))
			return 0;

		lua_pushsstring(L, sideParser.GetSideName(index));
		lua_pushsstring(L, sideParser.GetStartUnit(index));
		lua_pushsstring(L, sideParser.GetCaseName(index));
		return 3;
	}
	{
		const unsigned int sideCount = sideParser.GetCount();
		lua_createtable(L, sideCount, 0);
		for (unsigned int i = 0; i < sideCount; i++) {
			lua_createtable(L, 0, 3); {
				LuaPushNamedString(L, "sideName",  sideParser.GetSideName(i));
				LuaPushNamedString(L, "caseName",  sideParser.GetCaseName(i));
				LuaPushNamedString(L, "startUnit", sideParser.GetStartUnit(i));
			}
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
}


/******************************************************************************
 * Teams
 *
 * @section Teams
******************************************************************************/


/***
 *
 * @function Spring.GetGaiaTeamID
 *
 * @return TeamID teamID
 */
int LuaSyncedRead::GetGaiaTeamID(lua_State* L)
{
	if (!gs->useLuaGaia)
		return 0;

	lua_pushnumber(L, teamHandler.GaiaTeamID());
	return 1;
}


/***
 *
 * @function Spring.GetAllyTeamStartBox
 *
 * @param allyID AllyTeamID
 *
 * @return number? xMin
 * @return number? zMin
 * @return number? xMax
 * @return number? zMax
 */
int LuaSyncedRead::GetAllyTeamStartBox(lua_State* L)
{
	const unsigned int allyTeamID = luaL_checkint(L, 1);

	if (!teamHandler.IsValidAllyTeam(allyTeamID))
		return 0;

	const AllyTeam& allyTeam = teamHandler.GetAllyTeam(allyTeamID);
	const float xmin = (mapDims.mapx * SQUARE_SIZE) * allyTeam.startRectLeft;
	const float zmin = (mapDims.mapy * SQUARE_SIZE) * allyTeam.startRectTop;
	const float xmax = (mapDims.mapx * SQUARE_SIZE) * allyTeam.startRectRight;
	const float zmax = (mapDims.mapy * SQUARE_SIZE) * allyTeam.startRectBottom;

	lua_pushnumber(L, xmin);
	lua_pushnumber(L, zmin);
	lua_pushnumber(L, xmax);
	lua_pushnumber(L, zmax);
	return 4;
}


/***
 *
 * @function Spring.GetTeamStartPosition
 *
 * @param teamID TeamID
 *
 * @return number? x
 * @return number? y
 * @return number? x
 */
int LuaSyncedRead::GetTeamStartPosition(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;
	if (!LuaUtils::IsAlliedTeam(L, team->teamNum))
		return 0;

	const float3& pos = team->GetStartPos();

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	lua_pushboolean(L, team->HasValidStartPos());
	return 4;
}

/***
 *
 * @function Spring.GetMapStartPositions
 * @return float3[] startPositions array of positions indexed by teamID
 */
int LuaSyncedRead::GetMapStartPositions(lua_State* L)
{
	lua_createtable(L, MAX_TEAMS, 0);
	gameSetup->LoadStartPositionsFromMap(MAX_TEAMS, [&](MapParser& mapParser, int teamNum) {
		float3 pos;

		if (!mapParser.GetStartPos(teamNum, pos))
			return false;

		lua_createtable(L, 3, 0);
		lua_pushnumber(L, pos.x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, pos.y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, pos.z); lua_rawseti(L, -2, 3);
		lua_rawseti(L, -2, teamNum); // [i] = {x,y,z}
		return true;
	});

	return 1;
}


/***
 *
 * @function Spring.GetAllyTeamList
 * @return AllyTeamID[] allyTeamIDs
 */
int LuaSyncedRead::GetAllyTeamList(lua_State* L)
{
	lua_createtable(L, teamHandler.ActiveAllyTeams(), 0);

	unsigned int allyCount = 1;

	for (int at = 0; at < teamHandler.ActiveAllyTeams(); at++) {
		lua_pushnumber(L, at);
		lua_rawseti(L, -2, allyCount++);
	}

	return 1;
}


/***
 * Get all team IDs.
 * 
 * @function Spring.GetTeamList
 * @param allyTeamID -1|nil (Default: `-1`) 
 * @return TeamID[] teamIDs List of team IDs.
 */

/***
 * Get team IDs in a specific ally team.
 * 
 * @function Spring.GetTeamList
 * @param allyTeamID AllyTeamID The ally team ID to filter teams by. A value less than 0 will return all teams.
 * @return TeamID[]? teamIDs List of team IDs or `nil` if `allyTeamID` is invalid.
 */
int LuaSyncedRead::GetTeamList(lua_State* L)
{
	int allyTeamID = -1;

	const int args = lua_gettop(L); // number of arguments

	// peek the first argument, but gracefully ignore the rest
	if (args >= 1) {
		allyTeamID = luaL_checkinteger(L, 1);
		if (!teamHandler.IsValidAllyTeam(allyTeamID))
			return 0;
	}

	lua_createtable(L, teamHandler.ActiveTeams(), 0);

	unsigned int teamCount = 1;

	for (int t = 0; t < teamHandler.ActiveTeams(); t++) {
		if (teamHandler.Team(t) == nullptr)
			continue;

		if ((allyTeamID >= 0) && (allyTeamID != teamHandler.AllyTeam(t)))
			continue;

		lua_pushnumber(L, t);
		lua_rawseti(L, -2, teamCount++);
	}

	return 1;
}


/***
 *
 * @function Spring.GetPlayerList
 * @param teamID TeamID? (Default: `-1`) to filter by when >= 0
 * @param active boolean? (Default: `false`) whether to filter only active teams
 * @return PlayerID[]? playerIDs List of playerIDs.
 */
int LuaSyncedRead::GetPlayerList(lua_State* L)
{
	int teamID = -1;
	bool active = false;

	if (lua_isnumber(L, 1)) {
		teamID = lua_toint(L, 1);
		active = lua_isboolean(L, 2)? lua_toboolean(L, 2): active;
	}
	else if (lua_isboolean(L, 1)) {
		active = lua_toboolean(L, 1);
		teamID = lua_isnumber(L, 2)? lua_toint(L, 2): teamID;
	}

	if (teamID >= teamHandler.ActiveTeams())
		return 0;

	lua_createtable(L, playerHandler.ActivePlayers(), 0);

	for (unsigned int p = 0, playerCount = 1; p < playerHandler.ActivePlayers(); p++) {
		const CPlayer* player = playerHandler.Player(p);

		if (player == nullptr)
			continue;

		if (IsPlayerUnsynced(L, player))
			continue;

		if (active && !player->active)
			continue;

		if (teamID >= 0) {
			// exclude specs for normal team ID's
			if (player->spectator)
				continue;
			if (player->team != teamID)
				continue;
		}

		lua_pushnumber(L, p);
		lua_rawseti(L, -2, playerCount++);
	}

	return 1;
}


/***
 *
 * @function Spring.GetTeamInfo
 * @param teamID TeamID
 * @param getTeamKeys boolean? (Default: `true`) whether to return the customTeamKeys table
 * @return TeamID? teamID
 * @return PlayerID leader
 * @return number isDead
 * @return number hasAI
 * @return string side
 * @return AllyTeamID allyTeam
 * @return number incomeMultiplier
 * @return table<string,string> customTeamKeys when getTeamKeys is true, otherwise nil
 */
int LuaSyncedRead::GetTeamInfo(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	const CTeam* team = teamHandler.Team(teamID);
	if (team == nullptr)
		return 0;

	// read before modifying stack
	const bool getTeamOpts = luaL_optboolean(L, 2, true);

	lua_pushnumber(L,  team->teamNum);
	lua_pushnumber(L,  team->GetLeader());
	lua_pushboolean(L, team->isDead);
	lua_pushboolean(L, skirmishAIHandler.HasSkirmishAIsInTeam(teamID));
	lua_pushstring(L, team->GetSideName());
	lua_pushnumber(L,  teamHandler.AllyTeam(team->teamNum));
	lua_pushnumber(L, team->GetIncomeMultiplier());

	if (getTeamOpts) {
		const TeamBase::customOpts& teamOpts(team->GetAllValues());

		lua_createtable(L, 0, teamOpts.size());

		for (const auto& pair: teamOpts) {
			lua_pushsstring(L, pair.first);
			lua_pushsstring(L, pair.second);
			lua_rawset(L, -3);
		}
	}

	return 7 + getTeamOpts;
}


/***
 *
 * @function Spring.GetTeamAllyTeamID
 * @param teamID TeamID
 * @return AllyTeamID? allyTeamID
 */
int LuaSyncedRead::GetTeamAllyTeamID(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	const CTeam* const team = teamHandler.Team(teamID);
	if (team == nullptr)
		return 0;

	lua_pushnumber(L, teamHandler.AllyTeam(team->teamNum));
	return 1;
}


/***
 *
 * @function Spring.GetTeamResources
 * @param teamID TeamID
 * @param resource ResourceName
 * @return number? currentLevel The current amount of the resource that the team has in storage at this moment
 * @return number storage       The maximum storage capacity for the resource.
 * @return number pull          The total amount of the resource that is being requested/used by all units and buildings per second, regardless of whether the resource is actually available.
 * @return number income        The total amount of the resource being generated per second from all sources (e.g., mines, generators, reclaiming, etc.).
 * @return number expense       The total amount of the resource actually being spent per second. This is the real consumption, which may be less than pull if there isn’t enough resource available.
 * @return number share         The fraction (0.0 to 1.0) of the storage that the team is sharing with allied teams. A value of 0.0 means 100% of storage is shared, while 1.0 means only any excess is shared.
 * @return number sent          The total amount of the resource that has actually been sent to allies (via sharing or manual transfer).
 * @return number received      The total amount of the resource that has actually been received from allies (via sharing or manual transfer).
 * @return number excess        The amount of the resource that was lost due to storage overflow (wasted).
 */
int LuaSyncedRead::GetTeamResources(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	if (!LuaUtils::IsAlliedTeam(L, teamID))
		return 0;

	switch (luaL_checkstring(L, 2)[0]) {
		case 'm': {
			lua_pushnumber(L, team->res.metal);
			lua_pushnumber(L, team->resStorage.metal);
			lua_pushnumber(L, team->resPrevPull.metal);
			lua_pushnumber(L, team->resPrevIncome.metal);
			lua_pushnumber(L, team->resPrevExpense.metal);
			lua_pushnumber(L, team->resShare.metal);
			lua_pushnumber(L, team->resPrevSent.metal);
			lua_pushnumber(L, team->resPrevReceived.metal);
			lua_pushnumber(L, team->resPrevExcess.metal);
			return 9;
		} break;
		case 'e': {
			lua_pushnumber(L, team->res.energy);
			lua_pushnumber(L, team->resStorage.energy);
			lua_pushnumber(L, team->resPrevPull.energy);
			lua_pushnumber(L, team->resPrevIncome.energy);
			lua_pushnumber(L, team->resPrevExpense.energy);
			lua_pushnumber(L, team->resShare.energy);
			lua_pushnumber(L, team->resPrevSent.energy);
			lua_pushnumber(L, team->resPrevReceived.energy);
			lua_pushnumber(L, team->resPrevExcess.energy);
			return 9;
		} break;
		default: {
		} break;
	}

	return 0;
}


/***
 *
 * @function Spring.GetTeamUnitStats
 * @param teamID TeamID
 * @return integer? killed
 * @return integer died
 * @return integer capturedBy
 * @return integer capturedFrom
 * @return integer received
 * @return integer sent
 */
int LuaSyncedRead::GetTeamUnitStats(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr || game == nullptr)
		return 0;

	const int teamID = team->teamNum;

	if (!LuaUtils::IsAlliedTeam(L, teamID) && !game->IsGameOver())
		return 0;

	const TeamStatistics& stats = team->GetCurrentStats();
	lua_pushnumber(L, stats.unitsKilled);
	lua_pushnumber(L, stats.unitsDied);
	lua_pushnumber(L, stats.unitsCaptured);
	lua_pushnumber(L, stats.unitsOutCaptured);
	lua_pushnumber(L, stats.unitsReceived);
	lua_pushnumber(L, stats.unitsSent);

	return 6;
}


/***
 *
 * @function Spring.GetTeamResourceStats
 * @param teamID TeamID
 * @param resource ResourceName
 * @return number? used
 * @return number produced
 * @return number excessed
 * @return number received
 * @return number sent
 */
int LuaSyncedRead::GetTeamResourceStats(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr || game == nullptr)
		return 0;

	const int teamID = team->teamNum;

	if (!LuaUtils::IsAlliedTeam(L, teamID) && !game->IsGameOver())
		return 0;

	const TeamStatistics& stats = team->GetCurrentStats();

	switch (luaL_checkstring(L, 2)[0]) {
		case 'm': {
			lua_pushnumber(L, stats.metalUsed);
			lua_pushnumber(L, stats.metalProduced);
			lua_pushnumber(L, stats.metalExcess);
			lua_pushnumber(L, stats.metalReceived);
			lua_pushnumber(L, stats.metalSent);
			return 5;
		} break;
		case 'e': {
			lua_pushnumber(L, stats.energyUsed);
			lua_pushnumber(L, stats.energyProduced);
			lua_pushnumber(L, stats.energyExcess);
			lua_pushnumber(L, stats.energyReceived);
			lua_pushnumber(L, stats.energySent);
			return 5;
		} break;
		default: {
		} break;
	}

	return 0;
}


/*** Gets team damage dealt/received totals
 *
 * @function Spring.GetTeamDamageStats
 *
 * Returns a team's damage stats. Note that all damage is counted,
 * including self-inflicted and unconfirmed out-of-sight.
 *
 * @param teamID TeamID
 * @return number damageDealt
 * @return number damageReceived
 */
int LuaSyncedRead::GetTeamDamageStats(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr || game == nullptr)
		return 0;

	const int teamID = team->teamNum;

	if (!LuaUtils::IsAlliedTeam(L, teamID) && !game->IsGameOver())
		return 0;

	const TeamStatistics& stats = team->GetCurrentStats();

	lua_pushnumber(L, stats.damageDealt);
	lua_pushnumber(L, stats.damageReceived);

	return 2;
}


/***
 * @class TeamStats
 * @x_helper
 * @field time integer
 * @field frame integer
 * @field metalUsed number
 * @field metalProduced number
 * @field metalExcess number
 * @field metalReceived number
 * @field metalSent number
 * @field energyUsed number
 * @field energyProduced number
 * @field energyExcess number
 * @field energyReceived number
 * @field energySent number
 * @field damageDealt number
 * @field damageReceived number
 * @field unitsProduced integer
 * @field unitsDied integer
 * @field unitsReceived integer
 * @field unitsSent integer
 * @field unitsCaptured integer
 * @field unitsOutCaptured integer
 */

/***
 * Get the number of history entries.
 * @function Spring.GetTeamStatsHistory
 * @param teamID TeamID
 * @return integer? historyCount The number of history entries, or `nil` if unable to resolve team.
 */
/***
 * Get team stats history.
 * @function Spring.GetTeamStatsHistory
 * @param teamID TeamID
 * @param startIndex integer
 * @param endIndex integer? (Default: startIndex)
 * @return TeamStats[] teamStatsHistory The team stats history, or `nil` if unable to resolve team.
 */
int LuaSyncedRead::GetTeamStatsHistory(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr || game == nullptr)
		return 0;

	const int teamID = team->teamNum;

	if (!LuaUtils::IsAlliedTeam(L, teamID) && !game->IsGameOver())
		return 0;

	const int args = lua_gettop(L);

	if (args == 1) {
		lua_pushnumber(L, team->statHistory.size());
		return 1;
	}

	const auto& teamStats = team->statHistory;
	auto it = teamStats.cbegin();
	const int statCount = teamStats.size();

	int start = 0;
	if ((args >= 2) && lua_isnumber(L, 2)) {
		start = lua_toint(L, 2) - 1;
		start = max(0, min(statCount - 1, start));
	}

	int end = start;
	if ((args >= 3) && lua_isnumber(L, 3)) {
		end = lua_toint(L, 3) - 1;
		end = max(0, min(statCount - 1, end));
	}

	std::advance(it, start);

	lua_createtable(L, max(0, end - start), 0);
	if (statCount > 0) {
		int count = 1;
		for (int i = start; i <= end; ++i, ++it) {
			const TeamStatistics& stats = *it;
			lua_createtable(L, 0, 21); {
				if (i+1 == teamStats.size()) {
					// the `stats.frame` var indicates the frame when a new entry needs to get added,
					// for the most recent stats entry this lies obviously in the future,
					// so we just output the current frame here
					LuaPushNamedNumber(L, "time",         gs->GetLuaSimFrame() / GAME_SPEED);
					LuaPushNamedNumber(L, "frame",        gs->GetLuaSimFrame());
				} else {
					LuaPushNamedNumber(L, "time",         stats.frame / GAME_SPEED);
					LuaPushNamedNumber(L, "frame",        stats.frame);
				}

				LuaPushNamedNumber(L, "metalUsed",        stats.metalUsed);
				LuaPushNamedNumber(L, "metalProduced",    stats.metalProduced);
				LuaPushNamedNumber(L, "metalExcess",      stats.metalExcess);
				LuaPushNamedNumber(L, "metalReceived",    stats.metalReceived);
				LuaPushNamedNumber(L, "metalSent",        stats.metalSent);

				LuaPushNamedNumber(L, "energyUsed",       stats.energyUsed);
				LuaPushNamedNumber(L, "energyProduced",   stats.energyProduced);
				LuaPushNamedNumber(L, "energyExcess",     stats.energyExcess);
				LuaPushNamedNumber(L, "energyReceived",   stats.energyReceived);
				LuaPushNamedNumber(L, "energySent",       stats.energySent);

				LuaPushNamedNumber(L, "damageDealt",      stats.damageDealt);
				LuaPushNamedNumber(L, "damageReceived",   stats.damageReceived);

				LuaPushNamedNumber(L, "unitsProduced",    stats.unitsProduced);
				LuaPushNamedNumber(L, "unitsDied",        stats.unitsDied);
				LuaPushNamedNumber(L, "unitsReceived",    stats.unitsReceived);
				LuaPushNamedNumber(L, "unitsSent",        stats.unitsSent);
				LuaPushNamedNumber(L, "unitsCaptured",    stats.unitsCaptured);
				LuaPushNamedNumber(L, "unitsOutCaptured", stats.unitsOutCaptured);
				LuaPushNamedNumber(L, "unitsKilled",      stats.unitsKilled);
			}
			lua_rawseti(L, -2, count++);
		}
	}

	return 1;
}


/***
 *
 * @function Spring.GetTeamLuaAI
 * @param teamID TeamID
 * @return string
 */
int LuaSyncedRead::GetTeamLuaAI(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr)
		return 0;

	const std::string* luaAIName = nullptr;
	const std::vector<uint8_t>& teamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(team->teamNum);

	for (uint8_t id: teamAIs) {
		const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(id);

		if (!aiData->isLuaAI)
			continue;

		luaAIName = &aiData->shortName;
		break;
	}

	if (luaAIName == nullptr)
		return 0;

	lua_pushsstring(L, *luaAIName);
	return 1;
}


/*** Returns a team's unit cap.
 *
 * Also returns the current unit count for readable teams as the 2nd value.
 *
 * @function Spring.GetTeamMaxUnits
 * @param teamID TeamID
 * @return integer maxUnits
 * @return integer? currentUnits
 */
int LuaSyncedRead::GetTeamMaxUnits(lua_State* L)
{
	const auto team = ParseTeam(L, __func__, 1);
	if (team == nullptr)
		return 0;

	lua_pushnumber(L, team->GetMaxUnits());

	if (LuaUtils::IsAlliedTeam(L, team->teamNum))
		lua_pushnumber(L, team->GetNumUnits());
	else
		lua_pushnil(L);

	return 2;
}

/***
 *
 * @function Spring.GetPlayerInfo
 * @param playerID PlayerID
 * @param getPlayerOpts boolean? (Default: `true`) whether to return custom player options
 * @return string name
 * @return boolean active
 * @return boolean spectator
 * @return TeamID teamID
 * @return AllyTeamID allyTeamID
 * @return number pingTime
 * @return number cpuUsage
 * @return string country
 * @return integer rank
 * @return boolean hasSkirmishAIsInTeam
 * @return {[string]: string} playerOpts when playerOpts is true
 * @return boolean desynced
 */
int LuaSyncedRead::GetPlayerInfo(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	if (!playerHandler.IsValidPlayer(playerID))
		return 0;

	const CPlayer* player = playerHandler.Player(playerID);
	if (player == nullptr)
		return 0;

	if (IsPlayerUnsynced(L, player))
		return 0;

	// read before modifying stack
	const bool getPlayerOpts = luaL_optboolean(L, 2, true);

	lua_pushsstring(L, player->name);
	lua_pushboolean(L, player->active);
	lua_pushboolean(L, player->spectator);
	lua_pushnumber(L, player->team);
	lua_pushnumber(L, teamHandler.AllyTeam(player->team));
	lua_pushnumber(L, player->ping * 0.001f); // in seconds
	lua_pushnumber(L, player->cpuUsage);
	lua_pushsstring(L, player->countryCode);
	lua_pushnumber(L, player->rank);
	// same as select(4, GetTeamInfo(teamID=player->team))
	lua_pushboolean(L, skirmishAIHandler.HasSkirmishAIsInTeam(player->team));

	if (getPlayerOpts) {
		const PlayerBase::customOpts& playerOpts = player->GetAllValues();

		lua_createtable(L, 0, playerOpts.size());

		for (const auto& pair: playerOpts) {
			lua_pushsstring(L, pair.first);
			lua_pushsstring(L, pair.second);
			lua_rawset(L, -3);
		}
	} else {
		lua_pushnil(L);
	}
	lua_pushboolean(L, player->desynced);

	return 12;
}


/*** Returns unit controlled by player on FPS mode
 *
 * @function Spring.GetPlayerControlledUnit
 * @param playerID PlayerID
 * @return UnitID?
 */
int LuaSyncedRead::GetPlayerControlledUnit(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	if (!playerHandler.IsValidPlayer(playerID))
		return 0;

	const CPlayer* player = playerHandler.Player(playerID);
	if (player == nullptr)
		return 0;

	if (IsPlayerUnsynced(L, player))
		return 0;


	const FPSUnitController& con = player->fpsController;
	const CUnit* unit = con.GetControllee();

	if (unit == nullptr)
		return 0;

	if ((CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam) ||
	    ((CLuaHandle::GetHandleReadAllyTeam(L) >= 0) && !teamHandler.Ally(unit->allyteam, CLuaHandle::GetHandleReadAllyTeam(L)))) {
		return 0;
	}

	lua_pushnumber(L, unit->id);
	return 1;
}


/***
 *
 * @function Spring.GetAIInfo
 * @param teamID TeamID
 * @return integer skirmishAIID
 * @return string name
 * @return PlayerID hostingPlayerID
 * @return string shortName When synced `"SYNCED_NOSHORTNAME"`, otherwise the AI shortname or `"UNKNOWN"`.
 * @return string version When synced `"SYNCED_NOVERSION"`, otherwise the AI version or `"UNKNOWN"`.
 * @return table<string,string> options
 */
int LuaSyncedRead::GetAIInfo(lua_State* L)
{
	int numVals = 0;

	const int teamId = luaL_checkint(L, 1);
	if (!teamHandler.IsValidTeam(teamId))
		return numVals;

	const std::vector<uint8_t>& teamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(teamId);
	if (teamAIs.empty())
		return numVals;

	const size_t skirmishAIId    = teamAIs[0];
	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);

	// this is synced AI info
	lua_pushnumber(L, skirmishAIId);
	lua_pushsstring(L, aiData->name);
	lua_pushnumber(L, aiData->hostPlayer);
	numVals += 3;

	// no unsynced Skirmish AI info for synchronized scripts
	if (CLuaHandle::GetHandleSynced(L)) {
		LuaPushString(L, "SYNCED_NOSHORTNAME");
		LuaPushString(L, "SYNCED_NOVERSION");
		lua_newtable(L);
	} else if (skirmishAIHandler.IsLocalSkirmishAI(skirmishAIId)) {
		lua_pushsstring(L, aiData->shortName);
		lua_pushsstring(L, aiData->version);

		lua_createtable(L, 0, aiData->options.size());

		for (const auto& option: aiData->options) {
			lua_pushsstring(L, option.first);
			lua_pushsstring(L, option.second);
			lua_rawset(L, -3);
		}
	} else {
		LuaPushString(L, "UNKNOWN");
		LuaPushString(L, "UNKNOWN");
		lua_newtable(L);
	}
	numVals += 3;

	return numVals;
}


/***
 *
 * @function Spring.GetAllyTeamInfo
 * @param allyTeamID AllyTeamID
 * @return table<string,string>?
 */
int LuaSyncedRead::GetAllyTeamInfo(lua_State* L)
{
	const size_t allyteam = (size_t)luaL_checkint(L, -1);
	if (!teamHandler.ValidAllyTeam(allyteam))
		return 0;

	const AllyTeam& ally = teamHandler.GetAllyTeam(allyteam);
	const AllyTeam::customOpts& allyTeamOpts = ally.GetAllValues();

	lua_createtable(L, 0, allyTeamOpts.size());

	for (const auto& pair: allyTeamOpts) {
		lua_pushsstring(L, pair.first);
		lua_pushsstring(L, pair.second);
		lua_rawset(L, -3);
	}
	return 1;
}


/***
 *
 * @function Spring.AreTeamsAllied
 * @param teamID1 TeamID
 * @param teamID2 TeamID
 * @return boolean?
 */
int LuaSyncedRead::AreTeamsAllied(lua_State* L)
{
	const int teamId1 = (int)luaL_checkint(L, -1);
	const int teamId2 = (int)luaL_checkint(L, -2);

	if (!teamHandler.IsValidTeam(teamId1) || !teamHandler.IsValidTeam(teamId2))
		return 0;

	lua_pushboolean(L, teamHandler.AlliedTeams(teamId1, teamId2));
	return 1;
}


/***
 *
 * @function Spring.ArePlayersAllied
 * @param playerID1 PlayerID
 * @param playerID2 PlayerID
 * @return boolean?
 */
int LuaSyncedRead::ArePlayersAllied(lua_State* L)
{
	const int player1 = luaL_checkint(L, -1);
	const int player2 = luaL_checkint(L, -2);

	if (!playerHandler.IsValidPlayer(player1) || !playerHandler.IsValidPlayer(player2))
		return 0;

	const CPlayer* p1 = playerHandler.Player(player1);
	const CPlayer* p2 = playerHandler.Player(player2);

	if ((p1 == nullptr) || (p2 == nullptr))
		return 0;

	if ((IsPlayerUnsynced(L, p1)) || (IsPlayerUnsynced(L, p2)))
		return 0;

	lua_pushboolean(L, teamHandler.AlliedTeams(p1->team, p2->team));
	return 1;
}


/******************************************************************************
 * Unit queries
 *
 * @section unit_queries
******************************************************************************/


/*** Get a list of all unitIDs
 *
 * @function Spring.GetAllUnits
 *
 * Note that when called from a widget, this also returns units that are only
 * radar blips.
 *
 * For units that are radar blips, you may want to check if they are in los,
 * as GetUnitDefID() will still return true if they have previously been seen.
 *
 * @see UnsyncedRead.GetVisibleUnits
 *
 * @return UnitID[] unitIDs
 */
int LuaSyncedRead::GetAllUnits(lua_State* L)
{
	lua_createtable(L, (unitHandler.GetActiveUnits()).size(), 0);

	unsigned int unitCount = 1;
	if (CLuaHandle::GetHandleFullRead(L)) {
		for (const CUnit* unit: unitHandler.GetActiveUnits()) {
			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, unitCount++);
		}
	} else {
		for (const CUnit* unit: unitHandler.GetActiveUnits()) {
			if (!LuaUtils::IsUnitVisible(L, unit))
				continue;

			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, unitCount++);
		}
	}

	return 1;
}


/***
 *
 * @function Spring.GetTeamUnits
 * @param teamID TeamID
 * @return UnitID[]? unitIDs
 */
int LuaSyncedRead::GetTeamUnits(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	unsigned int unitCount = 1;

	// raw push for allies
	if (LuaUtils::IsAlliedTeam(L, teamID)) {
		lua_createtable(L, unitHandler.NumUnitsByTeam(teamID), 0);

		for (const CUnit* unit: unitHandler.GetUnitsByTeam(teamID)) {
			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, unitCount++);
		}

		return 1;
	}

	// check visibility for enemies
	lua_createtable(L, unitHandler.NumUnitsByTeam(teamID), 0);

	for (const CUnit* unit: unitHandler.GetUnitsByTeam(teamID)) {
		if (!LuaUtils::IsUnitVisible(L, unit))
			continue;
		lua_pushnumber(L, unit->id);
		lua_rawseti(L, -2, unitCount++);
	}

	return 1;
}



// used by GetTeamUnitsSorted (PushVisibleUnits) and GetTeamUnitsByDefs (InsertSearchUnitDefs)
static std::vector<int> gtuObjectIDs;
// used by GetTeamUnitsCounts
static std::vector< std::pair<int, int> > gtuDefCounts;

static bool PushVisibleUnits(
	lua_State* L,
	const std::vector<CUnit*>& defUnits,
	int unitDefID,
	unsigned int* unitCount,
	unsigned int* defCount
) {
	bool createdTable = false;

	for (const CUnit* unit: defUnits) {
		if (!LuaUtils::IsUnitVisible(L, unit))
			continue;

		if (!LuaUtils::IsUnitTyped(L, unit)) {
			gtuObjectIDs.push_back(unit->id);
			continue;
		}

		// push new table for first unit of type <unitDefID> to be visible
		if (!createdTable) {
			createdTable = true;

			lua_pushnumber(L, unitDefID);
			lua_createtable(L, defUnits.size(), 0);

			(*defCount)++;
		}

		// add count-th unitID to table
		lua_pushnumber(L, unit->id);
		lua_rawseti(L, -2, (*unitCount)++);
	}

	return createdTable;
}

static inline void InsertSearchUnitDefs(const UnitDef* ud, bool allied)
{
	if (ud == nullptr)
		return;

	if (!allied && ud->decoyDef)
		return;

	gtuObjectIDs.push_back(ud->id);
}


/***
 *
 * @function Spring.GetTeamUnitsSorted
 * @param teamID TeamID
 * @return table<UnitDefID,UnitID[]> unitsByDef A table where keys are unitDefIDs and values are arrays of unitIDs
 */
int LuaSyncedRead::GetTeamUnitsSorted(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	unsigned int defCount = 0;
	unsigned int unitCount = 1;

	// table = {[unitDefID] = {[1] = unitID, [2] = unitID, ...}}
	lua_createtable(L, unitDefHandler->NumUnitDefs(), 0);

	if (LuaUtils::IsAlliedTeam(L, teamID)) {
		// tally for allies
		for (unsigned int i = 0, n = unitDefHandler->NumUnitDefs(); i < n; i++) {
			const auto& unitsByDef = unitHandler.GetUnitsByTeamAndDef(teamID, i + 1);

			if (unitsByDef.empty())
				continue;

			lua_pushnumber(L, i + 1);
			lua_createtable(L, unitsByDef.size(), 0);
			defCount++;

			for (const CUnit* unit: unitsByDef) {
				lua_pushnumber(L, unit->id);
				lua_rawseti(L, -2, unitCount++);
			}
			lua_rawset(L, -3);
		}
	} else {
		// tally for enemies
		gtuObjectIDs.clear();
		gtuObjectIDs.reserve(16);

		for (unsigned int i = 0, n = unitDefHandler->NumUnitDefs(); i < n; i++) {
			const unsigned int unitDefID = i + 1;

			const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);

			// we deal with decoys later
			if (ud->decoyDef != nullptr)
				continue;

			bool createdTable = PushVisibleUnits(L, unitHandler.GetUnitsByTeamAndDef(teamID, unitDefID), unitDefID, &unitCount, &defCount);

			// for all decoy-defs of unitDefID, add decoy units under the same ID
			const auto& decoyMap = unitDefHandler->GetDecoyDefIDs();
			const auto decoyMapIt = decoyMap.find(unitDefID);

			if (decoyMapIt != decoyMap.end()) {
				for (int decoyDefID: decoyMapIt->second) {
					createdTable |= PushVisibleUnits(L, unitHandler.GetUnitsByTeamAndDef(teamID, decoyDefID), unitDefID, &unitCount, &defCount);
				}
			}

			if (createdTable)
				lua_rawset(L, -3);

		}

		if (!gtuObjectIDs.empty()) {
			LuaPushString(L, "unknown");

			defCount += 1;
			unitCount = 1;

			lua_createtable(L, gtuObjectIDs.size(), 0);

			for (int unitID: gtuObjectIDs) {
				lua_pushnumber(L, unitID);
				lua_rawseti(L, -2, unitCount++);
			}
			lua_rawset(L, -3);
		}
	}

	// UnitDef ID keys are not consecutive, so add the "n"
	hs_n.PushNumber(L, defCount);
	return 1;
}


/***
 *
 * @function Spring.GetTeamUnitsCounts
 * @param teamID TeamID
 * @return table<UnitDefID,integer>? countByUnit A table where keys are unitDefIDs and values are counts.
 */
int LuaSyncedRead::GetTeamUnitsCounts(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	unsigned int unknownCount = 0;
	unsigned int defCount = 0;

	// send the raw unitsByDefs counts for allies
	if (LuaUtils::IsAlliedTeam(L, teamID)) {
		lua_createtable(L, unitDefHandler->NumUnitDefs(), 0);

		for (unsigned int i = 0, n = unitDefHandler->NumUnitDefs(); i < n; i++) {
			const unsigned int unitDefID = i + 1;
			const unsigned int unitCount = unitHandler.NumUnitsByTeamAndDef(teamID, unitDefID);

			if (unitCount == 0)
				continue;

			lua_pushnumber(L, unitCount);
			lua_rawseti(L, -2, unitDefID);
			defCount++;
		}

		// keys are not necessarily consecutive here due to
		// the unitCount check, so add the "n" key manually
		hs_n.PushNumber(L, defCount);
		return 1;
	}

	// tally the counts for enemies
	gtuDefCounts.clear();
	gtuDefCounts.resize(unitDefHandler->NumUnitDefs() + 1, {0, 0});

	for (const CUnit* unit: unitHandler.GetUnitsByTeam(teamID)) {
		if (!LuaUtils::IsUnitVisible(L, unit))
			continue;

		if (!LuaUtils::IsUnitTyped(L, unit)) {
			unknownCount++;
		} else {
			const UnitDef* unitDef = LuaUtils::EffectiveUnitDef(L, unit);

			gtuDefCounts[unitDef->id].first = unitDef->id;
			gtuDefCounts[unitDef->id].second += 1;
		}
	}

	// push the counts
	lua_createtable(L, 0, gtuDefCounts.size());

	for (const auto& gtuDefCount: gtuDefCounts) {
		if (gtuDefCount.second == 0)
			continue;
		lua_pushnumber(L, gtuDefCount.second);
		lua_rawseti(L, -2, gtuDefCount.first);
		defCount++;
	}
	if (unknownCount > 0) {
		LuaPushNamedNumber(L, "unknown", unknownCount);
		defCount++;
	}

	// unitDef->id is used for ordering, so not consecutive
	hs_n.PushNumber(L, defCount);
	return 1;
}


/***
 *
 * @function Spring.GetTeamUnitsByDefs
 * @param teamID TeamID
 * @param unitDefIDs UnitDefID|UnitDefID[]
 * @return UnitID[]? unitIDs
 */
int LuaSyncedRead::GetTeamUnitsByDefs(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;
	const bool allied = LuaUtils::IsAlliedTeam(L, teamID);

	// parse the unitDefs
	gtuObjectIDs.clear();
	gtuObjectIDs.reserve(16);

	if (lua_isnumber(L, 2)) {
		InsertSearchUnitDefs(unitDefHandler->GetUnitDefByID(lua_toint(L, 2)), allied);
	} else if (lua_istable(L, 2)) {
		const int tableIdx = 2;

		for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
			if (!lua_isnumber(L, LUA_TABLE_VALUE_INDEX))
				continue;

			InsertSearchUnitDefs(unitDefHandler->GetUnitDefByID(lua_toint(L, LUA_TABLE_VALUE_INDEX)), allied);
		}
	} else {
		luaL_error(L, "Incorrect arguments to GetTeamUnitsByDefs()");
	}

	// sort the ID's so duplicates can be skipped
	spring::VectorSortUnique(gtuObjectIDs);

	std::vector<int> unitIDs;
	size_t lastOfsset = 0;
	bool isCalledFromSynced = CLuaHandle::GetHandleSynced(L);

	for (const int unitDefID: gtuObjectIDs) {
		for (const CUnit* unit: unitHandler.GetUnitsByTeam(teamID)) {
			if (!allied && !LuaUtils::IsUnitTyped(L, unit))
				continue;

			if (unit->unitDef->id == unitDefID || (!allied && unit->unitDef->decoyDef && unit->unitDef->decoyDef->id == unitDefID)) {
				unitIDs.emplace_back(unit->id);
			}
		}

		if (isCalledFromSynced)
			continue;

		/* `unitHandler.GetUnitsByTeam` returns units in creation order,
		 * which would reveal some extra information if passed unchanged. */
		spring::random_shuffle(unitIDs.begin() + lastOfsset, unitIDs.end(), guRNG);
		lastOfsset = unitIDs.size();
	}

	lua_createtable(L, unitIDs.size(), 0);

	for (int i = 0; i < unitIDs.size(); ++i) {
		lua_pushnumber(L, unitIDs[i]);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}


/***
 *
 * @function Spring.GetTeamUnitDefCount
 * @param teamID TeamID
 * @param unitDefID UnitDefID
 * @return integer? count
 */
int LuaSyncedRead::GetTeamUnitDefCount(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	const UnitDef* unitDef = unitDefHandler->GetUnitDefByID(luaL_checkint(L, 2));

	if (unitDef == nullptr)
		luaL_error(L, "Bad unitDefID in GetTeamUnitDefCount()");

	// use the unitsByDefs count for allies
	if (LuaUtils::IsAlliedTeam(L, teamID)) {
		lua_pushnumber(L, unitHandler.NumUnitsByTeamAndDef(teamID, unitDef->id));
		return 1;
	}

	// you can never count enemy decoys
	if (unitDef->decoyDef != nullptr) {
		lua_pushnumber(L, 0);
		return 1;
	}

	unsigned int unitCount = 0;

	// tally the given unitDef units
	for (const CUnit* unit: unitHandler.GetUnitsByTeamAndDef(teamID, unitDef->id)) {
		unitCount += (LuaUtils::IsUnitTyped(L, unit));
	}

	// tally the decoy units for the given unitDef
	const auto& decoyMap = unitDefHandler->GetDecoyDefIDs();
	const auto decoyMapIt = decoyMap.find(unitDef->id);

	if (decoyMapIt != decoyMap.end()) {
		for (const int udID: decoyMapIt->second) {
			for (const CUnit* unit: unitHandler.GetUnitsByTeamAndDef(teamID, udID)) {
				unitCount += (LuaUtils::IsUnitTyped(L, unit));
			}
		}
	}

	lua_pushnumber(L, unitCount);
	return 1;
}


/***
 *
 * @function Spring.GetTeamUnitCount
 * @param teamID TeamID
 * @return integer? count
 */
int LuaSyncedRead::GetTeamUnitCount(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;

	// use the raw team count for allies
	if (LuaUtils::IsAlliedTeam(L, team->teamNum)) {
		lua_pushnumber(L, unitHandler.NumUnitsByTeam(team->teamNum));
		return 1;
	}

	// loop through the units for enemies
	unsigned int unitCount = 0;

	for (const CUnit* unit: unitHandler.GetUnitsByTeam(team->teamNum)) {
		unitCount += int(LuaUtils::IsUnitVisible(L, unit));
	}

	lua_pushnumber(L, unitCount);
	return 1;
}


/******************************************************************************
 * Spatial unit queries
 *
 * @section spatial_unit_queries
 *
 * For the allegiance parameters: AllUnits = -1, MyUnits = -2, AllyUnits = -3, EnemyUnits = -4
******************************************************************************/

/* Apply team error to planar mins/maxs boxes */
void ApplyPlanarTeamError(lua_State* L, int allegiance, float3& mins, float3& maxs) {
	if ((allegiance >= 0 && !LuaUtils::IsAlliedTeam(L, allegiance)) ||
	   !(allegiance == LuaUtils::MyUnits || allegiance == LuaUtils::AllyUnits)) {
		const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);
		const float allyTeamError = losHandler->GetAllyTeamRadarErrorSize(readAllyTeam);
		const float3 allyTeamError3(allyTeamError, 0.0f, allyTeamError);
		mins -= allyTeamError3;
		maxs += allyTeamError3;
	}
}

/**
 * @brief This populates the Lua table with the unit IDs of units that are in a
 * particular region. 
 * 
 * @param allegiance The allegiance of the units to add to the table.
 * @param units The units to add to the table.
 * @param inRegion A lambda checking if the unit position is in the region
 */
template<typename InRegion>
static void GetFilteredUnits(lua_State *L, int allegiance, const std::vector<CUnit*>& units, InRegion inRegion) {
	const int readTeam = CLuaHandle::GetHandleReadTeam(L);
	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);
	const bool fullRead = CLuaHandle::GetHandleFullRead(L);

	auto runLoop = [&](auto disqualifier) {
		unsigned int count = 0;
		for (const CUnit* unit : units) {
			if (disqualifier(unit))
				continue;

			float3 pos = unit->midPos + unit->GetLuaErrorVector(readAllyTeam, fullRead);
			if (!inRegion(unit, pos))
				continue;

			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, ++count);
		}
	};

	switch (allegiance) {
		case LuaUtils::AllUnits:
			runLoop([L](const CUnit* u) { return !LuaUtils::IsUnitVisible(L, u); });
			break;
		case LuaUtils::MyUnits:
			runLoop([L, readTeam](const CUnit* u) { return u->team != readTeam || !LuaUtils::IsUnitVisible(L, u); });
			break;
		case LuaUtils::AllyUnits:
			runLoop([L, readAllyTeam](const CUnit* u) { return u->allyteam != readAllyTeam || !LuaUtils::IsUnitVisible(L, u); });
			break;
		case LuaUtils::EnemyUnits:
			runLoop([L, readAllyTeam](const CUnit* u) { return u->allyteam == readAllyTeam || !LuaUtils::IsUnitVisible(L, u); });
			break;
		default:
			runLoop([L, allegiance](const CUnit* u) { return u->team != allegiance || !LuaUtils::IsUnitVisible(L, u); });
			break;
	}
}

/***
 *
 * @function Spring.GetUnitsInRectangle
 * @param xmin number
 * @param zmin number
 * @param xmax number
 * @param zmax number
 * @param allegiance integer?
 * @return UnitID[] unitIDs
 */
int LuaSyncedRead::GetUnitsInRectangle(lua_State* L)
{
	const float xmin = luaL_checkfloat(L, 1);
	const float zmin = luaL_checkfloat(L, 2);
	const float xmax = luaL_checkfloat(L, 3);
	const float zmax = luaL_checkfloat(L, 4);

	float3 mins(xmin, 0.0f, zmin);
	float3 maxs(xmax, 0.0f, zmax);

	const int allegiance = LuaUtils::ParseAllegiance(L, __func__, 5);

	const auto rectangleCheck = [&](const CUnit *unit, const float3 &pos) {
		if((pos.x < xmin) || (pos.x > xmax))
			return false;
		if((pos.z < zmin) || (pos.z > zmax))
			return false;
		return true;
	};

	const bool fullRead = CLuaHandle::GetHandleFullRead(L);
	if (!fullRead)
		ApplyPlanarTeamError(L, allegiance, mins, maxs);

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, mins, maxs);
	const auto& units = (*qfQuery.units);

	lua_createtable(L, units.size(), 0);

	GetFilteredUnits(L, allegiance, units, rectangleCheck);

	return 1;
}


/***
 *
 * @function Spring.GetUnitsInBox
 * @param xmin number
 * @param ymin number
 * @param zmin number
 * @param xmax number
 * @param ymax number
 * @param zmax number
 * @param allegiance integer?
 * @return UnitID[] unitIDs
 */
int LuaSyncedRead::GetUnitsInBox(lua_State* L)
{
	const float xmin = luaL_checkfloat(L, 1);
	const float ymin = luaL_checkfloat(L, 2);
	const float zmin = luaL_checkfloat(L, 3);
	const float xmax = luaL_checkfloat(L, 4);
	const float ymax = luaL_checkfloat(L, 5);
	const float zmax = luaL_checkfloat(L, 6);

	float3 mins(xmin, 0.0f, zmin);
	float3 maxs(xmax, 0.0f, zmax);

	const int allegiance = LuaUtils::ParseAllegiance(L, __func__, 7);

	const auto boxCheck = [&](const CUnit *unit, float3 pos) {
		return AABB(float3(xmin, ymin, zmin), float3(xmax, ymax, zmax)).Contains(pos);
	};

	const bool fullRead = CLuaHandle::GetHandleFullRead(L);
	if (!fullRead)
		ApplyPlanarTeamError(L, allegiance, mins, maxs);

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, mins, maxs);
	const auto& units = (*qfQuery.units);

	lua_createtable(L, units.size(), 0);

	GetFilteredUnits(L, allegiance, units, boxCheck);

	return 1;
}

/***
 *
 * @function Spring.GetUnitsInCylinder
 * @param x number
 * @param z number
 * @param radius number
 * @param teamID TeamID? filter by team, defaults to all units
 * @return UnitID[] unitIDs
 */
int LuaSyncedRead::GetUnitsInCylinder(lua_State* L)
{
	const float x      = luaL_checkfloat(L, 1);
	const float z      = luaL_checkfloat(L, 2);
	const float radius = luaL_checkfloat(L, 3);
	const float radSqr = (radius * radius);

	float3 mins(x - radius, 0.0f, z - radius);
	float3 maxs(x + radius, 0.0f, z + radius);

	const int allegiance = LuaUtils::ParseAllegiance(L, __func__, 4);

	const auto cylinderCheck = [&](const CUnit *unit, const float3 &p) {
		return p.SqDistance2D(float3{x, 0.0, z}) <= radSqr;
	};

	const bool fullRead = CLuaHandle::GetHandleFullRead(L);
	if (!fullRead)
		ApplyPlanarTeamError(L, allegiance, mins, maxs);

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, mins, maxs);
	const auto& units = (*qfQuery.units);

	lua_createtable(L, units.size(), 0);

	GetFilteredUnits(L, allegiance, units, cylinderCheck);

	return 1;
}


/***
 *
 * @function Spring.GetUnitsInSphere
 * @param x number
 * @param y number
 * @param z number
 * @param radius number
 * @return UnitID[] unitIDs
 */
int LuaSyncedRead::GetUnitsInSphere(lua_State* L)
{
	const float x      = luaL_checkfloat(L, 1);
	const float y      = luaL_checkfloat(L, 2);
	const float z      = luaL_checkfloat(L, 3);
	const float radius = luaL_checkfloat(L, 4);
	const float radSqr = (radius * radius);

	float3 mins(x - radius, 0.0f, z - radius);
	float3 maxs(x + radius, 0.0f, z + radius);

	const int allegiance = LuaUtils::ParseAllegiance(L, __func__, 5);

	const auto sphereCheck = [&](const CUnit *unit, const float3 &p) {
		return p.SqDistance(float3(x, y, z)) <= radSqr;
	};

	const bool fullRead = CLuaHandle::GetHandleFullRead(L);
	if (!fullRead)
		ApplyPlanarTeamError(L, allegiance, mins, maxs);

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, mins, maxs);
	const auto& units = (*qfQuery.units);

	lua_createtable(L, units.size(), 0);

	GetFilteredUnits(L, allegiance, units, sphereCheck);

	return 1;
}


struct Plane {
	float x, y, z, d;  // ax + by + cz + d = 0
};


static inline bool UnitInPlanes(const float3& pos, const float radius, const vector<Plane>& planes)
{
	for (const Plane& p: planes) {
		const float dist = (pos.x * p.x) + (pos.y * p.y) + (pos.z * p.z) + p.d;
		if ((dist - radius) > 0.0f) {
			return false; // outside
		}
	}
	return true;
}

/***
 * @class Plane
 * @x_helper
 * @field normalVecX number
 * @field normalVecY number
 * @field normalVecZ number
 * @field d number
 */

/***
 *
 * @function Spring.GetUnitsInPlanes
 *
 * Plane normals point towards accepted space, so the acceptance criteria for each plane is:
 *
 * ```
 * radius     = unit radius
 * px, py, pz = unit position
 * [(nx * px) + (ny * py) + (nz * pz) + (d - radius)]  <=  0
 * ```
 *
 * @param planes Plane[]
 * @param allegiance integer?
 * @return UnitID[] unitIDs
 */
int LuaSyncedRead::GetUnitsInPlanes(lua_State* L)
{
	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetUnitsInPlanes()");
	}

	// parse the planes
	vector<Plane> planes;
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_istable(L, -1)) {
			float values[4];
			const int v = LuaUtils::ParseFloatArray(L, -1, values, 4);
			if (v == 4) {
				Plane plane = { values[0], values[1], values[2], values[3] };
				planes.push_back(plane);
			}
		}
	}

	int startTeam, endTeam;

	const int allegiance = LuaUtils::ParseAllegiance(L, __func__, 2);
	if (allegiance >= 0) {
		startTeam = allegiance;
		endTeam = allegiance;
	}
	else if (allegiance == LuaUtils::MyUnits) {
		const int readTeam = CLuaHandle::GetHandleReadTeam(L);
		startTeam = readTeam;
		endTeam = readTeam;
	}
	else {
		startTeam = 0;
		endTeam = teamHandler.ActiveTeams() - 1;
	}

	const auto planesTest = [&](const CUnit *unit, const float3 &pos) {
		return UnitInPlanes(pos, unit->radius, planes);
	};

	lua_newtable(L);

	for (int team = startTeam; team <= endTeam; team++) {
		if (allegiance == LuaUtils::AllyUnits && !LuaUtils::IsAlliedTeam(L, team))
			continue;
		if (allegiance == LuaUtils::EnemyUnits && LuaUtils::IsAlliedTeam(L, team))
			continue;

		const std::vector<CUnit*>& units = unitHandler.GetUnitsByTeam(team);

		GetFilteredUnits(L, allegiance, units, planesTest);
	}

	return 1;
}


static int GetUnitTableCentroid(lua_State *const L, const int indexWithinTable, const char *const caller)
{
	if (!lua_istable(L, 1))
		luaL_error(L, "[%s] argument must be a table", caller);

	float3 center {0.0f, 0.0f, 0.0f};
	size_t count = 0;
	for (lua_pushnil(L); lua_next(L, 1); lua_pop(L, 1)) {
		const auto unit = ParseUnit(L, caller, indexWithinTable);
		if (unit == nullptr)
			continue;

		center += unit->midPos;
		++ count;
	}

	if (!count)
		return 0;

	center /= static_cast <float> (count);

	lua_pushnumber(L, center.x);
	lua_pushnumber(L, center.y);
	lua_pushnumber(L, center.z);

	return 3;
}



/*** Returns the centroid of an array of units
 *
 * Returns nil for an empty array
 *
 * @function Spring.GetUnitArrayCentroid
 * @param units table { unitID, unitID, ... }
 * @return number centerX
 * @return number centerY
 * @return number centerZ
 */
int LuaSyncedRead::GetUnitArrayCentroid(lua_State* L)
{
	return GetUnitTableCentroid(L, -1, __func__);
}

/*** Returns the centroid of a map of units
 *
 * Returns nil for an empty map
 *
 * @function Spring.GetUnitMapCentroid
 * @param units table { [unitID] = true, [unitID] = true, ... }
 * @return number centerX
 * @return number centerY
 * @return number centerZ
 */
int LuaSyncedRead::GetUnitMapCentroid(lua_State* L)
{
	return GetUnitTableCentroid(L, -2, __func__);
}


/***
 *
 * @function Spring.GetUnitNearestAlly
 * @param unitID UnitID
 * @param range number? (Default: `1.0e9`)
 * @return UnitID? unitID
 */
int LuaSyncedRead::GetUnitNearestAlly(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const float range = luaL_optnumber(L, 2, 1.0e9f);
	const CUnit* target =
		CGameHelper::GetClosestFriendlyUnit(unit, unit->pos, range, unit->allyteam);

	if (target != nullptr) {
		lua_pushnumber(L, target->id);
		return 1;
	}
	return 0;
}


/***
 *
 * @function Spring.GetUnitNearestEnemy
 * @param unitID UnitID
 * @param range number? (Default: `1.0e9`) range of the search.
 * @param useLOS boolean? (Default: `true`) requires LOS/radar visibility of allied team.
 * @param sphereDistTest boolean? (Default: `false`) determines if using spherical(3D, includes target radius) or cylindrical(2D) search.
 * @param checkSightDist boolean? (Default: `false`) determine if during filter process, if candidate distance to be within candidate LOS radius.
 * @return UnitID? unitID
 */
int LuaSyncedRead::GetUnitNearestEnemy(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const bool wantLOS = !lua_isboolean(L, 3) || lua_toboolean(L, 3);
	const bool testLOS = !CLuaHandle::GetHandleFullRead(L) || wantLOS;

	const bool sphereDistTest = luaL_optboolean(L, 4, false);
	const bool checkSightDist = luaL_optboolean(L, 5, false);

	// if ignoring LOS, pass checkSightDist=false (by default)
	// such that enemies outside unit's los-range are included
	const CUnit* target = testLOS?
		CGameHelper::GetClosestEnemyUnit         (unit, unit->pos, luaL_optnumber(L, 2, 1.0e9f), unit->allyteam                                ):
		CGameHelper::GetClosestEnemyUnitNoLosTest(unit, unit->pos, luaL_optnumber(L, 2, 1.0e9f), unit->allyteam, sphereDistTest, checkSightDist);

	if (target == nullptr)
		return 0;

	lua_pushnumber(L, target->id);
	return 1;
}

/*** Returns the enemy unit closest to a position.
 *
 * @function Spring.GetClosestEnemyUnit
 * @param x number x coordinate of query position
 * @param y number y coordinate of query position
 * @param z number z coordinate of query position
 * @param range number? (Default: `1.0e9`)
 * @param allyTeamID AllyTeamID? whose enemies to consider, always own in non-full-read contexts
 * @param useLOS boolean? (Default: true) requires LOS/radar visibility or not. Always true in non-full-read contexts
 * @param sphereDistTest boolean? (Default: `false`) For non-LOS mode only. Determines if using spherical(3D, includes target radius) or cylindrical(2D) search
 * @param checkSightDist boolean? (Default: `false`) For non-LOS mode only. Determine if during filter process, if candidate distance to be within candidate LOS radius
 * @return UnitID? unitID
 */
int LuaSyncedRead::GetClosestEnemyUnit(lua_State* L)
{
	const float3 pos
		( luaL_checkfloat(L, 1)
		, luaL_checkfloat(L, 2)
		, luaL_checkfloat(L, 3)
	);

	const auto range = luaL_optnumber(L, 4, 1.0e9f);

	const auto allyTeamID = CLuaHandle::GetHandleFullRead(L)
		? luaL_checkint(L, 5)
		: CLuaHandle::GetHandleReadAllyTeam(L)
	;
	if (!teamHandler.IsValidAllyTeam(allyTeamID))
		return 0;

	const auto wantLOS = luaL_optboolean(L, 6, true);
	const auto testLOS = wantLOS || !CLuaHandle::GetHandleFullRead(L);

	const auto sphereDistTest = luaL_optboolean(L, 7, false);
	const auto checkSightDist = luaL_optboolean(L, 8, false);

	const auto target = testLOS
		? CGameHelper::GetClosestEnemyUnit         (nullptr, pos, range, allyTeamID)
		: CGameHelper::GetClosestEnemyUnitNoLosTest(nullptr, pos, range, allyTeamID, sphereDistTest, checkSightDist)
	;

	if (target == nullptr)
		return 0;

	lua_pushnumber(L, target->id);
	return 1;
}

/******************************************************************************
 * Spatial feature queries
 *
 * @section spatial_feature_queries
******************************************************************************/


inline void ProcessFeatures(lua_State* L, const vector<CFeature*>& features) {
	const unsigned int featureCount = features.size();
	unsigned int arrayIndex = 1;

	lua_createtable(L, featureCount, 0);

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		if (CLuaHandle::GetHandleFullRead(L)) {
			for (unsigned int i = 0; i < featureCount; i++) {
				const CFeature* feature = features[i];

				lua_pushnumber(L, feature->id);
				lua_rawseti(L, -2, arrayIndex++);
			}
		}
	} else {
		for (unsigned int i = 0; i < featureCount; i++) {
			const CFeature* feature = features[i];

			if (!LuaUtils::IsFeatureVisible(L, feature)) {
				continue;
			}

			lua_pushnumber(L, feature->id);
			lua_rawseti(L, -2, arrayIndex++);
		}
	}
}


/***
 *
 * @function Spring.GetFeaturesInRectangle
 * @param xmin number
 * @param zmin number
 * @param xmax number
 * @param zmax number
 * @return FeatureID[] featureIDs
 */
int LuaSyncedRead::GetFeaturesInRectangle(lua_State* L)
{
	const float xmin = luaL_checkfloat(L, 1);
	const float zmin = luaL_checkfloat(L, 2);
	const float xmax = luaL_checkfloat(L, 3);
	const float zmax = luaL_checkfloat(L, 4);

	const float3 mins(xmin, 0.0f, zmin);
	const float3 maxs(xmax, 0.0f, zmax);

	QuadFieldQuery qfQuery;
	quadField.GetFeaturesExact(qfQuery, mins, maxs);
	ProcessFeatures(L, *qfQuery.features);
	return 1;
}


/***
 *
 * @function Spring.GetFeaturesInSphere
 * @param x number
 * @param y number
 * @param z number
 * @param radius number
 * @return FeatureID[] featureIDs
 */
int LuaSyncedRead::GetFeaturesInSphere(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float y = luaL_checkfloat(L, 2);
	const float z = luaL_checkfloat(L, 3);
	const float rad = luaL_checkfloat(L, 4);

	const float3 pos(x, y, z);

	QuadFieldQuery qfQuery;
	quadField.GetFeaturesExact(qfQuery, pos, rad, true);
	ProcessFeatures(L, *qfQuery.features);
	return 1;
}


/***
 *
 * @function Spring.GetFeaturesInCylinder
 * @param x number
 * @param z number
 * @param radius number
 * @param allegiance integer?
 * @return FeatureID[] featureIDs
 */
int LuaSyncedRead::GetFeaturesInCylinder(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	const float rad = luaL_checkfloat(L, 3);

	const float3 pos(x, 0, z);

	QuadFieldQuery qfQuery;
	quadField.GetFeaturesExact(qfQuery, pos, rad, false);
	ProcessFeatures(L, *qfQuery.features);
	return 1;
}

static void GetProjectilesLuaTable(lua_State* L, const std::vector<CProjectile*>& projectiles,
                                      bool excludeWeaponProjectiles, bool excludePieceProjectiles)
{
	int arrayIndex = 1;

	lua_createtable(L, static_cast<int>(projectiles.size()), 0);

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		if (CLuaHandle::GetHandleFullRead(L)) {
			for (auto* pro : projectiles) {
				// filter out unsynced projectiles, the SyncedRead
				// projecile Get* functions accept only synced ID's
				// (specifically they interpret all ID's as synced)
				if (!pro->synced)
					continue;

				if (pro->weapon && excludeWeaponProjectiles)
					continue;
				if (pro->piece && excludePieceProjectiles)
					continue;

				lua_pushinteger(L, pro->id);
				lua_rawseti(L, -2, arrayIndex++);
			}
		}
	} else {
		for (auto* pro : projectiles) {
			// see above
			if (!pro->synced)
				continue;

			if (pro->weapon && excludeWeaponProjectiles)
				continue;
			if (pro->piece && excludePieceProjectiles)
				continue;

			if (!LuaUtils::IsProjectileVisible(L, pro))
				continue;

			lua_pushinteger(L, pro->id);
			lua_rawseti(L, -2, arrayIndex++);
		}
	}
}

/***
 *
 * @function Spring.GetAllProjectiles
 * @param excludeWeaponProjectiles boolean? (Default: `false`)
 * @param excludePieceProjectiles boolean? (Default: `false`)
 * @return ProjectileID[] projectileIDs
 */
int LuaSyncedRead::GetAllProjectiles(lua_State* L)
{
	const bool excludeWeaponProjectiles = luaL_optboolean(L, 1, false);
	const bool excludePieceProjectiles  = luaL_optboolean(L, 2, false);
	const auto& projVec = projectileHandler.GetActiveProjectiles(true).GetData();
	GetProjectilesLuaTable(L, projVec, excludeWeaponProjectiles, excludePieceProjectiles);
	return 1;
}

/***
 *
 * @function Spring.GetProjectilesInRectangle
 * @param xmin number
 * @param zmin number
 * @param xmax number
 * @param zmax number
 * @param excludeWeaponProjectiles boolean? (Default: `false`)
 * @param excludePieceProjectiles boolean? (Default: `false`)
 * @return ProjectileID[] projectileIDs
 */
int LuaSyncedRead::GetProjectilesInRectangle(lua_State* L)
{
	const float xmin = luaL_checkfloat(L, 1);
	const float zmin = luaL_checkfloat(L, 2);
	const float xmax = luaL_checkfloat(L, 3);
	const float zmax = luaL_checkfloat(L, 4);

	const bool excludeWeaponProjectiles = luaL_optboolean(L, 5, false);
	const bool excludePieceProjectiles = luaL_optboolean(L, 6, false);

	const float3 mins(xmin, 0.0f, zmin);
	const float3 maxs(xmax, 0.0f, zmax);

	QuadFieldQuery qfQuery;
	quadField.GetProjectilesExact(qfQuery, mins, maxs);
	GetProjectilesLuaTable(L, *qfQuery.projectiles, excludeWeaponProjectiles, excludePieceProjectiles);
	return 1;
}

/***
 *
 * @function Spring.GetProjectilesInSphere
 * @param x number
 * @param y number
 * @param z number
 * @param radius number
 * @param excludeWeaponProjectiles boolean? (Default: false)
 * @param excludePieceProjectiles boolean? (Default: false)
 * @return ProjectileID[] projectileIDs
 */
int LuaSyncedRead::GetProjectilesInSphere(lua_State* L)
{
	const float3 sphereCenter(luaL_checkfloat(L, 1), luaL_checkfloat(L, 2), luaL_checkfloat(L, 3));
	const float radius = luaL_checkfloat(L, 4);

	const bool excludeWeaponProjectiles = luaL_optboolean(L, 5, false);
	const bool excludePieceProjectiles = luaL_optboolean(L, 6, false);

	QuadFieldQuery qfQuery;
	quadField.GetProjectilesExact(qfQuery, sphereCenter, radius);
	GetProjectilesLuaTable(L, *qfQuery.projectiles, excludeWeaponProjectiles, excludePieceProjectiles);
	return 1;
}

/******************************************************************************
 * Unit state
 *
 * @section unit_state
******************************************************************************/


/*** Get whether a unitID is valid
 *
 * Dead units are not valid.
 *
 * @function Spring.ValidUnitID
 * @param unitID UnitID
 * @return boolean
 */
int LuaSyncedRead::ValidUnitID(lua_State* L)
{
	lua_pushboolean(L, lua_isnumber(L, 1) && ParseUnit(L, __func__, 1) != nullptr);
	return 1;
}


/***
 * @class UnitState
 * @x_helper
 * @field firestate integer
 * @field movestate integer
 * @field repeat boolean?
 * @field cloak boolean?
 * @field active boolean?
 * @field trajectory boolean?
 * @field autoland boolean?
 * @field autorepairlevel number?
 * @field loopbackattack boolean?
 */


/***
 *
 * Gets various states for the given unit.
 *
 * @function Spring.GetUnitStates
 * @param unitID UnitID
 * @param retTable false Return a table instead of multiple values. Defaults to `true`
 * @param binState true Include binary state (activated, etc)? Defaults to `retTable`
 * @param amtState true Include Air/Hover MoveType state if available? Defaults to `retTable`
 * @return number fireState
 * @return number moveState
 * @return number autorepairlevel `-1` if not set
 * @return boolean repeat
 * @return boolean cloak
 * @return boolean active
 * @return boolean trajectory
 * @return boolean? autoLand
 * @return boolean? loopbackAttack
 * @overload fun(unitID: UnitID, retTable: false, binState: false?, amtState: false?): number, number, number
 * @overload fun(unitID: UnitID, retTable: false, binState: true, amtState: false?): number, number, number, boolean, boolean, boolean, boolean
 * @overload fun(unitID: UnitID, retTable: false, binState: false?, amtState: true): number, number, number, boolean?, boolean?
 * @overload fun(unitID: UnitID, retTable: true?, binState: boolean?, amtState: boolean?): UnitState
 */
int LuaSyncedRead::GetUnitStates(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const AMoveType* mt = unit->moveType; // never null
	const CMobileCAI* mCAI = dynamic_cast<const CMobileCAI*>(unit->commandAI);

	const bool retTable = luaL_optboolean(L, 2,     true); // return state as table?
	const bool binState = luaL_optboolean(L, 3, retTable); // include binary state? (activated, etc)
	const bool amtState = luaL_optboolean(L, 4, retTable); // include (Air)MoveType state?

	if (!retTable) {
		{
			lua_pushnumber(L, unit->fireState);
			lua_pushnumber(L, unit->moveState);
			lua_pushnumber(L, (mCAI != nullptr)? mCAI->repairBelowHealth: -1.0f);
		}

		if (binState) {
			lua_pushboolean(L, unit->commandAI->repeatOrders);
			lua_pushboolean(L, unit->wantCloak);
			lua_pushboolean(L, unit->activated);
			lua_pushboolean(L, unit->useHighTrajectory);
		}

		if (amtState) {
			const CHoverAirMoveType* hAMT = nullptr;
			const CStrafeAirMoveType* sAMT = nullptr;

			if ((hAMT = dynamic_cast<const CHoverAirMoveType*>(mt)) != nullptr) {
				lua_pushboolean(L, hAMT->autoLand);
				lua_pushboolean(L, false);
				return (3 + (binState * 4) + 2);
			}

			if ((sAMT = dynamic_cast<const CStrafeAirMoveType*>(mt)) != nullptr) {
				lua_pushboolean(L, sAMT->autoLand);
				lua_pushboolean(L, sAMT->loopbackAttack);
				return (3 + (binState * 4) + 2);
			}
		}

		// reached only if AMT vars were not pushed
		return (3 + (binState * 4));
	}

	{
		lua_createtable(L, 0, 9);

		{
			LuaPushNamedNumber(L, "firestate",  unit->fireState);
			LuaPushNamedNumber(L, "movestate",  unit->moveState);
			LuaPushNamedNumber(L, "autorepairlevel", (mCAI != nullptr)? mCAI->repairBelowHealth: -1.0f);
		}

		if (binState) {
			LuaPushNamedBool(L, "repeat",     unit->commandAI->repeatOrders);
			LuaPushNamedBool(L, "cloak",      unit->wantCloak);
			LuaPushNamedBool(L, "active",     unit->activated);
			LuaPushNamedBool(L, "trajectory", unit->useHighTrajectory);
		}

		if (amtState) {
			const CHoverAirMoveType* hAMT = nullptr;
			const CStrafeAirMoveType* sAMT = nullptr;

			if ((hAMT = dynamic_cast<const CHoverAirMoveType*>(mt)) != nullptr) {
				LuaPushNamedBool(L, "autoland",       hAMT->autoLand);
				LuaPushNamedBool(L, "loopbackattack", false);
				return 1;
			}

			if ((sAMT = dynamic_cast<const CStrafeAirMoveType*>(mt)) != nullptr) {
				LuaPushNamedBool(L, "autoland",       sAMT->autoLand);
				LuaPushNamedBool(L, "loopbackattack", sAMT->loopbackAttack);
				return 1;
			}
		}

		return 1;
	}
}


/***
 *
 * @function Spring.GetUnitArmored
 * @param unitID UnitID
 * @return boolean? armored
 * @return number armorMultiple
 */
int LuaSyncedRead::GetUnitArmored(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->armoredState);
	lua_pushnumber(L, unit->armoredMultiple);
	return 2;
}


/***
 *
 * @function Spring.GetUnitIsActive
 * @param unitID UnitID
 * @return boolean? isActive
 */
int LuaSyncedRead::GetUnitIsActive(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->activated);
	return 1;
}


/***
 *
 * @function Spring.GetUnitIsCloaked
 * @param unitID UnitID
 * @return boolean? isCloaked
 */
int LuaSyncedRead::GetUnitIsCloaked(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->isCloaked);
	return 1;
}


/***
 *
 * @function Spring.GetUnitSeismicSignature
 * @param unitID UnitID
 * @return number? seismicSignature
 */
int LuaSyncedRead::GetUnitSeismicSignature(lua_State* L)
{
	const CUnit* const unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->seismicSignature);
	return 1;
}

/*** Get whether unit leaves static radar ghosts.
 *
 * @function Spring.GetUnitLeavesGhost
 * @number unitID
 * @return number?
 */
int LuaSyncedRead::GetUnitLeavesGhost(lua_State* L)
{
	const CUnit* const unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->leavesGhost);
	return 1;
}

/***
 *
 * @function Spring.GetUnitSelfDTime
 * @param unitID UnitID
 * @return integer? selfDTime
 */
int LuaSyncedRead::GetUnitSelfDTime(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->selfDCountdown);
	return 1;
}


/***
 *
 * @function Spring.GetUnitStockpile
 * @param unitID UnitID
 * @return integer? numStockpiled
 * @return integer? numStockpileQued
 * @return number? buildPercent
 */
int LuaSyncedRead::GetUnitStockpile(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if (unit->stockpileWeapon == nullptr)
		return 0;

	lua_pushnumber(L, unit->stockpileWeapon->numStockpiled);
	lua_pushnumber(L, unit->stockpileWeapon->numStockpileQued);
	lua_pushnumber(L, unit->stockpileWeapon->buildPercent);
	return 3;
}


/***
 *
 * @function Spring.GetUnitSensorRadius
 * @param unitID UnitID
 * @param type string one of los, airLos, radar, sonar, seismic, radarJammer, sonarJammer
 * @return integer? radius
 */
int LuaSyncedRead::GetUnitSensorRadius(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	switch (hashString(luaL_checkstring(L, 2))) {
		case hashString("los"): {
			lua_pushnumber(L, unit->losRadius);
		} break;
		case hashString("airLos"): {
			lua_pushnumber(L, unit->airLosRadius);
		} break;
		case hashString("radar"): {
			lua_pushnumber(L, unit->radarRadius);
		} break;
		case hashString("sonar"): {
			lua_pushnumber(L, unit->sonarRadius);
		} break;
		case hashString("seismic"): {
			lua_pushnumber(L, unit->seismicRadius);
		} break;
		case hashString("radarJammer"): {
			lua_pushnumber(L, unit->jammerRadius);
		} break;
		case hashString("sonarJammer"): {
			lua_pushnumber(L, unit->sonarJamRadius);
		} break;
		default: {
			luaL_error(L, "[%s] unknown sensor type \"%s\"", __func__, luaL_checkstring(L, 2));
		} break;
	}

	return 1;
}

/***
 *
 * @function Spring.GetUnitPosErrorParams
 * @param unitID UnitID
 * @param allyTeamID AllyTeamID?
 * @return number? posErrorVectorX
 * @return number posErrorVectorY
 * @return number posErrorVectorZ
 * @return number posErrorDeltaX
 * @return number posErrorDeltaY
 * @return number posErrorDeltaZ
 * @return integer nextPosErrorUpdate
 * @return boolean posErrorBit
 */
int LuaSyncedRead::GetUnitPosErrorParams(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int optAllyTeam = luaL_optinteger(L, 2, 0);
	const int argAllyTeam = std::clamp(optAllyTeam, 0, teamHandler.ActiveAllyTeams());

	lua_pushnumber(L, unit->posErrorVector.x);
	lua_pushnumber(L, unit->posErrorVector.y);
	lua_pushnumber(L, unit->posErrorVector.z);
	lua_pushnumber(L, unit->posErrorDelta.x);
	lua_pushnumber(L, unit->posErrorDelta.y);
	lua_pushnumber(L, unit->posErrorDelta.z);
	lua_pushnumber(L, unit->nextPosErrorUpdate);
	lua_pushboolean(L, unit->GetPosErrorBit(argAllyTeam));

	return (3 + 3 + 1 + 1);
}


/***
 *
 * @function Spring.GetUnitTooltip
 * @param unitID UnitID
 * @return string?
 */
int LuaSyncedRead::GetUnitTooltip(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	std::string tooltip;

	const CTeam* unitTeam = nullptr;
	const UnitDef* unitDef = unit->unitDef;
	const UnitDef* decoyDef = LuaUtils::IsAllyUnit(L, unit) ? nullptr : unitDef->decoyDef;
	const UnitDef* effectiveDef = LuaUtils::EffectiveUnitDef(L, unit);

	if (effectiveDef->showPlayerName) {
		if (teamHandler.IsValidTeam(unit->team))
			unitTeam = teamHandler.Team(unit->team);

		if (unitTeam != nullptr && unitTeam->HasLeader()) {
			tooltip = playerHandler.Player(unitTeam->GetLeader())->name;
			tooltip = (skirmishAIHandler.HasSkirmishAIsInTeam(unit->team)? "AI@": "") + tooltip;
		}
	} else {
		if (decoyDef == nullptr) {
			tooltip = unitToolTipMap.Get(unit->id);
		} else {
			tooltip = decoyDef->humanName + " - " + decoyDef->tooltip;
		}
	}

	lua_pushsstring(L, tooltip);
	return 1;
}


/***
 *
 * @function Spring.GetUnitDefID
 * @param unitID UnitID
 * @return UnitDefID?
 */
int LuaSyncedRead::GetUnitDefID(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if (LuaUtils::IsAllyUnit(L, unit)) {
		lua_pushnumber(L, unit->unitDef->id);
		return 1;
	}

	if (!LuaUtils::IsUnitTyped(L, unit))
		return 0;

	lua_pushnumber(L, LuaUtils::EffectiveUnitDef(L, unit)->id);
	return 1;
}

/***
* @function Spring.GetUnitMoveDefID
*
* Returns a numerical movedef ID and its name. For things that have
* no movedef, returns `false` (to tell them apart from unreadable
* units while keeping the `if not x` pattern usable). For now, the
* numerical ID is not too useful so you can use the name, but this
* may get deprecated at some point.
* 
* @param unitID UnitID
*
* @return integer|false|nil moveDefID
* @return string|nil moveDefName
*/

int LuaSyncedRead::GetUnitMoveDefID(lua_State* L) 
{
	const auto unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const auto moveDef = unit->moveDef;
	if (moveDef == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushnumber(L, moveDef->pathType);
	lua_pushsstring(L, moveDef->name);
	return 2;
}



/***
 *
 * @function Spring.GetUnitTeam
 * @param unitID UnitID
 * @return TeamID?
 */
int LuaSyncedRead::GetUnitTeam(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->team);
	return 1;
}


/***
 *
 * @function Spring.GetUnitAllyTeam
 * @param unitID UnitID
 * @return AllyTeamID?
 */
int LuaSyncedRead::GetUnitAllyTeam(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->allyteam);
	return 1;
}


/*** Checks if a unit is neutral (NOT Gaia!)
 *
 * @function Spring.GetUnitNeutral
 *
 * Note that a "neutral" unit can belong to any ally-team (ally, enemy, Gaia).
 * To check if a unit is Gaia, check its owner team.
 *
 * @param unitID UnitID
 * @return boolean?
 */
int LuaSyncedRead::GetUnitNeutral(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->IsNeutral());
	return 1;
}


/***
 *
 * @function Spring.GetUnitHealth
 * @param unitID UnitID
 * @return number? health
 * @return number maxHealth
 * @return number paralyzeDamage
 * @return number captureProgress
 * @return number buildProgress between 0.0-1.0
 */
int LuaSyncedRead::GetUnitHealth(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const UnitDef* ud = unit->unitDef;
	const bool enemyUnit = LuaUtils::IsEnemyUnit(L, unit);

	if (ud->hideDamage && enemyUnit) {
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	} else if (!enemyUnit || (ud->decoyDef == nullptr)) {
		lua_pushnumber(L, unit->health);
		lua_pushnumber(L, unit->maxHealth);
		lua_pushnumber(L, unit->paralyzeDamage);
	} else {
		const float scale = (ud->decoyDef->health / ud->health);
		lua_pushnumber(L, scale * unit->health);
		lua_pushnumber(L, scale * unit->maxHealth);
		lua_pushnumber(L, scale * unit->paralyzeDamage);
	}
	lua_pushnumber(L, unit->captureProgress);
	lua_pushnumber(L, unit->buildProgress);
	return 5;
}


/***
 *
 * @function Spring.GetUnitIsDead
 * @param unitID UnitID
 * @return boolean?
 */
int LuaSyncedRead::GetUnitIsDead(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->isDead);
	return 1;
}


/*** Checks whether a unit is disabled and can't act
 *
 * The first return value is a simple OR of the following ones,
 * any of those conditions is sufficient to disable the unit.
 *
 * Note that EMP and being transported are mechanically the same and thus lumped together.
 * Use other callouts to differentiate them if you need to.
 *
 * @function Spring.GetUnitIsStunned
 * @param unitID UnitID
 * @return boolean? stunnedOrBuilt unit is disabled
 * @return boolean stunned unit is either stunned via EMP or being transported by a non-fireplatform
 * @return boolean beingBuilt unit is under construction
 */
int LuaSyncedRead::GetUnitIsStunned(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->IsStunned() || unit->beingBuilt);
	lua_pushboolean(L, unit->IsStunned());
	lua_pushboolean(L, unit->beingBuilt);
	return 3;
}


/***
 *
 * @function Spring.GetUnitIsBeingBuilt
 * @param unitID UnitID
 * @return boolean beingBuilt
 * @return number buildProgress
 */
int LuaSyncedRead::GetUnitIsBeingBuilt(lua_State* L)
{
	const auto unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->beingBuilt);
	lua_pushnumber(L, unit->buildProgress);
	return 2;
}

/***
 *
 * @function Spring.GetUnitResources
 * @param unitID UnitID
 * @return number? metalMake
 * @return number metalUse
 * @return number energyMake
 * @return number energyUse
 */
int LuaSyncedRead::GetUnitResources(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->resourcesMake.metal);
	lua_pushnumber(L, unit->resourcesUse.metal);
	lua_pushnumber(L, unit->resourcesMake.energy);
	lua_pushnumber(L, unit->resourcesUse.energy);
	return 4;
}

/***
 * @function Spring.GetUnitStorage
 * @param unitID UnitID
 * @return number metalStorage Unit's metal storage
 * @return number energyStorage Unit's energy storage
 */
int LuaSyncedRead::GetUnitStorage(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->storage.metal);
	lua_pushnumber(L, unit->storage.energy);
	return 2;
}

/***
 * @function Spring.GetUnitCosts
 * @param unitID UnitID
 * @return number? buildTime
 * @return number metalCost
 * @return number energyCost
 */
int LuaSyncedRead::GetUnitCosts(lua_State* L)
{
	const CUnit* const unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->buildTime);
	lua_pushnumber(L, unit->cost.metal);
	lua_pushnumber(L, unit->cost.energy);
	return 3;
}

/***
 * @class ResourceCost
 * @x_helper
 * @field metal number
 * @field energy number
 */

/***
 * @function Spring.GetUnitCostTable
 * @param unitID UnitID
 * @return ResourceCost? cost The cost of the unit, or `nil` if invalid.
 * @return number? buildTime The build time the unit, or `nil` if invalid.
 */
int LuaSyncedRead::GetUnitCostTable(lua_State* L)
{
	const CUnit* const unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;
	lua_createtable(L, 0, 2);
	lua_pushstring(L, "metal");
	lua_pushnumber(L, unit->cost.metal);
	lua_rawset(L, -3);
	lua_pushstring(L, "energy");
	lua_pushnumber(L, unit->cost.energy);
	lua_rawset(L, -3);
	lua_pushnumber(L, unit->buildTime);
	return 2;
}


/***
 *
 * @function Spring.GetUnitMetalExtraction
 * @param unitID UnitID
 * @return number? metalExtraction
 */
int LuaSyncedRead::GetUnitMetalExtraction(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if (!unit->unitDef->extractsMetal)
		return 0;

	lua_pushnumber(L, unit->metalExtract);
	return 1;
}


/***
 *
 * @function Spring.GetUnitExperience
 * @param unitID UnitID
 * @return number xp [0.0; +∞)
 * @return number limXp [0.0; 1.0) as experience approaches infinity
 */
int LuaSyncedRead::GetUnitExperience(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->experience);
	lua_pushnumber(L, unit->limExperience);
	return 2;
}


/***
 *
 * @function Spring.GetUnitHeight
 * @param unitID UnitID
 * @return number?
 */
int LuaSyncedRead::GetUnitHeight(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->height);
	return 1;
}


/***
 *
 * @function Spring.GetUnitRadius
 * @param unitID UnitID
 * @return number?
 */
int LuaSyncedRead::GetUnitRadius(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->radius);
	return 1;
}

/***
 *
 * @function Spring.GetUnitBuildeeRadius
 * Gets the unit's radius for when targeted by build, repair, reclaim-type commands.
 * @param unitID UnitID
 * @return number?
 */
int LuaSyncedRead::GetUnitBuildeeRadius(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->buildeeRadius);
	return 1;
}

/***
 *
 * @function Spring.GetUnitMass
 * @param unitID UnitID
 * @return number?
 */
int LuaSyncedRead::GetUnitMass(lua_State* L)
{
	return (GetSolidObjectMass(L, ParseInLosUnit(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetUnitPosition
 * @param unitID UnitID
 * @param midPos boolean? (Default: `false`) return midpoint as well
 * @param aimPos boolean? (Default: `false`) return aimpoint as well
 * @return number? basePointX
 * @return number basePointY
 * @return number basePointZ
 * @return number? midPointX
 * @return number midPointY
 * @return number midPointZ
 * @return number? aimPointX
 * @return number aimPointY
 * @return number aimPointZ
 */
int LuaSyncedRead::GetUnitPosition(lua_State* L)
{
	return (GetSolidObjectPosition(L, ParseUnit(L, __func__, 1), false));
}

/***
 *
 * @function Spring.GetUnitBasePosition
 * The same as `Spring.GetUnitPosition`, but without the optional midpoint calculations.
 * @see Spring.GetUnitPosition 
 * @param unitID UnitID
 * @return number? posX
 * @return number? posY
 * @return number? posZ
 */
int LuaSyncedRead::GetUnitBasePosition(lua_State* L)
{
	return (GetUnitPosition(L));
}


/***
 *
 * @function Spring.GetUnitVectors
 * @param unitID UnitID
 * @return float3? front
 * @return float3 up
 * @return float3 right
 */
int LuaSyncedRead::GetUnitVectors(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

#define PACK_VECTOR(n) \
	lua_createtable(L, 3, 0);            \
	lua_pushnumber(L, unit-> n .x); lua_rawseti(L, -2, 1); \
	lua_pushnumber(L, unit-> n .y); lua_rawseti(L, -2, 2); \
	lua_pushnumber(L, unit-> n .z); lua_rawseti(L, -2, 3)

	PACK_VECTOR(frontdir);
	PACK_VECTOR(updir);
	PACK_VECTOR(rightdir);

	return 3;
}


/***
 *
 * @function Spring.GetUnitRotation
 * Note: PYR order
 * @param unitID UnitID
 * @return number pitch Rotation in X axis
 * @return number yaw Rotation in Y axis
 * @return number roll Rotation in Z axis
 */
int LuaSyncedRead::GetUnitRotation(lua_State* L)
{
	return (GetSolidObjectRotation(L, ParseInLosUnit(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetUnitDirection
 * @param unitID UnitID
 * @return number frontDirX
 * @return number frontDirY
 * @return number frontDirZ
 * @return number rightDirX
 * @return number rightDirY
 * @return number rightDirZ
 * @return number upDirX
 * @return number upDirY
 * @return number upDirZ
 */
int LuaSyncedRead::GetUnitDirection(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->frontdir.x);
	lua_pushnumber(L, unit->frontdir.y);
	lua_pushnumber(L, unit->frontdir.z);

	lua_pushnumber(L, unit->rightdir.x);
	lua_pushnumber(L, unit->rightdir.y);
	lua_pushnumber(L, unit->rightdir.z);

	lua_pushnumber(L, unit->updir.x);
	lua_pushnumber(L, unit->updir.y);
	lua_pushnumber(L, unit->updir.z);

	return 9;
}


/***
 *
 * @function Spring.GetUnitHeading
 * @param unitID UnitID
 * @param convertToRadians boolean? (Default: `false`)
 * @return number heading
 */
int LuaSyncedRead::GetUnitHeading(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	float heading = unit->heading;
	if (luaL_optboolean(L, 2, false)) {
		heading = ClampRadPi(math::PI / 32768.0f * heading);
	}

	lua_pushnumber(L, heading);
	return 1;
}


/***
 *
 * @function Spring.GetUnitVelocity
 * @param unitID UnitID
 */
int LuaSyncedRead::GetUnitVelocity(lua_State* L)
{
	return (GetWorldObjectVelocity(L, ParseInLosUnit(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetUnitBuildFacing
 * @param unitID UnitID
 * @return FacingInteger? buildFacing facing of footprint, `0` - `3`
 */
int LuaSyncedRead::GetUnitBuildFacing(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->buildFacing);
	return 1;
}


/*** Checks whether a unit is currently building another (NOT for checking if it's a structure)
 *
 * @function Spring.GetUnitIsBuilding
 *
 * Works for both mobile builders and factories.
 *
 * @param unitID UnitID
 * @return UnitID? buildeeUnitID
 */
int LuaSyncedRead::GetUnitIsBuilding(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CBuilder* builder = dynamic_cast<const CBuilder*>(unit);

	if (builder != nullptr && builder->curBuild) {
		lua_pushnumber(L, builder->curBuild->id);
		return 1;
	}

	const CFactory* factory = dynamic_cast<const CFactory*>(unit);

	if (factory != nullptr && factory->curBuild) {
		lua_pushnumber(L, factory->curBuild->id);
		return 1;
	}

	return 0;
}

static int GetBuilderWorkerTask(lua_State* L, const CBuilder *builder)
{
	assert(builder != nullptr);

	if (builder->curBuild) {
		lua_pushnumber(L, builder->curBuild->beingBuilt
			? -builder->curBuild->unitDef->id
			: CMD_REPAIR
		);
		lua_pushnumber(L, builder->curBuild->id);
		return 2;
	} else if (builder->curCapture) {
		lua_pushnumber(L, CMD_CAPTURE);
		lua_pushnumber(L, builder->curCapture->id);
		return 2;
	} else if (builder->curResurrect) {
		lua_pushnumber(L, CMD_RESURRECT);
		lua_pushnumber(L, builder->curResurrect->id + unitHandler.MaxUnits());
		return 2;
	} else if (builder->curReclaim) {
		lua_pushnumber(L, CMD_RECLAIM);
		if (builder->reclaimingUnit) {
			const auto reclaimee = dynamic_cast <const CUnit*> (builder->curReclaim);
			assert(reclaimee);
			lua_pushnumber(L, reclaimee->id);
		} else {
			const auto reclaimee = dynamic_cast <const CFeature*> (builder->curReclaim);
			assert(reclaimee);
			lua_pushnumber(L, reclaimee->id + unitHandler.MaxUnits());
		}
		return 2;
	} else if (builder->helpTerraform || builder->terraforming) {
		lua_pushnumber(L, CMD_RESTORE); // FIXME: could also be leveling ground before construction
		return 1;
	} else {
		return 0;
	}
}

static int GetFactoryWorkerTask(lua_State* L, const CFactory *factory)
{
	assert(factory != nullptr);

	if (factory->curBuild) {
		lua_pushnumber(L, factory->curBuild->beingBuilt
			? -factory->curBuild->unitDef->id
			: CMD_REPAIR // fullHealthFactory
		);
		lua_pushnumber(L, factory->curBuild->id);
		return 2;
	} else {
		return 0;
	}
}

/*** Checks a builder's current task
 *
 * @function Spring.GetUnitWorkerTask
 *
 * Checks what a builder is currently doing. This is not the same as `Spring.GetUnitCurrentCommand`,
 * because you can have a command at the front of the queue and not be doing it (for example because
 * the target is still too far away), and on the other hand you can also be doing a task despite not
 * having it in front of the queue (for example you're Guarding another builder who does). Also, it
 * resolves the Repair command into either actual repair, or construction assist (in which case it
 * returns the appropriate "build" command). Only build-related commands are returned (no Move or any
 * custom commands).
 *
 * The possible commands returned are repair, reclaim, resurrect, capture, restore,
 * and build commands (negative buildee unitDefID).
 *
 * @param unitID UnitID
 * @return integer cmdID of the relevant command
 * @return integer targetID if applicable (all except RESTORE)
 */
int LuaSyncedRead::GetUnitWorkerTask(lua_State* L)
{
	const auto unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	// perhaps this should be some sort of virtual function of CUnit?
	if (const auto builder = dynamic_cast <const CBuilder *> (unit); builder)
		return GetBuilderWorkerTask(L, builder);
	if (const auto factory = dynamic_cast <const CFactory *> (unit); factory)
		return GetFactoryWorkerTask(L, factory);

	return 0;
}

/***
 *
 * @function Spring.GetUnitEffectiveBuildRange
 * Useful for setting move goals manually.
 * @param unitID UnitID
 * @param buildeeDefID UnitDefID?
 * @return number effectiveBuildRange counted to the center of prospective buildee; buildRange if buildee nil
 */
int LuaSyncedRead::GetUnitEffectiveBuildRange(lua_State* L)
{
	const auto unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const auto builderCAI = dynamic_cast <const CBuilderCAI*> (unit->commandAI);
	if (builderCAI == nullptr)
		return 0;

	/* FIXME: there are some cases where a unitDefID does not suffice.
	 * This function was mostly created as a reactive afterthought so
	 * does not handle them properly, but accepting `nil` acknowledges
	 * their existence to some extent:
	 *
	 *  - features, for example reclaim. I think ideally a thingID would
	 *    be the third argument (exclusive with the unitDefID), but this
	 *    requires the featureID ticket (#717) to be done first.
	 *
	 *  - terraform (restore ground). Fourth boolean parameter? Sounds
	 *    like it's getting a bit bloated, though it's rare and doesn't
	 *    actually pollute the usual use cases.
	 *
	 *  - design question: would featureDefID ever be a sensible thing
	 *    to use here? I doubt, but it's something to keep in mind. */
	if (lua_isnoneornil(L, 2)) {
		lua_pushnumber(L, builderCAI->GetBuildRange(0.0f));
		return 1;
	}

	const auto buildeeDefID = luaL_checkint(L, 2);
	const auto unitDef = unitDefHandler->GetUnitDefByID(buildeeDefID);
	if (unitDef == nullptr)
		luaL_error(L, "Nonexistent buildeeDefID %d passed to Spring.GetUnitEffectiveBuildRange", (int) buildeeDefID);

	const auto model = unitDef->LoadModel();
	if (model == nullptr)
		return 0;

	/* FIXME: this is what BuilderCAI does, but can radius actually
	 * be negative? Sounds worth asserting otherwise at model load. */
	const auto radius = std::max(0.f, model->radius);

	const auto effectiveBuildRange = builderCAI->GetBuildRange(radius);
	lua_pushnumber(L, effectiveBuildRange);
	return 1;
}

/***
 *
 * @function Spring.GetUnitCurrentBuildPower
 * @param unitID UnitID
 * @return number? buildPower `nil` if the unit is neither a builder nor a factory.
 */
int LuaSyncedRead::GetUnitCurrentBuildPower(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const NanoPieceCache* pieceCache = nullptr;

	{
		const CBuilder* builder = dynamic_cast<const CBuilder*>(unit);

		if (builder != nullptr)
			pieceCache = &builder->GetNanoPieceCache();

		const CFactory* factory = dynamic_cast<const CFactory*>(unit);

		if (factory != nullptr)
			pieceCache = &factory->GetNanoPieceCache();
	}

	if (pieceCache == nullptr)
		return 0;

	lua_pushnumber(L, pieceCache->GetBuildPower());
	return 1;
}


/*** Get a unit's carried resources
 *
 * @function Spring.GetUnitHarvestStorage
 *
 * Checks resources being carried internally by the unit.
 *
 * @param unitID UnitID
 * @return number storedMetal
 * @return number maxStoredMetal
 * @return number storedEnergy
 * @return number maxStoredEnergy
 */
int LuaSyncedRead::GetUnitHarvestStorage(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
		lua_pushnumber(L, unit->harvested[i]);
		lua_pushnumber(L, unit->harvestStorage[i]);
	}
	return 2 * SResourcePack::MAX_RESOURCES;
}

/***
 *
 * @function Spring.GetUnitBuildParams
 * @param unitID UnitID
 * @param paramName "buildRange"|"buildDistance"|"buildRange3D" The build param to get
 * @return number|boolean|nil value number for `"buildRange"` or `"buildDistance"`,
 * boolean for `"buildRange3D"`, otherwise `nil` for unrecognized paramName or not
 * allied builder unit.
 */
int LuaSyncedRead::GetUnitBuildParams(lua_State* L)
{
	const CUnit * unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CBuilder* builder = dynamic_cast<const CBuilder*>(unit);

	if (builder == nullptr)
		return 0;

	switch (hashString(luaL_checkstring(L, 2))) {
	case hashString("buildRange"):
	case hashString("buildDistance"): {
		lua_pushnumber(L, builder->buildDistance);
		return 1;
	} break;
	case hashString("buildRange3D"): {
		lua_pushboolean(L, builder->range3D);
		return 1;
	} break;
	default: {} break;
	};

	return 0;
}

/*** Is builder in build stance
 *
 * @function Spring.GetUnitInBuildStance
 *
 * Checks if a builder is in build stance, i.e. can create nanoframes.
 * Returns nil for non-builders.
 *
 * @param unitID UnitID
 * @return boolean inBuildStance
 */
int LuaSyncedRead::GetUnitInBuildStance(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CBuilder* builder = dynamic_cast<const CBuilder*>(unit);

	if (builder == nullptr)
		return 0;

	lua_pushboolean(L, builder->inBuildStance);
	return 1;
}

/*** Get construction FX attachment points
 *
 * @function Spring.GetUnitNanoPieces
 *
 * Returns an array of pieces which represent construction
 * points. Default engine construction FX (nano spray) will
 * originate there.
 *
 * Only works on builders and factories, returns nil (NOT empty table)
 * for other units.
 *
 * @param unitID UnitID
 * @return integer[] pieceArray
 */
int LuaSyncedRead::GetUnitNanoPieces(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const NanoPieceCache* pieceCache = nullptr;
	const std::vector<int>* nanoPieces = nullptr;

	{
		const CBuilder* builder = dynamic_cast<const CBuilder*>(unit);

		if (builder != nullptr) {
			pieceCache = &builder->GetNanoPieceCache();
			nanoPieces = &pieceCache->GetNanoPieces();
		}

		const CFactory* factory = dynamic_cast<const CFactory*>(unit);

		if (factory != nullptr) {
			pieceCache = &factory->GetNanoPieceCache();
			nanoPieces = &pieceCache->GetNanoPieces();
		}
	}

	if (nanoPieces == nullptr || nanoPieces->empty())
		return 0;

	lua_createtable(L, nanoPieces->size(), 0);

	for (size_t p = 0; p < nanoPieces->size(); p++) {
		const int modelPieceNum = (*nanoPieces)[p];

		lua_pushnumber(L, modelPieceNum + 1); //lua 1-indexed, c++ 0-indexed
		lua_rawseti(L, -2, p + 1);
	}

	return 1;
}


/*** Get the transport carrying the unit
 *
 * @function Spring.GetUnitTransporter
 *
 * Returns the unit ID of the transport, if any.
 * Returns nil if the unit is not being transported.
 *
 * @param unitID UnitID
 * @return UnitID? transportUnitID
 */
int LuaSyncedRead::GetUnitTransporter(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if (unit->transporter == nullptr)
		return 0;

	lua_pushnumber(L, unit->transporter->id);
	return 1;
}


/***
 * Get units being transported
 *
 * @function Spring.GetUnitIsTransporting
 * @param unitID UnitID
 * @return UnitID[]? transporteeArray
 * An array of unitIDs being transported by this unit, or `nil` if not a transport.
 */
int LuaSyncedRead::GetUnitIsTransporting(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr || !unit->unitDef->IsTransportUnit())
		return 0;

	lua_createtable(L, unit->transportedUnits.size(), 0);

	unsigned int unitCount = 1;

	for (const CUnit::TransportedUnit& tu: unit->transportedUnits) {
		const CUnit* carried = tu.unit;

		lua_pushnumber(L, carried->id);
		lua_rawseti(L, -2, unitCount++);
	}

	return 1;
}


/***
 *
 * @function Spring.GetUnitShieldState
 * @param unitID UnitID
 * @param weaponNum integer? Optional if the unit has just one shield
 * @return integer isEnabled Warning, number not boolean. 0 or 1
 * @return number currentPower
 */
int LuaSyncedRead::GetUnitShieldState(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const CPlasmaRepulser* shield = nullptr;
	const size_t idx = luaL_optint(L, 2, -1) - LUA_WEAPON_BASE_INDEX;

	if (idx >= unit->weapons.size()) {
		shield = static_cast<const CPlasmaRepulser*>(unit->shieldWeapon);
	} else {
		shield = dynamic_cast<const CPlasmaRepulser*>(unit->weapons[idx]);
	}

	if (shield == nullptr)
		return 0;

	lua_pushnumber(L, shield->IsEnabled());
	lua_pushnumber(L, shield->GetCurPower());
	return 2;
}


/***
 *
 * @function Spring.GetUnitFlanking
 *
 * When called without a key, returns every value. When called with one of
 * `"mode"`, `"moveFactor"`, `"minDamage"` or `"maxDamage"` returns just that
 * single number, and with `"dir"` returns just `dirX`, `dirY`, `dirZ`.
 *
 * @param unitID UnitID
 * @return number mode
 * @return number moveFactor
 * @return number minDamage
 * @return number maxDamage
 * @return number dirX
 * @return number dirY
 * @return number dirZ
 * @return number mobility The amount of mobility the unit has collected up to now.
 * @overload fun(unitID: integer, param: "mode"|"moveFactor"|"minDamage"|"maxDamage"): number
 * @overload fun(unitID: integer, param: "dir"): number, number, number
 */
int LuaSyncedRead::GetUnitFlanking(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if (lua_israwstring(L, 2)) {
		const char* key = lua_tostring(L, 2);

		switch (hashString(key)) {
			case hashString("mode"): {
				lua_pushnumber(L, unit->flankingBonusMode);
				return 1;
			} break;
			case hashString("dir"): {
				lua_pushnumber(L, unit->flankingBonusDir.x);
				lua_pushnumber(L, unit->flankingBonusDir.y);
				lua_pushnumber(L, unit->flankingBonusDir.z);
				return 3;
			} break;
			case hashString("moveFactor"): {
				lua_pushnumber(L, unit->flankingBonusMobilityAdd);
				return 1;
			} break;
			case hashString("minDamage"): {
				lua_pushnumber(L, unit->flankingBonusAvgDamage - unit->flankingBonusDifDamage);
				return 1;
			} break;
			case hashString("maxDamage"): {
				lua_pushnumber(L, unit->flankingBonusAvgDamage + unit->flankingBonusDifDamage);
				return 1;
			} break;
			default: {
			} break;
		}
	}
	else if (lua_isnoneornil(L, 2)) {
		lua_pushnumber(L, unit->flankingBonusMode);
		lua_pushnumber(L, unit->flankingBonusMobilityAdd);
		lua_pushnumber(L, unit->flankingBonusAvgDamage - // min
		                  unit->flankingBonusDifDamage);
		lua_pushnumber(L, unit->flankingBonusAvgDamage + // max
		                  unit->flankingBonusDifDamage);
		lua_pushnumber(L, unit->flankingBonusDir.x);
		lua_pushnumber(L, unit->flankingBonusDir.y);
		lua_pushnumber(L, unit->flankingBonusDir.z);
		lua_pushnumber(L, unit->flankingBonusMobility); // the amount of mobility that the unit has collected up to now
		return 8;
	}

	return 0;
}


/*** Get a unit's engagement range
 *
 * @function Spring.GetUnitMaxRange
 *
 * Returns the range at which a unit will stop to engage.
 * By default this is the highest among the unit's weapon ranges (hence name),
 * but can be changed dynamically. Also note that unarmed units ignore this.
 *
 * @param unitID UnitID
 * @return number maxRange
 */
int LuaSyncedRead::GetUnitMaxRange(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->maxRange);
	return 1;
}


/******************************************************************************
 * Unit weapon state
 *
 * @section unit_weapon_state
******************************************************************************/


/*** Check the state of a unit's weapon
 *
 * @function Spring.GetUnitWeaponState
 *
 * Available states to poll:
 *   "reloadFrame" (frame on which the weapon will be ready to fire),
 *   "reloadSpeed" (reload time in seconds),
 *   "range" (in elmos),
 *   "autoTargetRangeBoost" (predictive aiming range buffer, in elmos),
 *   "projectileSpeed" (in elmos/frame),
 *   "reloadTimeXP" (reload time after XP bonus, in seconds),
 *   "reaimTime" (frames between AimWeapon calls),
 *   "burst" (shots in a burst),
 *   "burstRate" (delay between shots in a burst, in seconds),
 *   "projectiles" (projectiles per shot),
 *   "salvoLeft" (shots remaining in ongoing burst),
 *   "nextSalvo" (simframe of the next shot in an ongoing burst),
 *   "accuracy" (INaccuracy after XP bonus),
 *   "sprayAngle" (spray angle after XP bonus),
 *   "targetMoveError" (extra inaccuracy against moving targets, after XP bonus)
 *   "avoidFlags" (bitmask for targeting avoidance),
 *   "ttl" (number of seconds a projectile should live)
 *   "collisionFlags" (bitmask for collisions).
 *
 * The state "salvoError" is an exception and returns a table: {x, y, z},
 * which represents the inaccuracy error of the ongoing burst.
 *
 * @param unitID UnitID
 * @param weaponNum integer
 * @param stateName string?
 * @return number stateValue
 */
int LuaSyncedRead::GetUnitWeaponState(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const char* key = luaL_optstring(L, 3, "");

	if (key[0] == 0) { // backwards compatible
		lua_pushboolean(L, weapon->angleGood);
		lua_pushboolean(L, weapon->reloadStatus <= gs->frameNum);
		lua_pushnumber(L,  weapon->reloadStatus);
		lua_pushnumber(L,  weapon->salvoLeft);
		lua_pushnumber(L,  weapon->numStockpiled);
		return 5;
	}

	switch (hashString(key)) {
		case hashString("reloadState"):
		case hashString("reloadFrame"): {
			lua_pushnumber(L, weapon->reloadStatus);
		} break;

		case hashString("reloadTime"): {
			lua_pushnumber(L, weapon->reloadTime * INV_GAME_SPEED);
		} break;
		case hashString("reloadTimeXP"): {
			// reloadSpeed is affected by unit experience
			lua_pushnumber(L, (weapon->reloadTime / unit->reloadSpeed) / GAME_SPEED);
		} break;
		case hashString("reaimTime"): {
			lua_pushnumber(L, weapon->reaimTime);
		} break;

		case hashString("accuracy"): {
			lua_pushnumber(L, weapon->AccuracyExperience());
		} break;
		case hashString("sprayAngle"): {
			lua_pushnumber(L, weapon->SprayAngleExperience());
		} break;

		case hashString("range"): {
			lua_pushnumber(L, weapon->range);
		} break;
		case hashString("projectileSpeed"): {
			lua_pushnumber(L, weapon->projectileSpeed);
		} break;

		case hashString("autoTargetRangeBoost"): {
			lua_pushnumber(L, weapon->autoTargetRangeBoost);
		} break;

		case hashString("burst"): {
			lua_pushnumber(L, weapon->salvoSize);
		} break;
		case hashString("burstRate"): {
			lua_pushnumber(L, weapon->salvoDelay * INV_GAME_SPEED);
		} break;
		case hashString("windup"): {
			lua_pushnumber(L, float(weapon->salvoWindup) / GAME_SPEED);
		} break;

		case hashString("projectiles"): {
			lua_pushnumber(L, weapon->projectilesPerShot);
		} break;

		case hashString("salvoError"): {
			const float3 salvoError =  weapon->SalvoErrorExperience();

			lua_createtable(L, 3, 0);
			lua_pushnumber(L, salvoError.x); lua_rawseti(L, -2, 1);
			lua_pushnumber(L, salvoError.y); lua_rawseti(L, -2, 2);
			lua_pushnumber(L, salvoError.z); lua_rawseti(L, -2, 3);
		} break;

		case hashString("salvoLeft"): {
			lua_pushnumber(L, weapon->salvoLeft);
		} break;
		case hashString("nextSalvo"): {
			lua_pushnumber(L, weapon->nextSalvo);
		} break;

		case hashString("targetMoveError"): {
			lua_pushnumber(L, weapon->MoveErrorExperience());
		} break;

		case hashString("avoidFlags"): {
			lua_pushnumber(L, weapon->avoidFlags);
		} break;
		case hashString("collisionFlags"): {
			lua_pushnumber(L, weapon->collisionFlags);
		} break;
		case hashString("ttl"): {
			lua_pushnumber(L, weapon->ttl * INV_GAME_SPEED);
		} break;

		default: {
			return 0;
		} break;
	}

	return 1;
}


static inline int PushDamagesKey(lua_State* L, const DynDamageArray& damages, int index)
{
	if (lua_isnumber(L, index)) {
		const unsigned armType = lua_toint(L, index);

		if (armType >= damages.GetNumTypes())
			return 0;

		lua_pushnumber(L, damages.Get(armType));
		return 1;
	}

	switch (hashString(luaL_checkstring(L, index))) {
		case hashString("paralyzeDamageTime"): {
			lua_pushnumber(L, damages.paralyzeDamageTime);
		} break;

		case hashString("impulseFactor"): {
			lua_pushnumber(L, damages.impulseFactor);
		} break;
		case hashString("impulseBoost"): {
			lua_pushnumber(L, damages.impulseBoost);
		} break;

		case hashString("craterMult"): {
			lua_pushnumber(L, damages.craterMult);
		} break;
		case hashString("craterBoost"): {
			lua_pushnumber(L, damages.craterBoost);
		} break;

		case hashString("dynDamageExp"): {
			lua_pushnumber(L, damages.dynDamageExp);
		} break;
		case hashString("dynDamageMin"): {
			lua_pushnumber(L, damages.dynDamageMin);
		} break;
		case hashString("dynDamageRange"): {
			lua_pushnumber(L, damages.dynDamageRange);
		} break;
		case hashString("dynDamageInverted"): {
			lua_pushboolean(L, damages.dynDamageInverted);
		} break;

		case hashString("craterAreaOfEffect"): {
			lua_pushnumber(L, damages.craterAreaOfEffect);
		} break;
		case hashString("damageAreaOfEffect"): {
			lua_pushnumber(L, damages.damageAreaOfEffect);
		} break;

		case hashString("edgeEffectiveness"): {
			lua_pushnumber(L, damages.edgeEffectiveness);
		} break;
		case hashString("explosionSpeed"): {
			lua_pushnumber(L, damages.explosionSpeed);
		} break;

		default: {
			return 0;
		} break;
	}

	return 1;
}


/***
 *
 * @function Spring.GetUnitWeaponDamages
 * @param unitID UnitID
 */
int LuaSyncedRead::GetUnitWeaponDamages(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const DynDamageArray* damages;

	if (lua_israwstring(L, 2)) {
		const char* key = lua_tostring(L, 2);

		switch (hashString(key)) {
			case hashString("explode"     ): { damages = unit->deathExpDamages; } break;
			case hashString("selfDestruct"): { damages = unit->selfdExpDamages; } break;
			default                        : {                        return 0; } break;
		}
	} else {
		const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

		if (weaponNum >= unit->weapons.size())
			return 0;

		CWeapon* weapon = unit->weapons[weaponNum];

		damages = weapon->damages;
	}

	if (damages == nullptr)
		return 0;

	return PushDamagesKey(L, *damages, 3);
}


/***
 *
 * @function Spring.GetUnitWeaponVectors
 * @param unitID UnitID
 * @param weaponNum integer 1-indexed weapon number
 * @return number? posX
 * @return number posY
 * @return number posZ
 * @return number dirX
 * @return number dirY
 * @return number dirZ
 */
int LuaSyncedRead::GetUnitWeaponVectors(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const float3& pos = weapon->weaponMuzzlePos;
	const float3* dir = &weapon->wantedDir;

	switch (weapon->weaponDef->projectileType) {
		case WEAPON_MISSILE_PROJECTILE  : { dir = &weapon->weaponDir; } break;
		case WEAPON_TORPEDO_PROJECTILE  : { dir = &weapon->weaponDir; } break;
		case WEAPON_STARBURST_PROJECTILE: { dir = &weapon->weaponDir; } break;
		default                         : {                           } break;
	}

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);

	lua_pushnumber(L, dir->x);
	lua_pushnumber(L, dir->y);
	lua_pushnumber(L, dir->z);

	return 6;
}


/***
 *
 * @function Spring.GetUnitWeaponTryTarget
 * @param unitID UnitID
 * @param weaponNum integer
 * @param targetID UnitID
 * @return boolean canTarget
 */
/***
 *
 * @function Spring.GetUnitWeaponTryTarget
 * @param unitID UnitID
 * @param weaponNum integer
 * @param posX number
 * @param posY number
 * @param posZ number
 * @return boolean canTarget
 */
int LuaSyncedRead::GetUnitWeaponTryTarget(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = nullptr;

	float3 pos;

	//we cannot test calling TryTarget/TestTarget/HaveFreeLineOfFire directly
	// by passing a position thatis not approximately the wanted checked position, because
	//the checks for target using passed position for checking both free line of fire and range
	//which would result in wrong test unless target was by chance near coords <0,0,0>
	//while position alone works because NULL target omits target class validity checks

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);

	} else {
		enemy = ParseUnit(L, __func__, 3);

		if (enemy == nullptr)
			return 0;
	}

	lua_pushboolean(L, weapon->TryTarget(SWeaponTarget(enemy, pos, true)));
	return 1;
}


/***
 *
 * @function Spring.GetUnitWeaponTestTarget
 * @param unitID UnitID
 * @param weaponID integer weapon number (1-based Lua index)
 * @param targetUnitID UnitID enemy unit to test (when fewer than five arguments)
 * @return boolean validTarget
 */
/***
 *
 * @function Spring.GetUnitWeaponTestTarget
 * @param unitID UnitID
 * @param weaponID integer weapon number (1-based Lua index)
 * @param targetX number world X to test (with `targetY`, `targetZ`; used when at least five arguments are passed)
 * @param targetY number
 * @param targetZ number
 * @return boolean validTarget
 */
int LuaSyncedRead::GetUnitWeaponTestTarget(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = nullptr;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		if ((enemy = ParseUnit(L, __func__, 3)) == nullptr)
			return 0;

		pos = weapon->GetUnitLeadTargetPos(enemy);
	}

	lua_pushboolean(L, weapon->TestTarget(pos, SWeaponTarget(enemy, pos, true)));
	return 1;
}


/***
 *
 * @function Spring.GetUnitWeaponTestRange
 * @param unitID UnitID
 * @param weaponNum integer
 * @param targetID UnitID
 * @return boolean inRange
 */
/***
 *
 * @function Spring.GetUnitWeaponTestRange
 * @param unitID UnitID
 * @param weaponNum integer
 * @param posX number
 * @param posY number
 * @param posZ number
 * @return boolean inRange
 */
int LuaSyncedRead::GetUnitWeaponTestRange(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = nullptr;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		if ((enemy = ParseUnit(L, __func__, 3)) == nullptr)
			return 0;

		pos = weapon->GetUnitLeadTargetPos(enemy);
	}

	lua_pushboolean(L, weapon->TestRange(pos, SWeaponTarget(enemy, pos, true)));
	return 1;
}


/***
 *
 * @function Spring.GetUnitWeaponHaveFreeLineOfFire
 * @param unitID UnitID
 * @param weaponNum integer
 * @param targetID UnitID
 * @return boolean haveFreeLineOfFire
 */
/***
 *
 * @function Spring.GetUnitWeaponHaveFreeLineOfFire
 * @param unitID UnitID
 * @param weaponNum integer
 * @param srcPosX number
 * @param srcPosY number
 * @param srcPosZ number
 * @return boolean haveFreeLineOfFire
 */
/***
 *
 * @function Spring.GetUnitWeaponHaveFreeLineOfFire
 * @param unitID UnitID
 * @param weaponNum integer
 * @param srcPosX number
 * @param srcPosY number
 * @param srcPosZ number
 * @param targetID UnitID
 * @return boolean haveFreeLineOfFire
 */
/***
 *
 * @function Spring.GetUnitWeaponHaveFreeLineOfFire
 * @param unitID UnitID
 * @param weaponNum integer
 * @param srcPosX number
 * @param srcPosY number
 * @param srcPosZ number
 * @param tgtPosX number
 * @param tgtPosY number
 * @param tgtPosZ number
 * @return boolean haveFreeLineOfFire
 */
int LuaSyncedRead::GetUnitWeaponHaveFreeLineOfFire(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = nullptr;

	float3 srcPos = weapon->GetAimFromPos();
	float3 tgtPos;

	const auto ParsePos = [&L](int idx, int cnt, float* pos) {
		for (int i = 0; i < cnt; i++) {
			pos[i] = luaL_optnumber(L, idx + i, pos[i]);
		}
	};

	switch (lua_gettop(L)) {
		case 3: {
			// [3] := targetID
			if ((enemy = ParseUnit(L, __func__, 3)) == nullptr)
				return 0;

			tgtPos = weapon->GetUnitLeadTargetPos(enemy);
		} break;
		case 5: {
			// [3,4,5] := srcPos
			ParsePos(3, 3, &srcPos.x);
		} break;

		case 6: {
			// [3,4,5] := srcPos, [6] := targetID
			ParsePos(3, 3, &srcPos.x);

			if ((enemy = ParseUnit(L, __func__, 6)) == nullptr)
				return 0;

			tgtPos = weapon->GetUnitLeadTargetPos(enemy);
		} break;
		case 8: {
			// [3,4,5] := srcPos, [6,7,8] := tgtPos
			ParsePos(3, 3, &srcPos.x);
			ParsePos(6, 3, &tgtPos.x);
		} break;

		default: {
			return 0;
		} break;
	}

	lua_pushboolean(L, weapon->HaveFreeLineOfFire(srcPos, tgtPos, SWeaponTarget(enemy, tgtPos, true)));
	return 1;
}

/***
 *
 * @function Spring.GetUnitWeaponCanFire
 * @param unitID UnitID
 * @param weaponNum integer
 * @param ignoreAngleGood boolean?
 * @param ignoreTargetType boolean?
 * @param ignoreRequestedDir boolean?
 * @return boolean canFire
 */
int LuaSyncedRead::GetUnitWeaponCanFire(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const bool ignoreAngleGood = luaL_optboolean(L, 3, false);
	const bool ignoreTargetType = luaL_optboolean(L, 4, false);
	const bool ignoreRequestedDir = luaL_optboolean(L, 5, false);

	lua_pushboolean(L, unit->weapons[weaponNum]->CanFire(ignoreAngleGood, ignoreTargetType, ignoreRequestedDir));
	return 1;
}

/***
 * @alias TargetType
 * | 0 # none
 * | 1 # unit
 * | 2 # position
 * | 3 # projectile
 */

/***
 * Checks a weapon's target
 *
 * Note that this doesn't need to reflect the unit's Attack orders or such, and
 * that weapons can aim individually unless slaved.
 *
 * @function Spring.GetUnitWeaponTarget
 * @param unitID UnitID
 * @param weaponNum integer
 * @return 0 TargetType none
 * @return boolean isUserTarget
 */
/***
 * Checks a weapon's target
 *
 * Note that this doesn't need to reflect the unit's Attack orders or such, and
 * that weapons can aim individually unless slaved.
 *
 * @function Spring.GetUnitWeaponTarget
 * @param unitID UnitID
 * @param weaponNum integer
 * @return 1 TargetType unit
 * @return boolean isUserTarget
 * @return UnitID targetUnitID
 */
/***
 * Checks a weapon's target
 *
 * Note that this doesn't need to reflect the unit's Attack orders or such, and
 * that weapons can aim individually unless slaved.
 *
 * @function Spring.GetUnitWeaponTarget
 * @param unitID UnitID
 * @param weaponNum integer
 * @return 2 TargetType position
 * @return boolean isUserTarget
 * @return float3 targetPosition
 */
/***
 * Checks a weapon's target
 *
 * Note that this doesn't need to reflect the unit's Attack orders or such, and
 * that weapons can aim individually unless slaved.
 *
 * @function Spring.GetUnitWeaponTarget
 * @param unitID UnitID
 * @param weaponNum integer
 * @return 3 TargetType projectileID
 * @return boolean isUserTarget
 * @return ProjectileID targetProjectileId
 */
int LuaSyncedRead::GetUnitWeaponTarget(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	auto curTarget = weapon->GetCurrentTarget();

	lua_pushnumber(L, curTarget.type);

	switch (curTarget.type) {
		case Target_None:
			return 1;
			break;
		case Target_Unit: {
			lua_pushboolean(L, curTarget.isUserTarget);
			lua_pushnumber(L, curTarget.unit->id);
			break;
		}
		case Target_Pos: {
			lua_pushboolean(L, curTarget.isUserTarget);
			lua_createtable(L, 3, 0);
			lua_pushnumber(L, curTarget.groundPos.x); lua_rawseti(L, -2, 1);
			lua_pushnumber(L, curTarget.groundPos.y); lua_rawseti(L, -2, 2);
			lua_pushnumber(L, curTarget.groundPos.z); lua_rawseti(L, -2, 3);
			break;
		}
		case Target_Intercept: {
			lua_pushboolean(L, curTarget.isUserTarget);
			lua_pushnumber(L, curTarget.intercept->id);
			break;
		}
	}

	return 3;
}


/******************************************************************************
 * Misc
 *
 * @section misc
******************************************************************************/


int LuaSyncedRead::GetUnitTravel(lua_State* L) { lua_pushnumber(L, 0.0f); lua_pushnumber(L, 0.0f); return 2; } // FIXME: DELETE ME
int LuaSyncedRead::GetUnitFuel(lua_State* L) { lua_pushnumber(L, 0.0f); return 1; } // FIXME: DELETE ME


/***
 *
 * @function Spring.GetUnitEstimatedPath
 * @param unitID UnitID
 */
int LuaSyncedRead::GetUnitEstimatedPath(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const CGroundMoveType* gmt = dynamic_cast<const CGroundMoveType*>(unit->moveType);

	if (gmt == nullptr)
		return 0;

	return (LuaPathFinder::PushPathNodes(L, gmt->GetPathID()));
}


/***
 *
 * @function Spring.GetUnitLastAttacker
 * @param unitID UnitID
 * @return UnitID? attackerUnitID `nil` if the unit has no last attacker or the attacker is not visible.
 */
int LuaSyncedRead::GetUnitLastAttacker(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if ((unit->lastAttacker == nullptr) ||
	    !LuaUtils::IsUnitVisible(L, unit->lastAttacker)) {
		return 0;
	}
	lua_pushnumber(L, unit->lastAttacker->id);
	return 1;
}


/***
 *
 * @function Spring.GetUnitLastAttackedPiece
 * @param unitID UnitID
 */
int LuaSyncedRead::GetUnitLastAttackedPiece(lua_State* L)
{
	return (GetSolidObjectLastHitPiece(L, ParseAllyUnit(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetUnitCollisionVolumeData
 * @param unitID UnitID
 */
int LuaSyncedRead::GetUnitCollisionVolumeData(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	return LuaUtils::PushColVolData(L, &unit->collisionVolume);
}

/***
 *
 * @function Spring.GetUnitPieceCollisionVolumeData
 * @param unitID UnitID
 * @param pieceIndex integer 1-based local-model piece index
 * @return number? scaleX
 * @return number? scaleY
 * @return number? scaleZ
 * @return number? offsetX
 * @return number? offsetY
 * @return number? offsetZ
 * @return integer? volumeType
 * @return integer? testType
 * @return integer? primaryAxis
 * @return boolean? disabled
 *
 * Returns no values when `unitID` is invalid or not in line of sight, or when
 * `pieceIndex` is invalid.
 */
int LuaSyncedRead::GetUnitPieceCollisionVolumeData(lua_State* L)
{
	return (PushPieceCollisionVolumeData(L, ParseInLosUnit(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetUnitSeparation
 * @param unitID1 UnitID
 * @param unitID2 UnitID
 * @param direction boolean? (Default: `false`) to subtract from, default unitID1 - unitID2
 * @param subtractRadii boolean? (Default: `false`) whether units radii should be subtracted from the total
 * @return number?
 */
int LuaSyncedRead::GetUnitSeparation(lua_State* L)
{
	const CUnit* unit1 = ParseUnit(L, __func__, 1);
	const CUnit* unit2 = ParseUnit(L, __func__, 2);

	if (unit1 == nullptr || unit2 == nullptr)
		return 0;

	float3 pos1 = unit1->midPos;
	float3 pos2 = unit2->midPos;

	if (!LuaUtils::IsAllyUnit(L, unit1))
		pos1 = unit1->GetLuaErrorPos(CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleFullRead(L));
	if (!LuaUtils::IsAllyUnit(L, unit2))
		pos2 = unit2->GetLuaErrorPos(CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleFullRead(L));

	#if 0
	const float3 mask = XZVector + UpVector * (1 - luaL_optboolean(L, 3, false));
	const float3 diff = (pos1 - pos2) * mask;
	const float  dist = diff.Length();
	#else
	const float dist = (luaL_optboolean(L, 3, false))? pos1.distance2D(pos2): pos1.distance(pos2);
	#endif

	if (luaL_optboolean(L, 4, false)) {
		lua_pushnumber(L, std::max(0.0f, dist - unit1->radius - unit2->radius));
	} else {
		lua_pushnumber(L, dist);
	}

	return 1;
}

/***
 *
 * @function Spring.GetUnitFeatureSeparation
 * @param unitID UnitID
 * @param featureID FeatureID
 * @param flat boolean? (Default: `false`) if true, XZ (2D) distance; otherwise 3D distance
 * @return number distance
 */
int LuaSyncedRead::GetUnitFeatureSeparation(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CFeature* feature = ParseFeature(L, __func__, 2);

	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	float3 pos1 =    unit->midPos;
	float3 pos2 = feature->midPos;

	if (!LuaUtils::IsAllyUnit(L, unit))
		pos1 = unit->GetLuaErrorPos(CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleFullRead(L));

	#if 0
	const float3 mask = XZVector + UpVector * (1 - luaL_optboolean(L, 3, false));
	const float3 diff = (pos1 - pos2) * mask;

	lua_pushnumber(L, diff.Length());
	#else
	lua_pushnumber(L, (luaL_optboolean(L, 3, false))? pos1.distance2D(pos2): pos1.distance(pos2));
	#endif
	return 1;
}

/***
 * @class UnitDefDimensions
 * @x_helper
 * @field height number
 * @field radius number
 * @field midx number
 * @field minx number
 * @field maxx number
 * @field midy number
 * @field miny number
 * @field maxy number
 * @field midz number
 * @field minz number
 * @field maxz number
 */

/***
 *
 * @function Spring.GetUnitDefDimensions
 * @param unitDefID UnitDefID
 * @return UnitDefDimensions? dimensions
 */
int LuaSyncedRead::GetUnitDefDimensions(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == nullptr)
		return 0;

	const S3DModel* model = ud->LoadModel();
	if (model == nullptr)
		return 0;

	const S3DModel& m = *model;
	const float3& mid = model->relMidPos;
	lua_createtable(L, 0, 11);
	LuaPushNamedNumber(L, "height", m.height);
	LuaPushNamedNumber(L, "radius", m.radius);
	LuaPushNamedNumber(L, "midx",   mid.x);
	LuaPushNamedNumber(L, "minx",   m.mins.x);
	LuaPushNamedNumber(L, "maxx",   m.maxs.x);
	LuaPushNamedNumber(L, "midy",   mid.y);
	LuaPushNamedNumber(L, "miny",   m.mins.y);
	LuaPushNamedNumber(L, "maxy",   m.maxs.y);
	LuaPushNamedNumber(L, "midz",   mid.z);
	LuaPushNamedNumber(L, "minz",   m.mins.z);
	LuaPushNamedNumber(L, "maxz",   m.maxs.z);
	return 1;
}


/***
 *
 * @function Spring.GetCEGID
 * @return integer cegID
 */
int LuaSyncedRead::GetCEGID(lua_State* L)
{
	lua_pushnumber(L, explGenHandler.LoadCustomGeneratorID(luaL_checkstring(L, 1)));
	return 1;
}


/***
 *
 * @function Spring.GetUnitBlocking
 * @param unitID UnitID
 * @return boolean? isBlocking
 * @return boolean isSolidObjectCollidable
 * @return boolean isProjectileCollidable
 * @return boolean isRaySegmentCollidable
 * @return boolean crushable
 * @return boolean blockEnemyPushing
 * @return boolean blockHeightChanges
 */
int LuaSyncedRead::GetUnitBlocking(lua_State* L)
{
	return (GetSolidObjectBlocking(L, ParseTypedUnit(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetUnitMoveTypeData
 * @param unitID UnitID
 */
int LuaSyncedRead::GetUnitMoveTypeData(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	AMoveType* amt = unit->moveType;

	lua_createtable(L, 0, 26);
	LuaPushNamedNumber(L, "maxSpeed", amt->GetMaxSpeed() * GAME_SPEED);
	LuaPushNamedNumber(L, "maxWantedSpeed", amt->GetMaxWantedSpeed() * GAME_SPEED);
	LuaPushNamedNumber(L, "goalx", amt->goalPos.x);
	LuaPushNamedNumber(L, "goaly", amt->goalPos.y);
	LuaPushNamedNumber(L, "goalz", amt->goalPos.z);

	switch (amt->progressState) {
		case AMoveType::Done:
			LuaPushNamedString(L, "progressState", "done");
			break;
		case AMoveType::Active:
			LuaPushNamedString(L, "progressState", "active");
			break;
		case AMoveType::Failed:
			LuaPushNamedString(L, "progressState", "failed");
			break;
	}

	const CGroundMoveType* groundmt = dynamic_cast<CGroundMoveType*>(unit->moveType);

	if (groundmt != nullptr) {
		LuaPushNamedString(L, "name", "ground");

		LuaPushNamedNumber(L, "turnRate", groundmt->GetTurnRate());
		LuaPushNamedNumber(L, "accRate", groundmt->GetAccRate());
		LuaPushNamedNumber(L, "decRate", groundmt->GetDecRate());

		LuaPushNamedNumber(L, "maxReverseSpeed", groundmt->GetMaxReverseSpeed() * GAME_SPEED);
		LuaPushNamedNumber(L, "wantedSpeed", groundmt->GetWantedSpeed() * GAME_SPEED);
		LuaPushNamedNumber(L, "currentSpeed", groundmt->GetCurrentSpeed() * GAME_SPEED);

		LuaPushNamedNumber(L, "goalRadius", groundmt->GetGoalRadius());

		LuaPushNamedNumber(L, "currwaypointx", (groundmt->GetCurrWayPoint()).x);
		LuaPushNamedNumber(L, "currwaypointy", (groundmt->GetCurrWayPoint()).y);
		LuaPushNamedNumber(L, "currwaypointz", (groundmt->GetCurrWayPoint()).z);
		LuaPushNamedNumber(L, "nextwaypointx", (groundmt->GetNextWayPoint()).x);
		LuaPushNamedNumber(L, "nextwaypointy", (groundmt->GetNextWayPoint()).y);
		LuaPushNamedNumber(L, "nextwaypointz", (groundmt->GetNextWayPoint()).z);

		LuaPushNamedNumber(L, "requestedSpeed", 0.0f);

		LuaPushNamedNumber(L, "pathFailures", 0);

		return 1;
	}

	const CHoverAirMoveType* hAMT = dynamic_cast<CHoverAirMoveType*>(unit->moveType);

	if (hAMT != nullptr) {
		LuaPushNamedString(L, "name", "gunship");

		LuaPushNamedNumber(L, "wantedHeight", hAMT->wantedHeight);
		LuaPushNamedBool(L, "collide", hAMT->collide);
		LuaPushNamedBool(L, "useSmoothMesh", hAMT->useSmoothMesh);

		switch (hAMT->aircraftState) {
			case AAirMoveType::AIRCRAFT_LANDED:
				LuaPushNamedString(L, "aircraftState", "landed");
				break;
			case AAirMoveType::AIRCRAFT_FLYING:
				LuaPushNamedString(L, "aircraftState", "flying");
				break;
			case AAirMoveType::AIRCRAFT_LANDING:
				LuaPushNamedString(L, "aircraftState", "landing");
				break;
			case AAirMoveType::AIRCRAFT_CRASHING:
				LuaPushNamedString(L, "aircraftState", "crashing");
				break;
			case AAirMoveType::AIRCRAFT_TAKEOFF:
				LuaPushNamedString(L, "aircraftState", "takeoff");
				break;
			case AAirMoveType::AIRCRAFT_HOVERING:
				LuaPushNamedString(L, "aircraftState", "hovering");
				break;
		};

		switch (hAMT->flyState) {
			case CHoverAirMoveType::FLY_CRUISING:
				LuaPushNamedString(L, "flyState", "cruising");
				break;
			case CHoverAirMoveType::FLY_CIRCLING:
				LuaPushNamedString(L, "flyState", "circling");
				break;
			case CHoverAirMoveType::FLY_ATTACKING:
				LuaPushNamedString(L, "flyState", "attacking");
				break;
			case CHoverAirMoveType::FLY_LANDING:
				LuaPushNamedString(L, "flyState", "landing");
				break;
		}

		LuaPushNamedNumber(L, "goalDistance", hAMT->goalDistance);

		LuaPushNamedBool  (L, "bankingAllowed", hAMT->bankingAllowed);
		LuaPushNamedNumber(L, "currentBank", hAMT->currentBank);
		LuaPushNamedNumber(L, "currentPitch", hAMT->currentPitch);

		LuaPushNamedNumber(L, "turnRate",     hAMT->turnRate);
		LuaPushNamedNumber(L, "accRate",      hAMT->accRate);
		LuaPushNamedNumber(L, "decRate",      hAMT->decRate);
		LuaPushNamedNumber(L, "altitudeRate", hAMT->altitudeRate);

		LuaPushNamedNumber(L, "brakeDistance", -1.0f); // DEPRECATED
		LuaPushNamedBool  (L, "dontLand", hAMT->GetAllowLanding());
		LuaPushNamedNumber(L, "maxDrift", hAMT->maxDrift);

		return 1;
	}

	const CStrafeAirMoveType* sAMT = dynamic_cast<CStrafeAirMoveType*>(unit->moveType);

	if (sAMT != nullptr) {
		LuaPushNamedString(L, "name", "airplane");

		switch (sAMT->aircraftState) {
			case AAirMoveType::AIRCRAFT_LANDED:
				LuaPushNamedString(L, "aircraftState", "landed");
				break;
			case AAirMoveType::AIRCRAFT_FLYING:
				LuaPushNamedString(L, "aircraftState", "flying");
				break;
			case AAirMoveType::AIRCRAFT_LANDING:
				LuaPushNamedString(L, "aircraftState", "landing");
				break;
			case AAirMoveType::AIRCRAFT_CRASHING:
				LuaPushNamedString(L, "aircraftState", "crashing");
				break;
			case AAirMoveType::AIRCRAFT_TAKEOFF:
				LuaPushNamedString(L, "aircraftState", "takeoff");
				break;
			case AAirMoveType::AIRCRAFT_HOVERING:
				LuaPushNamedString(L, "aircraftState", "hovering");
				break;
		};
		LuaPushNamedNumber(L, "wantedHeight",  sAMT->wantedHeight);
		LuaPushNamedBool  (L, "collide",       sAMT->collide);
		LuaPushNamedBool  (L, "useSmoothMesh", sAMT->useSmoothMesh);

		LuaPushNamedNumber(L, "myGravity", sAMT->myGravity);

		LuaPushNamedNumber(L, "maxBank",    sAMT->maxBank);
		LuaPushNamedNumber(L, "maxPitch",   sAMT->maxBank);
		LuaPushNamedNumber(L, "turnRadius", sAMT->turnRadius);

		LuaPushNamedNumber(L, "maxAcc",      sAMT->accRate);
		LuaPushNamedNumber(L, "maxAileron",  sAMT->maxAileron);
		LuaPushNamedNumber(L, "maxElevator", sAMT->maxElevator);
		LuaPushNamedNumber(L, "maxRudder",   sAMT->maxRudder);

		return 1;
	}

	const CStaticMoveType* staticmt = dynamic_cast<CStaticMoveType*>(unit->moveType);

	if (staticmt != nullptr) {
		LuaPushNamedString(L, "name", "static");
		return 1;
	}

	const CScriptMoveType* scriptmt = dynamic_cast<CScriptMoveType*>(unit->moveType);

	if (scriptmt != nullptr) {
		LuaPushNamedString(L, "name", "script");
		return 1;
	}

	LuaPushNamedString(L, "name", "unknown");
	return 1;
}



/******************************************************************************/

/***
 * @class Command
 * @x_helper
 * @field id integer
 * @field params number[]?
 * @field options CommandOptions?
 */
static void PackCommand(lua_State* L, const Command& cmd)
{
	lua_createtable(L, 0, 4);

	LuaPushNamedNumber(L, "id", cmd.GetID());

	// t["params"] = {[1] = param1, ...}
	LuaUtils::PushCommandParamsTable(L, cmd, true);
	// t["options"] = {key1 = val1, ...}
	LuaUtils::PushCommandOptionsTable(L, cmd, true);

	LuaPushNamedNumber(L, "tag", cmd.GetTag());
}


static void PackCommandQueue(lua_State* L, const CCommandQueue& commands, size_t count)
{
	size_t c = 0;

	// get the desired number of commands to return
	if (count == -1u)
		count = commands.size();

	// count can exceed the queue size, clamp
	lua_createtable(L, std::min(count, commands.size()), 0);

	// {[1] = cq[0], [2] = cq[1], ...}
	for (const auto& command: commands) {
		if (c >= count)
			break;

		PackCommand(L, command);
		lua_rawseti(L, -2, ++c);
	}
}

// FIXME: Probably more aptly named `Spring.GetUnitCommand`? It's just default argument that gets current command. Also in line with `Spring.GetUnitCommands`.
/*** Get a command from a units command queue.
 * 
 * For factories, this function uses the command queue automatically assigned to new units.
 * 
 * @see Spring.GetFactoryCommands for getting factory build queue commands
 *
 * @function Spring.GetUnitCurrentCommand
 *
 * @param unitID UnitID unitID when invalid this function returns nil.
 * @param cmdIndex integer? (Default: `0`) Command index to get. If negative will count from the end of the queue, e.g. -1 will be the last command.
 * @return CMD? cmdID
 * @return integer|CommandOptionBit|nil options
 * @return integer|nil tag
 * @return number? ... Command parameters.
 *
 */
int LuaSyncedRead::GetUnitCurrentCommand(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI; // never null
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	const CCommandQueue* queue = (factoryCAI == nullptr)? &commandAI->commandQue : &factoryCAI->newUnitCommands;

	int cmdIndex = luaL_optint(L, 2, 1);
	if (cmdIndex > 0) {
		// - 1 to convert from lua index to C index
		cmdIndex -= 1;
	} else {
		cmdIndex = queue->size() + cmdIndex;
	}

	if (cmdIndex >= queue->size() || cmdIndex < 0)
		return 0;

	const Command& cmd = queue->at(cmdIndex);
	lua_pushnumber(L, cmd.GetID());
	lua_pushnumber(L, cmd.GetOpts());
	lua_pushnumber(L, cmd.GetTag());

	const unsigned int numParams = cmd.GetNumParams();
	for (unsigned int i = 0; i < numParams; ++i)
		lua_pushnumber(L, cmd.GetParam(i));

	return 3 + numParams;
}


// FIXME: Remove the undocumented third argument when deprecations expire: `boolean (Default: true) When true returns a list of commands, otherwise returns the count`
/***
 * Get the commands for a unit.
 *
 * @function Spring.GetUnitCommands
 *
 * Same as `Spring.GetCommandQueue`
 *
 * @param unitID UnitID
 * @param count integer Maximum amount of commands to return, `-1` returns all commands.
 * @return Command[] commands
 */
/***
 * Get the count of commands for a unit.
 *
 * @deprecated This overload is deprecated, use `Spring.GetUnitCommandCount(unitId)` instead.
 * @function Spring.GetUnitCommands
 *
 * @param unitID UnitID
 * @param count 0 Returns the number of commands in the units queue.
 * @return integer cmdCount The number of commands in the unit queue.
 */
int LuaSyncedRead::GetUnitCommands(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;
	// send the new unit commands for factories, otherwise the normal commands
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	const CCommandQueue* queue = (factoryCAI == nullptr)? &commandAI->commandQue : &factoryCAI->newUnitCommands;

	const int  numCmds   = luaL_checkint(L, 2); // must always be given, -1 is a performance pitfall
	const bool cmdsTable = luaL_optboolean(L, 3, true); // deprecated, prefer to set 2nd arg to 0

	if (cmdsTable && (numCmds != 0)) {
		// *get wants the actual commands
		PackCommandQueue(L, *queue, numCmds);
	} else {
		LOG_DEPRECATED("This game is issuing `Spring.GetUnitCommands(unitId, 0)`, `Spring.GetCommandQueue(unitId, 0)` or passing a third argument to these functions. This usage is deprecated, please use `Spring.GetUnitCommandCount(unitId)` instead or fix some underlying bug.");
		// *get just wants the queue's size
		lua_pushnumber(L, queue->size());
	}

	return 1;
}

/*** Get the number or list of commands for a factory
 *
 * @function Spring.GetFactoryCommands
 *
 * @param unitID UnitID
 * @param count integer Maximum amount of commands to return, `-1` returns all commands.
 * @return Command[] commands
 *
 * @see Spring.GetFactoryCommandCount to get the count of commands.
 * @see Spring.GetFactoryCounts to get command counts grouped by cmdID.
 */
/***
 * Get the count of commands for a factory.
 *
 * @deprecated This overload is deprecated, use `Spring.GetFactoryCommandCount(unitId)` instead.
 * @function Spring.GetFactoryCommands
 *
 * @param unitID UnitID
 * @param count 0 Returns the number of commands in the factory queue.
 * @return integer cmdCount The number of commands in the factory queue.
 *
 * @see Spring.GetFactoryCommandCount for replacement function.
 */
int LuaSyncedRead::GetFactoryCommands(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);

	// bail if not a factory
	if (factoryCAI == nullptr)
		return 0;

	const CCommandQueue& commandQue = factoryCAI->commandQue;

	const int  numCmds   = luaL_checkint(L, 2);
	const bool cmdsTable = luaL_optboolean(L, 3, true); // deprecated, prefer to set 2nd arg to 0

	if (cmdsTable && (numCmds != 0)) {
		PackCommandQueue(L, commandQue, numCmds);
	} else {
		LOG_DEPRECATED("This game is issuing `Spring.GetFactoryCommands(unitId, 0)`, or passing a third argument. This usage is deprecated, please use `Spring.GetFactoryCommandCount(unitId)` instead or fix some underlying bug.");
		lua_pushnumber(L, commandQue.size());
	}

	return 1;
}

/*** Get the number of commands in a unit's queue.
 *
 * @function Spring.GetUnitCommandCount
 * @param unitID UnitID
 * @return integer cmdCount The number of commands in the unit's queue.
 */
int LuaSyncedRead::GetUnitCommandCount(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;

	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	const CCommandQueue* queue = (factoryCAI == nullptr)? &commandAI->commandQue : &factoryCAI->newUnitCommands;

	lua_pushnumber(L, queue->size());

	return 1;
}

/*** Get the number of commands in a factory queue.
 *
 * @function Spring.GetFactoryCommandCount
 * @param unitID UnitID
 * @return integer cmdCount The number of commands in the factory queue.
 *
 * @see Spring.GetFactoryCommands to get the factory commands.
 * @see Spring.GetFactoryCounts to get command counts grouped by cmdID.
 */
int LuaSyncedRead::GetFactoryCommandCount(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);

	// bail if not a factory
	if (factoryCAI == nullptr)
		return 0;

	const CCommandQueue& queue = commandAI->commandQue;

	lua_pushnumber(L, queue.size());

	return 1;
}

/***
 *
 * @function Spring.GetFactoryBuggerOff
 * @param unitID UnitID
 * @return boolean? boPerform `nil` if the unit does not exist or is not a factory.
 * @return number boOffset
 * @return number boRadius
 * @return number boRelHeading
 * @return boolean boSherical
 * @return boolean boForced
 */
int LuaSyncedRead::GetFactoryBuggerOff(lua_State* L)
{
	const CUnit* u = ParseUnit(L, __func__, 1);
	if (u == nullptr)
		return 0;

	const CFactory* f = dynamic_cast<const CFactory*>(u);
	if (f == nullptr)
		return 0;

	lua_pushboolean(L, f->boPerform    );
	lua_pushnumber (L, f->boOffset     );
	lua_pushnumber (L, f->boRadius     );
	lua_pushnumber (L, f->boRelHeading );
	lua_pushboolean(L, f->boSherical   );
	lua_pushboolean(L, f->boForced     );

	return 6;
}


static void PackFactoryCounts(lua_State* L,
                              const CCommandQueue& q, int count, bool noCmds)
{
	lua_createtable(L, count + 1, 0);

	int entry = 0;
	int currentCmd = 0;
	int currentCount = 0;

	CCommandQueue::const_iterator it = q.begin();
	for (it = q.begin(); it != q.end(); ++it) {
		if (entry >= count) {
			currentCount = 0;
			break;
		}
		const int cmdID = it->GetID();
		if (noCmds && (cmdID >= 0))
			continue;

		if (entry == 0) {
			currentCmd = cmdID;
			currentCount = 1;
			entry = 1;
		}
		else if (cmdID == currentCmd) {
			currentCount++;
		}
		else {
			entry++;
			// Here and below, negative integer keys in lua tables are stored in the
			// hash part of the table, hence we set nrec to 1 instead of narr.
			// Lua Gems Chapter 2: About tables.
			lua_createtable(L, 0, 1); {
				lua_pushnumber(L, currentCount);
				lua_rawseti(L, -2, -currentCmd);
			}
			lua_rawseti(L, -2, entry);
			currentCmd = cmdID;
			currentCount = 1;
		}
	}
	if (currentCount > 0) {
		entry++;
		lua_createtable(L, 0, 1); {
			lua_pushnumber(L, currentCount);
			lua_rawseti(L, -2, -currentCmd);
		}
		lua_rawseti(L, -2, entry);
	}

	hs_n.PushNumber(L, entry);
}


/*** Gets the build queue of a factory
 *
 * @function Spring.GetFactoryCounts
 * @param unitID UnitID
 * @param count integer? (Default: `-1`) Number of commands to retrieve, `-1` for all.
 * @param addCmds boolean? (Default: `false`) Retrieve commands other than buildunit
 *
 * @return table<number,number>? counts Build queue count by `unitDefID` or `-cmdID`, or `nil` if unit is not found.
 */
int LuaSyncedRead::GetFactoryCounts(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);

	if (factoryCAI == nullptr)
		return 0; // not a factory, bail

	const CCommandQueue& commandQue = factoryCAI->commandQue;

	// get the desired number of commands to return
	int count = luaL_optint(L, 2, -1);
	if (count < 0)
		count = (int)commandQue.size();

	const bool noCmds = !luaL_optboolean(L, 3, false);

	PackFactoryCounts(L, commandQue, count, noCmds);

	return 1;
}


/***
 * Get the commands for a unit.
 *
 * @deprecated Use Spring.GetUnitCommands instead.
 * @function Spring.GetCommandQueue
 *
 * Same as `Spring.GetUnitCommands`
 *
 * @param unitID UnitID
 * @param count integer Number of commands to return, `-1` returns all commands, `0` returns command count.
 * @return Command[] commands
 */
/***
 * Get the count of commands for a unit.
 *
 * @deprecated Use Spring.GetUnitCommandCount instead.
 * @function Spring.GetCommandQueue
 *
 * Same as `Spring.GetUnitCommands`
 *
 * @param unitID UnitID
 * @param count 0 Returns the number of commands in the units queue.
 * @return integer cmdCount The number of commands in the unit queue.
 *
 */

int LuaSyncedRead::GetCommandQueue(lua_State* L)
{
	LOG_DEPRECATED("Spring.GetCommandQueue is deprecated, please use Spring.GetUnitCommands/Spring.GetUnitCommandCount instead.");
	return (GetUnitCommands(L));
}


static int PackBuildQueue(lua_State* L, bool canBuild, const char* caller)
{
	const CUnit* unit = ParseAllyUnit(L, caller, 1);
	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;
	const CCommandQueue& commandQue = commandAI->commandQue;

	lua_createtable(L, commandQue.size(), 0);

	int entry = 0;
	int currentType = -1;
	int currentCount = 0;

	for (const Command& cmd: commandQue) {
		// not a build command
		if (cmd.GetID() >= 0)
			continue;

		const int unitDefID = -cmd.GetID();

		if (canBuild) {
			// skip build orders that this unit can not start
			const UnitDef* buildeeDef = unitDefHandler->GetUnitDefByID(unitDefID);
			const UnitDef* builderDef = unit->unitDef;

			// if something is wrong, bail
			if ((buildeeDef == nullptr) || (builderDef == nullptr))
				continue;

			using P = decltype(UnitDef::buildOptions)::value_type;

			const auto& buildOptCmp = [&](const P& e) { return (STRCASECMP(e.second.c_str(), buildeeDef->name.c_str()) == 0); };
			const auto& buildOpts = builderDef->buildOptions;
			const auto  buildOptIt = std::find_if(buildOpts.cbegin(), buildOpts.cend(), buildOptCmp);

			// didn't find a matching entry
			if (buildOptIt == buildOpts.end())
				continue;
		}

		if (currentType == unitDefID) {
			currentCount++;
		} else if (currentType == -1) {
			currentType = unitDefID;
			currentCount = 1;
		} else {
			entry++;
			lua_newtable(L);
			lua_pushnumber(L, currentCount);
			lua_rawseti(L, -2, currentType);
			lua_rawseti(L, -2, entry);
			currentType = unitDefID;
			currentCount = 1;
		}
	}

	if (currentCount > 0) {
		entry++;
		lua_newtable(L);
		lua_pushnumber(L, currentCount);
		lua_rawseti(L, -2, currentType);
		lua_rawseti(L, -2, entry);
	}

	lua_pushnumber(L, entry);

	return 2;
}


/*** Returns the build queue
 *
 * @function Spring.GetFullBuildQueue
 * @param unitID UnitID
 * @return table<UnitDefID,integer>? buildqueue indexed by unitDefID with count values
 */
int LuaSyncedRead::GetFullBuildQueue(lua_State* L)
{
	return PackBuildQueue(L, false, __func__);
}


/*** Returns the build queue cleaned of things the unit can't build itself
 *
 * @function Spring.GetRealBuildQueue
 * @param unitID UnitID
 * @return table<UnitDefID,integer>? buildqueue indexed by unitDefID with count values
 */
int LuaSyncedRead::GetRealBuildQueue(lua_State* L)
{
	return PackBuildQueue(L, true, __func__);
}



/******************************************************************************/

/***
 *
 * @function Spring.GetUnitCmdDescs
 * @param unitID UnitID
 */
int LuaSyncedRead::GetUnitCmdDescs(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const std::vector<const SCommandDescription*>& cmdDescs = unit->commandAI->GetPossibleCommands();
	const int lastDesc = (int)cmdDescs.size() - 1;

	const int args = lua_gettop(L); // number of arguments
	int startIndex = 0;
	int endIndex = lastDesc;
	if ((args >= 2) && lua_isnumber(L, 2)) {
		startIndex = lua_toint(L, 2) - 1;
		if ((args >= 3) && lua_isnumber(L, 3)) {
			endIndex = lua_toint(L, 3) - 1;
		} else {
			endIndex = startIndex;
		}
	}
	startIndex = std::clamp(startIndex, 0, lastDesc);
	endIndex   = std::clamp(endIndex  , 0, lastDesc);

	lua_createtable(L, endIndex - startIndex, 0);
	int count = 1;
	for (int i = startIndex; i <= endIndex; i++) {
		LuaUtils::PushCommandDesc(L, *cmdDescs[i]);
		lua_rawseti(L, -2, count++);
	}

	return 1;
}


/***
 *
 * @function Spring.FindUnitCmdDesc
 * @param unitID UnitID
 * @param cmdID integer
 * @return integer?
 */
int LuaSyncedRead::FindUnitCmdDesc(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const int cmdID = luaL_checkint(L, 2);

	const std::vector<const SCommandDescription*>& cmdDescs = unit->commandAI->GetPossibleCommands();
	for (int i = 0; i < (int)cmdDescs.size(); i++) {
		if (cmdDescs[i]->id == cmdID) {
			lua_pushnumber(L, i + 1);
			return 1;
		}
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/

/***
 *
 * @function Spring.ValidFeatureID
 * @param featureID FeatureID
 * @return boolean
 */
int LuaSyncedRead::ValidFeatureID(lua_State* L)
{
	lua_pushboolean(L, lua_isnumber(L, 1) && ParseFeature(L, __func__, 1) != nullptr);
	return 1;
}


/***
 * @function Spring.GetAllFeatures
 * @return FeatureID[] featureIDs
 */
int LuaSyncedRead::GetAllFeatures(lua_State* L)
{
	int count = 0;
	const auto& activeFeatureIDs = featureHandler.GetActiveFeatureIDs();

	lua_createtable(L, activeFeatureIDs.size(), 0);

	if (CLuaHandle::GetHandleFullRead(L)) {
		for (const int featureID: activeFeatureIDs) {
			lua_pushnumber(L, featureID);
			lua_rawseti(L, -2, ++count);
		}
	} else {
		for (const int featureID: activeFeatureIDs) {
			if (LuaUtils::IsFeatureVisible(L, featureHandler.GetFeature(featureID))) {
				lua_pushnumber(L, featureID);
				lua_rawseti(L, -2, ++count);
			}
		}
	}
	return 1;
}


/***
 *
 * @function Spring.GetFeatureDefID
 * @param featureID FeatureID
 * @return FeatureDefID?
 */
int LuaSyncedRead::GetFeatureDefID(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->def->id);
	return 1;
}


/***
 *
 * @function Spring.GetFeatureTeam
 * @param featureID FeatureID
 * @return TeamID?
 */
int LuaSyncedRead::GetFeatureTeam(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	if (feature->allyteam < 0) {
		lua_pushnumber(L, -1);
	} else {
		lua_pushnumber(L, feature->team);
	}
	return 1;
}


/***
 *
 * @function Spring.GetFeatureAllyTeam
 * @param featureID FeatureID
 * @return AllyTeamID?
 */
int LuaSyncedRead::GetFeatureAllyTeam(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->allyteam);
	return 1;
}


/***
 *
 * @function Spring.GetFeatureHealth
 * @param featureID FeatureID
 * @return number? health
 * @return number defHealth
 * @return number resurrectProgress
 */
int LuaSyncedRead::GetFeatureHealth(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->health);
	lua_pushnumber(L, feature->def->health);
	lua_pushnumber(L, feature->resurrectProgress);
	return 3;
}


/***
 *
 * @function Spring.GetFeatureHeight
 * @param featureID FeatureID
 * @return number?
 */
int LuaSyncedRead::GetFeatureHeight(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->height);
	return 1;
}


/***
 *
 * @function Spring.GetFeatureRadius
 * @param featureID FeatureID
 * @return number?
 */
int LuaSyncedRead::GetFeatureRadius(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->radius);
	return 1;
}

/***
 *
 * @function Spring.GetFeatureMass
 * @param featureID FeatureID
 * @return number?
 */
int LuaSyncedRead::GetFeatureMass(lua_State* L)
{
	return (GetSolidObjectMass(L, ParseFeature(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetFeaturePosition
 * @param featureID FeatureID
 * @return number? x
 * @return number? y
 * @return number? z
 */
int LuaSyncedRead::GetFeaturePosition(lua_State* L)
{
	return (GetSolidObjectPosition(L, ParseFeature(L, __func__, 1), true));
}


/***
 *
 * @function Spring.GetFeatureSeparation
 * @param featureID1 FeatureID
 * @param featureID2 FeatureID
 * @param direction boolean? (Default: `false`) to subtract from, default featureID1 - featureID2
 * @return number?
 */
int LuaSyncedRead::GetFeatureSeparation(lua_State* L)
{
	const CFeature* feature1 = ParseFeature(L, __func__, 1);
	if (feature1 == nullptr || !LuaUtils::IsFeatureVisible(L, feature1))
		return 0;

	const CFeature* feature2 = ParseFeature(L, __func__, 2);
	if (feature2 == nullptr || !LuaUtils::IsFeatureVisible(L, feature2))
		return 0;

	float3 pos1 = feature1->pos;
	float3 pos2 = feature2->pos;

	float dist;
	if (lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		dist = pos1.distance2D(pos2);
	} else {
		dist = pos1.distance(pos2);
	}

	lua_pushnumber(L, dist);
	return 1;
}

/***
 *
 * @function Spring.GetFeatureRotation
 * Note: PYR order
 * @param featureID FeatureID
 * @return number? pitch Rotation in X axis
 * @return number? yaw Rotation in Y axis
 * @return number? roll Rotation in Z axis
 */
int LuaSyncedRead::GetFeatureRotation(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	return GetSolidObjectRotation(L, feature);
}

/***
 *
 * @function Spring.GetFeatureDirection
 * @param featureID FeatureID
 * @return number? frontDirX
 * @return number? frontDirY
 * @return number? frontDirZ
 * @return number? rightDirX
 * @return number? rightDirY
 * @return number? rightDirZ
 * @return number? upDirX
 * @return number? upDirY
 * @return number? upDirZ
 */
int LuaSyncedRead::GetFeatureDirection(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	const CMatrix44f& mat = feature->GetTransformMatrixRef(true);
	const float3& xdir = mat.GetX();
	const float3& ydir = mat.GetY();
	const float3& zdir = mat.GetZ();

	lua_pushnumber(L, zdir.x);
	lua_pushnumber(L, zdir.y);
	lua_pushnumber(L, zdir.z);

	lua_pushnumber(L, xdir.x);
	lua_pushnumber(L, xdir.y);
	lua_pushnumber(L, xdir.z);

	lua_pushnumber(L, ydir.x);
	lua_pushnumber(L, ydir.y);
	lua_pushnumber(L, ydir.z);

	return 9;
}

/***
 *
 * @function Spring.GetFeatureVelocity
 * Returns nil if no feature found with ID.
 * @param featureID FeatureID
 * @return number? x 
 * @return number? y
 * @return number? z
 * @return number? w
 */
int LuaSyncedRead::GetFeatureVelocity(lua_State* L)
{
	return (GetWorldObjectVelocity(L, ParseFeature(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetFeatureHeading
 * @param featureID FeatureID
 * @return number? heading
 */
int LuaSyncedRead::GetFeatureHeading(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->heading);
	return 1;
}


/***
 *
 * @function Spring.GetFeatureResources
 * @param featureID FeatureID
 * @return number? metal
 * @return number defMetal
 * @return number energy
 * @return number defEnergy
 * @return number reclaimLeft
 * @return number reclaimTime
 */
int LuaSyncedRead::GetFeatureResources(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L,  feature->resources.metal);
	lua_pushnumber(L,  feature->defResources.metal);
	lua_pushnumber(L,  feature->resources.energy);
	lua_pushnumber(L,  feature->defResources.energy);
	lua_pushnumber(L,  feature->reclaimLeft);
	lua_pushnumber(L,  feature->reclaimTime);
	return 6;
}


/***
 *
 * @function Spring.GetFeatureBlocking
 * @param featureID FeatureID
 * @return boolean? isBlocking
 * @return boolean? isSolidObjectCollidable
 * @return boolean? isProjectileCollidable
 * @return boolean? isRaySegmentCollidable
 * @return boolean? crushable
 * @return boolean? blockEnemyPushing
 * @return boolean? blockHeightChanges
 */
int LuaSyncedRead::GetFeatureBlocking(lua_State* L)
{
	return (GetSolidObjectBlocking(L, ParseFeature(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetFeatureNoSelect
 * @param featureID FeatureID
 * @return boolean?
 */
int LuaSyncedRead::GetFeatureNoSelect(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr || !LuaUtils::IsFeatureVisible(L, feature))
		return 0;

	lua_pushboolean(L, feature->noSelect);
	return 1;
}


/***
 *
 * @function Spring.GetFeatureResurrect
 * Returns nil if no feature found with ID.
 * @param featureID FeatureID
 * @return string|""|nil featureDefName
 * @return FacingInteger buildFacing facing of footprint, 0 - 3
 */
int LuaSyncedRead::GetFeatureResurrect(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	if (feature->udef == nullptr) {
		lua_pushliteral(L, "");
	} else {
		lua_pushsstring(L, feature->udef->name);
	}

	lua_pushnumber(L, feature->buildFacing);
	return 2;
}


/***
 *
 * @function Spring.GetFeatureLastAttackedPiece
 * @param featureID FeatureID
 * @return string|""|nil pieceName Last hit piece name
 * @return integer? frame frame it was last hit on, `nil` when featureID is not valid
 */
int LuaSyncedRead::GetFeatureLastAttackedPiece(lua_State* L)
{
	return (GetSolidObjectLastHitPiece(L, ParseFeature(L, __func__, 1)));
}

/*** Parameters related to a collision volume.
 *
 * @class CollisionVolumeData
 * @x_helper
 *
 * @field type "ellipsoid"|"cylinder"|"box"|"sphere" type
 * @field scaleX number
 * @field scaleY number
 * @field scaleZ number
 * @field offsetX number
 * @field offsetY number
 * @field offsetZ number
 * @field boundingRadius number
 * @field defaultToSphere boolean
 * @field defaultToFootPrint boolean
 * @field defaultToPieceTree boolean
*/

/***
 *
 * @function Spring.GetFeatureCollisionVolumeData
 * @param featureID FeatureID
 * @return CollisionVolumeData?
 */
int LuaSyncedRead::GetFeatureCollisionVolumeData(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	return LuaUtils::PushColVolData(L, &feature->collisionVolume);
}

/***
 *
 * @function Spring.GetFeaturePieceCollisionVolumeData
 * @param featureID FeatureID
 * @return CollisionVolumeData?
 */
int LuaSyncedRead::GetFeaturePieceCollisionVolumeData(lua_State* L)
{
	return (PushPieceCollisionVolumeData(L, ParseFeature(L, __func__, 1)));
}


/*** Get the feature current fire timer.
 *
 * @function Spring.GetFeatureFireTime
 *
 * @param featureID FeatureID
 * @return number? fireTime in seconds, nil when featureID is invalid.
 */
int LuaSyncedRead::GetFeatureFireTime(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	lua_pushnumber(L, feature->fireTime * INV_GAME_SPEED);
	return 1;
}


/*** Get the feature current smoke timer.
 *
 * @function Spring.GetFeatureSmokeTime
 *
 * @param featureID FeatureID
 * @return number? smokeTime in seconds, nil when featureID is invalid.
 */
int LuaSyncedRead::GetFeatureSmokeTime(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	lua_pushnumber(L, feature->smokeTime * INV_GAME_SPEED);
	return 1;
}


/******************************************************************************
 * Projectile state
 *
 * @section projectile_state
******************************************************************************/


/***
 *
 * @function Spring.GetProjectilePosition
 * @param projectileID ProjectileID
 * @return number? posX
 * @return number? posY
 * @return number? posZ
 */
int LuaSyncedRead::GetProjectilePosition(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	lua_pushnumber(L, pro->pos.x);
	lua_pushnumber(L, pro->pos.y);
	lua_pushnumber(L, pro->pos.z);
	return 3;
}

/***
 *
 * @function Spring.GetProjectileDirection
 * @param projectileID ProjectileID
 * @return number? dirX
 * @return number? dirY
 * @return number? dirZ
 */
int LuaSyncedRead::GetProjectileDirection(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	lua_pushnumber(L, pro->dir.x);
	lua_pushnumber(L, pro->dir.y);
	lua_pushnumber(L, pro->dir.z);
	return 3;
}

/***
 *
 * @function Spring.GetProjectileVelocity
 * @param projectileID ProjectileID
 * @return number? velX
 * @return number? velY
 * @return number? velZ
 * @return number? velW
 */
int LuaSyncedRead::GetProjectileVelocity(lua_State* L)
{
	return (GetWorldObjectVelocity(L, ParseProjectile(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetProjectileGravity
 * @param projectileID ProjectileID
 * @return number?
 */
int LuaSyncedRead::GetProjectileGravity(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	lua_pushnumber(L, pro->mygravity);
	return 1;
}



/***
 *
 * @function Spring.GetPieceProjectileParams
 * @param projectileID ProjectileID
 * @return integer? explosionFlags encoded bitwise with SHATTER = 1, EXPLODE = 2, EXPLODE_ON_HIT = 2, FALL = 4, SMOKE = 8, FIRE = 16, NONE = 32, NO_CEG_TRAIL = 64, NO_HEATCLOUD = 128
 * @return number spinAngle
 * @return number spinSpeed
 * @return number spinVectorX
 * @return number spinVectorY
 * @return number spinVectorZ
 */
int LuaSyncedRead::GetPieceProjectileParams(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr || !pro->piece)
		return 0;

	const CPieceProjectile* ppro = static_cast<const CPieceProjectile*>(pro);

	lua_pushnumber(L, ppro->explFlags);
	lua_pushnumber(L, ppro->spinAngle);
	lua_pushnumber(L, ppro->spinSpeed);
	lua_pushnumber(L, ppro->spinVec.x);
	lua_pushnumber(L, ppro->spinVec.y);
	lua_pushnumber(L, ppro->spinVec.z);
	return (1 + 1 + 1 + 3);
}


/***
 *
 * @function Spring.GetProjectileTarget
 * @param projectileID ProjectileID
 * @return integer? targetTypeInt where
 * string.byte('g') := GROUND
 * string.byte('u') := UNIT
 * string.byte('f') := FEATURE
 * string.byte('p') := PROJECTILE
 * @return UnitID|FeatureID|ProjectileID|float3 target targetID or targetPos when targetTypeInt == string.byte('g')
 */
int LuaSyncedRead::GetProjectileTarget(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr || !pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);
	const CWorldObject* wtgt = wpro->GetTargetObject();

	if (wtgt == nullptr) {
		lua_pushnumber(L, int('g')); // ground
		lua_createtable(L, 3, 0);
		lua_pushnumber(L, (wpro->GetTargetPos()).x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, (wpro->GetTargetPos()).y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, (wpro->GetTargetPos()).z); lua_rawseti(L, -2, 3);
		return 2;
	}

	if (dynamic_cast<const CUnit*>(wtgt) != nullptr) {
		lua_pushnumber(L, int('u'));
		lua_pushnumber(L, wtgt->id);
		return 2;
	}
	if (dynamic_cast<const CFeature*>(wtgt) != nullptr) {
		lua_pushnumber(L, int('f'));
		lua_pushnumber(L, wtgt->id);
		return 2;
	}
	if (dynamic_cast<const CWeaponProjectile*>(wtgt) != nullptr) {
		lua_pushnumber(L, int('p'));
		lua_pushnumber(L, wtgt->id);
		return 2;
	}

	// projectile target cannot be anything else
	assert(false);
	return 0;
}


/***
 *
 * @function Spring.GetProjectileIsIntercepted
 * @param projectileID ProjectileID
 * @return boolean?
 */
int LuaSyncedRead::GetProjectileIsIntercepted(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr || !pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);

	lua_pushboolean(L, wpro->IsBeingIntercepted());
	return 1;
}


/***
 *
 * @function Spring.GetProjectileTimeToLive
 * @param projectileID ProjectileID
 * @return integer?
 */
int LuaSyncedRead::GetProjectileTimeToLive(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr || !pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);

	lua_pushnumber(L, wpro->GetTimeToLive());
	return 1;
}


/***
 *
 * @function Spring.GetProjectileOwnerID
 * @param projectileID ProjectileID
 * @return UnitID?
 */
int LuaSyncedRead::GetProjectileOwnerID(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	const int unitID = pro->GetOwnerID();
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= unitHandler.MaxUnits()))
		return 0;

	lua_pushnumber(L, unitID);
	return 1;
}


/***
 *
 * @function Spring.GetProjectileTeamID
 * @param projectileID ProjectileID
 * @return TeamID?
 */
int LuaSyncedRead::GetProjectileTeamID(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	if (!teamHandler.IsValidTeam(pro->GetTeamID()))
		return 0;

	lua_pushnumber(L, pro->GetTeamID());
	return 1;
}


/***
 *
 * @function Spring.GetProjectileAllyTeamID
 * @param projectileID ProjectileID
 * @return AllyTeamID?
 */
int LuaSyncedRead::GetProjectileAllyTeamID(lua_State* L)
{
	const CProjectile* const pro = ParseProjectile(L, __func__, 1);
	if (pro == nullptr)
		return 0;

	const auto allyTeamID = pro->GetAllyteamID();
	if (!teamHandler.IsValidAllyTeam(allyTeamID))
		return 0;

	lua_pushnumber(L, allyTeamID);
	return 1;
}


/***
 *
 * @function Spring.GetProjectileType
 * @param projectileID ProjectileID
 * @return boolean? weapon
 * @return boolean piece
 */
int LuaSyncedRead::GetProjectileType(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	lua_pushboolean(L, pro->weapon);
	lua_pushboolean(L, pro->piece);
	return 2;
}


/***
 *
 * @function Spring.GetProjectileDefID
 *
 * @param projectileID ProjectileID
 * @return WeaponDefID?
 */
int LuaSyncedRead::GetProjectileDefID(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;
	if (!pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);
	const WeaponDef* wdef = wpro->GetWeaponDef();

	if (wdef == nullptr)
		return 0;

	lua_pushnumber(L, wdef->id);
	return 1;
}

/*** Returns the name of the model piece from which a piece projectile was spawned. Returns nil for other projectiles including weapons
 *
 * @function Spring.GetPieceProjectileName
 * @param projectileID ProjectileID
 * @return string? pieceName
 */
int LuaSyncedRead::GetPieceProjectileName(lua_State* L)
{
	const auto* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	if (!pro->piece)
		return 0;

	const auto* ppro = static_cast <const CPieceProjectile*> (pro);
	if (ppro == nullptr || ppro->omp == nullptr) // FIXME: assert? neither should happen if pro->piece was true
		return 0;

	lua_pushsstring(L, ppro->omp->name);
	return 1;
}


/***
 *
 * @function Spring.GetProjectileDamages
 * @param projectileID ProjectileID
 * @param tag string one of:
 *     "paralyzeDamageTime"
 *     "impulseFactor"
 *     "impulseBoost"
 *     "craterMult"
 *     "craterBoost"
 *     "dynDamageExp"
 *     "dynDamageMin"
 *     "dynDamageRange"
 *     "dynDamageInverted"
 *     "craterAreaOfEffect"
 *     "damageAreaOfEffect"
 *     "edgeEffectiveness"
 *     "explosionSpeed"
 *     - or -
 *     an armor type index to get the damage against it.
 * @return number?
 */
int LuaSyncedRead::GetProjectileDamages(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	if (!pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);
	const std::string key = luaL_checkstring(L, 2);

	return PushDamagesKey(L, *wpro->damages, 2);
}




/******************************************************************************
 * Ground
 *
 * @section ground
******************************************************************************/


/***
 *
 * @function Spring.IsPosInMap
 * @param x number
 * @param z number
 * @return boolean inPlayArea whether the position is in the active play area
 * @return boolean inMap whether the position is in the full map area (currently this is the same as above)
 */
int LuaSyncedRead::IsPosInMap(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);

	const float mapX = mapDims.mapx * SQUARE_SIZE;
	const float mapZ = mapDims.mapy * SQUARE_SIZE;

	const bool inMap
		=  x >= 0
		&& z >= 0
		&& x <= mapX
		&& z <= mapZ
	;

	/* Currently, the engine does not support limiting
	 * the active play area natively, but it would be
	 * a good feature to have, so let's be future-proof.
	 *
	 * This would be things like:
	 *  - dynamically expanding map. Primarily for single
	 *    player missions (think Supcom) but not necessarily.
	 *  - circular maps, think 0 A.D. (where the technical map
	 *    stays a square but corners are outside the play area).
	 *  - just a decoration / flavor area outside the map proper.
	 */
	const bool inPlayArea = inMap;

	lua_pushboolean(L, inPlayArea);
	lua_pushboolean(L, inMap);
	return 2;
}

/*** Get ground height
 *
 * On sea, this returns the negative depth of the seafloor
 *
 * @function Spring.GetGroundHeight
 * @param x number
 * @param z number
 * @return number
 */
int LuaSyncedRead::GetGroundHeight(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	lua_pushnumber(L, CGround::GetHeightReal(x, z, CLuaHandle::GetHandleSynced(L)));
	return 1;
}

/*** Get water plane height
 *
 * Water may at some point become shaped (rivers etc) but for now it is always a flat plane.
 * Use this function instead of GetWaterLevel to denote you are relying on that assumption.
 *
 * @see Spring.GetWaterLevel
 * @function Spring.GetWaterPlaneLevel
 * @return number waterPlaneLevel
 */
int LuaSyncedRead::GetWaterPlaneLevel(lua_State* L)
{
	lua_pushnumber(L, CGround::GetWaterPlaneLevel());
	return 1;
}

/*** Get water level in a specific position
 *
 * Water is currently a flat plane, so this returns the same value regardless of XZ.
 * However water may become more dynamic at some point so by using this you are future-proof.
 *
 * @function Spring.GetWaterLevel
 * @param x number
 * @param z number
 * @return number waterLevel
 */
int LuaSyncedRead::GetWaterLevel(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	lua_pushnumber(L, CGround::GetWaterLevel(x, z));
	return 1;
}


/*** Get ground height as it was at game start
 *
 * Returns the original height before the ground got deformed
 *
 * @function Spring.GetGroundOrigHeight
 * @param x number
 * @param z number
 * @return number
 */
int LuaSyncedRead::GetGroundOrigHeight(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	lua_pushnumber(L, CGround::GetOrigHeight(x, z));
	return 1;
}


/***
 *
 * @function Spring.GetGroundNormal
 * @param x number
 * @param z number
 * @param smoothed boolean? (Default: `false`) raw or smoothed center normal
 * @return number normalX
 * @return number normalY
 * @return number normalZ
 * @return number slope
 */
int LuaSyncedRead::GetGroundNormal(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);

	// raw or smoothed center normal
	const float3& normal = luaL_optboolean(L, 3, false)?
		CGround::GetNormal(x, z, CLuaHandle::GetHandleSynced(L)):
		CGround::GetSmoothNormal(x, z, CLuaHandle::GetHandleSynced(L));

	lua_pushnumber(L, normal.x);
	lua_pushnumber(L, normal.y);
	lua_pushnumber(L, normal.z);
	// slope derives from face normals, include it here
	lua_pushnumber(L, CGround::GetSlope(x, z, CLuaHandle::GetHandleSynced(L)));
	return 4;
}


/***
 *
 * @function Spring.GetGroundInfo
 * @param x number
 * @param z number
 * @return integer ix
 * @return integer iz
 * @return integer terrainTypeIndex
 * @return string name
 * @return number metalExtraction
 * @return number hardness
 * @return number tankSpeed
 * @return number kbotSpeed
 * @return number hoverSpeed
 * @return number shipSpeed
 * @return boolean receiveTracks
 */
int LuaSyncedRead::GetGroundInfo(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);

	const int ix = std::clamp(x, 0.0f, float3::maxxpos) / (SQUARE_SIZE * 2);
	const int iz = std::clamp(z, 0.0f, float3::maxzpos) / (SQUARE_SIZE * 2);

	const int maxIndex = (mapDims.hmapx * mapDims.hmapy) - 1;
	const int sqrIndex = std::min(maxIndex, (mapDims.hmapx * iz) + ix);
	const int  ttIndex = readMap->GetTypeMapSynced()[sqrIndex];

	assert(ttIndex < CMapInfo::NUM_TERRAIN_TYPES);
	assert(lua_gettop(L) == 2);

	// LuaMetalMap::GetMetalAmount uses absolute indexing,
	// replace the top two elements (x and z) by ix and iz
	lua_pop(L, 2);
	lua_pushnumber(L, ix);
	lua_pushnumber(L, iz);

	return (PushTerrainTypeData(L, &mapInfo->terrainTypes[ttIndex], true));
}


// similar to ParseMapParams in LuaSyncedCtrl
static void ParseMapCoords(lua_State* L, const char* caller,
                           int& tx1, int& tz1, int& tx2, int& tz2)
{
	float fx1 = 0, fz1 = 0, fx2 = 0, fz2 = 0;

	const int args = lua_gettop(L); // number of arguments
	if (args == 2) {
		fx1 = fx2 = luaL_checkfloat(L, 1);
		fz1 = fz2 = luaL_checkfloat(L, 2);
	}
	else if (args == 4) {
		fx1 = luaL_checkfloat(L, 1);
		fz1 = luaL_checkfloat(L, 2);
		fx2 = luaL_checkfloat(L, 3);
		fz2 = luaL_checkfloat(L, 4);
	}
	else {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	// quantize and clamp
	tx1 = std::clamp((int)(fx1 / SQUARE_SIZE), 0, mapDims.mapxm1);
	tx2 = std::clamp((int)(fx2 / SQUARE_SIZE), 0, mapDims.mapxm1);
	tz1 = std::clamp((int)(fz1 / SQUARE_SIZE), 0, mapDims.mapym1);
	tz2 = std::clamp((int)(fz2 / SQUARE_SIZE), 0, mapDims.mapym1);
}


/***
 *
 * @function Spring.GetGroundBlocked
 * @param x number world x coordinate (or xMin when using 4-arg form)
 * @param z number world z coordinate (or zMin when using 4-arg form)
 * @param x2 number? world xMax (4-arg rectangle form)
 * @param z2 number? world zMax (4-arg rectangle form)
 * @return string? objectType `"feature"` or `"unit"`
 * @return ObjectID? objectID the feature or unit ID
 */
int LuaSyncedRead::GetGroundBlocked(lua_State* L)
{
	if ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && !CLuaHandle::GetHandleFullRead(L))
		return 0;

	int tx1, tx2, tz1, tz2;
	ParseMapCoords(L, __func__, tx1, tz1, tx2, tz2);

	for (int z = tz1; z <= tz2; z++){
		for (int x = tx1; x <= tx2; x++){
			const CSolidObject* s = groundBlockingObjectMap.GroundBlocked(x, z);

			const CFeature* feature = dynamic_cast<const CFeature*>(s);
			if (feature != nullptr) {
				if (LuaUtils::IsFeatureVisible(L, feature)) {
					LuaPushString(L, "feature");
					lua_pushnumber(L, feature->id);
					return 2;
				}

				continue;
			}

			const CUnit* unit = dynamic_cast<const CUnit*>(s);
			if (unit != nullptr) {
				if (CLuaHandle::GetHandleFullRead(L) || (unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & LOS_INLOS)) {
					LuaPushString(L, "unit");
					lua_pushnumber(L, unit->id);
					return 2;
				}

				continue;
			}
		}
	}

	lua_pushboolean(L, false);
	return 1;
}


/***
 *
 * @function Spring.GetGroundExtremes
 * @return number initMinHeight
 * @return number initMaxHeight
 * @return number currMinHeight
 * @return number currMaxHeight
 */
int LuaSyncedRead::GetGroundExtremes(lua_State* L)
{
	lua_pushnumber(L, readMap->GetInitMinHeight());
	lua_pushnumber(L, readMap->GetInitMaxHeight());
	lua_pushnumber(L, readMap->GetCurrMinHeight());
	lua_pushnumber(L, readMap->GetCurrMaxHeight());
	return 4;
}


/***
 *
 * @function Spring.GetTerrainTypeData
 * @param terrainTypeInfo integer
 * @return integer index
 * @return string name
 * @return number hardness
 * @return number tankSpeed
 * @return number kbotSpeed
 * @return number hoverSpeed
 * @return number shipSpeed
 * @return boolean receiveTracks
 */
int LuaSyncedRead::GetTerrainTypeData(lua_State* L)
{
	const int tti = luaL_checkint(L, 1);

	if (tti < 0 || tti >= CMapInfo::NUM_TERRAIN_TYPES)
		return 0;

	return (PushTerrainTypeData(L, &mapInfo->terrainTypes[tti], false));
}


/***
 *
 * @function Spring.GetGrass
 * @param x number
 * @param z number
 * @return number
 */
int LuaSyncedRead::GetGrass(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1), 0.0f, luaL_checkfloat(L, 2));
	lua_pushnumber(L, grassDrawer->GetGrass(pos.cClampInBounds()));
	return 1;
}

/******************************************************************************/

/***
 *
 * @function Spring.GetSmoothMeshHeight
 * @param x number
 * @param z number
 * @return number height
 */
int LuaSyncedRead::GetSmoothMeshHeight(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);

	lua_pushnumber(L, smoothGround.GetHeight(x, z));
	return 1;
}


/******************************************************************************
 * Tests
 *
 * @section tests
******************************************************************************/


/***
 *
 * @function Spring.TestMoveOrder
 * @param unitDefID UnitDefID
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param dirX number? (Default: `0.0`)
 * @param dirY number? (Default: `0.0`)
 * @param dirZ number? (Default: `0.0`)
 * @param testTerrain boolean? (Default: `true`)
 * @param testObjects boolean? (Default: `true`)
 * @param centerOnly boolean? (Default: `false`)
 * @return boolean
 */
int LuaSyncedRead::TestMoveOrder(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* unitDef = unitDefHandler->GetUnitDefByID(unitDefID);

	if (unitDef == nullptr || unitDef->pathType == -1u) {
		lua_pushboolean(L, false);
		return 1;
	}

	const MoveDef* moveDef = moveDefHandler.GetMoveDefByPathType(unitDef->pathType);

	if (moveDef == nullptr) {
		lua_pushboolean(L, !unitDef->IsImmobileUnit());
		return 1;
	}

	const float3 pos(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4));
	const float3 dir(luaL_optfloat(L, 5, 0.0f), luaL_optfloat(L, 6, 0.0f), luaL_optfloat(L, 7, 0.0f));

	const bool testTerrain = luaL_optboolean(L, 8, true);
	const bool testObjects = luaL_optboolean(L, 9, true);
	const bool centerOnly = luaL_optboolean(L, 10, false);

	bool los = false;
	bool ret = false;

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		los = CLuaHandle::GetHandleFullRead(L);
	} else {
		los = losHandler->InLos(pos, CLuaHandle::GetHandleReadAllyTeam(L));
	}

	if (los){
		MoveTypes::CheckCollisionQuery collisionQuery(moveDef, pos);
		ret = moveDef->TestMoveSquare(collisionQuery, pos, dir, testTerrain, testObjects, centerOnly);
	}

	lua_pushboolean(L, ret);
	return 1;
}

/***
 * @alias BuildOrderBlockedStatus
 * | 0 # blocked
 * | 1 # mobile unit on the way
 * | 2 # reclaimable
 * | 3 # open
 */

/***
 * @function Spring.TestBuildOrder
 * @param unitDefID UnitDefID
 * @param x number
 * @param y number
 * @param z number
 * @param facing Facing
 * @return BuildOrderBlockedStatus blocking
 * @return FeatureID? featureID A reclaimable feature in the way.
 */
int LuaSyncedRead::TestBuildOrder(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* unitDef = unitDefHandler->GetUnitDefByID(unitDefID);

	if (unitDef == nullptr) {
		lua_pushnumber(L, 0);
		return 1;
	}

	BuildInfo bi;
	bi.buildFacing = LuaUtils::ParseFacing(L, __func__, 5);
	bi.def = unitDef;
	bi.pos = {luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4)};
	bi.pos = CGameHelper::Pos2BuildPos(bi, CLuaHandle::GetHandleSynced(L));
	CFeature* feature;

	// negative allyTeam values have full visibility in TestUnitBuildSquare()
	// 0 = BUILDSQUARE_BLOCKED
	// 1 = BUILDSQUARE_OCCUPIED
	// 2 = BUILDSQUARE_RECLAIMABLE
	// 3 = BUILDSQUARE_OPEN
	int retval = CGameHelper::TestUnitBuildSquare(bi, feature, CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleSynced(L));

	// the output of TestUnitBuildSquare was changed after this API function was written
	// keep backward-compatibility by mapping BUILDSQUARE_OPEN to BUILDSQUARE_RECLAIMABLE
	if (retval == CGameHelper::BUILDSQUARE_OPEN)
		retval = CGameHelper::BUILDSQUARE_RECLAIMABLE;

	if (feature == nullptr) {
		lua_pushnumber(L, retval);
		return 1;
	}

	lua_pushnumber(L, retval);
	lua_pushnumber(L, feature->id);
	return 2;
}


/***
 * @class BuildOrderSpec
 * @x_helper
 * @field [1] integer unitDefID
 * @field [2] number x
 * @field [3] number y
 * @field [4] number z
 * @field [5] Facing facing
 */

static BuildInfo ParseBuildOrderSpec(lua_State* L, int tableIndex)
{
	luaL_checktype(L, tableIndex, LUA_TTABLE);

	BuildInfo buildInfo;

	lua_rawgeti(L, tableIndex, 1);
	buildInfo.def = unitDefHandler->GetUnitDefByID(luaL_checkint(L, -1));
	lua_pop(L, 1);

	float pos[3];
	for (int i = 0; i < 3; ++i) {
		lua_rawgeti(L, tableIndex, i + 2);
		pos[i] = luaL_checkfloat(L, -1);
		lua_pop(L, 1);
	}
	buildInfo.pos = {pos[0], pos[1], pos[2]};

	lua_rawgeti(L, tableIndex, 5);
	buildInfo.buildFacing = LuaUtils::ParseFacing(L, __func__, -1);
	lua_pop(L, 1);

	return buildInfo;
}

/***
 * @function Spring.TestBuildOrderOverlap
 * @param queuedBuild BuildOrderSpec
 * @param proposedBuild BuildOrderSpec
 * @return boolean overlaps Whether the builds conflict under the queued-build overlap rules.
 * @return boolean cancels Whether the proposed build would cancel the earlier queued build.
 */
int LuaSyncedRead::TestBuildOrderOverlap(lua_State* L)
{
	BuildInfo queuedBuild = ParseBuildOrderSpec(L, 1);
	BuildInfo proposedBuild = ParseBuildOrderSpec(L, 2);

	if (queuedBuild.def == nullptr || proposedBuild.def == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}

	const bool synced = CLuaHandle::GetHandleSynced(L);
	queuedBuild.pos = CGameHelper::Pos2BuildPos(queuedBuild, synced);
	proposedBuild.pos = CGameHelper::Pos2BuildPos(proposedBuild, synced);

	const auto result = CGameHelper::TestQueuedBuildOverlap(queuedBuild, proposedBuild);
	lua_pushboolean(L, result != QueuedBuildOverlap::Result::NONE);
	lua_pushboolean(L, result == QueuedBuildOverlap::Result::CANCEL);
	return 2;
}

/*** Snaps a position to the building grid
 *
 * @function Spring.Pos2BuildPos
 * @param unitDefID UnitDefID
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param buildFacing number? (Default: `0`) one of SOUTH = 0, EAST = 1, NORTH = 2, WEST  = 3
 * @return number buildPosX
 * @return number buildPosY
 * @return number buildPosZ
 */
int LuaSyncedRead::Pos2BuildPos(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == nullptr)
		return 0;

	const float3 worldPos = {luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4)};
	const float3 buildPos = CGameHelper::Pos2BuildPos({ud, worldPos, luaL_optint(L, 5, FACING_SOUTH)}, CLuaHandle::GetHandleSynced(L));

	lua_pushnumber(L, buildPos.x);
	lua_pushnumber(L, buildPos.y);
	lua_pushnumber(L, buildPos.z);
	return 3;
}


/***
 *
 * @function Spring.ClosestBuildPos
 * @param teamID TeamID
 * @param unitDefID UnitDefID
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param searchRadius number
 * @param minDistance number
 * @param buildFacing number one of SOUTH = 0, EAST = 1, NORTH = 2, WEST  = 3
 * @return number buildPosX
 * @return number buildPosY
 * @return number buildPosZ
 */
int LuaSyncedRead::ClosestBuildPos(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	const int udefID = luaL_checkint(L, 2);

	const float searchRadius = luaL_checkfloat(L, 6);

	const int minDistance = luaL_checkfloat(L, 7);
	const int buildFacing = luaL_checkint(L, 8);

	const float3 worldPos = {luaL_checkfloat(L, 3), luaL_checkfloat(L, 4), luaL_checkfloat(L, 5)};
	const float3 buildPos = CGameHelper::ClosestBuildPos(teamID, unitDefHandler->GetUnitDefByID(udefID), worldPos, searchRadius, minDistance, buildFacing, CLuaHandle::GetHandleSynced(L));

	lua_pushnumber(L, buildPos.x);
	lua_pushnumber(L, buildPos.y);
	lua_pushnumber(L, buildPos.z);
	return 3;
}


/******************************************************************************
 * Visibility
 *
 * @section visibility
******************************************************************************/


static int GetEffectiveLosAllyTeam(lua_State* L, int arg)
{
	if (lua_isnoneornil(L, arg))
		return (CLuaHandle::GetHandleReadAllyTeam(L));

	const int aat = luaL_optint(L, arg, CEventClient::MinSpecialTeam - 1);

	if (aat == CEventClient::NoAccessTeam)
		return aat;

	if (CLuaHandle::GetHandleFullRead(L)) {
		if (teamHandler.IsValidAllyTeam(aat))
			return aat;

		if (aat == CEventClient::AllAccessTeam)
			return aat;
	} else {
		if (aat == CLuaHandle::GetHandleReadAllyTeam(L))
			return aat;
	}

	// never returns
	return (luaL_argerror(L, arg, "Invalid allyTeam"));
}


/***
 *
 * @function Spring.GetPositionLosState
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param allyTeamID AllyTeamID?
 * @return boolean inLosOrRadar
 * @return boolean inLos
 * @return boolean inRadar
 * @return boolean inJammer
 */
int LuaSyncedRead::GetPositionLosState(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);
	if (allyTeamID < 0) {
		const bool fullView = (allyTeamID == CEventClient::AllAccessTeam);
		lua_pushboolean(L, fullView);
		lua_pushboolean(L, fullView);
		lua_pushboolean(L, fullView);
		lua_pushboolean(L, fullView);
		return 4;
	}

	const bool inLos    = losHandler->InLos(pos, allyTeamID);
	const bool inRadar  = losHandler->InRadar(pos, allyTeamID);
	const bool inJammer = losHandler->InJammer(pos, allyTeamID);

	lua_pushboolean(L, inLos || inRadar);
	lua_pushboolean(L, inLos);
	lua_pushboolean(L, inRadar);
	lua_pushboolean(L, inJammer);
	return 4;
}


/***
 *
 * @function Spring.IsPosInLos
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param allyTeamID AllyTeamID?
 * @return boolean
 */
int LuaSyncedRead::IsPosInLos(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InLos(pos, allyTeamID));
	return 1;
}


/***
 *
 * @function Spring.IsPosInRadar
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param allyTeamID AllyTeamID?
 * @return boolean
 */
int LuaSyncedRead::IsPosInRadar(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InRadar(pos, allyTeamID));
	return 1;
}


/***
 *
 * @function Spring.IsPosInAirLos
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param allyTeamID AllyTeamID?
 * @return boolean
 */
int LuaSyncedRead::IsPosInAirLos(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InAirLos(pos, allyTeamID));
	return 1;
}

/*** Get unit los state (bitmask)
 *
 * @function Spring.GetUnitLosState
 * @param unitID UnitID
 * @param allyTeamID AllyTeamID?
 * @param raw true Return a bitmask.
 * @return LosMask|integer? bitmask A bitmask of `LosMask` bits
 */

/*** Get unit los state (table)
 *
 * @function Spring.GetUnitLosState
 * @param unitID UnitID
 * @param allyTeamID AllyTeamID?
 * @param raw false? (Default: `false`) Return a table.
 * @return table<"los"|"radar"|"typed",boolean>? los A table of LOS state names as keys and booleans as values, or `nil` if `unitID` is invalid.
 */
int LuaSyncedRead::GetUnitLosState(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	unsigned short losStatus;
	if (allyTeamID < 0) {
		losStatus = (allyTeamID == CEventClient::AllAccessTeam) ? (LOS_ALL_MASK_BITS | LOS_ALL_BITS) : 0;
	} else {
		losStatus = unit->losStatus[allyTeamID];
	}

	constexpr int currMask = LOS_INLOS   | LOS_INRADAR;
	constexpr int prevMask = LOS_PREVLOS | LOS_CONTRADAR;

	const bool isTyped = ((losStatus & prevMask) == prevMask);

	if (luaL_optboolean(L, 3, false)) {
		// return a numeric value
		if (!CLuaHandle::GetHandleFullRead(L))
			losStatus &= ((prevMask * isTyped) | currMask);

		lua_pushnumber(L, losStatus);
		return 1;
	}

	lua_createtable(L, 0, 3);
	if (losStatus & LOS_INLOS) {
		LuaPushNamedBool(L, "los", true);
	}
	if (losStatus & LOS_INRADAR) {
		LuaPushNamedBool(L, "radar", true);
	}
	if ((losStatus & LOS_INLOS) || isTyped) {
		LuaPushNamedBool(L, "typed", true);
	}
	return 1;
}


/***
 *
 * @function Spring.IsUnitInLos
 * @param unitID UnitID
 * @param allyTeamID AllyTeamID? defaults to the calling widget/gadget's ally team
 * @return boolean inLos
 */
int LuaSyncedRead::IsUnitInLos(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InLos(unit, allyTeamID));
	return 1;
}


/***
 *
 * @function Spring.IsUnitInAirLos
 * @param unitID UnitID
 * @param allyTeamID AllyTeamID? defaults to the calling widget/gadget's ally team
 * @return boolean inAirLos
 */
int LuaSyncedRead::IsUnitInAirLos(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InAirLos(unit, allyTeamID));
	return 1;
}


/***
 *
 * @function Spring.IsUnitInRadar
 * @param unitID UnitID
 * @param allyTeamID AllyTeamID? defaults to the calling widget/gadget's ally team
 * @return boolean inRadar
 */
int LuaSyncedRead::IsUnitInRadar(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InRadar(unit, allyTeamID));
	return 1;
}


/***
 *
 * @function Spring.IsUnitInJammer
 * @param unitID UnitID
 * @param allyTeamID AllyTeamID
 * @return boolean inJammer
 */
int LuaSyncedRead::IsUnitInJammer(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	if (allyTeamID < 0) {
		luaL_argerror(L, 2, "Invalid allyTeam");
		return 0;
	}

	lua_pushboolean(L, losHandler->InJammer(unit, allyTeamID)); //FIXME
	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetClosestValidPosition(lua_State* L)
{
	// FIXME -- finish this
	/*const int unitDefID = luaL_checkint(L, 1);
	const float x     = luaL_checkfloat(L, 2);
	const float z     = luaL_checkfloat(L, 3);
	const float r     = luaL_checkfloat(L, 4);*/
	//const int mx = (int)max(0 , min(mapDims.mapxm1, (int)(x / SQUARE_SIZE)));
	//const int mz = (int)max(0 , min(mapDims.mapym1, (int)(z / SQUARE_SIZE)));
	return 0;
}


/******************************************************************************
 * Piece/Script
 *
 * @section piecescript
******************************************************************************/


static int GetModelRootPiece(lua_State* L, const std::string& modelName)
{
	const auto model = modelLoader.LoadModel(modelName);
	if (model == nullptr)
		return 0;

	lua_pushnumber(L, model->GetRootPieceIndex() + 1);
	return 1;
}

static int GetModelPieceMap(lua_State* L, const std::string& modelName)
{
	if (modelName.empty())
		return 0;

	const auto* model = modelLoader.LoadModel(modelName);
	if (model == nullptr)
		return 0;

	lua_createtable(L, 0, model->numPieces);

	// {"piece" = 123, ...}
	for (size_t i = 0; i < model->numPieces; i++) {
		const auto* p = model->pieceObjects[i];
		lua_pushsstring(L, p->name);
		lua_pushnumber(L, i + 1);
		lua_rawset(L, -3);
	}

	return 1;
}

static int GetModelPieceList(lua_State* L, const std::string& modelName)
{
	if (modelName.empty())
		return 0;

	const auto* model = modelLoader.LoadModel(modelName);
	if (model == nullptr)
		return 0;

	lua_createtable(L, model->numPieces, 0);

	// {[1] = "piece", ...}
	for (size_t i = 0; i < model->numPieces; i++) {
		const auto* p = model->pieceObjects[i];
		lua_pushsstring(L, p->name);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

static int GetSolidObjectRootPiece(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	lua_pushnumber(L, o->localModel.GetRoot()->GetLModelPieceIndex() + 1);
	return 1;
}

static int GetSolidObjectPieceMap(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModel& localModel = o->localModel;

	lua_createtable(L, 0, localModel.pieces.size());

	// {"piece" = 123, ...}
	for (size_t i = 0; i < localModel.pieces.size(); i++) {
		const LocalModelPiece& lp = localModel.pieces[i];
		lua_pushsstring(L, lp.original->name);
		lua_pushnumber(L, i + 1);
		lua_rawset(L, -3);
	}

	return 1;
}

static int GetSolidObjectPieceList(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModel& localModel = o->localModel;

	lua_createtable(L, localModel.pieces.size(), 0);

	// {[1] = "piece", ...}
	for (size_t i = 0; i < localModel.pieces.size(); i++) {
		const LocalModelPiece& lp = localModel.pieces[i];
		lua_pushsstring(L, lp.original->name);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}


/***
 * @class PieceInfo
 * @x_helper
 * @field name string
 * @field parent string
 * @field children string[] names
 * @field empty boolean
 * @field min [number,number,number] (x,y,z)
 * @field max [number,number,number] (x,y,z)
 * @field offset [number,number,number] (x,y,z)
 */


static int GetSolidObjectPieceInfoHelper(lua_State* L, const S3DModelPiece& op)
{
	lua_createtable(L, 0, 7);
	LuaPushNamedString(L, "name",   op.name);
	LuaPushNamedString(L, "parent", ((op.parent != nullptr) ? op.parent->name : "[null]"));

	LuaPushString(L, "children");
	lua_createtable(L, op.children.size(), 0);
	for (size_t c = 0; c < op.children.size(); c++) {
		lua_pushsstring(L, op.children[c]->name);
		lua_rawseti(L, -2, c + 1);
	}
	lua_rawset(L, -3);

	LuaPushNamedBool(L, "isEmpty", !op.HasGeometryData());

	LuaPushString(L, "min");
	lua_createtable(L, 3, 0); {
		lua_pushnumber(L, op.mins.x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, op.mins.y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, op.mins.z); lua_rawseti(L, -2, 3);
	}
	lua_rawset(L, -3);

	LuaPushString(L, "max");
	lua_createtable(L, 3, 0); {
		lua_pushnumber(L, op.maxs.x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, op.maxs.y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, op.maxs.z); lua_rawseti(L, -2, 3);
	}
	lua_rawset(L, -3);

	LuaPushString(L, "offset");
	lua_createtable(L, 3, 0); {
		lua_pushnumber(L, op.offset.x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, op.offset.y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, op.offset.z); lua_rawseti(L, -2, 3);
	}
	lua_rawset(L, -3);
	return 1;
}

static int GetSolidObjectPieceInfo(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	return (::GetSolidObjectPieceInfoHelper(L, *(lmp->original)));
}

static int GetSolidObjectPiecePosition(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	const float3 pos = lmp->GetAbsolutePos();

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}

static int GetSolidObjectPieceDirection(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	const float3 dir = lmp->GetDirection();

	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 3;
}

static int GetSolidObjectPiecePosDir(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	float3 dir;
	float3 pos;
	lmp->GetEmitDirPos(pos, dir);

	// transform to object's space
	pos = o->GetObjectSpacePos(pos);
	dir = o->GetObjectSpaceVec(dir);

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 6;
}

static int GetSolidObjectPieceMatrix(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	const CMatrix44f& mat = lmp->GetModelSpaceMatrix();

	for (float mi: mat.m) {
		lua_pushnumber(L, mi);
	}

	return 16;
}

/***
 *
 * @function Spring.GetModelRootPiece
 * @param modelName string
 * @return integer index of the root piece
 */
int LuaSyncedRead::GetModelRootPiece(lua_State* L) {
	return ::GetModelRootPiece(L, luaL_optsstring(L, 1, ""));
}

/***
 *
 * @function Spring.GetModelPieceMap
 * @param modelName string
 * @return table<string,integer>? pieceInfos where keys are piece names and values are indices
 */
int LuaSyncedRead::GetModelPieceMap(lua_State* L) {
	return ::GetModelPieceMap(L, luaL_optsstring(L, 1, ""));
}


/***
 *
 * @function Spring.GetModelPieceList
 * @param modelName string
 * @return string[]? pieceNames
 */
int LuaSyncedRead::GetModelPieceList(lua_State* L) {
	return ::GetModelPieceList(L, luaL_optsstring(L, 1, ""));
}


/***
 *
 * @function Spring.GetUnitRootPiece
 * @param unitID UnitID
 * @return integer index of the root piece
 */
int LuaSyncedRead::GetUnitRootPiece(lua_State* L) {
	return (GetSolidObjectRootPiece(L, ParseTypedUnit(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetUnitPieceMap
 * @param unitID UnitID
 * @return table<string,integer>? pieceInfos where keys are piece names and values are indices
 */
int LuaSyncedRead::GetUnitPieceMap(lua_State* L) {
	return (GetSolidObjectPieceMap(L, ParseTypedUnit(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetUnitPieceList
 * @param unitID UnitID
 * @return string[] pieceNames
 */
int LuaSyncedRead::GetUnitPieceList(lua_State* L) {
	return (GetSolidObjectPieceList(L, ParseTypedUnit(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetUnitPieceInfo
 * @param unitID UnitID
 * @param pieceIndex integer
 * @return PieceInfo? pieceInfo
 */
int LuaSyncedRead::GetUnitPieceInfo(lua_State* L) {
	return (GetSolidObjectPieceInfo(L, ParseTypedUnit(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetUnitPiecePosDir
 * @param unitID UnitID
 * @param pieceIndex integer
 * @return number? posX
 * @return number     posY
 * @return number     posZ
 * @return number     dirX
 * @return number     dirY
 * @return number     dirZ
 */
int LuaSyncedRead::GetUnitPiecePosDir(lua_State* L) {
	return (GetSolidObjectPiecePosDir(L, ParseTypedUnit(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetUnitPiecePosition
 * @param unitID UnitID
 * @param pieceIndex integer
 * @return number? posX
 * @return number     posY
 * @return number     posZ
 */
int LuaSyncedRead::GetUnitPiecePosition(lua_State* L) {
	return (GetSolidObjectPiecePosition(L, ParseTypedUnit(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetUnitPieceDirection
 * @param unitID UnitID
 * @param pieceIndex integer
 * @return number? dirX
 * @return number     dirY
 * @return number     dirZ
 */
int LuaSyncedRead::GetUnitPieceDirection(lua_State* L) {
	return (GetSolidObjectPieceDirection(L, ParseTypedUnit(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetUnitPieceMatrix
 * @param unitID UnitID
 * @param pieceIndex integer
 * @return number? m11
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
int LuaSyncedRead::GetUnitPieceMatrix(lua_State* L) {
	return (GetSolidObjectPieceMatrix(L, ParseTypedUnit(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetFeatureRootPiece
 * @param featureID FeatureID
 * @return integer index of the root piece
 */
int LuaSyncedRead::GetFeatureRootPiece(lua_State* L) {
	return (GetSolidObjectRootPiece(L, ParseFeature(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetFeaturePieceMap
 * @param featureID FeatureID
 * @return table<string,integer> pieceInfos where keys are piece names and values are indices
 */
int LuaSyncedRead::GetFeaturePieceMap(lua_State* L) {
	return (GetSolidObjectPieceMap(L, ParseFeature(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetFeaturePieceList
 * @param featureID FeatureID
 * @return string[] pieceNames
 */
int LuaSyncedRead::GetFeaturePieceList(lua_State* L) {
	return (GetSolidObjectPieceList(L, ParseFeature(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetFeaturePieceInfo
 * @param featureID FeatureID
 * @param pieceIndex integer
 * @return PieceInfo? pieceInfo
 */
int LuaSyncedRead::GetFeaturePieceInfo(lua_State* L) {
	return (GetSolidObjectPieceInfo(L, ParseFeature(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetFeaturePiecePosDir
 * @param featureID FeatureID
 * @param pieceIndex integer
 * @return number? posX
 * @return number     posY
 * @return number     posZ
 * @return number     dirX
 * @return number     dirY
 * @return number     dirZ
 */
int LuaSyncedRead::GetFeaturePiecePosDir(lua_State* L) {
	return (GetSolidObjectPiecePosDir(L, ParseFeature(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetFeaturePiecePosition
 * @param featureID FeatureID
 * @param pieceIndex integer
 * @return number? posX
 * @return number     posY
 * @return number     posZ
 */
int LuaSyncedRead::GetFeaturePiecePosition(lua_State* L) {
	return (GetSolidObjectPiecePosition(L, ParseFeature(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetFeaturePieceDirection
 * @param featureID FeatureID
 * @param pieceIndex integer
 * @return number? dirX
 * @return number     dirY
 * @return number     dirZ
 */
int LuaSyncedRead::GetFeaturePieceDirection(lua_State* L) {
	return (GetSolidObjectPieceDirection(L, ParseFeature(L, __func__, 1)));
}


/***
 *
 * @function Spring.GetFeaturePieceMatrix
 * @param featureID FeatureID
 * @param pieceIndex integer
 * @return number? m11
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
int LuaSyncedRead::GetFeaturePieceMatrix(lua_State* L) {
	return (GetSolidObjectPieceMatrix(L, ParseFeature(L, __func__, 1)));
}

/***
 *
 * @function Spring.GetUnitScriptPiece
 *
 * @param unitID UnitID
 * @return integer[] pieceIndices
 */
/***
 *
 * @function Spring.GetUnitScriptPiece
 *
 * @param unitID UnitID
 * @param scriptPiece integer
 * @return integer pieceIndex
 */
int LuaSyncedRead::GetUnitScriptPiece(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const CUnitScript* script = unit->script;

	if (!lua_isnumber(L, 2)) {
		// return the whole script->piece map
		lua_newtable(L);
		for (size_t sp = 0; sp < script->pieces.size(); sp++) {
			const int piece = script->ScriptToModel(sp);
			if (piece != -1) {
				lua_pushnumber(L, piece + 1);
				lua_rawseti(L, -2, sp);
			}
		}
		return 1;
	}

	const int scriptPiece = lua_toint(L, 2);
	const int piece = script->ScriptToModel(scriptPiece);
	if (piece < 0)
		return 0;

	lua_pushnumber(L, piece + 1);
	return 1;
}


/***
 *
 * @function Spring.GetUnitScriptNames
 *
 * @param unitID UnitID
 *
 * @return table<string,integer> pieceInfos where keys are piece names and values are piece indices
 */
int LuaSyncedRead::GetUnitScriptNames(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const vector<LocalModelPiece*>& pieces = unit->script->pieces;

	lua_createtable(L, pieces.size(), 0);

	for (size_t sp = 0; sp < pieces.size(); sp++) {
		lua_pushsstring(L, pieces[sp]->original->name);
		lua_pushnumber(L, sp);
		lua_rawset(L, -3);
	}

	return 1;
}


static int TraceRayImpl(lua_State *const L, const float3 &pos, const float3 &dir, const float maxLen, std::string_view type)
{
	if (type != "unit" && type != "feature" && type != "both")
		return luaL_error(L, "invalid type '%s', expected 'unit', 'feature', or 'both'", type.data());

	const bool testUnits = (type == "unit" || type == "both");
	const bool testFeatures = (type == "feature" || type == "both");

	QuadFieldQuery qfQuery;
	quadField.GetQuadsOnRay(qfQuery, pos, dir, maxLen);

	spring::unordered_set <int> testedUnitIDs;
	spring::unordered_set <int> testedFeatureIDs;
	std::vector <std::tuple<float, int, const char*>> hits;

	for (const int quadIdx : *qfQuery.quads) {
		const CQuadField::Quad& quad = quadField.GetQuad(quadIdx);

		if (testUnits) {
			for (const auto *unit : quad.units) {
				if (!unit->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				if (!testedUnitIDs.insert(unit->id).second)
					continue;

				if (!LuaUtils::IsUnitInLos(L, unit))
					continue;

				CollisionQuery cq;
				if (CCollisionHandler::DetectHit(unit, unit->GetTransformMatrix(true), pos, pos + dir * maxLen, &cq, true)) {
					const float len = cq.GetHitPosDist(pos, dir);
					if (len > maxLen) // possibly a bug in CCollisionHandler::DetectHit?
						continue;
					hits.emplace_back(len, unit->id, "unit");
				}
			}
		}

		if (testFeatures) {
			for (const auto *feature : quad.features) {
				if (!feature->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				if (!testedFeatureIDs.insert(feature->id).second)
					continue;

				if (!LuaUtils::IsFeatureVisible(L, feature))
					continue;

				CollisionQuery cq;
				if (CCollisionHandler::DetectHit(feature, feature->GetTransformMatrix(true), pos, pos + dir * maxLen, &cq, true)) {
					const float len = cq.GetHitPosDist(pos, dir);
					if (len > maxLen)
						continue;
					hits.emplace_back(len, feature->id, "feature");
				}
			}
		}
	}

	std::stable_sort(hits.begin(), hits.end(), [] (const auto& a, const auto& b) {
		return std::get<0>(a) < std::get<0>(b);
	});

	lua_createtable(L, hits.size(), 0);

	int num = 0;
	for (const auto& [hitLength, objectID, objectType] : hits) {
		lua_createtable(L, 3, 0);

		lua_pushnumber(L, hitLength);
		lua_rawseti(L, -2, 1);
		lua_pushnumber(L, objectID);
		lua_rawseti(L, -2, 2);
		lua_pushstring(L, objectType);
		lua_rawseti(L, -2, 3);

		lua_rawseti(L, -2, ++num);
	}

	return 1;
}

/*** Traces a ray from a position in a direction
 *
 * @function Spring.TraceRayInDirection
 *
 * Returns all unit and/or feature hits along a ray, sorted by distance
 * from the start position.
 *
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param dirX number
 * @param dirY number
 * @param dirZ number
 * @param maxLength number
 * @param type string Object type to test: `"unit"`, `"feature"`, or `"both"`
 * @return table[] hits Array of `{hitLength, objectID, objectType}` entries
 */
int LuaSyncedRead::TraceRayInDirection(lua_State* L)
{
	float3 pos(luaL_checkfloat(L, 1), luaL_checkfloat(L, 2), luaL_checkfloat(L, 3));
	float3 dir(luaL_checkfloat(L, 4), luaL_checkfloat(L, 5), luaL_checkfloat(L, 6));
	const float maxLen = luaL_optfloat(L, 7, 999999.f);
	const char* type = luaL_checkstring(L, 8);
	return TraceRayImpl(L, pos, dir, maxLen, type);
}

/*** Traces a ray between two positions
 *
 * @function Spring.TraceRayBetweenPositions
 *
 * Checks for unit and/or feature collisions between two positions
 * and returns all hits sorted by distance from the start position.
 *
 * @param startX number
 * @param startY number
 * @param startZ number
 * @param endX number
 * @param endY number
 * @param endZ number
 * @param type string Object type to test: `"unit"`, `"feature"`, or `"both"`
 * @return table[] hits Array of `{hitLength, objectID, objectType}` entries
 */
int LuaSyncedRead::TraceRayBetweenPositions(lua_State* L)
{
	float3 start(luaL_checkfloat(L, 1), luaL_checkfloat(L, 2), luaL_checkfloat(L, 3));
	float3   end(luaL_checkfloat(L, 4), luaL_checkfloat(L, 5), luaL_checkfloat(L, 6));
	const char* type = luaL_checkstring(L, 7);
	const auto [dir, length] = (end - start).GetNormalized();
	return TraceRayImpl(L, start, dir, length, type);
}

static int TraceRayGroundImpl(lua_State *const L, const float3 &pos, const float3 &dir, const float maxLen, const bool testWater)
{
	const float rayLength = CGround::LineGroundWaterCol(pos, dir, maxLen, testWater, CLuaHandle::GetHandleSynced(L));
	if (rayLength == -1.0f)
		return 0;

	const auto collisionSpot = pos + dir * rayLength; // FIXME: would be nice if the CGround:: functions returned this so we wouldn't have to recalculate

	lua_pushnumber(L, rayLength);
	lua_pushnumber(L, collisionSpot.x);
	lua_pushnumber(L, collisionSpot.y);
	lua_pushnumber(L, collisionSpot.z);
	return 4;
}

/*** Checks for a ground collision in given direction
 *
 * @function Spring.TraceRayGroundInDirection
 *
 * Checks if there is surface (ground, optionally water) towards a vector
 * and returns the distance to the closest hit and its position, if any.
 *
 * @param posX number
 * @param posY number
 * @param posZ number
 * @param dirX number
 * @param dirY number
 * @param dirZ number
 * @param maxLength number? (Default: `999999`)
 * @param testWater boolean? (Default: `true`)
 * @return number? rayLength
 * @return number? posX
 * @return number? posY
 * @return number? posZ
 */
int LuaSyncedRead::TraceRayGroundInDirection(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1), luaL_checkfloat(L, 2), luaL_checkfloat(L, 3));
	const auto dir = float3(luaL_checkfloat(L, 4), luaL_checkfloat(L, 5), luaL_checkfloat(L, 6)).Normalize();
	const float maxLen = luaL_optfloat(L, 7, 999999.f);
	const bool testWater = luaL_optboolean(L, 8, true);

	return TraceRayGroundImpl(L, pos, dir, maxLen, testWater);
}

/*** Checks for a ground collision between two positions
 *
 * @function Spring.TraceRayGroundBetweenPositions
 *
 * Checks if there is surface (ground, optionally water) between two positions
 * and returns the distance to the closest hit and its position, if any.
 *
 * @param startX number
 * @param startY number
 * @param startZ number
 * @param endX number
 * @param endY number
 * @param endZ number
 * @param testWater boolean? (Default: `true`)
 * @return number rayLength
 * @return number posX
 * @return number posY
 * @return number posZ
 */
int LuaSyncedRead::TraceRayGroundBetweenPositions(lua_State* L)
{
	const float3 start (luaL_checkfloat(L, 1), luaL_checkfloat(L, 2), luaL_checkfloat(L, 3));
	const float3 end (luaL_checkfloat(L, 4), luaL_checkfloat(L, 5), luaL_checkfloat(L, 6));
	const bool testWater = luaL_optboolean(L, 7, true);

	const auto [dir, length] = (end - start).GetNormalized();

	return TraceRayGroundImpl(L, start, dir, length, testWater);
}


/******************************************************************************
 * Misc
 *
 * @section misc
******************************************************************************/


/***
 *
 * @function Spring.GetRadarErrorParams
 *
 * @param allyTeamID AllyTeamID
 *
 * @return number? radarErrorSize actual radar error size (when allyTeamID is allied to current team) or base radar error size
 * @return number baseRadarErrorSize
 * @return number baseRadarErrorMult
 */
int LuaSyncedRead::GetRadarErrorParams(lua_State* L)
{
	const int allyTeamID = lua_tonumber(L, 1);

	if (!teamHandler.IsValidAllyTeam(allyTeamID))
		return 0;

	if (LuaUtils::IsAlliedAllyTeam(L, allyTeamID)) {
		lua_pushnumber(L, losHandler->GetAllyTeamRadarErrorSize(allyTeamID));
	} else {
		lua_pushnumber(L, losHandler->GetBaseRadarErrorSize());
	}
	lua_pushnumber(L, losHandler->GetBaseRadarErrorSize());
	lua_pushnumber(L, losHandler->GetBaseRadarErrorMult());
	return 3;
}


/******************************************************************************/
/******************************************************************************/
