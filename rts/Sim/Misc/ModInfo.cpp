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

	{
		// system
		const LuaTable& system = root.SubTable("system");

		pathFinderSystem = system.GetInt("pathFinderSystem", pathFinderSystem);
		pfRawDistMult = system.GetFloat("pathFinderRawDistMult", pfRawDistMult);
		pfUpdateRateScale = system.GetFloat("pathFinderUpdateRateScale", pfUpdateRateScale);
		pfRepathDelayInFrames = system.GetInt("pfRepathDelayInFrames", pfRepathDelayInFrames);
		pfRepathMaxRateInFrames = system.GetInt("pfRepathMaxRateInFrames", pfRepathMaxRateInFrames);
		pfRawMoveSpeedThreshold = system.GetFloat("pfRawMoveSpeedThreshold", pfRawMoveSpeedThreshold);
		qtMaxNodesSearched = system.GetInt("qtMaxNodesSearched", qtMaxNodesSearched);
		qtRefreshPathMinDist = system.GetFloat("qtRefreshPathMinDist", qtRefreshPathMinDist);
		qtMaxNodesSearchedRelativeToMapOpenNodes = system.GetFloat("qtMaxNodesSearchedRelativeToMapOpenNodes", qtMaxNodesSearchedRelativeToMapOpenNodes);
		qtLowerQualityPaths = system.GetBool("qtLowerQualityPaths", qtLowerQualityPaths);

		enableSmoothMesh = system.GetBool("enableSmoothMesh", enableSmoothMesh);
		smoothMeshResDivider = system.GetInt("smoothMeshResDivider", smoothMeshResDivider);
		smoothMeshSmoothRadius = system.GetInt("smoothMeshSmoothRadius", smoothMeshSmoothRadius);

		quadFieldQuadSizeInElmos = system.GetInt("quadFieldQuadSizeInElmos", quadFieldQuadSizeInElmos);

		// Specify in megabytes: 1 << 20 = (1024 * 1024)
		SLuaAllocLimit::MAX_ALLOC_BYTES = static_cast<decltype(SLuaAllocLimit::MAX_ALLOC_BYTES)>(system.GetInt("LuaAllocLimit", SLuaAllocLimit::MAX_ALLOC_BYTES >> 20u)) << 20u;

		allowTake = system.GetBool("allowTake", allowTake);
		allowEnginePlayerlist = system.GetBool("allowEnginePlayerlist", allowEnginePlayerlist);
	}

	{
		// movement
		const LuaTable& movementTbl = root.SubTable("movement");

		allowAircraftToLeaveMap = movementTbl.GetBool("allowAirPlanesToLeaveMap", allowAircraftToLeaveMap);
		allowAircraftToHitGround = movementTbl.GetBool("allowAircraftToHitGround", allowAircraftToHitGround);
		allowPushingEnemyUnits = movementTbl.GetBool("allowPushingEnemyUnits", allowPushingEnemyUnits);
		allowCrushingAlliedUnits = movementTbl.GetBool("allowCrushingAlliedUnits", allowCrushingAlliedUnits);
		allowUnitCollisionDamage = movementTbl.GetBool("allowUnitCollisionDamage", allowUnitCollisionDamage);
		allowUnitCollisionOverlap = movementTbl.GetBool("allowUnitCollisionOverlap", allowUnitCollisionOverlap);
		allowSepAxisCollisionTest = movementTbl.GetBool("allowSepAxisCollisionTest", allowSepAxisCollisionTest);
		allowGroundUnitGravity = movementTbl.GetBool("allowGroundUnitGravity", allowGroundUnitGravity);
		allowHoverUnitStrafing = movementTbl.GetBool("allowHoverUnitStrafing", allowHoverUnitStrafing);
		maxCollisionPushMultiplier = movementTbl.GetFloat("maxCollisionPushMultiplier", maxCollisionPushMultiplier);
		unitQuadPositionUpdateRate = movementTbl.GetInt("unitQuadPositionUpdateRate",  unitQuadPositionUpdateRate);
		groundUnitCollisionAvoidanceUpdateRate = movementTbl.GetInt("groundUnitCollisionAvoidanceUpdateRate",  groundUnitCollisionAvoidanceUpdateRate);

	}

	{
		// construction
		const LuaTable& constructionTbl = root.SubTable("construction");

		constructionDecay = constructionTbl.GetBool("constructionDecay", constructionDecay);
		constructionDecayTime = (int)(constructionTbl.GetFloat("constructionDecayTime", (float)constructionDecayTime / GAME_SPEED) * GAME_SPEED);
		constructionDecaySpeed = constructionTbl.GetFloat("constructionDecaySpeed", constructionDecaySpeed);
		insertBuiltUnitMoveCommand = constructionTbl.GetBool("insertBuiltUnitMoveCommand", insertBuiltUnitMoveCommand);
	}

	{
		const LuaTable& damageTbl = root.SubTable("damage");

		debrisDamage = damageTbl.GetFloat("debris", debrisDamage);
	}
	{
		// reclaim
		const LuaTable& reclaimTbl = root.SubTable("reclaim");

		multiReclaim  = reclaimTbl.GetInt("multiReclaim",  multiReclaim);
		reclaimMethod = reclaimTbl.GetInt("reclaimMethod", reclaimMethod);
		reclaimUnitMethod = reclaimTbl.GetInt("unitMethod", reclaimUnitMethod);
		reclaimUnitEnergyCostFactor = reclaimTbl.GetFloat("unitEnergyCostFactor", reclaimUnitEnergyCostFactor);
		reclaimUnitEfficiency = reclaimTbl.GetFloat("unitEfficiency", reclaimUnitEfficiency);
		reclaimFeatureEnergyCostFactor = reclaimTbl.GetFloat("featureEnergyCostFactor", reclaimFeatureEnergyCostFactor);
		reclaimUnitDrainHealth = reclaimTbl.GetBool("unitDrainHealth", reclaimUnitDrainHealth);
		reclaimAllowEnemies = reclaimTbl.GetBool("allowEnemies", reclaimAllowEnemies);
		reclaimAllowAllies = reclaimTbl.GetBool("allowAllies", reclaimAllowAllies);
	}

	{
		// repair
		const LuaTable& repairTbl = root.SubTable("repair");
		repairEnergyCostFactor = repairTbl.GetFloat("energyCostFactor", repairEnergyCostFactor);
	}

	{
		// resurrect
		const LuaTable& resurrectTbl = root.SubTable("resurrect");
		resurrectEnergyCostFactor  = resurrectTbl.GetFloat("energyCostFactor", resurrectEnergyCostFactor);
	}

	{
		// capture
		const LuaTable& captureTbl = root.SubTable("capture");
		captureEnergyCostFactor = captureTbl.GetFloat("energyCostFactor", captureEnergyCostFactor);
	}

	{
		// paralyze
		const LuaTable& paralyzeTbl = root.SubTable("paralyze");
		paralyzeDeclineRate = paralyzeTbl.GetFloat("paralyzeDeclineRate", paralyzeDeclineRate);
		paralyzeOnMaxHealth = paralyzeTbl.GetBool("paralyzeOnMaxHealth", paralyzeOnMaxHealth);
	}

	{
		// fire-at-dead-units
		const LuaTable& fireAtDeadTbl = root.SubTable("fireAtDead");

		fireAtKilled   = fireAtDeadTbl.GetBool("fireAtKilled", fireAtKilled);
		fireAtCrashing = fireAtDeadTbl.GetBool("fireAtCrashing", fireAtCrashing);
	}

	{
		// transportability
		const LuaTable& transportTbl = root.SubTable("transportability");

		transportAir    = transportTbl.GetBool("transportAir",    transportAir   );
		transportShip   = transportTbl.GetBool("transportShip",   transportShip  );
		transportHover  = transportTbl.GetBool("transportHover",  transportHover );
		transportGround = transportTbl.GetBool("transportGround", transportGround);

		targetableTransportedUnits = transportTbl.GetBool("targetableTransportedUnits", targetableTransportedUnits);
	}

	{
		// experience
		const LuaTable& experienceTbl = root.SubTable("experience");

		unitExpMultiplier  = experienceTbl.GetFloat( "experienceMult", unitExpMultiplier);
		unitExpPowerScale  = experienceTbl.GetFloat(     "powerScale", unitExpPowerScale);
		unitExpHealthScale = experienceTbl.GetFloat(    "healthScale", unitExpHealthScale);
		unitExpReloadScale = experienceTbl.GetFloat(    "reloadScale", unitExpReloadScale);
		unitExpGrade       = experienceTbl.GetFloat("experienceGrade", unitExpGrade);
	}

	{
		// flanking bonus
		const LuaTable& flankingBonusTbl = root.SubTable("flankingBonus");
		flankingBonusModeDefault = flankingBonusTbl.GetInt("defaultMode", flankingBonusModeDefault);
		flankingBonusMaxDefault = flankingBonusTbl.GetFloat("defaultMax", flankingBonusMaxDefault);
		flankingBonusMinDefault = flankingBonusTbl.GetFloat("defaultMin", flankingBonusMinDefault);
	}

	{
		// feature visibility
		const LuaTable& featureLOS = root.SubTable("featureLOS");

		featureVisibility = featureLOS.GetInt("featureVisibility", featureVisibility);
	}

	{
		// sensors, line-of-sight
		const LuaTable& sensors = root.SubTable("sensors");
		const LuaTable& los = sensors.SubTable("los");

		requireSonarUnderWater = sensors.GetBool("requireSonarUnderWater", requireSonarUnderWater);
		alwaysVisibleOverridesCloaked = sensors.GetBool("alwaysVisibleOverridesCloaked", alwaysVisibleOverridesCloaked);
		decloakRequiresLineOfSight = sensors.GetBool("decloakRequiresLineOfSight", decloakRequiresLineOfSight);
		separateJammers = sensors.GetBool("separateJammers", separateJammers);

		losMipLevel = los.GetInt("losMipLevel", losMipLevel);
		airMipLevel = los.GetInt("airMipLevel", airMipLevel);
		radarMipLevel = los.GetInt("radarMipLevel", radarMipLevel);

	}
	{
		//misc
		const LuaTable& misc = root.SubTable("misc");

		windChangeReportPeriod = static_cast<int>(math::roundf(misc.GetFloat("windChangeReportPeriod", static_cast<float>(windChangeReportPeriod) / GAME_SPEED) * GAME_SPEED));
	}

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

