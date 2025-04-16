/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ModInfo.h"

#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "Lua/LuaAllocState.h"
#include "Map/ReadMap.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"

#include "lib/fmt/format.h"

#include <bit>

CModInfo modInfo;

void CModInfo::ResetState()
{
	filename.clear();
	humanName.clear();
	humanNameVersioned.clear();
	shortName.clear();
	version.clear();
	mutator.clear();
	description.clear();

	{
		allowAircraftToLeaveMap    = true;
		allowAircraftToHitGround   = true;
		allowPushingEnemyUnits     = false;
		allowCrushingAlliedUnits   = false;
		allowUnitCollisionDamage   = false;
		allowUnitCollisionOverlap  = true;
		allowSepAxisCollisionTest  = false;
		allowGroundUnitGravity     = false;
		allowHoverUnitStrafing     = false;

		maxCollisionPushMultiplier = std::numeric_limits<float>::infinity();
		unitQuadPositionUpdateRate = 3;
		groundUnitCollisionAvoidanceUpdateRate = 3;
	}
	{
		constructionDecay      = true;
		constructionDecayTime  = int(6.66 * GAME_SPEED);
		constructionDecaySpeed = 0.03f;
		insertBuiltUnitMoveCommand = true;
	}
	{
		debrisDamage = 50.0f;
	}
	{
		multiReclaim                   = 0;
		reclaimMethod                  = 1;
		reclaimUnitMethod              = 1;
		reclaimUnitEnergyCostFactor    = 0.0f;
		reclaimUnitEfficiency          = 1.0f;
		reclaimFeatureEnergyCostFactor = 0.0f;
		reclaimUnitDrainHealth         = true;
		reclaimAllowEnemies            = true;
		reclaimAllowAllies             = true;
	}
	{
		repairEnergyCostFactor    = 0.0f;
		resurrectEnergyCostFactor = 0.5f;
		captureEnergyCostFactor   = 0.0f;
	}
	{
		unitExpMultiplier  = 1.0f;
		unitExpPowerScale  = 1.0f;
		unitExpHealthScale = 0.7f;
		unitExpReloadScale = 0.4f;
		unitExpGrade       = 0.0f;
	}
	{
		paralyzeDeclineRate = 40.0f;
		paralyzeOnMaxHealth = true;
	}
	{
		transportGround = true;
		transportHover  = false;
		transportShip   = false;
		transportAir    = false;
		targetableTransportedUnits = 0;
	}
	{
		fireAtKilled   = false;
		fireAtCrashing = false;
	}
	{
		flankingBonusModeDefault = 1;
		flankingBonusMaxDefault = 1.9f;
		flankingBonusMinDefault = 0.9f;
	}
	{
		losMipLevel = 1;
		airMipLevel = 1;
		radarMipLevel = 2;

		requireSonarUnderWater = true;
		alwaysVisibleOverridesCloaked = false;
		decloakRequiresLineOfSight = false;
		separateJammers = true;
	}
	{
		featureVisibility = FEATURELOS_ALL;
	}
	{
		pathFinderSystem = HAPFS_TYPE;
		pfRawDistMult    = 1.25f;
		pfUpdateRateScale = 1.f;
		pfRepathDelayInFrames = 60;
		pfRepathMaxRateInFrames = 150;
		pfRawMoveSpeedThreshold = 0.f;
		qtMaxNodesSearched = 8192;
		qtRefreshPathMinDist = 512.f;
		qtMaxNodesSearchedRelativeToMapOpenNodes = 0.25;
		qtLowerQualityPaths = false;

		enableSmoothMesh = true;
		smoothMeshResDivider = 2;
		smoothMeshSmoothRadius = 40;
		quadFieldQuadSizeInElmos = 128;

		SLuaAllocLimit::MAX_ALLOC_BYTES = SLuaAllocLimit::MAX_ALLOC_BYTES_DEFAULT;

		allowTake = true;

		allowEnginePlayerlist = true;
	}
	{
		// make windChangeReportPeriod equal to EnvResourceHandler::WIND_UPDATE_RATE = 15 * GAME_SPEED;
		windChangeReportPeriod = 15 * GAME_SPEED;
	}
}

void CModInfo::Init(const std::string& modFileName)
{
	{
		filename = modFileName;
		humanNameVersioned = archiveScanner->GameHumanNameFromArchive(modFileName);

		const CArchiveScanner::ArchiveData& md = archiveScanner->GetArchiveData(humanNameVersioned);

		humanName   = md.GetName();
		shortName   = md.GetShortName();
		version     = md.GetVersion();
		mutator     = md.GetMutator();
		description = md.GetDescription();
	}

	LuaParser parser("gamedata/modrules.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	// customize the defs environment
	parser.GetTable("Spring");
	parser.AddFunc("GetModOptions", LuaSyncedRead::GetModOptions);
	parser.EndTable();
	parser.Execute();

	if (!parser.IsValid())
		LOG_L(L_ERROR, "[ModInfo::%s] error \"%s\" loading mod-rules, using defaults", __func__, parser.GetErrorLog().c_str());

	const LuaTable& root = parser.GetRoot();

	auto Get = [&root] (auto &var, const std::optional <std::string> & flatName, const std::optional <std::string> & legacyName) {
		assert(legacyName || flatName);
		if (legacyName)
			var = root.Get(*legacyName, var);
		if (flatName)
			var = root.Get(  *flatName, var);
	};

	// Some values have a unit conversion going on.
	int LuaAllocLimitMB = SLuaAllocLimit::MAX_ALLOC_BYTES >> 20u; // NB the limit doesnt apply to parsing modrules itself
	float constructionDecayTimeSeconds = float(constructionDecayTime) / GAME_SPEED;
	bool isReclaimContinuous = (root.GetInt("reclaim.reclaimMethod", reclaimMethod) == 0);
	bool isUnitReclaimContinuous = (root.GetInt("reclaim.unitMethod", reclaimUnitMethod) == 0);

	//      C++ name                          Lua name                       legacy Lua name in a pointless subtable
	Get (debrisDamage                  , "debrisDamage"                 , std::nullopt                                      );
	Get (allowEnginePlayerlist         , "allowEnginePlayerlist"        , std::nullopt                                      );
	Get (paralyzeDeclineRate           , "paralyzeDeclineRate"          ,         "paralyze.paralyzeDeclineRate"            );
	Get (paralyzeOnMaxHealth           , "paralyzeOnMaxHealth"          ,         "paralyze.paralyzeOnMaxHealth"            );
	Get (targetableTransportedUnits    , "targetableTransportedUnits"   , "transportability.targetableTransportedUnits"     );
	Get (fireAtKilled                  , "fireAtKilled"                 ,       "fireAtDead.fireAtKilled"                   );
	Get (fireAtCrashing                , "fireAtCrashing"               ,       "fireAtDead.fireAtCrashing"                 );
	Get (featureVisibility             , "featureVisibility"            ,       "featureLOS.featureVisibility"              );
	Get (unitExpMultiplier             , "unitExpGainMult"              ,       "experience.experienceMult"                 );
	Get (unitExpPowerScale             , "unitExpPowerScale"            ,       "experience.powerScale"                     );
	Get (unitExpHealthScale            , "unitExpHealthScale"           ,       "experience.healthScale"                    );
	Get (unitExpReloadScale            , "unitExpReloadScale"           ,       "experience.reloadScale"                    );
	Get (unitExpGrade                  , "unitExpCallinRate"            ,       "experience.grade"                          );
	Get (separateJammers               , "separateJammers"              ,          "sensors.separateJammers"                );
	Get (requireSonarUnderWater        , "requireSonarUnderWater"       ,          "sensors.requireSonarUnderWater"         );
	Get (decloakRequiresLineOfSight    , "decloakRequiresLineOfSight"   ,          "sensors.decloakRequiresLineOfSight"     );
	Get (alwaysVisibleOverridesCloaked , "alwaysVisibleOverridesCloaked",          "sensors.alwaysVisibleOverridesCloaked"  );
	Get (  losMipLevel                 , "losMipLevel"                  ,          "sensors.los.losMipLevel"                );
	Get (  airMipLevel                 , "airMipLevel"                  ,          "sensors.los.airMipLevel"                );
	Get (radarMipLevel                 , "radarMipLevel"                ,          "sensors.los.radarMipLevel"              );
	Get (LuaAllocLimitMB               , "LuaAllocLimitMB"              ,           "system.luaAllocLimit"                  );
	Get (allowTake                     , "allowTake"                    ,           "system.allowTake"                      );
	Get (enableSmoothMesh              , "enableSmoothMesh"             ,           "system.enableSmoothMesh"               );
	Get (allowAircraftToLeaveMap       , "allowAirPlanesToLeaveMap"     ,         "movement.allowAirPlanesToLeaveMap"       );
	Get (allowAircraftToHitGround      , "allowAircraftToHitGround"     ,         "movement.allowAircraftToHitGround"       );
	Get (allowPushingEnemyUnits        , "allowPushingEnemyUnits"       ,         "movement.allowPushingEnemyUnits"         );
	Get (allowCrushingAlliedUnits      , "allowCrushingAlliedUnits"     ,         "movement.allowCrushingAlliedUnits"       );
	Get (allowUnitCollisionDamage      , "allowUnitCollisionDamage"     ,         "movement.allowUnitCollisionDamage"       );
	Get (allowUnitCollisionOverlap     , "allowUnitCollisionOverlap"    ,         "movement.allowUnitCollisionOverlap"      );
	Get (allowGroundUnitGravity        , "allowGroundUnitGravity"       ,         "movement.allowGroundUnitGravity"         );
	Get (allowHoverUnitStrafing        , "allowHoverUnitStrafing"       ,         "movement.allowHoverUnitStrafing"         );
	Get (allowSepAxisCollisionTest     , "preciseCollisionChecks"       ,         "movement.allowSepAxisCollisionTest"      );
	Get (maxCollisionPushMultiplier    , "maxCollisionPushMultiplier"   ,         "movement.maxCollisionPushMultiplier"     );
	Get (unitQuadPositionUpdateRate    , "unitQuadPositionUpdateRate"   ,         "movement.unitQuadPositionUpdateRate"     );
	Get (pfUpdateRateScale             , "pathFinderUpdateRateScale"    ,         "movement.pathFinderUpdateRateScale"      );
	Get (pfRawDistMult                 , "pathFinderRawDistMult"        ,         "movement.pathFinderRawDistMult"          );
	Get (qtLowerQualityPaths           , "qtpfsLowerQualityPaths"       ,         "movement.qtLowerQualityPaths"            );
	Get (pfRepathDelayInFrames         , "repathDelayInFrames"          ,           "system.pfRepathDelayInFrames"          );
	Get (pfRepathMaxRateInFrames       , "repathMaxRateInFrames"        ,           "system.pfRepathMaxRateInFrames"        );
	Get (pfRawMoveSpeedThreshold       , "rawMoveSpeedThreshold"        ,           "system.pfRawMoveSpeedThreshold"        );
	Get (qtMaxNodesSearched            , "qtpfsMaxNodesSearched"        ,           "system.qtMaxNodesSearched"             );
	Get (qtRefreshPathMinDist          , "qtpfsRefreshPathMinDist"      ,           "system.qtRefreshPathMinDist"           );
	Get (quadFieldQuadSizeInElmos      , "qtpfsQuadSizeElmos"           ,           "system.quadFieldQuadSizeInElmos"       );
	Get (reclaimAllowEnemies           , "allowReclaimLiveEnemies"      ,          "reclaim.allowEnemies"                   );
	Get (reclaimAllowAllies            , "allowReclaimLiveAllies"       ,          "reclaim.allowAllies"                    );
	Get (reclaimUnitDrainHealth        , "reclaimUnitDrainHealth"       ,          "reclaim.unitDrainHealth"                );
	Get (multiReclaim                  , "multiReclaim"                 ,          "reclaim.multiReclaim"                   );
	Get (constructionDecay             , "constructionDecayEnabled"     ,     "construction.constructionDecay"              );
	Get (constructionDecaySpeed        , "constructionDecaySpeed"       ,     "construction.constructionDecaySpeed"         );
	Get (constructionDecayTimeSeconds  , "constructionDecayTime"        ,     "construction.constructionDecayTime"          );
	Get (windChangeReportPeriod        , "windChangeReportPeriod"       ,             "misc.windChangeReportPeriod"         );
	Get (isReclaimContinuous           , "isReclaimContinuous"          , std::nullopt /* cf reclaim.reclaimMethod above */ );
	Get (isUnitReclaimContinuous       , "isUnitReclaimContinuous"      , std::nullopt /* cf reclaim.unitMethod    above */ );

	// should be a string
	Get (pathFinderSystem              , std::nullopt                   ,           "system.pathFinderSystem"               );

	// legacy-only until resource generalisation work is complete
	Get (        repairEnergyCostFactor, std::nullopt                   ,           "repair.energyCostFactor"               );
	Get (     resurrectEnergyCostFactor, std::nullopt                   ,        "resurrect.energyCostFactor"               );
	Get (       captureEnergyCostFactor, std::nullopt                   ,          "capture.energyCostFactor"               );
	Get (reclaimFeatureEnergyCostFactor, std::nullopt                   ,          "reclaim.featureEnergyCostFactor"        );
	Get (   reclaimUnitEnergyCostFactor, std::nullopt                   ,          "reclaim.unitEnergyCostFactor"           );
	Get (   reclaimUnitEfficiency      , std::nullopt                   ,          "reclaim.unitEfficiency"                 );

	// defs post-processing. Legacy-only, use unitdefs_post.lua instead
	Get (flankingBonusModeDefault      , std::nullopt                   ,    "flankingBonus.defaultMode"                    );
	Get (flankingBonusMaxDefault       , std::nullopt                   ,    "flankingBonus.defaultMax"                     );
	Get (flankingBonusMinDefault       , std::nullopt                   ,    "flankingBonus.defaultMin "                    );
	Get (transportAir                  , std::nullopt                   , "transportability.transportAir"                   );
	Get (transportShip                 , std::nullopt                   , "transportability.transportShip"                  );
	Get (transportHover                , std::nullopt                   , "transportability.transportHover"                 );
	Get (transportGround               , std::nullopt                   , "transportability.transportGround"                );

	Get(groundUnitCollisionAvoidanceUpdateRate  , "groundUnitCollisionAvoidanceUpdateRate"     , "movement.groundUnitCollisionAvoidanceUpdateRate"   );
	Get(qtMaxNodesSearchedRelativeToMapOpenNodes, "qtpfsMaxNodesSearchedRelativeToMapOpenNodes",   "system.qtMaxNodesSearchedRelativeToMapOpenNodes" );

	// basic post processing
	SLuaAllocLimit::MAX_ALLOC_BYTES = LuaAllocLimitMB << 20u;
	constructionDecayTime = constructionDecayTimeSeconds * GAME_SPEED;
	reclaimMethod = isReclaimContinuous ? 0 : 1;
	reclaimUnitMethod = isUnitReclaimContinuous ? 0 : 1;

	// Hard checks
	static constexpr int MAX_HEIGHT_BASED_MIP_LEVEL = CReadMap::numHeightMipMaps - 1;
	if ((losMipLevel < 0) || (losMipLevel > MAX_HEIGHT_BASED_MIP_LEVEL))
		throw content_error(fmt::format("Sensors\\Los\\LosMipLevel out of bounds (integer 0-{})", MAX_HEIGHT_BASED_MIP_LEVEL));

	if ((radarMipLevel < 0) || (radarMipLevel > MAX_HEIGHT_BASED_MIP_LEVEL))
		throw content_error(fmt::format("Sensors\\Los\\RadarMipLevel out of bounds (integer 0-{})", MAX_HEIGHT_BASED_MIP_LEVEL));

	static constexpr int MAX_AIR_MIP_LEVEL = 30; // no logical limit, but it's used in various bit-shifts
	if ((airMipLevel < 0) || (airMipLevel > MAX_AIR_MIP_LEVEL))
		throw content_error(fmt::format("Sensors\\Los\\AirLosMipLevel out of bounds (integer 0-{})", MAX_AIR_MIP_LEVEL));

	if (!std::has_single_bit <unsigned> (quadFieldQuadSizeInElmos))
		throw content_error("quadFieldQuadSizeInElmos modrule has to be a power of 2");

	// Soft constraints that should really be hard ones
	pathFinderSystem = std::clamp(pathFinderSystem, int(NOPFS_TYPE), int(PFS_TYPE_MAX));
	featureVisibility = std::clamp(featureVisibility, int(FEATURELOS_NONE), int(FEATURELOS_ALL));

	// Soft constraints                                                                               min     max
	constructionDecaySpeed                   = std::max  (constructionDecaySpeed                  ,    0.01f      );
	groundUnitCollisionAvoidanceUpdateRate   = std::clamp(groundUnitCollisionAvoidanceUpdateRate  ,    1    ,   15);
	pfRawMoveSpeedThreshold                  = std::max  (pfRawMoveSpeedThreshold                 ,    0.0f       );
	pfRepathDelayInFrames                    = std::clamp(pfRepathDelayInFrames                   ,    0    ,  300);
	pfRepathMaxRateInFrames                  = std::clamp(pfRepathMaxRateInFrames                 ,    0    , 3600);
	qtMaxNodesSearched                       = std::max  (qtMaxNodesSearched                      , 1024          );
	qtMaxNodesSearchedRelativeToMapOpenNodes = std::max  (qtMaxNodesSearchedRelativeToMapOpenNodes,    0.0f       );
	qtRefreshPathMinDist                     = std::max  (qtRefreshPathMinDist                    ,    0.0f       );
	quadFieldQuadSizeInElmos                 = std::clamp(quadFieldQuadSizeInElmos                ,    8    , 1024);
	smoothMeshResDivider                     = std::max  (smoothMeshResDivider                    ,    1          );
	smoothMeshSmoothRadius                   = std::max  (smoothMeshSmoothRadius                  ,    1          );
	unitQuadPositionUpdateRate               = std::clamp(unitQuadPositionUpdateRate              ,    1    ,   15);
}

