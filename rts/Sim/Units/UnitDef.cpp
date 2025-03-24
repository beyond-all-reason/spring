/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDef.h"
#include "UnitDefHandler.h"
#include "Game/GameSetup.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Rendering/IconHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"

#include "System/Misc/TracyDefs.h"

/******************************************************************************/

UnitDefWeapon::UnitDefWeapon(const WeaponDef* weaponDef) {
	*this = UnitDefWeapon();
	this->def = weaponDef;
}

UnitDefWeapon::UnitDefWeapon(const WeaponDef* weaponDef, const LuaTable& weaponTable) {
	*this = UnitDefWeapon();
	this->def = weaponDef;

	this->slavedTo = weaponTable.GetInt("slaveTo", 0);

	// NOTE:
	//     <maxAngleDif> specifies the full-width arc,
	//     but we want the half-width arc internally
	//     (arcs are always symmetric around mainDir)
	this->maxMainDirAngleDif = math::cos((weaponTable.GetFloat("maxAngleDif", 360.0f) * 0.5f) * math::DEG_TO_RAD);

	const string& btcString = weaponTable.GetString("badTargetCategory", "");
	const string& otcString = weaponTable.GetString("onlyTargetCategory", "");

	this->badTargetCat =                                   CCategoryHandler::Instance()->GetCategories(btcString);
	this->onlyTargetCat = (otcString.empty())? 0xffffffff: CCategoryHandler::Instance()->GetCategories(otcString);

	this->mainDir = weaponTable.GetFloat3("mainDir", FwdVector);
	this->mainDir.SafeNormalize();

	// multiplier weight applied to target selection based on how far the weapon has to turn to face the target.
	weaponAimAdjustPriority = weaponTable.GetFloat("weaponAimAdjustPriority", weaponAimAdjustPriority);

	// allow weapon to select a new target immediately after the current target is destroyed, without waiting for slow update.
	fastAutoRetargeting = weaponTable.GetBool("fastAutoRetargeting", fastAutoRetargeting);

	// allow weapon to swap muzzles every frame and accurately determine friendly fire, without waiting for slow update.
	fastQueryPointUpdate = weaponTable.GetBool("fastQueryPointUpdate", fastQueryPointUpdate);

	// Determines how to handle burst fire, when target is out of arc. 0 = no restrictions (default), 1 = don't fire, 2 = fire in current direction of weapon 
	burstControlWhenOutOfArc = weaponTable.GetInt("burstControlWhenOutOfArc", burstControlWhenOutOfArc);
}



/******************************************************************************/

UnitDef::UnitDef()
	: SolidObjectDef()
	, cobID(-1)
	, decoyDef(nullptr)
	, upkeep(0.0f)
	, resourceMake(0.0f)
	, makesMetal(0.0f)
	, buildTime(0.0f)
	, buildeeBuildRadius(-1.f)
	, extractsMetal(0.0f)
	, extractRange(0.0f)
	, windGenerator(0.0f)
	, tidalGenerator(0.0f)
	, storage(0.0f)
	, harvestStorage(0.0f)
	, autoHeal(0.0f)
	, idleAutoHeal(0.0f)
	, idleTime(0)
	, power(0.0f)
	, category(-1)
	, speed(0.0f)
	, rSpeed(0.0f)
	, turnRate(0.0f)
	, turnInPlace(false)
	, turnInPlaceSpeedLimit(0.0f)
	, turnInPlaceAngleLimit(0.0f)
	, collide(false)
	, losHeight(0.0f)
	, radarHeight(0.0f)
	, losRadius(0.0f)
	, airLosRadius(0.0f)
	, radarRadius(0.0f)
	, sonarRadius(0.0f)
	, jammerRadius(0.0f)
	, sonarJamRadius(0.0f)
	, seismicRadius(0.0f)
	, seismicSignature(0.0f)
	, stealth(false)
	, sonarStealth(false)
	, buildRange3D(false)
	, buildDistance(16.0f) // 16.0f is the minimum distance between two 1x1 units
	, buildSpeed(0.0f)
	, reclaimSpeed(0.0f)
	, repairSpeed(0.0f)
	, maxRepairSpeed(0.0f)
	, resurrectSpeed(0.0f)
	, captureSpeed(0.0f)
	, terraformSpeed(0.0f)

	, canSubmerge(false)
	, canfly(false)
	, floatOnWater(false)
	, pushResistant(false)
	, strafeToAttack(false)
	, stopToAttack(false)
	, minCollisionSpeed(0.0f)
	, slideTolerance(0.0f)
	, rollingResistanceCoefficient(0.0f)
	, groundFrictionCoefficient(0.0f)
	, atmosphericDragCoefficient(0.0f)
	, maxHeightDif(0.0f)
	, waterline(0.0f)
	, minWaterDepth(0.0f)
	, maxWaterDepth(0.0f)
	, upDirSmoothing(0.0f)
	, separationDistance(0.0f)
	, pathType(-1U)
	, armoredMultiple(0.0f)
	, armorType(0)
	, flankingBonusMode(0)
	, flankingBonusDir(ZeroVector)
	, flankingBonusMax(0.0f)
	, flankingBonusMin(0.0f)
	, flankingBonusMobilityAdd(0.0f)
	, shieldWeaponDef(nullptr)
	, stockpileWeaponDef(nullptr)
	, maxWeaponRange(0.0f)
	, maxCoverage(0.0f)
	, deathExpWeaponDef(nullptr)
	, selfdExpWeaponDef(nullptr)
	, buildPic(nullptr)
	, selfDCountdown(0)
	, builder(false)
	, activateWhenBuilt(false)
	, onoffable(false)
	, fullHealthFactory(false)
	, factoryHeadingTakeoff(false)
	, capturable(false)
	, repairable(false)

	, canmove(false)
	, canAttack(false)
	, canFight(false)
	, canPatrol(false)
	, canGuard(false)
	, canRepeat(false)
	, canResurrect(false)
	, canCapture(false)
	, canCloak(false)
	, canSelfD(true)
	, canKamikaze(false)

	, canRestore(false)
	, canRepair(false)
	, canReclaim(false)
	, canAssist(false)

	, canBeAssisted(false)
	, canSelfRepair(false)

	, canFireControl(false)
	, canManualFire(false)

	, fireState(FIRESTATE_HOLDFIRE)
	, moveState(MOVESTATE_HOLDPOS)
	, wingDrag(0.0f)
	, wingAngle(0.0f)
	, frontToSpeed(0.0f)
	, speedToFront(0.0f)
	, myGravity(0.0f)
	, maxBank(0.0f)
	, maxPitch(0.0f)
	, turnRadius(0.0f)
	, wantedHeight(0.0f)
	, verticalSpeed(0.0f)
	, useSmoothMesh(false)
	, hoverAttack(false)
	, airStrafe(false)
	, dlHoverFactor(0.0f)
	, bankingAllowed(false)
	, maxAcc(0.0f)
	, maxDec(0.0f)
	, maxAileron(0.0f)
	, maxElevator(0.0f)
	, maxRudder(0.0f)
	, crashDrag(0.0f)
	, loadingRadius(0.0f)
	, unloadSpread(0.0f)
	, transportCapacity(0)
	, transportSize(0)
	, minTransportSize(0)
	, isFirePlatform(false)
	, transportMass(0.0f)
	, minTransportMass(0.0f)
	, holdSteady(false)
	, releaseHeld(false)
	, cantBeTransported(false)
	, transportByEnemy(false)
	, transportUnloadMethod(0)
	, fallSpeed(0.0f)
	, unitFallSpeed(0.0f)
	, startCloaked(false)
	, cloakCost(0.0f)
	, cloakCostMoving(0.0f)
	, decloakDistance(0.0f)
	, decloakSpherical(false)
	, decloakOnFire(false)

	, kamikazeDist(0.0f)
	, kamikazeUseLOS(false)
	, targfac(false)
	, needGeo(false)
	, isFeature(false)
	, hideDamage(false)
	, showPlayerName(false)
	, highTrajectoryType(0)
	, noChaseCategory(0)
	, canDropFlare(false)
	, flareReloadTime(0.0f)
	, flareEfficiency(0.0f)
	, flareDelay(0.0f)
	, flareDropVector(ZeroVector)
	, flareTime(0)
	, flareSalvoSize(0)
	, flareSalvoDelay(0)
	, canLoopbackAttack(false)
	, levelGround(false)
	, showNanoFrame(false)
	, showNanoSpray(false)
	, nanoColor(ZeroVector)
	, maxThisUnit(0)
	, realCost(0.0f)
	, realUpkeep(0.0f)
	, realBuildTime(0.0f)
{
	memset(&modelCEGTags[0], 0, sizeof(modelCEGTags));
	memset(&pieceCEGTags[0], 0, sizeof(pieceCEGTags));
	memset(&crashCEGTags[0], 0, sizeof(crashCEGTags));

	// filled in later by UnitDrawer
	modelExplGenIDs.fill(-1u); modelExplGenIDs[0] = 0;
	pieceExplGenIDs.fill(-1u); pieceExplGenIDs[0] = 0;
	crashExplGenIDs.fill(-1u); crashExplGenIDs[0] = 0;
}


/***
 * Defines a unit type.
 * 
 * @class UnitDef
 */
UnitDef::UnitDef(const LuaTable& udTable, const std::string& unitName, int id)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// rely on default-ctor to initialize all members
	*this = UnitDef();
	this->id = id;

	name = unitName;

	/* Many entries read two keys. The backup key is usually what Total
	 * Annihilation expected and is typically "robust" in that invalid
	 * values get clamped. Sometimes these values are also in a different
	 * unit (for example modern "speed" is in elmo/second but the legacy
	 * "maxVelocity" is in elmo/frame). The new ones reject the whole def
	 * if a value is invalid so as not to subtly hide errors, and the key
	 * matches what is exposed in UnitDefs. */

	/*** @field UnitDef.humanName string? */
	humanName = udTable.GetString("humanName", udTable.GetString("name", "")); // note, `UnitDefs[x].name` is the _internal_ name
	/*** @field UnitDef.description string? */
	tooltip = udTable.GetString("description", name);
	/*** @field UnitDef.corpse string? */
	wreckName = udTable.GetString("corpse", "");
	/*** @field UnitDef.buildPic string? */
	buildPicName = udTable.GetString("buildPic", "");
	/*** @field UnitDef.decoyFor string? */
	decoyName = udTable.GetString("decoyFor", "");

	storage =
		/*** @field UnitDef.metalStorage number? */
		{ udTable.GetFloat( "metalStorage", 0.0f)
		/*** @field UnitDef.energyStorage number? */
		, udTable.GetFloat("energyStorage", 0.0f)
	};
	harvestStorage =
		/*** @field UnitDef.harvestMetalStorage number? */
		{ udTable.GetFloat("harvestMetalStorage", udTable.GetFloat("harvestStorage", 0.0f))
		/*** @field UnitDef.harvestEnergyStorage number? */
		, udTable.GetFloat("harvestEnergyStorage", 0.0f)
	};

	/*** @field UnitDef.extractsMetal number? */
	extractsMetal  = udTable.GetFloat("extractsMetal",  0.0f);
	/*** @field UnitDef.windGenerator number? */
	windGenerator  = udTable.GetFloat("windGenerator",  0.0f);
	/*** @field UnitDef.tidalGenerator number? */
	tidalGenerator = udTable.GetFloat("tidalGenerator", 0.0f);

	upkeep =
		/*** @field UnitDef.metalUpkeep number? */
		{ udTable.GetFloat( "metalUpkeep", udTable.GetFloat( "metalUse", 0.0f))
		/*** @field UnitDef.energyUpkeep number? */
		, udTable.GetFloat("energyUpkeep", udTable.GetFloat("energyUse", 0.0f))
	};
	resourceMake =
		/*** @field UnitDef.metalMake number? */
		{ udTable.GetFloat( "metalMake", 0.0f)
		/*** @field UnitDef.energyMake number? */
		, udTable.GetFloat("energyMake", 0.0f)
	};
	/*** @field UnitDef.makesMetal number? */
	makesMetal   = udTable.GetFloat("makesMetal", 0.0f);

	/*** @field UnitDef.autoHeal number? */
	autoHeal     = udTable.GetFloat("autoHeal",      0.0f) * (UNIT_SLOWUPDATE_RATE * INV_GAME_SPEED);
	/*** @field UnitDef.idleAutoHeal number? */
	idleAutoHeal = udTable.GetFloat("idleAutoHeal", 10.0f) * (UNIT_SLOWUPDATE_RATE * INV_GAME_SPEED);
	/*** @field UnitDef.idleTime integer? (Default: `600`) */
	idleTime     = udTable.GetInt("idleTime", 600);

	/*** @field UnitDef.health number? (Default: `100.0`)*/
	health = udTable.GetFloat("health", udTable.GetFloat("maxDamage", 100.0f));
	if (health <= 0.0f)
		throw content_error (unitName + ".health <= 0");

	/*** @field UnitDef.metalCost number? */
	cost.metal = udTable.GetFloat("metalCost", udTable.GetFloat("buildCostMetal", 0.0f));
	if (cost.metal < 0.0f)
		throw content_error (unitName + ".metalCost < 0");

	/*** @field UnitDef.energyCost number? */
	cost.energy = udTable.GetFloat("energyCost", udTable.GetFloat("buildCostEnergy", 0.0f));
	if (cost.energy < 0.0f)
		throw content_error (unitName + ".energyCost < 0");

	/*** @field UnitDef.buildTime number? */
	buildTime = udTable.GetFloat("buildTime", 100.0f);
	if (buildTime <= 0.0f)
		throw content_error (unitName + ".buildTime <= 0");

	/*** @field UnitDef.buildeeBuildRadius number? (Default: `-1.0 */
	buildeeBuildRadius = udTable.GetFloat("buildeeBuildRadius", -1.f);

	/*** @field UnitDef.mass number? (Default: `UnitDef.metalCost`) */
	mass = std::clamp(udTable.GetFloat("mass", cost.metal), CSolidObject::MINIMUM_MASS, CSolidObject::MAXIMUM_MASS);
	/*** @field UnitDef.crushResistance number? (Default: `UnitDef.mass`) */
	crushResistance = udTable.GetFloat("crushResistance", mass);

	/*** @field UnitDef.cobID integer? (Default: `-1`) */
	cobID = udTable.GetInt("cobID", -1);

	/*** @field UnitDef.buildRange3D boolean? (Default: `false`) */
	buildRange3D = udTable.GetBool("buildRange3D", false);

	// 128.0f is the ancient default
	/*** @field UnitDef.buildDistance number? (Default: `128.0`) In range `[38.0, ∞)`. */
	buildDistance = udTable.GetFloat("buildDistance", 128.0f);
	// 38.0f was evaluated by bobthedinosaur and FLOZi to be the bare minimum
	// to not overlap for a 1x1 constructor building a 1x1 structure
	buildDistance = std::max(38.0f, buildDistance);
	/*** @field UnitDef.workerTime number? Build speed. */
	buildSpeed = udTable.GetFloat("workerTime", 0.0f);
	/*** @field UnitDef.builder boolean? (Default: `false`) Ignored unless `workerTime` is greater than zero. */
	builder = udTable.GetBool("builder", false);
	builder &= IsBuilderUnit();

	/*** @field UnitDef.repairSpeed number? (Default: `UnitDef.buildSpeed`) */
	repairSpeed    = udTable.GetFloat("repairSpeed",    buildSpeed);
	/*** @field UnitDef.maxRepairSpeed number? (Default: `1e20`) */
	maxRepairSpeed = udTable.GetFloat("maxRepairSpeed",      1e20f);
	/*** @field UnitDef.reclaimSpeed number? (Default: `UnitDef.buildSpeed`) */
	reclaimSpeed   = udTable.GetFloat("reclaimSpeed",   buildSpeed);
	/*** @field UnitDef.resurrectSpeed number? (Default: `UnitDef.buildSpeed`) */
	resurrectSpeed = udTable.GetFloat("resurrectSpeed", buildSpeed);
	/*** @field UnitDef.captureSpeed number? (Default: `UnitDef.buildSpeed`) */
	captureSpeed   = udTable.GetFloat("captureSpeed",   buildSpeed);
	/*** @field UnitDef.terraformSpeed number? (Default: `UnitDef.buildSpeed`) */
	terraformSpeed = udTable.GetFloat("terraformSpeed", buildSpeed);

	/*** @field UnitDef.upDirSmoothing number? In range `[0.0, 0.95]`. */
	upDirSmoothing = std::clamp(udTable.GetFloat("upDirSmoothing", 0.0f), 0.0f, 0.95f);
	/*** @field UnitDef.separationDistance integer? (Default: `0`) */
	separationDistance = std::max(udTable.GetInt("separationDistance", 0), 0);

	/*** @field UnitDef.reclaimable boolean? (Default: `true`) */
	reclaimable  = udTable.GetBool("reclaimable",  true);
	/*** @field UnitDef.capturable boolean? (Default: `true`) */
	capturable   = udTable.GetBool("capturable",   true);
	/*** @field UnitDef.repairable boolean? (Default: `true`) */
	repairable   = udTable.GetBool("repairable",   true);

	/*** @field UnitDef.canMove boolean? (Default: `false`) */
	canmove      = udTable.GetBool("canMove",         false);
	/*** @field UnitDef.canAttack boolean? (Default: `true`) */
	canAttack    = udTable.GetBool("canAttack",       true);
	/*** @field UnitDef.canFight boolean? (Default: `true`) */
	canFight     = udTable.GetBool("canFight",        true);
	/*** @field UnitDef.canPatrol boolean? (Default: `true`) */
	canPatrol    = udTable.GetBool("canPatrol",       true);
	/*** @field UnitDef.canGuard boolean? (Default: `true`) */
	canGuard     = udTable.GetBool("canGuard",        true);
	/*** @field UnitDef.canRepeat boolean? (Default: `true`) */
	canRepeat    = udTable.GetBool("canRepeat",       true);
	/*** @field UnitDef.canCloak boolean? (Default: `UnitDef.cloakCost != 0`) */
	canCloak     = udTable.GetBool("canCloak",        (udTable.GetFloat("cloakCost", 0.0f) != 0.0f));
	/*** @field UnitDef.canSelfDestruct boolean? (Default: `true`) */
	canSelfD     = udTable.GetBool("canSelfDestruct", true);
	/*** @field UnitDef.kamikaze boolean? (Default: `false`) */
	canKamikaze  = udTable.GetBool("kamikaze",        false);

	// capture and resurrect count as special abilities
	// (because captureSpeed and resurrectSpeed default
	// to buildSpeed, canCapture and canResurrect would
	// otherwise become true for all regular builders)
	/*** @field UnitDef.canRestore boolean? (Default: `UnitDef.builder`) Ignored if `terraformSpeed` is zero. */
	canRestore   = udTable.GetBool("canRestore",   builder) && (terraformSpeed > 0.0f);
	/*** @field UnitDef.canRepair boolean? (Default: `UnitDef.builder`) Ignored if `repairSpeed` is zero. */
	canRepair    = udTable.GetBool("canRepair",    builder) && (   repairSpeed > 0.0f);
	/*** @field UnitDef.canReclaim boolean? (Default: `UnitDef.builder`) Ignored if `reclaimSpeed` is zero. */
	canReclaim   = udTable.GetBool("canReclaim",   builder) && (  reclaimSpeed > 0.0f);
	/*** @field UnitDef.canCapture boolean? (Default: `false`) Ignored if `captureSpeed` is zero. */
	canCapture   = udTable.GetBool("canCapture",     false) && (  captureSpeed > 0.0f);
	/*** @field UnitDef.canResurrect boolean? (Default: `false`) Ignored if `resurrectSpeed` is zero. */
	canResurrect = udTable.GetBool("canResurrect",   false) && (resurrectSpeed > 0.0f);

	/* Note that a mobile builder with canAssist = false will be able
	 * to place a nanoframe and pour buildpower into it, but will not
	 * be able to resume building if interrupted for any reason (with
	 * the exception of the wait command). It will be unable to pour
	 * buildpower into a nanoframe placed by another unit. It will be
	 * unable to repair an incomplete nanoframe or place a nanoframe
	 * on top of an existing nanoframe, even if it is the exact same
	 * structure in the exact same location. */
	/*** @field UnitDef.canAssist boolean? (Default: `UnitDef.builder`) */
	canAssist    = udTable.GetBool("canAssist",    builder);

	/*** @field UnitDef.canBeAssisted boolean? (Default: `true`) */
	canBeAssisted = udTable.GetBool("canBeAssisted", true);
	/*** @field UnitDef.canSelfRepair boolean? (Default: `false`) */
	canSelfRepair = udTable.GetBool("canSelfRepair", false);

	/*** @field UnitDef.noAutoFire boolean? (Default: `false`) */
	canFireControl = !udTable.GetBool("noAutoFire", false);
	/*** @field UnitDef.canManualFire boolean? (Default: `false`) */
	canManualFire = udTable.GetBool("canManualFire", udTable.GetBool("canDGun", false));

	/*** @field UnitDef.fullHealthFactory boolean? (Default: `false`) */
	fullHealthFactory = udTable.GetBool("fullHealthFactory", false);
	/*** @field UnitDef.factoryHeadingTakeoff boolean? (Default: `true`) */
	factoryHeadingTakeoff = udTable.GetBool("factoryHeadingTakeoff", true);

	/*** @field UnitDef.upright boolean? (Default: `false`) */
	upright = udTable.GetBool("upright", false);
	/*** @field UnitDef.blocking boolean? (Default: `true`) */
	collidable = udTable.GetBool("blocking", true);
	/*** @field UnitDef.collider boolean? (Default: `true`) */
	collide = udTable.GetBool("collide", true);

	/*** @field UnitDef.maxSlope number? Maximum slope in degrees. In range `[0, 89.0]` */
	const float maxSlopeDeg = std::clamp(udTable.GetFloat("maxSlope", 0.0f), 0.0f, 89.0f);
	const float maxSlopeRad = maxSlopeDeg * math::DEG_TO_RAD;

	// FIXME: kill the magic constant
	maxHeightDif = 40.0f * math::tanf(maxSlopeRad);

	/*** @field UnitDef.minWaterDepth number? (Default: `-10e6`) */
	minWaterDepth = udTable.GetFloat("minWaterDepth", -10e6f);
	/*** @field UnitDef.maxWaterDepth number? (Default: `+10e6`) */
	maxWaterDepth = udTable.GetFloat("maxWaterDepth", +10e6f);
	/*** @field UnitDef.waterline number? (Default: `0.0`) */
	waterline = udTable.GetFloat("waterline", 0.0f);
	/*** @field UnitDef.minCollisionSpeed number? (Default: `1.0`) */
	minCollisionSpeed = udTable.GetFloat("minCollisionSpeed", 1.0f);
	/*** @field UnitDef.slideTolerance number? (Default: `0.0`) */
	slideTolerance = udTable.GetFloat("slideTolerance", 0.0f); // disabled
	/*** @field UnitDef.rollingResistanceCoefficient number? (Default: `0.05`) */
	rollingResistanceCoefficient = udTable.GetFloat("rollingResistanceCoefficient", 0.05f);
	/*** @field UnitDef.groundFrictionCoefficient number? (Default: `0.01`) */
	groundFrictionCoefficient = udTable.GetFloat("groundFrictionCoefficient", 0.01f);
	/*** @field UnitDef.atmosphericDragCoefficient number? (Default: `1.0`) */
	atmosphericDragCoefficient = udTable.GetFloat("atmosphericDragCoefficient", 1.0f);
	/*** @field UnitDef.pushResistant boolean? (Default: `false`) */
	pushResistant = udTable.GetBool("pushResistant", false);
	/*** @field UnitDef.selfDestructCountdown integer? (Default: `5`) */
	selfDCountdown = udTable.GetInt("selfDestructCountdown", 5);

	/* Note that the legacy unit is elmo/frame
	 * whereas the modern one is elmo/second */

	/*** @field UnitDef.speed maxVelocity? (Default: `0`) Maximum speed in elmo/second. */
	const decltype(speed) speedLegacy = math::fabs(udTable.GetFloat("maxVelocity", 0.0f) * GAME_SPEED);
	speed = udTable.GetFloat("speed", speedLegacy);
	if (speed < 0.0f)
		throw content_error(unitName + ".speed < 0");

	/*** @field UnitDef.speed maxReverseVelocity? (Default: `0`) Maximum reverse speed in elmo/second. */
	const decltype(rSpeed) rSpeedLegacy = math::fabs(udTable.GetFloat("maxReverseVelocity", 0.0f) * GAME_SPEED);
	rSpeed = udTable.GetFloat("rSpeed", rSpeedLegacy);
	if (rSpeed < 0.0f)
		throw content_error(unitName + ".rSpeed < 0");

	/* The unit here is elmo/frame^2 for both spellings of the key.
	 * At some point, 'acceleration' should change to elmo/second^2
	 * and get exposed to UnitDefs. Let games migrate first though. */

	/*** @field UnitDef.speed maxAcc? (Default: `0.5`) Maximum acceleration in elmo/frame². */
	maxAcc = udTable.GetFloat("maxAcc", udTable.GetFloat("acceleration", 0.5f));
	if (maxAcc < 0.0f)
		throw content_error(unitName + ".acceleration < 0");

	/*** @field UnitDef.speed maxDec? (Default: `UnitDef.maxAcc`) Maximum deceleration in elmo/frame². */
	maxDec = udTable.GetFloat("maxDec", udTable.GetFloat("brakeRate", maxAcc));
	if (maxDec < 0.0f)
		throw content_error(unitName + ".brakeRate < 0");

	/*** @field UnitDef.fireState integer? (Default: If `UnitDef.noAutoFire` then `FIRESTATE_FIREATWILL`, otherwise `FIRESTATE_NONE`) */
	fireState = udTable.GetInt("fireState", canFireControl? FIRESTATE_NONE: FIRESTATE_FIREATWILL);
	fireState = std::min(fireState, int(FIRESTATE_FIREATNEUTRAL));
	/*** @field UnitDef.fireState integer? (Default: `MOVESTATE_MANEUVER`, or `MOVESTATE_NONE` if unable to move) */
	moveState = udTable.GetInt("moveState", (canmove && speed > 0.0f)? MOVESTATE_NONE: MOVESTATE_MANEUVER);
	moveState = std::min(moveState, int(MOVESTATE_ROAM));

	/*** @field UnitDef.flankingBonusMode integer? */
	flankingBonusMode = udTable.GetInt("flankingBonusMode", modInfo.flankingBonusModeDefault);
	/*** @field UnitDef.flankingBonusMax number? */
	flankingBonusMax  = udTable.GetFloat("flankingBonusMax", modInfo.flankingBonusMaxDefault);
	/*** @field UnitDef.flankingBonusMax number? */
	flankingBonusMin  = udTable.GetFloat("flankingBonusMin", modInfo.flankingBonusMinDefault);
	/*** @field UnitDef.flankingBonusDir float3String? */
	flankingBonusDir  = udTable.GetFloat3("flankingBonusDir", FwdVector);
	/*** @field UnitDef.flankingBonusMobilityAdd number? */
	flankingBonusMobilityAdd = udTable.GetFloat("flankingBonusMobilityAdd", 0.01f);

	/*** @field UnitDef.damageModifier number? (Default: `1.0`) */
	armoredMultiple = udTable.GetFloat("damageModifier", 1.0f);
	armorType = damageArrayHandler.GetTypeFromName(name);

	/*** @field UnitDef.sightEmitHeight number? (Default: `20.0`) */
	losHeight = udTable.GetFloat("sightEmitHeight", udTable.GetFloat("losEmitHeight", 20.0f));
	/*** @field UnitDef.radarEmitHeight number? (Default: `UnitDef.sightEmitHeight`) */
	radarHeight = udTable.GetFloat("radarEmitHeight", losHeight);

	/*** @field UnitDef.sightDistance number? */
	losRadius = udTable.GetFloat("sightDistance", 0.0f);
	/*** @field UnitDef.airSightDistance number? (Default: `1.5 * UnitDef.sightDistance`) */
	airLosRadius = udTable.GetFloat("airSightDistance", 1.5f * losRadius);
	/*** @field UnitDef.radarDistance integer? (Default: `0`)*/
	radarRadius    = udTable.GetInt("radarDistance",    0);
	/*** @field UnitDef.sonarDistance integer? (Default: `0`) */
	sonarRadius    = udTable.GetInt("sonarDistance",    0);
	/*** @field UnitDef.radarDistanceJam integer? (Default: `0`) */
	jammerRadius   = udTable.GetInt("radarDistanceJam", 0);
	/*** @field UnitDef.sonarDistanceJam integer? (Default: `0`) */
	sonarJamRadius = udTable.GetInt("sonarDistanceJam", 0);

	/*** @field UnitDef.seismicDistance integer? (Default: `0`) */
	seismicRadius    = udTable.GetInt("seismicDistance", 0);
	/*** @field UnitDef.seismicDistance number? (Default: `-1.0`) */
	seismicSignature = udTable.GetFloat("seismicSignature", -1.0f);

	/*** @field UnitDef.stealth boolean? (Default: `false`) */
	stealth        = udTable.GetBool("stealth",            false);
	/*** @field UnitDef.sonarStealth boolean? (Default: `false`) */
	sonarStealth   = udTable.GetBool("sonarStealth",       false);
	/*** @field UnitDef.isTargetingUpgrade boolean? (Default: `false`) */
	targfac        = udTable.GetBool("isTargetingUpgrade", false);
	/*** @field UnitDef.isFeature boolean? (Default: `false`) */
	isFeature      = udTable.GetBool("isFeature",          false);
	/*** @field UnitDef.hideDamage boolean? (Default: `false`) */
	hideDamage     = udTable.GetBool("hideDamage",         false);
	/*** @field UnitDef.showPlayerName boolean? (Default: `false`) */
	showPlayerName = udTable.GetBool("showPlayerName",     false);

	/*** @field UnitDef.cloakCost number? (Default: `0.0`) */
	cloakCost = udTable.GetFloat("cloakCost", 0.0f);
	/*** @field UnitDef.cloakCostMoving number? (Default: `cloakCost`) */
	cloakCostMoving = udTable.GetFloat("cloakCostMoving", cloakCost);

	/*** @field UnitDef.startCloaked boolean? (Default: `false`) */
	startCloaked     = udTable.GetBool("initCloaked", false);
	/*** @field UnitDef.decloakDistance number? (Default: `0.0`) */
	decloakDistance  = udTable.GetFloat("minCloakDistance", 0.0f);
	/*** @field UnitDef.decloakSpherical boolean? (Default: `true`) */
	decloakSpherical = udTable.GetBool("decloakSpherical", true);
	/*** @field UnitDef.decloakOnFire boolean? (Default: `true`) */
	decloakOnFire    = udTable.GetBool("decloakOnFire",    true);

	/*** @field UnitDef.highTrajectory integer? (Default: `0`) */
	highTrajectoryType = udTable.GetInt("highTrajectory", 0);

	/*** @field UnitDef.kamikazeDistance number? (Default: `-25.0`) 3D kamikaze distance. This number is increased by 25. */
	// we count 3d distance while ta count 2d distance so increase slightly
	kamikazeDist = udTable.GetFloat("kamikazeDistance", -25.0f) + 25.0f;
	/*** @field UnitDef.kamikazeUseLOS boolean? (Default: `false`) */
	kamikazeUseLOS = udTable.GetBool("kamikazeUseLOS", false);

	/*** @field UnitDef.showNanoFrame boolean? (Default: `true`) */
	showNanoFrame = udTable.GetBool("showNanoFrame", true);
	/*** @field UnitDef.showNanoSpray boolean? (Default: `true`) */
	showNanoSpray = udTable.GetBool("showNanoSpray", true);
	nanoColor = udTable.GetFloat3("nanoColor", float3(0.2f,0.7f,0.2f));

	/*** @field UnitDef.canFly boolean? (Default: `false`) */
	canfly      = udTable.GetBool("canFly",      false);
	/*** @field UnitDef.canSubmerge boolean? (Default: `false`) */
	canSubmerge = udTable.GetBool("canSubmerge", false) && canfly;

	/*** @field UnitDef.airStrafe boolean? (Default: `true`) */
	airStrafe      = udTable.GetBool("airStrafe", true);
	/*** @field UnitDef.hoverAttack boolean? (Default: `false`) */
	hoverAttack    = udTable.GetBool("hoverAttack", false);
	/*** @field UnitDef.cruiseAltitude number? (Default: `0`) */
	wantedHeight   = udTable.GetFloat("cruiseAltitude", udTable.GetFloat("cruiseAlt", 0.0f));
	/*** @field UnitDef.airHoverFactor number? (Default: `-1.0`) */
	dlHoverFactor  = udTable.GetFloat("airHoverFactor", -1.0f);
	/*** @field UnitDef.bankingAllowed boolean? (Default: `true`) */
	bankingAllowed = udTable.GetBool("bankingAllowed", true);
	/*** @field UnitDef.useSmoothMesh boolean? (Default: `true`) */
	useSmoothMesh  = udTable.GetBool("useSmoothMesh", true);

	/*** @field UnitDef.turnRate number? (Default: `0`) */
	turnRate    = udTable.GetFloat("turnRate", 0.0f);
	/*** @field UnitDef.turnInPlace boolean? (Default: `true`) */
	turnInPlace = udTable.GetBool("turnInPlace", true);
	turnInPlaceSpeedLimit = turnRate / SPRING_CIRCLE_DIVS;
	turnInPlaceSpeedLimit *= (math::TWOPI * SQUARE_SIZE);
	turnInPlaceSpeedLimit /= std::max(speed / GAME_SPEED, 1.0f);
	/*** @field UnitDef.turnInPlaceSpeedLimit number? (Default: `speed`) */
	turnInPlaceSpeedLimit = udTable.GetFloat("turnInPlaceSpeedLimit", std::min(speed, turnInPlaceSpeedLimit));
	/*** @field UnitDef.turnInPlaceAngleLimit number? (Default: `0.0`) The turning angle in degrees above which it starts to brake. */
	turnInPlaceAngleLimit = udTable.GetFloat("turnInPlaceAngleLimit", 0.0f);


	/*** @field UnitDef.transportSize integer? (Default: `0`) */
	transportSize     = udTable.GetInt("transportSize",      0);
	/*** @field UnitDef.minTransportSize integer? (Default: `0`) */
	minTransportSize  = udTable.GetInt("minTransportSize",   0);
	/*** @field UnitDef.transportCapacity integer? (Default: `0`) */
	transportCapacity = udTable.GetInt("transportCapacity",  0);
	/*** @field UnitDef.isFirePlatform boolean? (Default: `false`) */
	isFirePlatform    = udTable.GetBool("isFirePlatform",    false);
	/*** @field UnitDef.loadingRadius number? (Default: `220.0`) */
	loadingRadius     = udTable.GetFloat("loadingRadius",    220.0f);
	/*** @field UnitDef.unloadSpread number? (Default: `5.0`) */
	unloadSpread      = udTable.GetFloat("unloadSpread",     5.0f);
	/*** @field UnitDef.transportMass number? (Default: `100000.0`) */
	transportMass     = udTable.GetFloat("transportMass",    100000.0f);
	/*** @field UnitDef.minTransportMass number? (Default: `0.0`) */
	minTransportMass  = udTable.GetFloat("minTransportMass", 0.0f);
	/*** @field UnitDef.holdSteady boolean? (Default: `false`) */
	holdSteady        = udTable.GetBool("holdSteady",        false);
	/*** @field UnitDef.releaseHeld boolean? (Default: `false`) */
	releaseHeld       = udTable.GetBool("releaseHeld",       false);
	/*** @field UnitDef.cantBeTransported boolean? */
	cantBeTransported = udTable.GetBool("cantBeTransported", !RequireMoveDef());
	/*** @field UnitDef.transportByEnemy boolean? (Default: `true`) */
	transportByEnemy  = udTable.GetBool("transportByEnemy",  true);
	/*** @field UnitDef.fallSpeed number? (Default: `0.2`) */
	fallSpeed         = udTable.GetFloat("fallSpeed",    0.2f);
	/*** @field UnitDef.unitFallSpeed number? (Default: `0`) */
	unitFallSpeed     = udTable.GetFloat("unitFallSpeed",  0);
	/*** @field UnitDef.transportUnloadMethod integer? (Default: `0`) */
	transportUnloadMethod = udTable.GetInt("transportUnloadMethod" , 0);

	/*** @field UnitDef.wingDrag number? (Default: `0.07`) Drag caused by wings. */
	wingDrag     = udTable.GetFloat("wingDrag",     0.07f);
	wingDrag     = std::clamp(wingDrag, 0.0f, 1.0f);
	/*** @field UnitDef.wingAngle number? (Default: `0.08`) Angle between front and the wing plane. */
	wingAngle    = udTable.GetFloat("wingAngle",    0.08f);
	/*** @field UnitDef.frontToSpeed number? (Default: `0.1`) Fudge factor for lining up speed and front of plane. */
	frontToSpeed = udTable.GetFloat("frontToSpeed", 0.1f); 
	/*** @field UnitDef.speedToFront number? (Default: `0.07`) Fudge factor for lining up speed and front of plane. */
	speedToFront = udTable.GetFloat("speedToFront", 0.07f);
	/*** @field UnitDef.myGravity number? (Default: `0.4`) Planes are slower than real airplanes so lower gravity to compensate. */
	myGravity    = udTable.GetFloat("myGravity",    0.4f); 
	/*** @field UnitDef.crashDrag number? (Default: `0.005`) Drag used when crashing. */
	crashDrag    = udTable.GetFloat("crashDrag",    0.005f);
	crashDrag    = std::clamp(crashDrag, 0.0f, 1.0f);

	/*** @field UnitDef.maxBank number? (Default: `0.8`) Max roll. */
	maxBank = udTable.GetFloat("maxBank", 0.8f);
	/*** @field UnitDef.maxPitch number? (Default: `0.45`) Max pitch this plane tries to keep. */
	maxPitch = udTable.GetFloat("maxPitch", 0.45f);
	/*** @field UnitDef.turnRadius number? (Default: `500.0`) Hint to CStrafeAirMoveType about required turn-radius. */
	turnRadius = udTable.GetFloat("turnRadius", 500.0f); // hint to CStrafeAirMoveType about required turn-radius
	/*** @field UnitDef.verticalSpeed number? (Default: `3.0`) Speed of takeoff and landing, at least for gunships. */
	verticalSpeed = udTable.GetFloat("verticalSpeed", 3.0f); // speed of takeoff and landing, at least for gunships

	/*** @field UnitDef.maxAileron number? (Default: `0.015`) Turn speed around roll axis. */
	maxAileron  = udTable.GetFloat("maxAileron",  0.015f);
	/*** @field UnitDef.maxElevator number? (Default: `0.01`) Turn speed around pitch axis. */
	maxElevator = udTable.GetFloat("maxElevator", 0.01f);
	/*** @field UnitDef.maxRudder number? (Default: `0.004`) Turn speed around yaw axis. */
	maxRudder   = udTable.GetFloat("maxRudder",   0.004f);

	/*** @field UnitDef.maxThisUnit integer? (Default: `MAX_UNITS`) Can be overridden by game setup.*/
	maxThisUnit = udTable.GetInt("maxThisUnit", udTable.GetInt("unitRestricted", MAX_UNITS));
	maxThisUnit = std::min(maxThisUnit, gameSetup->GetRestrictedUnitLimit(name, MAX_UNITS));

	/*** @field UnitDef.category string? (Default: `""`) */
	categoryString = udTable.GetString("category", "");

	category = CCategoryHandler::Instance()->GetCategories(udTable.GetString("category", ""));
	/*** @field UnitDef.noChaseCategory string? (Default: `""`) */
	noChaseCategory = CCategoryHandler::Instance()->GetCategories(udTable.GetString("noChaseCategory", ""));

	/*** @field UnitDef.iconType string? (Default: `"default"`) */
	iconType = icon::iconHandler.GetIcon(udTable.GetString("iconType", "default"));

	shieldWeaponDef    = nullptr;
	stockpileWeaponDef = nullptr;

	maxWeaponRange = 0.0f;
	maxCoverage = 0.0f;

	/*** @field UnitDef.weapons table[]? */
	LuaTable weaponsTable = udTable.SubTable("weapons");
	ParseWeaponsTable(weaponsTable);

	needGeo = false;
	extractRange = mapInfo->map.extractorRadius * int(extractsMetal > 0.0f);

	{
		MoveDef* moveDef = nullptr;

		// aircraft have MoveTypes but no MoveDef;
		// static structures have no use for either
		// (but get StaticMoveType instances)
		if (RequireMoveDef()) {
			/*** @field UnitDef.movementClass string? (Default: `""`) */
			const std::string& moveClass = StringToLower(udTable.GetString("movementClass", ""));
			const std::string errMsg = "WARNING: Couldn't find a MoveClass named " + moveClass + " (used in UnitDef: " + unitName + ")";

			// invalidate this unitDef; caught in ParseUnitDef
			if ((moveDef = moveDefHandler.GetMoveDefByName(moveClass)) == nullptr)
				throw content_error(errMsg);

			this->pathType = moveDef->pathType;
		}

		if (moveDef == nullptr) {
			upright           |= !canfly;
			/*** @field UnitDef.floater boolean? (Default: `false`) Does this unit float on water? */
			floatOnWater      |= udTable.GetBool("floater", udTable.KeyExists("WaterLine"));

			// we have no MoveDef, so pathType == -1 and IsAirUnit() MIGHT be true
			cantBeTransported |= (!modInfo.transportAir && canfly);
		} else {
			//upright           |= (moveDef->speedModClass == MoveDef::Hover);
			//upright           |= (moveDef->speedModClass == MoveDef::Ship );

			// we have a MoveDef, so pathType != -1 and IsGroundUnit() MUST be true
			cantBeTransported |= (!modInfo.transportGround && moveDef->speedModClass == MoveDef::Tank );
			cantBeTransported |= (!modInfo.transportGround && moveDef->speedModClass == MoveDef::KBot );
			cantBeTransported |= (!modInfo.transportShip   && moveDef->speedModClass == MoveDef::Ship );
			cantBeTransported |= (!modInfo.transportHover  && moveDef->speedModClass == MoveDef::Hover);
		}

		if (seismicSignature == -1.0f) {
			const bool isTank = (moveDef != nullptr && moveDef->speedModClass == MoveDef::Tank);
			const bool isKBot = (moveDef != nullptr && moveDef->speedModClass == MoveDef::KBot);

			// seismic signatures only make sense for certain mobile ground units
			if (isTank || isKBot) {
				seismicSignature = math::sqrt(mass / 100.0f);
			} else {
				seismicSignature = 0.0f;
			}
		}
	}

	if (IsAirUnit()) {
		if (IsFighterAirUnit() || IsBomberAirUnit()) {
			// double turn-radius for bombers if not set explicitly
			turnRadius *= (1.0f + (IsBomberAirUnit() && turnRadius == 500.0f));
			maxAcc = udTable.GetFloat("maxAcc", 0.065f); // engine power
		}
	}


	/*** @field UnitDef.objectName string? (Default: `""`) */
	modelName = udTable.GetString("objectName", "");
	/*** @field UnitDef.scriptName string? (Default: `unitName .. ".cob"`) Path to unit script, relative to `scripts/` folder. Excludes `.cob` file extension. */
	scriptName = "scripts/" + udTable.GetString("script", unitName + ".cob");

	/*** @field UnitDef.explodeAs string? (Default: `""`) */
	deathExpWeaponDef = weaponDefHandler->GetWeaponDef(udTable.GetString("explodeAs", ""));
	/*** @field UnitDef.selfDestructAs string? (Default: `UnitDef.explodeAs`) */
	selfdExpWeaponDef = weaponDefHandler->GetWeaponDef(udTable.GetString("selfDestructAs", udTable.GetString("explodeAs", "")));

	if (deathExpWeaponDef == nullptr && (deathExpWeaponDef = weaponDefHandler->GetWeaponDef("NOWEAPON")) == nullptr) {
		LOG_L(L_ERROR, "Couldn't find WeaponDef NOWEAPON and explodeAs for %s is missing!", unitName.c_str());
	}
	if (selfdExpWeaponDef == nullptr && (selfdExpWeaponDef = weaponDefHandler->GetWeaponDef("NOWEAPON")) == nullptr) {
		LOG_L(L_ERROR, "Couldn't find WeaponDef NOWEAPON and selfDestructAs for %s is missing!", unitName.c_str());
	}

	/*** @field UnitDef.power number? (Default: `cost.metal + (cost.energy / 60.0f)`) */
	power = udTable.GetFloat("power", (cost.metal + (cost.energy / 60.0f)));

	// Prevent a division by zero in experience calculations.
	if (power < 1.0e-3f) {
		LOG_L(L_WARNING, "Unit '%s' (%s) has really low power? %f", humanName.c_str(), unitName.c_str(), power);
		LOG_L(L_WARNING, "This can cause a division by zero in experience calculations.");
		power = 1.0e-3f;
	}

	/*** @field UnitDef.activateWhenBuilt boolean? (Default: `false`) */
	activateWhenBuilt = udTable.GetBool("activateWhenBuilt", false);
	/*** @field UnitDef.onoffable boolean? (Default: `false`) */
	onoffable = udTable.GetBool("onoffable", false);

	xsize = std::max(1 * SPRING_FOOTPRINT_SCALE, (udTable.GetInt("footprintX", 1) * SPRING_FOOTPRINT_SCALE));
	zsize = std::max(1 * SPRING_FOOTPRINT_SCALE, (udTable.GetInt("footprintZ", 1) * SPRING_FOOTPRINT_SCALE));

	buildingMask = (std::uint16_t)udTable.GetInt("buildingMask", 1); //1st bit set to 1 constitutes for "normal building"
	if (IsImmobileUnit())
		CreateYardMap(udTable.GetString("yardMap", ""));

	decalDef.Parse(udTable);

	/*** @field UnitDef.canDropFlare boolean? (Default: `false`) */
	canDropFlare    = udTable.GetBool("canDropFlare", false);
	/*** @field UnitDef.flareReload number? (Default: `5.0`) */
	flareReloadTime = udTable.GetFloat("flareReload",     5.0f);
	/*** @field UnitDef.flareDelay number? (Default: `0.3`) */
	flareDelay      = udTable.GetFloat("flareDelay",      0.3f);
	/*** @field UnitDef.flareEfficiency number? (Default: `0.5`) */
	flareEfficiency = udTable.GetFloat("flareEfficiency", 0.5f);
	/*** @field UnitDef.flareDropVector float3String? (Default: `"0 0 0"`) */
	flareDropVector = udTable.GetFloat3("flareDropVector", ZeroVector);
	/*** @field UnitDef.flareTime integer? (Default: `3`) */
	flareTime       = udTable.GetInt("flareTime", 3) * GAME_SPEED;
	/*** @field UnitDef.flareSalvoSize integer? (Default: `4`) */
	flareSalvoSize  = udTable.GetInt("flareSalvoSize",  4);
	/*** @field UnitDef.flareSalvoDelay integer? (Default: `0`) */
	flareSalvoDelay = udTable.GetInt("flareSalvoDelay", 0) * GAME_SPEED;

	/*** @field UnitDef.canLoopbackAttack boolean? (Default: `false`) */
	canLoopbackAttack = udTable.GetBool("canLoopbackAttack", false);
	/*** @field UnitDef.levelGround boolean? (Default: `true`) */
	levelGround = udTable.GetBool("levelGround", true);
	/*** @field UnitDef.strafeToAttack boolean? (Default: `false`) */
	strafeToAttack = udTable.GetBool("strafeToAttack", false);
	/*** @field UnitDef.stopToAttack boolean? (Default: `false`) */
	stopToAttack = udTable.GetBool("stopToAttack", false);


	// initialize the (per-unitdef) collision-volume
	// all CUnit instances hold a copy of this object
	ParseCollisionVolume(udTable);
	ParseSelectionVolume(udTable);

	{
		/*** @field UnitDef.buildOptions table? */
		const LuaTable& buildsTable = udTable.SubTable("buildOptions");
		/*** @field UnitDef.customParams table<string, any>? */
		const LuaTable& paramsTable = udTable.SubTable("customParams");

		if (buildsTable.IsValid())
			buildsTable.GetMap(buildOptions);

		// custom parameters table
		paramsTable.GetMap(customParams);
	}
	{
		/*** @class UnitDefSFXTypes */
		/*** @field UnitDef.SFXTypes UnitDefSyxTypes? */
		const LuaTable&      sfxTable =  udTable.SubTable("SFXTypes");
		/*** @field UnitDefSFXTypes.explosionGenerators string[]? */
		const LuaTable& modelCEGTable = sfxTable.SubTable(     "explosionGenerators");
		/*** @field UnitDefSFXTypes.pieceExplosionGenerators string[]? */
		const LuaTable& pieceCEGTable = sfxTable.SubTable("pieceExplosionGenerators");
		/*** @field UnitDefSFXTypes.crashExplosionGenerators string[]? */
		const LuaTable& crashCEGTable = sfxTable.SubTable("crashExplosionGenerators");

		std::vector<int> cegKeys;
		std::array<const LuaTable*, 3> cegTbls = {&modelCEGTable, &pieceCEGTable, &crashCEGTable};
		std::array<char[64], MAX_UNITDEF_EXPGEN_IDS>* cegTags[3] = {&modelCEGTags, &pieceCEGTags, &crashCEGTags};

		for (int i = 0; i < 3; i++) {
			auto& tagStrs = *cegTags[i];

			cegKeys.clear();
			cegKeys.reserve(tagStrs.size());

			cegTbls[i]->GetKeys(cegKeys);

			// get at most N tags, discard the rest
			for (unsigned int j = 0, k = 0; j < cegKeys.size() && k < tagStrs.size(); j++) {
				const std::string& tag = cegTbls[i]->GetString(cegKeys[j], "");

				if (tag.empty())
					continue;

				strncpy(tagStrs[k++], tag.c_str(), sizeof(tagStrs[0]));
			}
		}
	}
}


void UnitDef::ParseWeaponsTable(const LuaTable& weaponsTable)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const WeaponDef* noWeaponDef = weaponDefHandler->GetWeaponDef("NOWEAPON");

	for (int k = 0, w = 0; w < MAX_WEAPONS_PER_UNIT; w++) {
		LuaTable wTable;
		std::string wdName = weaponsTable.GetString(w + 1, "");

		if (wdName.empty()) {
			wTable = weaponsTable.SubTable(w + 1);
			wdName = wTable.GetString("name", "");
		}

		const WeaponDef* wd = nullptr;

		if (!wdName.empty())
			wd = weaponDefHandler->GetWeaponDef(wdName);

		if (wd == nullptr) {
			// allow any of the first three weapons to be null; these will be
			// replaced by NoWeapon's if there is a valid WeaponDef among this
			// set
			if (w <= 3)
				continue;

			// otherwise stop trying
			break;
		}

		while (k < w) {
			if (noWeaponDef == nullptr) {
				LOG_L(L_ERROR, "[%s] missing NOWEAPON for WeaponDef %s (#%d) of UnitDef %s", __func__, wdName.c_str(), w, humanName.c_str());
				break;
			}

			weapons[k++] = {noWeaponDef};
		}

		weapons[k++] = {wd, wTable};

		maxWeaponRange = std::max(maxWeaponRange, wd->range);

		if (wd->interceptor && wd->coverageRange > maxCoverage)
			maxCoverage = wd->coverageRange;

		if (wd->isShield) {
			// use the biggest shield
			if (shieldWeaponDef == nullptr || (shieldWeaponDef->shieldRadius < wd->shieldRadius))
				shieldWeaponDef = wd;
		}

		if (wd->stockpile) {
			// interceptors have priority
			if (wd->interceptor || stockpileWeaponDef == nullptr || !stockpileWeaponDef->interceptor)
				stockpileWeaponDef = wd;
		}
	}
}



void UnitDef::CreateYardMap(std::string&& yardMapStr)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// if a unit is immobile but does *not* have a yardmap
	// defined, assume it is not supposed to be a building
	// (so do not assign a default per facing)
	if (yardMapStr.empty())
		return;

	const bool highResMap = (tolower(yardMapStr[0]) == 'h');

	// determine number of characters to parse from str
	const unsigned int hxSize = xsize >> (1 - highResMap);
	const unsigned int hzSize = zsize >> (1 - highResMap);
	const unsigned int ymSize = hxSize * hzSize;

	// if high-res yardmap, start reading at second character
	unsigned int ymReadIdx = highResMap;
	unsigned int ymCopyIdx = 0;

	std::array<YardMapStatus, 256 * 256> defYardMap;

	if (ymSize > defYardMap.size()) {
		LOG_L(L_WARNING, "[%s] %s: footprint{x=%u,z=%u} too large to create %s-res yardmap", __func__, name.c_str(), xsize, zsize, highResMap? "high": "low");
		return;
	}

	yardmap.resize(xsize * zsize);
	defYardMap.fill(YARDMAP_BLOCKED);

	// read the yardmap from the LuaDef string
	while (ymReadIdx < yardMapStr.size()) {
		const char c = tolower(yardMapStr[ymReadIdx++]);

		if (isspace(c))
			continue;
		// continue rather than break s.t. the excess-count can be shown
		if ((ymCopyIdx++) >= ymSize)
			continue;

		switch (c) {
			case 'g': { defYardMap[ymCopyIdx - 1] = YARDMAP_GEO;          needGeo = true; } break;
			case 'j': { defYardMap[ymCopyIdx - 1] = YARDMAP_GEOSTACKABLE; needGeo = true; } break;
			case 'y': { defYardMap[ymCopyIdx - 1] = YARDMAP_OPEN;                         } break;
			case 's': { defYardMap[ymCopyIdx - 1] = YARDMAP_STACKABLE;                    } break;
			case 'c': { defYardMap[ymCopyIdx - 1] = YARDMAP_YARD;                         } break;
			case 'i': { defYardMap[ymCopyIdx - 1] = YARDMAP_YARDINV;                      } break;
			case 'b': { defYardMap[ymCopyIdx - 1] = YARDMAP_BUILDONLY;                    } break;
			case 'u': { defYardMap[ymCopyIdx - 1] = YARDMAP_UNBUILDABLE;                  } break;
			case 'e': { defYardMap[ymCopyIdx - 1] = YARDMAP_EXITONLY;                     } break;
			case 'w':
			case 'x':
			case 'f':
			case 'o': { defYardMap[ymCopyIdx - 1] = YARDMAP_BLOCKED;                      } break;
			default : {                                                                   } break;
		}
	}

	// print warnings
	if (ymCopyIdx > ymSize)
		LOG_L(L_WARNING, "[%s] %s: given yardmap contains %u excess char(s)!", __func__, name.c_str(), ymCopyIdx - ymSize);

	if (ymCopyIdx > 0 && ymCopyIdx < ymSize)
		LOG_L(L_WARNING, "[%s] %s: given yardmap requires %u extra char(s)!", __func__, name.c_str(), ymSize - ymCopyIdx);

	// write the final yardmap at blocking-map resolution
	// (in case of a high-res map this becomes a 1:1 copy,
	// otherwise the given yardmap will be upsampled 2:1)
	for (unsigned int bmz = 0; bmz < zsize; bmz++) {
		for (unsigned int bmx = 0; bmx < xsize; bmx++) {
			const unsigned int ymx = bmx >> (1 - highResMap);
			const unsigned int ymz = bmz >> (1 - highResMap);

			yardmap[bmx + bmz * xsize] = defYardMap[ymx + ymz * hxSize];
		}
	}
}



void UnitDef::SetNoCost(bool noCost)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (noCost) {
		// initialized from UnitDefHandler::PushNewUnitDef
		realCost         = cost;
		realUpkeep       = upkeep;
		realBuildTime    = buildTime;

		cost         =  1.0f;
		buildTime    = 10.0f;
		upkeep       =  0.0f;
	} else {
		cost         = realCost;
		buildTime    = realBuildTime;
		upkeep       = realUpkeep;
	}
}

bool UnitDef::HasBomberWeapon(unsigned int idx) const {
	RECOIL_DETAILED_TRACY_ZONE;
	// checked by Is*AirUnit
	assert(HasWeapon(idx));
	return (weapons[idx].def->IsAircraftWeapon());
}

