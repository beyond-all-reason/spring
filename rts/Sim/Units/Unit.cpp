/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDef.h"
#include "Unit.h"
#include "UnitHandler.h"
#include "UnitDefHandler.h"
#include "UnitLoader.h"
#include "UnitMemPool.h"
#include "UnitToolTipMap.hpp"
#include "UnitTypes/Building.h"
#include "UnitTypes/ExtractorBuilding.h"
#include "Scripts/NullUnitScript.h"
#include "Scripts/UnitScriptFactory.h"
#include "Scripts/CobInstance.h" // for TAANG2RAD

#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "CommandAI/AirCAI.h"
#include "CommandAI/BuilderCAI.h"
#include "CommandAI/MobileCAI.h"
#include "CommandAI/BuilderCaches.h"

#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/Player.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

#include "Rendering/GroundFlash.h"

#include "Game/UI/Groups/Group.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/MoveTypeFactory.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/Projectiles/FlareProjectile.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/WeaponLoader.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "System/creg/DefTypes.h"
#include "System/creg/STL_List.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/SyncedPrimitive.h"

#undef near

#include "System/Misc/TracyDefs.h"

GlobalUnitParams globalUnitParams;

// See end of source for member bindings
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnit::CUnit(): CSolidObject()
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(unitMemPool.alloced(this));

	static_assert((sizeof(los) / sizeof(los[0])) == ILosType::LOS_TYPE_COUNT, "");
	static_assert((sizeof(losStatus) / sizeof(losStatus[0])) == MAX_TEAMS, "");
	static_assert((sizeof(posErrorMask) == 32), "");

	losStatus.fill(0);
	posErrorMask.fill(0xFFFFFFFF);

	fireState = FIRESTATE_FIREATWILL;
	moveState = MOVESTATE_MANEUVER;

	lastNanoAdd = gs->frameNum;
}

CUnit::~CUnit()
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(unitMemPool.mapped(this));
	// clean up if we are still under MoveCtrl here
	DisableScriptMoveType();

	// NOTE:
	//   could also do this in Update() or even in CUnitKilledCB(), but not
	//   in KillUnit() since we have to wait for deathScriptFinished there
	//   we nevertheless want the sim-frame latency between CUnitKilledCB()
	//   and the CreateWreckage() call to be as low as possible to prevent
	//   position discontinuities
	if (delayedWreckLevel >= 0)
		featureHandler.CreateWreckage({this, unitDef, featureDefHandler->GetFeatureDefByID(featureDefID),  {}, {},  -1, team, -1,  heading, buildFacing,  delayedWreckLevel - 1, 1});

	if (deathExpDamages != nullptr)
		DynDamageArray::DecRef(deathExpDamages);
	if (selfdExpDamages != nullptr)
		DynDamageArray::DecRef(selfdExpDamages);

	if (fpsControlPlayer != nullptr) {
		fpsControlPlayer->StopControllingUnit();
		assert(fpsControlPlayer == nullptr);
	}

	if (activated && unitDef->targfac)
		losHandler->IncreaseAllyTeamRadarErrorSize(allyteam);

	SetStorage(0.0f);

	// not all unit deletions run through KillUnit(),
	// but we always want to call this for ourselves
	UnBlock();

	// Remove us from our group, if we were in one
	SetGroup(nullptr);

	// delete script first so any callouts still see valid ptrs
	DeleteScript();

	spring::SafeDestruct(commandAI);
	spring::SafeDestruct(moveType);
	spring::SafeDestruct(prevMoveType);

	// ScriptCallback may reference weapons, so delete the script first
	CWeaponLoader::FreeWeapons(this);
	quadField.RemoveUnit(this);
}


void CUnit::InitStatic()
{
	RECOIL_DETAILED_TRACY_ZONE;
	globalUnitParams.empDeclineRate = 1.0f / modInfo.paralyzeDeclineRate;
	globalUnitParams.expMultiplier = modInfo.unitExpMultiplier;
	globalUnitParams.expPowerScale = modInfo.unitExpPowerScale;
	globalUnitParams.expHealthScale = modInfo.unitExpHealthScale;
	globalUnitParams.expReloadScale = modInfo.unitExpReloadScale;
	globalUnitParams.expGrade = modInfo.unitExpGrade;

	CBuilderCaches::InitStatic();
	unitToolTipMap.Clear();
}


void CUnit::SanityCheck() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	pos.AssertNaNs();
	midPos.AssertNaNs();
	relMidPos.AssertNaNs();
	preFrameTra.AssertNaNs();

	speed.AssertNaNs();

	rightdir.AssertNaNs();
	updir.AssertNaNs();
	frontdir.AssertNaNs();

	if (unitDef->IsGroundUnit()) {
		assert(pos.x >= -(float3::maxxpos * 16.0f));
		assert(pos.x <=  (float3::maxxpos * 16.0f));
		assert(pos.z >= -(float3::maxzpos * 16.0f));
		assert(pos.z <=  (float3::maxzpos * 16.0f));
	}
}

void CUnit::PreUpdate()
{
	preFrameTra = Transform{ CQuaternion::MakeFrom(GetTransformMatrix(true)), pos };
	for (auto& lmp : localModel.pieces) {
		lmp.SavePrevModelSpaceTransform();
	}
}


void CUnit::PreInit(const UnitLoadParams& params)
{
	ZoneScoped;
	// if this is < 0, UnitHandler will give us a random ID
	id = params.unitID;
	featureDefID = -1;

	unitDef = params.unitDef;

	{
		const FeatureDef* wreckFeatureDef = featureDefHandler->GetFeatureDef(unitDef->wreckName);

		if (wreckFeatureDef != nullptr) {
			featureDefID = wreckFeatureDef->id;

			while (wreckFeatureDef != nullptr) {
				wreckFeatureDef->PreloadModel();
				wreckFeatureDef = featureDefHandler->GetFeatureDefByID(wreckFeatureDef->deathFeatureDefID);
			}
		}
	}
	for (const auto& bo: unitDef->buildOptions) {
		const UnitDef* ud = unitDefHandler->GetUnitDefByName(bo.second);
		if (ud == nullptr)
			continue;
		ud->PreloadModel();
	}

	team = params.teamID;
	allyteam = teamHandler.AllyTeam(team);

	buildFacing = std::abs(params.facing) % NUM_FACINGS;
	xsize = ((buildFacing & 1) == 0) ? unitDef->xsize : unitDef->zsize;
	zsize = ((buildFacing & 1) == 1) ? unitDef->xsize : unitDef->zsize;


	localModel.SetModel(model = unitDef->LoadModel());

	collisionVolume = unitDef->collisionVolume;
	selectionVolume = unitDef->selectionVolume;

	// specialize defaults if non-custom sphere or footprint-box
	collisionVolume.InitDefault(float4(model->radius, model->height,  xsize * SQUARE_SIZE, zsize * SQUARE_SIZE));
	selectionVolume.InitDefault(float4(model->radius, model->height,  xsize * SQUARE_SIZE, zsize * SQUARE_SIZE));


	mapSquare = CGround::GetSquare((params.pos).cClampInMap());

	heading  = GetHeadingFromFacing(buildFacing);
	upright  = unitDef->upright;

	SetVelocity(params.speed);
	preFrameTra = Transform(CQuaternion::MakeFrom(GetTransformMatrix(true)), params.pos.cClampInMap(), 1.0f);
	Move(preFrameTra.t, false);

	UpdateDirVectors(!upright && IsOnGround(), false, 0.0f);
	SetMidAndAimPos(model->relMidPos, model->relMidPos, true);
	SetRadiusAndHeight(model);
	UpdateMidAndAimPos();

	buildeeRadius = (unitDef->buildeeBuildRadius >= 0.f) ? unitDef->buildeeBuildRadius : radius;

	unitHandler.AddUnit(this);
	quadField.MovedUnit(this);

	losStatus[allyteam] = LOS_ALL_MASK_BITS | LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR;

	ASSERT_SYNCED(pos);

	footprint = int2(unitDef->xsize, unitDef->zsize);

	beingBuilt = params.beingBuilt;
	mass = (beingBuilt)? mass: unitDef->mass;
	crushResistance = unitDef->crushResistance;
	power = unitDef->power;
	maxHealth = unitDef->health;
	health = beingBuilt? 0.1f: unitDef->health;
	cost = unitDef->cost;
	buildTime = unitDef->buildTime;
	armoredMultiple = unitDef->armoredMultiple;
	armorType = unitDef->armorType;
	category = unitDef->category;
	leaveTracks = unitDef->decalDef.leaveTrackDecals;

	unitToolTipMap.Set(id, unitDef->humanName + " - " + unitDef->tooltip);


	// sensor parameters
	realLosRadius    = std::clamp(int(unitDef->losRadius)    , 0, MAX_UNIT_SENSOR_RADIUS);
	realAirLosRadius = std::clamp(int(unitDef->airLosRadius) , 0, MAX_UNIT_SENSOR_RADIUS);
	radarRadius      = std::clamp(    unitDef->radarRadius   , 0, MAX_UNIT_SENSOR_RADIUS);
	sonarRadius      = std::clamp(    unitDef->sonarRadius   , 0, MAX_UNIT_SENSOR_RADIUS);
	jammerRadius     = std::clamp(    unitDef->jammerRadius  , 0, MAX_UNIT_SENSOR_RADIUS);
	sonarJamRadius   = std::clamp(    unitDef->sonarJamRadius, 0, MAX_UNIT_SENSOR_RADIUS);
	seismicRadius    = std::clamp(    unitDef->seismicRadius , 0, MAX_UNIT_SENSOR_RADIUS);
	seismicSignature = unitDef->seismicSignature;

	stealth = unitDef->stealth;
	sonarStealth = unitDef->sonarStealth;

	// can be overridden by cloak orders during construction
	wantCloak |= unitDef->startCloaked;
	decloakDistance = unitDef->decloakDistance;

	flankingBonusMode        = unitDef->flankingBonusMode;
	flankingBonusDir         = unitDef->flankingBonusDir;
	flankingBonusMobility    = unitDef->flankingBonusMobilityAdd * 1000;
	flankingBonusMobilityAdd = unitDef->flankingBonusMobilityAdd;
	flankingBonusAvgDamage   = (unitDef->flankingBonusMax + unitDef->flankingBonusMin) * 0.5f;
	flankingBonusDifDamage   = (unitDef->flankingBonusMax - unitDef->flankingBonusMin) * 0.5f;

	useHighTrajectory = (unitDef->highTrajectoryType == 1);

	harvestStorage = unitDef->harvestStorage;

	moveType = MoveTypeFactory::GetMoveType(this, unitDef);
	script = CUnitScriptFactory::CreateScript(this, unitDef);

	if (unitDef->selfdExpWeaponDef != nullptr)
		selfdExpDamages = DynDamageArray::IncRef(&unitDef->selfdExpWeaponDef->damages);
	if (unitDef->deathExpWeaponDef != nullptr)
		deathExpDamages = DynDamageArray::IncRef(&unitDef->deathExpWeaponDef->damages);

	commandAI = CUnitLoader::NewCommandAI(this, unitDef);
}


void CUnit::PostInit(const CUnit* builder)
{
	ZoneScoped;
	CWeaponLoader::LoadWeapons(this);
	CWeaponLoader::InitWeapons(this);

	// does nothing for LUS, calls Create+SetMaxReloadTime for COB
	script->Create();

	// all units are blocking (ie. register on the blocking-map
	// when not flying) except mines, since their position would
	// be given away otherwise by the PF, etc.
	// NOTE: this does mean that mines can be stacked indefinitely
	// (an extra yardmap character would be needed to prevent this)
	immobile = unitDef->IsImmobileUnit();

	UpdateCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS, unitDef->collidable && (!immobile || !unitDef->canKamikaze));
	Block();

	// done once again in UnitFinished() too
	// but keep the old behavior for compatibility purposes
	if (unitDef->windGenerator > 0.0f)
		envResHandler.AddGenerator(this);

	UpdateTerrainType();
	UpdatePhysicalState(0.1f);
	UpdatePosErrorParams(true, true);

	if (FloatOnWater() && IsInWater())
		Move(UpVector * (std::max(CGround::GetHeightReal(pos.x, pos.z), -moveType->GetWaterline()) - pos.y), true);

	if (unitDef->canmove || unitDef->builder) {
		if (unitDef->moveState <= MOVESTATE_NONE) {
			// always inherit our builder's movestate
			// if none, set CUnit's default (maneuver)
			if (builder != nullptr)
				moveState = builder->moveState;

		} else {
			// use our predefined movestate
			moveState = unitDef->moveState;
		}

		commandAI->GiveCommand(Command(CMD_MOVE_STATE, 0, moveState));
	}

	if (commandAI->CanChangeFireState()) {
		if (unitDef->fireState <= FIRESTATE_NONE) {
			// inherit our builder's firestate (if it is a factory)
			// if no builder, CUnit's default (fire-at-will) is set
			if (builder != nullptr && dynamic_cast<CFactoryCAI*>(builder->commandAI) != nullptr)
				fireState = builder->fireState;

		} else {
			// use our predefined firestate
			fireState = unitDef->fireState;
		}

		commandAI->GiveCommand(Command(CMD_FIRE_STATE, 0, fireState));
	}

	eventHandler.RenderUnitPreCreated(this);

	// Lua might call SetUnitHealth within UnitCreated
	// and trigger FinishedBuilding before we get to it
	const bool preBeingBuilt = beingBuilt;

	// these must precede UnitFinished from FinishedBuilding
	eventHandler.UnitCreated(this, builder);
	eoh->UnitCreated(*this, builder);

	// skip past the gradual build-progression
	if (!preBeingBuilt && !beingBuilt)
		FinishedBuilding(true);

	eventHandler.RenderUnitCreated(this, isCloaked);
}


void CUnit::PostLoad()
{
	RECOIL_DETAILED_TRACY_ZONE;
	eventHandler.RenderUnitPreCreated(this);
	eventHandler.RenderUnitCreated(this, isCloaked);
}

//////////////////////////////////////////////////////////////////////
//

void CUnit::FinishedBuilding(bool postInit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!beingBuilt && !postInit)
		return;

	beingBuilt = false;
	buildProgress = 1.0f;
	mass = unitDef->mass;

	if (soloBuilder != nullptr) {
		DeleteDeathDependence(soloBuilder, DEPENDENCE_BUILDER);
		soloBuilder = nullptr;
	}

	if (isDead) // Lua can kill a freshy spawned unit in UnitCreated
		return;

	ChangeLos(realLosRadius, realAirLosRadius);

	if (unitDef->activateWhenBuilt)
		Activate();

	SetStorage(unitDef->storage);

	// Sets the frontdir in sync with heading.
	UpdateDirVectors(!upright && IsOnGround(), false, 0.0f);

	if (unitDef->windGenerator > 0.0f) {
		// trigger sending the wind update by removing
		envResHandler.DelGenerator(this);
		//  and adding back this windgen
		envResHandler.AddGenerator(this);
	}

	eventHandler.UnitFinished(this);
	eoh->UnitFinished(*this);

	{
		if (!unitDef->isFeature)
			return;

		CFeature* f = featureHandler.CreateWreckage({this, nullptr, featureDefHandler->GetFeatureDefByID(featureDefID),  {}, {},  -1, team, allyteam,  heading, buildFacing,  0, 0});

		if (f == nullptr)
			return;

		f->blockHeightChanges = true;

		UnBlock();
		KillUnit(nullptr, false, true, -CSolidObject::DAMAGE_TURNED_INTO_FEATURE);
	}
}


void CUnit::KillUnit(CUnit* attacker, bool selfDestruct, bool reclaimed, int weaponDefID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (IsCrashing() && !beingBuilt)
		return;

	ForcedKillUnit(attacker, selfDestruct, reclaimed, weaponDefID);
}

void CUnit::ForcedKillUnit(CUnit* attacker, bool selfDestruct, bool reclaimed, int weaponDefID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (isDead)
		return;

	isDead = true;

	// release attached units
	ReleaseTransportees(attacker, selfDestruct, reclaimed);

	// pre-destruction event; unit may be kept around for its death sequence
	eventHandler.UnitDestroyed(this, attacker, weaponDefID);
	eoh->UnitDestroyed(*this, attacker, weaponDefID);

	if (unitDef->windGenerator > 0.0f)
		envResHandler.DelGenerator(this);

	blockHeightChanges = false;
	deathScriptFinished = (reclaimed || beingBuilt);

	if (deathScriptFinished)
		return;

	const WeaponDef* wd = selfDestruct? unitDef->selfdExpWeaponDef: unitDef->deathExpWeaponDef;
	const DynDamageArray* da = selfDestruct? selfdExpDamages: deathExpDamages;

	if (wd != nullptr) {
		assert(da != nullptr);
		const CExplosionParams params = {
			.pos                  = pos,
			.dir                  = ZeroVector,
			.damages              = *da,
			.weaponDef            = wd,
			.owner                = this,
			.hitUnit              = nullptr,
			.hitFeature           = nullptr,
			.craterAreaOfEffect   = da->craterAreaOfEffect,
			.damageAreaOfEffect   = da->damageAreaOfEffect,
			.edgeEffectiveness    = da->edgeEffectiveness,
			.explosionSpeed       = da->explosionSpeed,
			.gfxMod               = (da->GetDefault() > 500.0f)? 1.0f: 2.0f,
			.maxGroundDeformation = 0.0f,
			.impactOnly           = false,
			.ignoreOwner          = false,
			.damageGround         = true,
			.projectileID         = static_cast<uint32_t>(-1u)
		};

		helper->Explosion(params);
	}

	recentDamage += (maxHealth * 2.0f * selfDestruct);

	// start running the unit's kill-script
	script->Killed();
}


void CUnit::ForcedMove(const float3& newPos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	UnBlock();
	preFrameTra = Transform(CQuaternion::MakeFrom(GetTransformMatrix(true)), newPos, 1.0f);
	Move(preFrameTra.t - pos, true);
	Block();

	eventHandler.UnitMoved(this);
	quadField.MovedUnit(this);
}



float3 CUnit::GetErrorVector(int argAllyTeam) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// false indicates LuaHandle without full read access
	if (!teamHandler.IsValidAllyTeam(argAllyTeam))
		return (posErrorVector * losHandler->GetBaseRadarErrorSize() * 2.0f);

	const int atErrorMask = GetPosErrorBit(argAllyTeam);
	const int atSightMask = losStatus[argAllyTeam];

	const int isVisible = 2 * ((atSightMask & LOS_INLOS  ) != 0 ||                  teamHandler.Ally(argAllyTeam, allyteam)); // in LOS or allied, no error
	const int seenGhost = 4 * ((atSightMask & LOS_PREVLOS) != 0 && gameSetup->ghostedBuildings && unitDef->IsBuildingUnit()); // seen ghosted building, no error
	const int isOnRadar = 8 * ((atSightMask & LOS_INRADAR) != 0                                                            ); // current radar contact

	float errorMult = 0.0f;

	switch (isVisible | seenGhost | isOnRadar) {
		case  0: { errorMult = losHandler->GetBaseRadarErrorSize() * 2.0f        ; } break; //  !isVisible && !seenGhost  && !isOnRadar
		case  8: { errorMult = losHandler->GetAllyTeamRadarErrorSize(argAllyTeam); } break; //  !isVisible && !seenGhost  &&  isOnRadar
		default: {                                                                 } break; // ( isVisible ||  seenGhost) && !isOnRadar
	}

	return (posErrorVector * errorMult * (atErrorMask != 0));
}

void CUnit::UpdatePosErrorParams(bool updateError, bool updateDelta)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// every frame, magnitude of error increases
	// error-direction is fixed until next delta
	if (updateError)
		posErrorVector += posErrorDelta;
	if (!updateDelta)
		return;

	if ((--nextPosErrorUpdate) > 0)
		return;

	constexpr float errorScale = 1.0f / 256.0f;
	constexpr float errorMults[] = {1.0f, -1.0f};

	float3 newPosError = gsRNG.NextVector();

	newPosError.y *= 0.2f;
	newPosError *= errorMults[posErrorVector.dot(newPosError) < 0.0f];

	posErrorDelta = (newPosError - posErrorVector) * errorScale;
	nextPosErrorUpdate = UNIT_SLOWUPDATE_RATE;
}

void CUnit::Drop(const float3& parentPos, const float3& parentDir, CUnit* parent)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// drop unit from position
	fallSpeed = mix(unitDef->unitFallSpeed, parent->unitDef->fallSpeed, unitDef->unitFallSpeed <= 0.0f);

	frontdir = parentDir * XZVector;

	SetVelocityAndSpeed(speed * XZVector);
	Move(UpVector * ((parentPos.y - height) - pos.y), true);
	UpdateMidAndAimPos();
	SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FALLING);

	// start parachute animation
	script->Falling();
}


void CUnit::DeleteScript()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (script != &CNullUnitScript::value)
		spring::SafeDestruct(script);

	script = &CNullUnitScript::value;
}

void CUnit::EnableScriptMoveType()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (UsingScriptMoveType())
		return;

	prevMoveType = moveType;
	prevMoveType->Disconnect();
	moveType = MoveTypeFactory::GetScriptMoveType(this);
}

void CUnit::DisableScriptMoveType()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!UsingScriptMoveType())
		return;

	spring::SafeDestruct(moveType);

	moveType = prevMoveType;
	moveType->Connect();
	prevMoveType = nullptr;

	// ensure unit does not try to move back to the
	// position it was at when MoveCtrl was enabled
	// FIXME: prevent the issuing of extra commands?
	if (moveType == nullptr)
		return;

	moveType->SetGoal(moveType->oldPos = pos);
	moveType->StopMoving();
}


void CUnit::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	ASSERT_SYNCED(pos);

	UpdatePhysicalState(0.1f);
	UpdatePosErrorParams(true, false);

	if (beingBuilt)
		return;
	if (isDead)
		return;

	recentDamage *= 0.9f;
	flankingBonusMobility += flankingBonusMobilityAdd;

	if (IsStunned()) {
		// paralyzed weapons shouldn't reload
		for (CWeapon* w: weapons) {
			++(w->reloadStatus);
		}

		return;
	}

	restTime += 1;
}

void CUnit::UpdateWeaponVectors()
{
	ZoneScoped;

	if (!CanUpdateWeapons())
		return;

	for (CWeapon* w : weapons) {
		w->UpdateWeaponErrorVector();
		w->UpdateWeaponVectors();
	}
}

void CUnit::UpdateWeapons()
{
	ZoneScoped;

	if (!CanUpdateWeapons())
			return;

	for (CWeapon* w: weapons) {
		w->Update();
	}
}

void CUnit::UpdateTransportees()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (TransportedUnit& tu: transportedUnits) {
		CUnit* transportee = tu.unit;

		transportee->mapSquare = mapSquare;

		float3 relPiecePos = ZeroVector;
		float3 absPiecePos = pos;

		if (tu.piece >= 0) {
			relPiecePos = script->GetPiecePos(tu.piece);
			absPiecePos = this->GetObjectSpacePos(relPiecePos);
		}

		if (unitDef->holdSteady) {
			// slave transportee orientation to piece
			if (tu.piece >= 0) {
				const CMatrix44f& transMat = GetTransformMatrix(true);
				const auto pieceMat = script->GetPieceMatrix(tu.piece);

				transportee->SetDirVectors(transMat * pieceMat);
			}
		} else {
			// slave transportee orientation to body
			transportee->heading  = heading;
			transportee->updir    = updir;
			transportee->frontdir = frontdir;
			transportee->rightdir = rightdir;
		}

		transportee->Move(absPiecePos, false);
		transportee->UpdateMidAndAimPos();
		transportee->SetHeadingFromDirection();

		// see ::AttachUnit
		if (transportee->IsStunned()) {
			quadField.MovedUnit(transportee);
		}
	}
}

void CUnit::ReleaseTransportees(CUnit* attacker, bool selfDestruct, bool reclaimed)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (TransportedUnit& tu: transportedUnits) {
		CUnit* transportee = tu.unit;
		assert(transportee != this);

		if (transportee->isDead)
			continue;

		transportee->SetTransporter(nullptr);
		transportee->DeleteDeathDependence(this, DEPENDENCE_TRANSPORTER);
		transportee->UpdateVoidState(false);

		if (!unitDef->releaseHeld) {
			// we don't want transportees to leave a corpse
			if (!selfDestruct)
				transportee->DoDamage(DamageArray(1e6f), ZeroVector, nullptr, -CSolidObject::DAMAGE_TRANSPORT_KILLED, -1);

			transportee->KillUnit(attacker, selfDestruct, reclaimed, -CSolidObject::DAMAGE_TRANSPORT_KILLED);
		} else {
			// NOTE: game's responsibility to deal with edge-cases now
			transportee->Move(transportee->pos.cClampInBounds(), false);

			// if this transporter uses the piece-underneath-ground
			// method to "hide" transportees, place transportee near
			// the transporter's place of death
			if (transportee->pos.y < CGround::GetHeightReal(transportee->pos.x, transportee->pos.z)) {
				const float r1 = transportee->radius + radius;
				const float r2 = r1 * std::max(unitDef->unloadSpread, 1.0f);

				// try to unload in a presently unoccupied spot
				// (if no such spot, unload on transporter wreck)
				for (int i = 0; i < 10; ++i) {
					float3 pos = transportee->pos;
					pos.x += (gsRNG.NextFloat() * 2.0f * r2 - r2);
					pos.z += (gsRNG.NextFloat() * 2.0f * r2 - r2);
					pos.y = CGround::GetHeightReal(pos.x, pos.z);

					if (!pos.IsInBounds())
						continue;

					if (quadField.NoSolidsExact(pos, transportee->radius + 2.0f, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS)) {
						transportee->Move(pos, false);
						break;
					}
				}
			} else {
				if (transportee->unitDef->IsGroundUnit()) {
					transportee->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
					transportee->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);
				}
			}

			transportee->moveType->SlowUpdate();
			transportee->moveType->LeaveTransport();

			// issue a move order so that units dropped from flying
			// transports won't try to return to their pick-up spot
			if (unitDef->canfly && transportee->unitDef->canmove)
				transportee->commandAI->GiveCommand(Command(CMD_MOVE, transportee->pos));

			transportee->SetStunned(transportee->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? transportee->maxHealth: transportee->health));
			transportee->SetVelocityAndSpeed(speed * (0.5f + 0.5f * gsRNG.NextFloat()));

			eventHandler.UnitUnloaded(transportee, this);
		}
	}

	transportedUnits.clear();
}

void CUnit::TransporteeKilled(const CObject* o)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto pred = [&](const TransportedUnit& tu) { return (tu.unit == o); };
	const auto iter = std::find_if(transportedUnits.begin(), transportedUnits.end(), pred);

	if (iter == transportedUnits.end())
		return;

	const CUnit* unit = iter->unit;

	transportCapacityUsed -= (unit->xsize / SPRING_FOOTPRINT_SCALE);
	transportMassUsed -= unit->mass;

	SetMass(mass - unit->mass);

	*iter = transportedUnits.back();
	transportedUnits.pop_back();
}

void CUnit::UpdateResources()
{
	RECOIL_DETAILED_TRACY_ZONE;
	resourcesMake = resourcesMakeI + resourcesMakeOld;
	resourcesUse  = resourcesUseI  + resourcesUseOld;

	resourcesMakeOld = resourcesMakeI;
	resourcesUseOld  = resourcesUseI;

	resourcesMakeI = resourcesUseI = 0.0f;
}

void CUnit::SetLosStatus(int at, unsigned short newStatus)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const unsigned short currStatus = losStatus[at];
	const unsigned short diffBits = (currStatus ^ newStatus);

	// add to the state before running the callins
	//
	// note that is not symmetric: UnitEntered* and
	// UnitLeft* are after-the-fact events, yet the
	// Left* call-ins would still see the old state
	// without first clearing the IN{LOS, RADAR} bit
	losStatus[at] |= newStatus;

	if (diffBits) {
		if (diffBits & LOS_INLOS) {
			if (newStatus & LOS_INLOS) {
				eventHandler.UnitEnteredLos(this, at);
				eoh->UnitEnteredLos(*this, at);
			} else {
				// clear before sending the event
				losStatus[at] &= ~LOS_INLOS;

				eventHandler.UnitLeftLos(this, at);
				eoh->UnitLeftLos(*this, at);
			}
		}

		if (diffBits & LOS_INRADAR) {
			if (newStatus & LOS_INRADAR) {
				eventHandler.UnitEnteredRadar(this, at);
				eoh->UnitEnteredRadar(*this, at);
			} else {
				// clear before sending the event
				losStatus[at] &= ~LOS_INRADAR;

				eventHandler.UnitLeftRadar(this, at);
				eoh->UnitLeftRadar(*this, at);
			}
		}
	}

	// remove from the state after running the callins
	losStatus[at] &= newStatus;
}


unsigned short CUnit::CalcLosStatus(int at)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const unsigned short currStatus = losStatus[at];

	unsigned short newStatus = currStatus;
	unsigned short mask = ~(currStatus >> LOS_MASK_SHIFT);

	if (losHandler->InLos(this, at)) {
		newStatus |= (mask & (LOS_INLOS   | LOS_INRADAR |
		                      LOS_PREVLOS | LOS_CONTRADAR));
	}
	else if (losHandler->InRadar(this, at)) {
		newStatus |=  (mask & LOS_INRADAR);
		newStatus &= ~(mask & LOS_INLOS);
	}
	else {
		newStatus &= ~(mask & (LOS_INLOS | LOS_INRADAR | LOS_CONTRADAR));
	}

	return newStatus;
}


void CUnit::UpdateLosStatus(int at)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const unsigned short currStatus = losStatus[at];
	if ((currStatus & LOS_ALL_MASK_BITS) == LOS_ALL_MASK_BITS) {
		return; // no need to update, all changes are masked
	}
	SetLosStatus(at, CalcLosStatus(at));
}


void CUnit::SetStunned(bool stun) {
	RECOIL_DETAILED_TRACY_ZONE;
	stunned = stun;

	if (moveType->progressState == AMoveType::Active) {
		if (stunned) {
			script->StopMoving();
		} else {
			script->StartMoving(moveType->IsReversing());
		}
	}

	eventHandler.UnitStunned(this, stun);
}


void CUnit::SlowUpdate()
{
	ZoneScoped;
	UpdatePosErrorParams(false, true);

	DoWaterDamage();

	if (health < 0.0f) {
		KillUnit(nullptr, false, true, -CSolidObject::DAMAGE_NEGATIVE_HEALTH);
		return;
	}

	repairAmount = 0.0f;

	if (paralyzeDamage > 0.0f) {
		// NOTE: the paralysis degradation-rate has to vary, because
		// when units are paralyzed based on their current health (in
		// DoDamage) we potentially start decaying from a lower damage
		// level and would otherwise be de-paralyzed more quickly than
		// specified by <paralyzeTime>
		paralyzeDamage -= ((modInfo.paralyzeOnMaxHealth? maxHealth: health) * (UNIT_SLOWUPDATE_RATE * INV_GAME_SPEED) * globalUnitParams.empDeclineRate);
		paralyzeDamage = std::max(paralyzeDamage, 0.0f);
	}

	UpdateResources();

	if (IsStunned()) {
		// call this because we can be pushed into a different quad while stunned
		// which would make us invulnerable to most non-/small-AOE weapon impacts
		static_cast<AMoveType*>(moveType)->SlowUpdate();

		const bool notStunned = (paralyzeDamage <= (modInfo.paralyzeOnMaxHealth? maxHealth: health));
		const bool inFireBase = (transporter == nullptr || !transporter->unitDef->IsTransportUnit() || transporter->unitDef->isFirePlatform);

		// de-stun only if we are not (still) inside a non-firebase transport
		if (notStunned && inFireBase)
			SetStunned(false);

		SlowUpdateCloak(true);
		return;
	}

	if (selfDCountdown > 0) {
		if ((selfDCountdown -= 1) == 0) {
			// avoid unfinished buildings making an explosion
			KillUnit(nullptr, !beingBuilt, beingBuilt, -CSolidObject::DAMAGE_SELFD_EXPIRED);
			return;
		}

		if ((selfDCountdown & 1) && (team == gu->myTeam) && !gu->spectating)
			LOG("%s: self-destruct in %is", unitDef->humanName.c_str(), (selfDCountdown >> 1) + 1);
	}

	if (beingBuilt) {
		const auto framesSinceLastNanoAdd = gs->frameNum - lastNanoAdd;
		if (modInfo.constructionDecay && (modInfo.constructionDecayTime < framesSinceLastNanoAdd)) {
			float buildDecay = buildTime * modInfo.constructionDecaySpeed;

			buildDecay = 1.0f / std::max(0.001f, buildDecay);
			buildDecay = std::min(buildProgress, buildDecay);

			health         = std::max(0.0f, health - maxHealth * buildDecay);
			buildProgress -= buildDecay;

			AddMetal(cost.metal * buildDecay, false);

			eventHandler.UnitConstructionDecayed(this
				, INV_GAME_SPEED * framesSinceLastNanoAdd
				, INV_GAME_SPEED * UNIT_SLOWUPDATE_RATE
				, buildDecay
			);

			if (health <= 0.0f || buildProgress <= 0.0f)
				KillUnit(nullptr, false, true, -CSolidObject::DAMAGE_CONSTRUCTION_DECAY);
		}
		moveType->SlowUpdate();

		ScriptDecloak(nullptr, nullptr);
		return;
	}

	// should not be run while being built
	commandAI->SlowUpdate();

	moveType->SlowUpdate();


	// FIXME: scriptMakeMetal ...?
	AddResources(resourcesUncondMake);
	UseResources(resourcesUncondUse);

	if (activated && UseResources(resourcesCondUse))
		AddResources(resourcesCondMake);

	AddResources(unitDef->resourceMake * 0.5f);

	if (activated) {
		if (UseEnergy(unitDef->upkeep.energy * 0.5f)) {
			AddMetal(unitDef->makesMetal * 0.5f);

			if (unitDef->extractsMetal > 0.0f)
				AddMetal(metalExtract * 0.5f);
		}

		UseMetal(unitDef->upkeep.metal * 0.5f);

		if (unitDef->windGenerator > 0.0f) {
			if (envResHandler.GetCurrentWindStrength() > unitDef->windGenerator) {
 				AddEnergy(unitDef->windGenerator * 0.5f);
			} else {
				AddEnergy(envResHandler.GetCurrentWindStrength() * 0.5f);
			}
		}
	}

	// FIXME: tidal part should be under "if (activated)"?
	AddEnergy((unitDef->tidalGenerator * envResHandler.GetCurrentTidalStrength()) * 0.5f);


	if (health < maxHealth) {
		health += (unitDef->idleAutoHeal * (restTime > unitDef->idleTime));
		health += unitDef->autoHeal;
		health = std::min(health, maxHealth);
	}

	SlowUpdateCloak(false);
	SlowUpdateKamikaze(fireState >= FIRESTATE_FIREATWILL);

	if (moveType->progressState == AMoveType::Active)
		DoSeismicPing(seismicSignature);

	CalculateTerrainType();
	UpdateTerrainType();
}


void CUnit::SlowUpdateWeapons()
{
	ZoneScoped;
	if (!CanUpdateWeapons())
		return;

	for (CWeapon* w: weapons) {
		w->SlowUpdate();
	}
}

void CUnit::SlowUpdateKamikaze(bool scanForTargets)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!unitDef->canKamikaze)
		return;

	// if on FAW, actively look for targets close to us
	if (scanForTargets) {
		constexpr decltype(&CGameHelper::GetEnemyUnits) helperFuncs[] = {CGameHelper::GetEnemyUnitsNoLosTest, CGameHelper::GetEnemyUnits};

		auto& helperFunc = helperFuncs[unitDef->kamikazeUseLOS];
		auto& targetIDs = helper->targetUnitIDs;

		// NB: no code reached from here [should] call[s] GameHelper::GetEnemyUnits*(..., targetIDs) while iterating
		for (size_t i = 0, n = helperFunc(pos, unitDef->kamikazeDist, allyteam, targetIDs); i < n; i++, assert(n == targetIDs.size())) {
			const CUnit* target = unitHandler.GetUnitUnsafe(targetIDs[i]);

			if (pos.SqDistance(target->pos) >= Square(unitDef->kamikazeDist))
				continue;

			if (!eventHandler.AllowUnitKamikaze(this, target, target->speed.dot(pos - target->pos) > 0.0f))
				continue;

			// (by default) self-destruct when target starts moving away from us, should maximize damage
			KillUnit(nullptr, true, false, -CSolidObject::DAMAGE_KAMIKAZE_ACTIVATED);
			return;
		}
	}

	bool near = false;
	bool kill = false;

	switch (curTarget.type) {
		case Target_Unit: {
			near = (pos.SqDistance(curTarget.unit->pos) < Square(unitDef->kamikazeDist));
			kill = (near && eventHandler.AllowUnitKamikaze(this, curTarget.unit, near));
		} break;
		case Target_Pos: {
			near = (pos.SqDistance(curTarget.groundPos) < Square(unitDef->kamikazeDist));
			kill = (near && eventHandler.AllowUnitKamikaze(this, this, near)); // weird
		} break;
		default: {
		} break;
	}

	if (!kill)
		return;

	KillUnit(nullptr, true, false, -CSolidObject::DAMAGE_KAMIKAZE_ACTIVATED);
}


float CUnit::GetFlankingDamageBonus(const float3& attackDir)
{
	RECOIL_DETAILED_TRACY_ZONE;
	float flankingBonus = 1.0f;

	if (flankingBonusMode <= 0)
		return flankingBonus;

	if (flankingBonusMode == 1) {
		// mode 1 = global coordinates, mobile
		flankingBonusDir += (attackDir * flankingBonusMobility);
		flankingBonusDir.Normalize();
		flankingBonusMobility = 0.0f;
		flankingBonus = (flankingBonusAvgDamage - attackDir.dot(flankingBonusDir) * flankingBonusDifDamage);
	} else {
		float3 adirRelative;
		adirRelative.x = attackDir.dot(rightdir);
		adirRelative.y = attackDir.dot(updir);
		adirRelative.z = attackDir.dot(frontdir);

		if (flankingBonusMode == 2) {
			// mode 2 = unit coordinates, mobile
			flankingBonusDir += (adirRelative * flankingBonusMobility);
			flankingBonusDir.Normalize();
			flankingBonusMobility = 0.0f;
		}

		// modes 2 and 3 both use this; 3 is unit coordinates, immobile
		flankingBonus = (flankingBonusAvgDamage - adirRelative.dot(flankingBonusDir) * flankingBonusDifDamage);
	}

	return flankingBonus;
}

void CUnit::DoWaterDamage()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (mapInfo->water.damage <= 0.0f)
		return;
	if (!pos.IsInBounds())
		return;
	// note: hovercraft could also use a negative waterline
	// ("hoverline"?) to avoid being damaged but that would
	// confuse GMTPathController --> damage must be removed
	// via UnitPreDamaged if not wanted
	if (!IsInWater())
		return;

	DoDamage(DamageArray(mapInfo->water.damage), ZeroVector, NULL, -DAMAGE_EXTSOURCE_WATER, -1);
}



static void AddUnitDamageStats(CUnit* unit, float damage, bool dealt)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (unit == nullptr)
		return;

	CTeam* team = teamHandler.Team(unit->team);
	TeamStatistics& stats = team->GetCurrentStats();

	if (dealt) {
		stats.damageDealt += damage;
	} else {
		stats.damageReceived += damage;
	}
}

void CUnit::ApplyDamage(CUnit* attacker, const DamageArray& damages, float& baseDamage, float& experienceMod)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (damages.paralyzeDamageTime == 0) {
		// real damage
		if (baseDamage > 0.0f) {
			// do not log overkill damage, so nukes etc do not inflate values
			AddUnitDamageStats(attacker, std::clamp(maxHealth - health, 0.0f, baseDamage), true);
			AddUnitDamageStats(this, std::clamp(maxHealth - health, 0.0f, baseDamage), false);

			health -= baseDamage;
		} else {
			// healing
			health -= baseDamage;
			health = std::min(health, maxHealth);

			if (health > paralyzeDamage && !modInfo.paralyzeOnMaxHealth) {
				SetStunned(false);
			}
		}
	} else {
		// paralyzation damage (adds reduced experience for the attacker)
		experienceMod *= 0.1f;

		// paralyzeDamage may not get higher than baseHealth * (paralyzeTime + 1),
		// which means the unit will be destunned after <paralyzeTime> seconds.
		// (maximum paralyzeTime of all paralyzer weapons which recently hit it ofc)
		//
		// rate of paralysis-damage reduction is lower if the unit has less than
		// maximum health to ensure stun-time is always equal to <paralyzeTime>
		const float baseHealth = (modInfo.paralyzeOnMaxHealth? maxHealth: health);
		const float paralysisDecayRate = baseHealth * globalUnitParams.empDeclineRate;
		const float sumParalysisDamage = paralysisDecayRate * damages.paralyzeDamageTime;
		const float maxParalysisDamage = std::max(baseHealth + sumParalysisDamage - paralyzeDamage, 0.0f);

		if (baseDamage > 0.0f) {
			// clamp the dealt paralysis-damage to [0, maxParalysisDamage]
			baseDamage = std::clamp(baseDamage, 0.0f, maxParalysisDamage);

			// no attacker gains experience from a stunned target
			experienceMod *= (1 - IsStunned());
			// increase the current level of paralysis-damage
			paralyzeDamage += baseDamage;

			if (paralyzeDamage >= baseHealth) {
				SetStunned(true);
			}
		} else {
			// no experience from healing a non-stunned target
			experienceMod *= (paralyzeDamage > 0.0f);
			// decrease ("heal") the current level of paralysis-damage
			paralyzeDamage += baseDamage;
			paralyzeDamage = std::max(paralyzeDamage, 0.0f);

			if (paralyzeDamage <= baseHealth) {
				SetStunned(false);
			}
		}
	}

	recentDamage += baseDamage;
}

void CUnit::DoDamage(
	const DamageArray& damages,
	const float3& impulse,
	CUnit* attacker,
	int weaponDefID,
	int projectileID
) {
	if (isDead)
		return;
	if (IsCrashing() || IsInVoid())
		return;

	float baseDamage = damages.Get(armorType);
	float experienceMod = globalUnitParams.expMultiplier;
	float impulseMult = 1.0f;

	const bool isCollision = (weaponDefID == -CSolidObject::DAMAGE_COLLISION_OBJECT || weaponDefID == -CSolidObject::DAMAGE_COLLISION_GROUND);
	const bool isParalyzer = (damages.paralyzeDamageTime != 0);

	if (!isCollision && baseDamage > 0.0f) {
		if (attacker != nullptr) {
			SetLastAttacker(attacker);

			// FIXME -- not the impulse direction?
			baseDamage *= GetFlankingDamageBonus((attacker->pos - pos).SafeNormalize());
		}

		baseDamage *= curArmorMultiple;
		restTime = 0; // bleeding != resting
	}

	if (eventHandler.UnitPreDamaged(this, attacker, baseDamage, weaponDefID, projectileID, isParalyzer, &baseDamage, &impulseMult))
		return;

	script->WorldHitByWeapon(-(impulse * impulseMult).SafeNormalize2D(), weaponDefID, /*inout*/ baseDamage);
	ApplyImpulse((impulse * impulseMult) / mass);
	ApplyDamage(attacker, damages, baseDamage, experienceMod);

	{
		eventHandler.UnitDamaged(this, attacker, baseDamage, weaponDefID, projectileID, isParalyzer);

		// unit might have been killed via Lua from within UnitDamaged (e.g.
		// through a recursive DoDamage call from AddUnitDamage or directly
		// via DestroyUnit); can skip the rest
		if (isDead)
			return;

		eoh->UnitDamaged(*this, attacker, baseDamage, weaponDefID, projectileID, isParalyzer);
	}

	if (!isCollision && baseDamage > 0.0f) {
		if ((attacker != nullptr) && !teamHandler.Ally(allyteam, attacker->allyteam)) {
			const float scaledExpMod = 0.1f * experienceMod * (power / attacker->power);
			const float scaledDamage = std::max(0.0f, (baseDamage + std::min(0.0f, health))) / maxHealth;
			// alternative
			// scaledDamage = (max(healthPreDamage, 0) - max(health, 0)) / maxHealth

			attacker->AddExperience(scaledExpMod * scaledDamage);
		}
	}

	if (health > 0.0f)
		return;

	KillUnit(attacker, false, false, weaponDefID);

	if (!isDead)
		return;
	if (beingBuilt)
		return;
	if (attacker == nullptr)
		return;

	if (teamHandler.Ally(allyteam, attacker->allyteam))
		return;

	CTeam* attackerTeam = teamHandler.Team(attacker->team);
	TeamStatistics& attackerStats = attackerTeam->GetCurrentStats();

	attackerStats.unitsKilled += (1 - isCollision);
}



void CUnit::ApplyImpulse(const float3& impulse) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (GetTransporter() != nullptr) {
		// transfer impulse to unit transporting us, scaled by its mass
		// assume we came here straight from DoDamage, not LuaSyncedCtrl
		GetTransporter()->ApplyImpulse((impulse * mass) / (GetTransporter()->mass));
		return;
	}

	const float3& groundNormal = CGround::GetNormal(pos.x, pos.z);
	const float3  scaledNormal = groundNormal * std::min(0.0f, impulse.dot(groundNormal)) * IsOnGround();
	const float3    modImpulse = impulse - scaledNormal;

	if (!moveType->CanApplyImpulse(modImpulse))
		return;

	CSolidObject::ApplyImpulse(modImpulse);
}



/******************************************************************************/
/******************************************************************************/

CMatrix44f CUnit::GetTransformMatrix(bool synced, bool fullread) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	float3 interPos = synced ? pos : drawPos;

	if (!synced && !fullread && !gu->spectatingFullView)
		interPos += GetErrorVector(gu->myAllyTeam);

	return (ComposeMatrix(interPos));
}

/******************************************************************************/
/******************************************************************************/

void CUnit::AddExperience(float exp)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (exp == 0.0f)
		return;

	assert((experience + exp) >= 0.0f);

	const float oldExperience = experience;
	const float oldMaxHealth = maxHealth;

	experience += exp;
	limExperience = experience / (experience + 1.0f);

	if (globalUnitParams.expGrade != 0.0f) {
		const int oldGrade = (int)(oldExperience / globalUnitParams.expGrade);
		const int newGrade = (int)(   experience / globalUnitParams.expGrade);
		if (oldGrade != newGrade) {
			eventHandler.UnitExperience(this, oldExperience);
		}
	}

	if (globalUnitParams.expPowerScale > 0.0f)
		power = unitDef->power * (1.0f + (limExperience * globalUnitParams.expPowerScale));

	if (globalUnitParams.expReloadScale > 0.0f)
		reloadSpeed = (1.0f + (limExperience * globalUnitParams.expReloadScale));

	if (globalUnitParams.expHealthScale > 0.0f) {
		maxHealth = std::max(0.1f, unitDef->health * (1.0f + (limExperience * globalUnitParams.expHealthScale)));
		health *= (maxHealth / oldMaxHealth);
	}
}


void CUnit::SetMass(float newMass)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (transporter != nullptr)
		transporter->SetMass(transporter->mass + (newMass - mass));

	CSolidObject::SetMass(newMass);
}


void CUnit::DoSeismicPing(float pingSize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (GetTransporter() != nullptr)
		return;
	if (pingSize <= 0.0f)
		return;

	const float rx = 0.5f - gsRNG.NextFloat();
	const float rz = 0.5f - gsRNG.NextFloat();

	const float3 rndVec = {rx, 0.0f, rz};

	if (!(losStatus[gu->myAllyTeam] & LOS_INLOS) && losHandler->InSeismicDistance(this, gu->myAllyTeam)) {
		const float3 errVec = rndVec * losHandler->GetAllyTeamRadarErrorSize(gu->myAllyTeam);
		const float3 pingPos = pos + errVec;

		projMemPool.alloc<CSeismicGroundFlash>(pingPos, 30, 15, 0, pingSize, 1, float3(0.8f, 0.0f, 0.0f));
	}

	for (int a = 0; a < teamHandler.ActiveAllyTeams(); ++a) {
		if (!losHandler->InSeismicDistance(this, a))
			continue;

		const float3 errVec = rndVec * losHandler->GetAllyTeamRadarErrorSize(a);
		const float3 pingPos = pos + errVec;

		eventHandler.UnitSeismicPing(this, a, pingPos, pingSize);
		eoh->SeismicPing(a, *this, pingPos, pingSize);
	}
}



void CUnit::ChangeLos(int losRad, int airRad)
{
	RECOIL_DETAILED_TRACY_ZONE;
	losRadius = losRad;
	airLosRadius = airRad;
}


bool CUnit::ChangeTeam(int newteam, ChangeType type)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (isDead)
		return false;

	// do not allow unit count violations due to team swapping
	// (this includes unit captures)
	if (unitHandler.NumUnitsByTeamAndDef(newteam, unitDef->id) >= unitDef->maxThisUnit)
		return false;

	if (!eventHandler.AllowUnitTransfer(this, newteam, type == ChangeCaptured))
		return false;

	// do not allow old player to keep controlling the unit
	if (fpsControlPlayer != nullptr) {
		fpsControlPlayer->StopControllingUnit();
		assert(fpsControlPlayer == nullptr);
	}

	const int oldteam = team;

	selectedUnitsHandler.RemoveUnit(this);
	SetGroup(nullptr);

	eventHandler.UnitTaken(this, oldteam, newteam);
	eoh->UnitCaptured(*this, oldteam, newteam);

	// remove for old allyteam
	quadField.RemoveUnit(this);


	if (type == ChangeGiven) {
		teamHandler.Team(oldteam)->RemoveUnit(this, CTeam::RemoveGiven);
		teamHandler.Team(newteam)->AddUnit(this,    CTeam::AddGiven);
	} else {
		teamHandler.Team(oldteam)->RemoveUnit(this, CTeam::RemoveCaptured);
		teamHandler.Team(newteam)->AddUnit(this,    CTeam::AddCaptured);
	}

	if (!beingBuilt) {
		teamHandler.Team(oldteam)->resStorage -= storage;
		teamHandler.Team(newteam)->resStorage += storage;
	}


	team = newteam;
	allyteam = teamHandler.AllyTeam(newteam);
	neutral = false;

	unitHandler.ChangeUnitTeam(this, oldteam, newteam);

	for (int at = 0; at < teamHandler.ActiveAllyTeams(); ++at) {
		if (teamHandler.Ally(at, allyteam)) {
			SetLosStatus(at, LOS_ALL_MASK_BITS | LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR);
		} else {
			// re-calc LOS status
			losStatus[at] = 0;
			UpdateLosStatus(at);
		}
	}

	// insert for new allyteam
	quadField.MovedUnit(this);

	eventHandler.UnitGiven(this, oldteam, newteam);
	eoh->UnitGiven(*this, oldteam, newteam);

	// reset states and clear the queues
	if (!teamHandler.AlliedTeams(oldteam, newteam))
		ChangeTeamReset();

	return true;
}


void CUnit::ChangeTeamReset()
{
	RECOIL_DETAILED_TRACY_ZONE;
	{
		std::array<int, 1 + MAX_UNITS> alliedUnitIDs;
		std::function<bool(const CObject*, int*)> alliedUnitPred = [&](const CObject* obj, int* id) {
			const CUnit* u = dynamic_cast<const CUnit*>(obj);

			if (u == nullptr)
				return false;
			if (!teamHandler.AlliedTeams(team, u->team))
				return false;

			return (*id = u->id, true);
		};

		FilterListeners(alliedUnitPred, alliedUnitIDs);

		// stop friendly units shooting at us
		for (int i = 0, n = alliedUnitIDs[0]; i < n; i++) {
			CUnit* unit = unitHandler.GetUnit(alliedUnitIDs[1 + i]);
			unit->StopAttackingAllyTeam(allyteam);
		}
		// and stop shooting at friendly ally teams
		for (int t = 0; t < teamHandler.ActiveAllyTeams(); ++t) {
			if (teamHandler.Ally(t, allyteam))
				StopAttackingAllyTeam(t);
		}
	}

	// clear the commands (newUnitCommands for factories)
	commandAI->GiveCommand(Command(CMD_STOP));

	{
		// clear the build commands for factories
		CFactoryCAI* facAI = dynamic_cast<CFactoryCAI*>(commandAI);

		if (facAI != nullptr) {
			std::vector<Command> clearCommands;
			clearCommands.reserve(facAI->commandQue.size());

			for (auto& cmd: facAI->commandQue) {
				clearCommands.emplace_back(cmd.GetID(), RIGHT_MOUSE_KEY);
			}
			for (auto& cmd: clearCommands) {
				facAI->GiveCommand(cmd);
			}
		}
	}

	{
		//FIXME reset to unitdef defaults

		// deactivate to prevent the old give metal maker trick
		// TODO remove, *A specific
		commandAI->GiveCommand(Command(CMD_ONOFF, 0, 0));
		// reset repeat state
		commandAI->GiveCommand(Command(CMD_REPEAT, 0, 0));

		// reset cloak state
		if (unitDef->canCloak)
			commandAI->GiveCommand(Command(CMD_CLOAK, 0, 0));

		// reset move state
		if (unitDef->canmove || unitDef->builder)
			commandAI->GiveCommand(Command(CMD_MOVE_STATE, 0, MOVESTATE_MANEUVER));

		// reset fire state
		if (commandAI->CanChangeFireState())
			commandAI->GiveCommand(Command(CMD_FIRE_STATE, 0, FIRESTATE_FIREATWILL));

		// reset trajectory state
		if (unitDef->highTrajectoryType > 1)
			commandAI->GiveCommand(Command(CMD_TRAJECTORY, 0, 0));
	}
}

void CUnit::SetNeutral(bool b) {
	RECOIL_DETAILED_TRACY_ZONE;
	// only intervene for units *becoming* neutral
	if (!(neutral = b))
		return;

	std::array<int, 1 + MAX_UNITS> unitIDs;
	std::function<bool(const CObject*, int*)> unitPred = [&](const CObject* obj, int* id) {
		const CUnit* u = dynamic_cast<const CUnit*>(obj);

		if (u == nullptr)
			return false;

		return (*id = u->id, true);
	};

	FilterListeners(unitPred, unitIDs);

	// stop any units targeting us manually or automatically
	// TestTarget tests for neutrality only if !isUserTarget
	for (int i = 0, n = unitIDs[0]; i < n; i++) {
		CUnit* unit = unitHandler.GetUnit(unitIDs[1 + i]);
		CCommandAI* cai = unit->commandAI;

		if (unit->curTarget.type != Target_Unit || unit->curTarget.unit != this)
			continue;

		unit->DropCurrentAttackTarget();
		cai->StopAttackingTargetIf([&](const CUnit* t) { return (t == this); });
	}
}


bool CUnit::FloatOnWater() const {
	RECOIL_DETAILED_TRACY_ZONE;
	if (moveDef != nullptr)
		return (moveDef->FloatOnWater());

	// aircraft or building
	return (unitDef->floatOnWater);
}

bool CUnit::IsIdle() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (beingBuilt)
		return false;

	return (commandAI->commandQue.empty());
}


bool CUnit::AttackUnit(CUnit* targetUnit, bool isUserTarget, bool wantManualFire, bool fpsMode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// don't self-target
	if (targetUnit == this)
		return false;

	if (targetUnit == nullptr) {
		DropCurrentAttackTarget();
		return false;
	}

	SWeaponTarget newTarget = SWeaponTarget(targetUnit, isUserTarget);
	newTarget.isManualFire = wantManualFire || fpsMode;

	if (curTarget != newTarget) {
		DropCurrentAttackTarget();
		curTarget = newTarget;
		AddDeathDependence(targetUnit, DEPENDENCE_TARGET);
	}

	bool ret = false;
	for (CWeapon* w: weapons) {
		ret |= w->Attack(curTarget);
	}
	return ret;
}

bool CUnit::AttackGround(const float3& pos, bool isUserTarget, bool wantManualFire, bool fpsMode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	SWeaponTarget newTarget = SWeaponTarget(pos, isUserTarget);
	newTarget.isManualFire = wantManualFire || fpsMode;

	if (curTarget != newTarget) {
		DropCurrentAttackTarget();
		curTarget = newTarget;
	}

	bool ret = false;
	for (CWeapon* w: weapons) {
		ret |= w->Attack(curTarget);
	}
	return ret;
}


void CUnit::DropCurrentAttackTarget()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (curTarget.type == Target_Unit)
		DeleteDeathDependence(curTarget.unit, DEPENDENCE_TARGET);

	for (CWeapon* w: weapons) {
		if (w->GetCurrentTarget() == curTarget)
			w->DropCurrentTarget();
	}

	curTarget = SWeaponTarget();
}


bool CUnit::SetSoloBuilder(CUnit* builder, const UnitDef* buildeeDef)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (builder == nullptr)
		return false;
	if (buildeeDef->canBeAssisted)
		return false;

	AddDeathDependence(soloBuilder = builder, DEPENDENCE_BUILDER);
	return true;
}

void CUnit::SetLastAttacker(CUnit* attacker)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(attacker != nullptr);

	if (teamHandler.AlliedTeams(team, attacker->team))
		return;

	if (lastAttacker != nullptr)
		DeleteDeathDependence(lastAttacker, DEPENDENCE_ATTACKER);

	lastAttackFrame = gs->frameNum;
	lastAttacker = attacker;

	AddDeathDependence(attacker, DEPENDENCE_ATTACKER);
}

void CUnit::DependentDied(CObject* o)
{
	RECOIL_DETAILED_TRACY_ZONE;
	TransporteeKilled(o);

	if (o == curTarget.unit)
		DropCurrentAttackTarget();
	if (o == soloBuilder)
		soloBuilder = nullptr;
	if (o == transporter)
		transporter  = nullptr;
	if (o == lastAttacker)
		lastAttacker = nullptr;

	const auto missileIter = std::find(incomingMissiles.begin(), incomingMissiles.end(), static_cast<CMissileProjectile*>(o));

	if (missileIter != incomingMissiles.end())
		*missileIter = nullptr;

	CSolidObject::DependentDied(o);
}



void CUnit::UpdatePhysicalState(float eps)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const bool inAir      = IsInAir();
	const bool inWater    = IsInWater();
	const bool underWater = IsUnderWater();

	CSolidObject::UpdatePhysicalState(eps);

	if (IsInAir() != inAir) {
		if (IsInAir()) {
			eventHandler.UnitEnteredAir(this);
		} else {
			eventHandler.UnitLeftAir(this);
		}
	}
	if (IsInWater() != inWater) {
		if (IsInWater()) {
			eventHandler.UnitEnteredWater(this);
		} else {
			eventHandler.UnitLeftWater(this);
		}
	}
	if (IsUnderWater() != underWater) {
		if (underWater) {
			eventHandler.UnitLeftUnderwater(this);
		} else {
			eventHandler.UnitEnteredUnderwater(this);
		}
	}
}

void CUnit::UpdateTerrainType()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (curTerrainType != lastTerrainType) {
		script->SetSFXOccupy(curTerrainType);
		lastTerrainType = curTerrainType;
	}
}

void CUnit::CalculateTerrainType()
{
	RECOIL_DETAILED_TRACY_ZONE;
	enum {
		SFX_TERRAINTYPE_NONE    = 0,
		SFX_TERRAINTYPE_WATER_A = 1,
		SFX_TERRAINTYPE_WATER_B = 2,
		SFX_TERRAINTYPE_LAND    = 4,
	};

	// optimization: there's only about one unit that actually needs this information
	// ==> why are we even bothering with it? the callin parameter barely makes sense
	if (!script->HasSetSFXOccupy())
		return;

	if (GetTransporter() != nullptr) {
		curTerrainType = SFX_TERRAINTYPE_NONE;
		return;
	}

	const float height = CGround::GetApproximateHeight(pos.x, pos.z);

	// water
	if (height < -5.0f) {
		if (upright)
			curTerrainType = SFX_TERRAINTYPE_WATER_B;
		else
			curTerrainType = SFX_TERRAINTYPE_WATER_A;

		return;
	}
	// shore
	if (height < 0.0f) {
		if (upright)
			curTerrainType = SFX_TERRAINTYPE_WATER_A;

		return;
	}

	// land (or air)
	curTerrainType = SFX_TERRAINTYPE_LAND;
}


bool CUnit::SetGroup(CGroup* newGroup, bool fromFactory, bool autoSelect)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// factory is not necessarily selected
	if (fromFactory && !selectedUnitsHandler.AutoAddBuiltUnitsToFactoryGroup())
		return false;

	CGroup* group = GetGroup();

	if (group != nullptr)
		group->RemoveUnit(this);

	if (!uiGroupHandlers[team].SetUnitGroup(id, newGroup))
		return true;

	assert(newGroup != nullptr);

	if (!newGroup->AddUnit(this)) {
		// new group did not accept us
		uiGroupHandlers[team].SetUnitGroup(id, nullptr);
		return false;
	}

	if (!autoSelect)
		return true;

	// add unit to the set of selected units iff its new group is already selected
	// and (user wants the unit to be auto-selected or the unit is not newly built)
	if (selectedUnitsHandler.IsGroupSelected(newGroup->id) && (selectedUnitsHandler.AutoAddBuiltUnitsToSelectedGroup() || !fromFactory))
		selectedUnitsHandler.AddUnit(this);

	return true;
}

const CGroup* CUnit::GetGroup() const { return uiGroupHandlers[team].GetUnitGroup(id); }
      CGroup* CUnit::GetGroup()       { return uiGroupHandlers[team].GetUnitGroup(id); }


/******************************************************************************/
/******************************************************************************/

void CUnit::TurnIntoNanoframe()
{
	if (beingBuilt)
		return;

	beingBuilt = true;
	SetStorage(0.0f);

	// make sure neighbor extractors update
	const auto extractor = dynamic_cast <CExtractorBuilding*> (this);
	if (extractor != nullptr)
		extractor->ResetExtraction();

	eventHandler.UnitReverseBuilt(this);
}

bool CUnit::AddBuildPower(CUnit* builder, float amount)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (isDead || IsCrashing())
		return false;

	// stop decaying on building AND reclaim
	lastNanoAdd = gs->frameNum;

	CTeam* builderTeam = teamHandler.Team(builder->team);

	if (amount >= 0.0f) {
		// build or repair
		if (!beingBuilt && (health >= maxHealth))
			return false;

		if (beingBuilt) {
			// build
			const float step = std::min(amount / buildTime, 1.0f - buildProgress);
			const auto resourceUse = cost * step;

			if (!builderTeam->HaveResources(resourceUse)) {
				builderTeam->resPull += resourceUse;
				return false;
			}

			if (!eventHandler.AllowUnitBuildStep(builder, this, step))
				return false;

			/* Note, eventHandler.AllowUnitBuildStep() may have
			 * changed stored resources. That is fine though. */
			if (builder->UseResources(resourceUse)) {
				health += (maxHealth * step);
				health = std::min(health, maxHealth);

				buildProgress += step;
				if (buildProgress >= 1.0f)
					FinishedBuilding(false);
			}

			return true;
		}
		else if (health < maxHealth) {
			// repair
			const float step = std::min(amount / buildTime, 1.0f - (health / maxHealth));
			const float energyUse = (cost.energy * step);
			const float energyUseScaled = energyUse * modInfo.repairEnergyCostFactor;

			if ((builderTeam->res.energy < energyUseScaled)) {
				// update the energy and metal required counts
				builderTeam->resPull.energy += energyUseScaled;
				return false;
			}

			if (!eventHandler.AllowUnitBuildStep(builder, this, step))
				return false;

	  		if (!builder->UseEnergy(energyUseScaled)) {
				return false;
			}

			repairAmount += amount;
			health += (maxHealth * step);
			health = std::min(health, maxHealth);

			return true;
		}
	} else {
		// reclaim
		if (!AllowedReclaim(builder)) {
			builder->DependentDied(this);
			return false;
		}

		const float step = std::max(amount / buildTime, -buildProgress);
		const float energyRefundStep = cost.energy * step;
		const float metalRefundStep  =  cost.metal * step;
		const float metalRefundStepScaled  =  metalRefundStep * modInfo.reclaimUnitEfficiency;
		const float energyRefundStepScaled = energyRefundStep * modInfo.reclaimUnitEnergyCostFactor;
		const float healthStep        = modInfo.reclaimUnitDrainHealth ? maxHealth * step : 0;
		const float buildProgressStep = int(modInfo.reclaimUnitMethod == 0) * step;
		const float postHealth        = health + healthStep;
		const float postBuildProgress = buildProgress + buildProgressStep;

		if (!eventHandler.AllowUnitBuildStep(builder, this, step))
			return false;

		restTime = 0;

		bool killMe = false;
		SResourceOrder order;
		order.quantum    = false;
		order.overflow   = true;
		order.use.energy = -energyRefundStepScaled;
		if (modInfo.reclaimUnitMethod == 0) {
			// gradual reclamation of invested metal
			order.add.metal = -metalRefundStepScaled;
		} else {
			// lump reclamation of invested metal
			if (postHealth <= 0.0f || postBuildProgress <= 0.0f) {
				order.add.metal = (cost.metal * buildProgress) * modInfo.reclaimUnitEfficiency;
				killMe = true; // to make 100% sure the unit gets killed, and so no resources are reclaimed twice!
			}
		}

		if (!builder->IssueResourceOrder(&order)) {
			return false;
		}

		// turn reclaimee into nanoframe (even living units)
		if (modInfo.reclaimUnitMethod == 0)
			TurnIntoNanoframe();

		// reduce health & resources
		health = postHealth;
		buildProgress = postBuildProgress;

		// reclaim finished?
		if (killMe || buildProgress <= 0.0f || health <= 0.0f) {
			health = 0.0f;
			buildProgress = 0.0f;
			KillUnit(builder, false, true, -CSolidObject::DAMAGE_RECLAIMED);
			return false;
		}

		return true;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////
//

bool CUnit::AllowedReclaim(CUnit* builder) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Don't allow the reclaim if the unit is finished and we arent allowed to reclaim it
	if (!beingBuilt) {
		if (allyteam == builder->allyteam) {
			if ((team != builder->team) && (!modInfo.reclaimAllowAllies)) return false;
		} else {
			if (!modInfo.reclaimAllowEnemies) return false;
		}
	}

	return true;
}


bool CUnit::UseMetal(float metal)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (metal < 0.0f) {
		AddMetal(-metal);
		return true;
	}

	CTeam* myTeam = teamHandler.Team(team);
	myTeam->resPull.metal += metal;

	if (myTeam->UseMetal(metal)) {
		resourcesUseI.metal += metal;
		return true;
	}

	return false;
}

void CUnit::AddMetal(float metal, bool useIncomeMultiplier)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (metal < 0.0f) {
		UseMetal(-metal);
		return;
	}

	resourcesMakeI.metal += metal;
	teamHandler.Team(team)->AddMetal(metal, useIncomeMultiplier);
}


bool CUnit::UseEnergy(float energy)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (energy < 0.0f) {
		AddEnergy(-energy);
		return true;
	}

	CTeam* myTeam = teamHandler.Team(team);
	myTeam->resPull.energy += energy;

	if (myTeam->UseEnergy(energy)) {
		resourcesUseI.energy += energy;
		return true;
	}

	return false;
}

void CUnit::AddEnergy(float energy, bool useIncomeMultiplier)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (energy < 0.0f) {
		UseEnergy(-energy);
		return;
	}
	resourcesMakeI.energy += energy;
	teamHandler.Team(team)->AddEnergy(energy, useIncomeMultiplier);
}


bool CUnit::AddHarvestedMetal(float metal)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (harvestStorage.metal <= 0.0f) {
		AddMetal(metal, false);
		return true;
	}

	if (harvested.metal >= harvestStorage.metal) {
		eventHandler.UnitHarvestStorageFull(this);
		return false;
	}

	//FIXME what do with exceeding metal?
	harvested.metal = std::min(harvested.metal + metal, harvestStorage.metal);
	if (harvested.metal >= harvestStorage.metal) {
		eventHandler.UnitHarvestStorageFull(this);
	}
	return true;
}


void CUnit::SetStorage(const SResourcePack& newStorage)
{
	RECOIL_DETAILED_TRACY_ZONE;
	teamHandler.Team(team)->resStorage -= storage;
	storage = newStorage;
	teamHandler.Team(team)->resStorage += storage;
}


bool CUnit::HaveResources(const SResourcePack& pack) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return teamHandler.Team(team)->HaveResources(pack);
}


bool CUnit::UseResources(const SResourcePack& pack)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//FIXME
	/*if (energy < 0.0f) {
		AddEnergy(-energy);
		return true;
	}*/

	CTeam* myTeam = teamHandler.Team(team);
	myTeam->resPull += pack;

	if (myTeam->UseResources(pack)) {
		resourcesUseI += pack;
		return true;
	}
	return false;
}


void CUnit::AddResources(const SResourcePack& pack, bool useIncomeMultiplier)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//FIXME
	/*if (energy < 0.0f) {
		UseEnergy(-energy);
		return true;
	}*/
	resourcesMakeI += pack;
	teamHandler.Team(team)->AddResources(pack, useIncomeMultiplier);
}


static bool CanDispatch(const CUnit* u, const CTeam* team, const SResourceOrder& order)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const bool haveEnoughResources = (team->res >= order.use);
	bool canDispatch = haveEnoughResources;

	if (order.overflow)
		return canDispatch;

	if (u->harvestStorage.empty()) {
		const bool haveEnoughStorageFree = ((order.add + team->res) <= team->resStorage);
		canDispatch = canDispatch && haveEnoughStorageFree;
	} else {
		const bool haveEnoughHarvestStorageFree = ((order.add + u->harvested) <= u->harvestStorage);
		canDispatch = canDispatch && haveEnoughHarvestStorageFree;
	}

	return canDispatch;
}


static void GetScale(const float x1, const float x2, float* scale)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float v = std::min(x1, x2);
	*scale = (x1 == 0.0f) ? *scale : std::min(*scale, v / x1);
}


static bool LimitToFullStorage(const CUnit* u, const CTeam* team, SResourceOrder* order)
{
	RECOIL_DETAILED_TRACY_ZONE;
	float scales[SResourcePack::MAX_RESOURCES];

	for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
		scales[i] = 1.0f;
		float& scale = order->separate ? scales[i] : scales[0];

		GetScale(order->use[i], team->res[i], &scale);

		if (u->harvestStorage.empty()) {
			GetScale(order->add[i], team->resStorage.res[i] - team->res[i], &scale);
		} else {
			GetScale(order->add[i], u->harvestStorage[i] - u->harvested[i], &scale);
		}
	}

	if (order->separate) {
		bool nonempty = false;
		for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
			if ((order->use[i] != 0.0f || order->add[i] != 0.0f) && scales[i] != 0.0f) nonempty = true;
			order->use[i] *= scales[i];
			order->add[i] *= scales[i];
		}
		return nonempty;
	}

	order->use *= scales[0];
	order->add *= scales[0];
	return (scales[0] != 0.0f);
}


bool CUnit::IssueResourceOrder(SResourceOrder* order)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//FIXME assert(order.use.energy >= 0.0f && order.use.metal >= 0.0f);
	//FIXME assert(order.add.energy >= 0.0f && order.add.metal >= 0.0f);

	CTeam* myTeam = teamHandler.Team(team);
	myTeam->resPull += order->use;

	// check
	if (!CanDispatch(this, myTeam, *order)) {
		if (order->quantum)
			return false;

		if (!LimitToFullStorage(this, myTeam, order))
			return false;
	}

	// use
	if (!order->use.empty()) {
		UseResources(order->use);
	}

	// add
	if (!order->add.empty()) {
		if (harvestStorage.empty()) {
			AddResources(order->add);
		} else {
			bool isFull = false;
			for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
				if (order->add[i] > 0.0f) {
					harvested[i] += order->add[i];
					harvested[i]  = std::min(harvested[i], harvestStorage[i]);
					isFull |= (harvested[i] >= harvestStorage[i]);
				}
			}

			if (isFull) {
				eventHandler.UnitHarvestStorageFull(this);
			}
		}
	}

	return true;
}


/******************************************************************************/
/******************************************************************************/

void CUnit::Activate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activated)
		return;

	activated = true;
	script->Activate();

	if (unitDef->targfac)
		losHandler->DecreaseAllyTeamRadarErrorSize(allyteam);

	if (IsInLosForAllyTeam(gu->myAllyTeam))
		Channels::General->PlayRandomSample(unitDef->sounds.activate, this);
}


void CUnit::Deactivate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!activated)
		return;

	activated = false;
	script->Deactivate();

	if (unitDef->targfac)
		losHandler->IncreaseAllyTeamRadarErrorSize(allyteam);

	if (IsInLosForAllyTeam(gu->myAllyTeam))
		Channels::General->PlayRandomSample(unitDef->sounds.deactivate, this);
}



void CUnit::UpdateWind(float x, float z, float strength)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float windHeading = ClampRad(GetHeadingFromVectorF(-x, -z) - heading * TAANG2RAD);
	const float windStrength = std::min(strength, unitDef->windGenerator);

	script->WindChanged(windHeading, windStrength);
}


void CUnit::IncomingMissile(CMissileProjectile* missile)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!unitDef->canDropFlare)
		return;

	// always drop a flare for dramatic effect; only the
	// first <N> missiles have a chance to go after this
	if (lastFlareDrop < (gs->frameNum - unitDef->flareReloadTime * GAME_SPEED))
		projMemPool.alloc<CFlareProjectile>(pos, speed, this, int((lastFlareDrop = gs->frameNum) + unitDef->flareDelay * (1 + gsRNG.NextFloat()) * 15));

	const auto missileIter = std::find(incomingMissiles.begin(), incomingMissiles.end(), nullptr);

	if (missileIter == incomingMissiles.end())
		return;

	// no risk of duplicates; only caller is MissileProjectile ctor
	AddDeathDependence(*missileIter = missile, DEPENDENCE_INCOMING);
}



void CUnit::TempHoldFire(int cmdID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (weapons.empty())
		return;
	if (!eventHandler.AllowBuilderHoldFire(this, cmdID))
		return;

	// block the {Slow}UpdateWeapons cycle
	SetHoldFire(true);

	// clear current target (if any)
	AttackUnit(nullptr, false, false);
}


#if 0
void CUnit::StopAttackingTargetIf(
	const std::function<bool(const SWeaponTarget&)>& weaponPred,
	const std::function<bool(const CUnit*)>& commandPred
) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!pred(curTarget))
		return;

	DropCurrentAttackTarget();
	commandAI->StopAttackingTargetIf(commandPred);

	for (CWeapon* w: weapons) {
		w->StopAttackingTargetIf(weaponPred);
	}
}
#endif
void CUnit::StopAttackingAllyTeam(int ally)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (lastAttacker != nullptr && lastAttacker->allyteam == ally) {
		DeleteDeathDependence(lastAttacker, DEPENDENCE_ATTACKER);
		lastAttacker = nullptr;
	}
	if (curTarget.type == Target_Unit && curTarget.unit->allyteam == ally)
		DropCurrentAttackTarget();

	commandAI->StopAttackingAllyTeam(ally);

	for (CWeapon* w: weapons) {
		w->StopAttackingAllyTeam(ally);
	}
}


bool CUnit::GetNewCloakState(bool stunCheck) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(wantCloak);

	// grab nearest enemy wrt our default decloak-distance
	// (pass NoLosTest=true s.t. gadgets can decide how to
	// react to cloaked enemy units within decloakDistance)
	// Lua code can do more elaborate scans if it wants to
	// and has access to cloakCost{Moving}/decloakDistance
	//
	// NB: for stun checks, set enemy to <this> instead of
	// a nullptr s.t. Lua can deduce the context
	const CUnit* closestEnemy = this;

	if (!stunCheck)
		closestEnemy = CGameHelper::GetClosestEnemyUnitNoLosTest(this, midPos, decloakDistance, allyteam, unitDef->decloakSpherical, modInfo.decloakRequiresLineOfSight);

	return (eventHandler.AllowUnitCloak(this, closestEnemy));
}


void CUnit::SlowUpdateCloak(bool stunCheck)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const bool oldCloak = isCloaked;
	const bool newCloak = wantCloak && GetNewCloakState(stunCheck);

	if (oldCloak != newCloak) {
		if (newCloak) {
			eventHandler.UnitCloaked(this);
		} else {
			eventHandler.UnitDecloaked(this);
		}
	}

	isCloaked = newCloak;
}


#if 0
// no use for this currently
bool CUnit::ScriptCloak()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (isCloaked)
		return true;

	if (!eventHandler.AllowUnitCloak(this, nullptr, nullptr, nullptr))
		return false;

	wantCloak = true;
	isCloaked = true;

	eventHandler.UnitCloaked(this);
	return true;
}
#endif

bool CUnit::ScriptDecloak(const CSolidObject* object, const CWeapon* weapon)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// horrific ScriptCloak asymmetry for Lua's sake
	// maintaining internal consistency requires the
	// decloak event to only fire if isCloaked
	if (!isCloaked)
		return (!wantCloak || eventHandler.AllowUnitDecloak(this, object, weapon));

	if (!eventHandler.AllowUnitDecloak(this, object, weapon))
		return false;

	// wantCloak = false;
	isCloaked = false;

	eventHandler.UnitDecloaked(this);
	return true;
}


/******************************************************************************/
/******************************************************************************/

bool CUnit::CanTransport(const CUnit* unit) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!unitDef->IsTransportUnit())
		return false;
	if (unit->GetTransporter() != nullptr)
		return false;

	if (!eventHandler.AllowUnitTransport(this, unit))
		return false;

	if (!unit->unitDef->transportByEnemy && !teamHandler.AlliedTeams(unit->team, team))
		return false;
	if (transportCapacityUsed >= unitDef->transportCapacity)
		return false;
	if (unit->unitDef->cantBeTransported)
		return false;

	// don't transport cloaked enemies
	if (unit->isCloaked && !teamHandler.AlliedTeams(unit->team, team))
		return false;

	if (unit->xsize > (unitDef->transportSize * SPRING_FOOTPRINT_SCALE))
		return false;
	if (unit->xsize < (unitDef->minTransportSize * SPRING_FOOTPRINT_SCALE))
		return false;

	if (unit->mass >= CSolidObject::DEFAULT_MASS || unit->beingBuilt)
		return false;
	if (unit->mass < unitDef->minTransportMass)
		return false;
	if ((unit->mass + transportMassUsed) > unitDef->transportMass)
		return false;

	if (!CanLoadUnloadAtPos(unit->pos, unit))
		return false;

	// check if <unit> is already (in)directly transporting <this>
	const CUnit* u = this;

	while (u != nullptr) {
		if (u == unit)
			return false;

		u = u->GetTransporter();
	}

	return true;
}


bool CUnit::AttachUnit(CUnit* unit, int piece, bool force)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(unit != this);

	if (unit->GetTransporter() == this) {
		// assume we are already transporting this unit,
		// and just want to move it to a different piece
		// with script logic (this means the UnitLoaded
		// event is only sent once)
		//
		for (TransportedUnit& tu: transportedUnits) {
			if (tu.unit == unit) {
				tu.piece = piece;
				break;
			}
		}

		unit->UpdateVoidState(piece < 0);
		return false;
	}

	// handle transfers from another transport to us
	// (can still fail depending on CanTransport())
	if (unit->GetTransporter() != nullptr)
		unit->GetTransporter()->DetachUnit(unit);

	// covers the case where unit->transporter != NULL
	if (!force && !CanTransport(unit))
		return false;

	AddDeathDependence(unit, DEPENDENCE_TRANSPORTEE);
	unit->AddDeathDependence(this, DEPENDENCE_TRANSPORTER);

	unit->SetTransporter(this);
	unit->loadingTransportId = -1;
	unit->SetStunned(!unitDef->isFirePlatform && unitDef->IsTransportUnit());
	unit->UpdateVoidState(piece < 0);

	// make sure unit does not fire etc in transport
	if (unit->IsStunned())
		selectedUnitsHandler.RemoveUnit(unit);

	unit->UnBlock();

	// do not remove unit from QF, otherwise projectiles
	// will not be able to connect with (ie. damage) it
	//
	// for NON-stunned transportees, QF position is kept
	// up-to-date by MoveType::SlowUpdate, otherwise by
	// ::Update
	//
	// quadField.RemoveUnit(unit);

	if (dynamic_cast<CBuilding*>(unit) != nullptr)
		unitLoader->RestoreGround(unit);

	if (dynamic_cast<CHoverAirMoveType*>(moveType) != nullptr)
		unit->moveType->UseHeading(false);

	TransportedUnit tu;
		tu.unit = unit;
		tu.piece = piece;

	transportCapacityUsed += unit->xsize / SPRING_FOOTPRINT_SCALE;
	transportMassUsed += unit->mass;
	SetMass(mass + unit->mass);

	transportedUnits.push_back(tu);

	unit->moveType->StopMoving(true, true);
	unit->CalculateTerrainType();
	unit->UpdateTerrainType();

	eventHandler.UnitLoaded(unit, this);
	commandAI->BuggerOff(pos, -1.0f);
	return true;
}


bool CUnit::DetachUnitCore(CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (unit->GetTransporter() != this)
		return false;

	if (unit->detached)
		return false;

	for (TransportedUnit& tu: transportedUnits) {
		if (tu.unit != unit)
			continue;

		this->DeleteDeathDependence(unit, DEPENDENCE_TRANSPORTEE);
		unit->DeleteDeathDependence(this, DEPENDENCE_TRANSPORTER);
		unit->SetTransporter(nullptr);
		unit->unloadingTransportId = id;

		if (dynamic_cast<CHoverAirMoveType*>(moveType) != nullptr)
			unit->moveType->UseHeading(true);

		// de-stun detaching units in case we are not a fire-platform
		unit->SetStunned(unit->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? unit->maxHealth: unit->health));

		unit->moveType->SlowUpdate();
		unit->moveType->LeaveTransport();

		if (CBuilding* building = dynamic_cast<CBuilding*>(unit))
			building->ForcedMove(building->pos);

		transportCapacityUsed -= unit->xsize / SPRING_FOOTPRINT_SCALE;
		transportMassUsed -= unit->mass;
		mass = std::clamp(mass - unit->mass, CSolidObject::MINIMUM_MASS, CSolidObject::MAXIMUM_MASS);

		tu = transportedUnits.back();
		transportedUnits.pop_back();

		unit->UpdateVoidState(false);
		unit->CalculateTerrainType();
		unit->UpdateTerrainType();

		eventHandler.UnitUnloaded(unit, this);
		return true;
	}

	return false;
}


bool CUnit::DetachUnit(CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (DetachUnitCore(unit)) {
		unit->Block();

		// erase command queue unless it's a wait command
		const CCommandQueue& queue = unit->commandAI->commandQue;

		if (unitDef->IsTransportUnit() && (queue.empty() || (queue.front().GetID() != CMD_WAIT)))
			unit->commandAI->GiveCommand(Command(CMD_STOP));

		return true;
	}

	return false;
}


bool CUnit::DetachUnitFromAir(CUnit* unit, const float3& pos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (DetachUnitCore(unit)) {
		unit->Drop(this->pos, this->frontdir, this);

		// add an additional move command for after we land
		if (unitDef->IsTransportUnit() && unit->unitDef->canmove)
			unit->commandAI->GiveCommand(Command(CMD_MOVE, pos));

		return true;
	}

	return false;
}

bool CUnit::CanLoadUnloadAtPos(const float3& wantedPos, const CUnit* unit, float* wantedHeightPtr) const {
	RECOIL_DETAILED_TRACY_ZONE;
	bool canLoadUnload = false;
	float wantedHeight = GetTransporteeWantedHeight(wantedPos, unit, &canLoadUnload);

	if (wantedHeightPtr != nullptr)
		*wantedHeightPtr = wantedHeight;

	return canLoadUnload;
}

float CUnit::GetTransporteeWantedHeight(const float3& wantedPos, const CUnit* unit, bool* allowedPos) const {
	RECOIL_DETAILED_TRACY_ZONE;
	bool isAllowedTerrain = true;

	float wantedHeight = unit->pos.y;
	float wantedSlope = 90.0f;
	float clampedHeight = wantedHeight;

	const UnitDef* transporteeUnitDef = unit->unitDef;
	const MoveDef* transporteeMoveDef = unit->moveDef;

	if (unit->GetTransporter() != nullptr) {
		// if unit is being transported, set <clampedHeight>
		// to the altitude at which to UNload the transportee
		wantedHeight = CGround::GetHeightReal(wantedPos.x, wantedPos.z);
		wantedSlope = CGround::GetSlope(wantedPos.x, wantedPos.z);

		if ((isAllowedTerrain = CGameHelper::CheckTerrainConstraints(transporteeUnitDef, transporteeMoveDef, wantedHeight, wantedHeight, wantedSlope, &clampedHeight))) {
			if (transporteeMoveDef != nullptr) {
				// transportee is a mobile ground unit
				switch (transporteeMoveDef->speedModClass) {
					case MoveDef::Ship: {
						wantedHeight = std::max(-unit->moveType->GetWaterline(), wantedHeight);
						clampedHeight = wantedHeight;
					} break;
					case MoveDef::Hover: {
						wantedHeight = std::max(0.0f, wantedHeight);
						clampedHeight = wantedHeight;
					} break;
					default: {
					} break;
				}
			} else {
				// transportee is a building or an airplane
				wantedHeight *= (1 - transporteeUnitDef->floatOnWater);
				clampedHeight = wantedHeight;
			}
		}

		if (dynamic_cast<const CBuilding*>(unit) != nullptr) {
			// for transported structures, <wantedPos> must be free/buildable
			// (note: TestUnitBuildSquare calls CheckTerrainConstraints again)
			BuildInfo bi(transporteeUnitDef, wantedPos, unit->buildFacing);
			bi.pos = CGameHelper::Pos2BuildPos(bi, true);
			CFeature* f = nullptr;

			if (isAllowedTerrain && (!CGameHelper::TestUnitBuildSquare(bi, f, -1, true) || f != nullptr))
				isAllowedTerrain = false;
		}
	}


	float rawContactHeight = clampedHeight + unit->height;
	float modContactHeight = rawContactHeight;

	// *we* must be capable of reaching the point-of-contact height
	// however this check fails for eg. ships that want to (un)load
	// land units on shore --> would require too many special cases
	// therefore restrict its use to transport aircraft
	if (this->moveDef == nullptr)
		isAllowedTerrain &= CGameHelper::CheckTerrainConstraints(unitDef, nullptr, rawContactHeight, rawContactHeight, 90.0f, &modContactHeight);

	if (allowedPos != nullptr)
		*allowedPos = isAllowedTerrain;

	return modContactHeight;
}

short CUnit::GetTransporteeWantedHeading(const CUnit* unit) const {
	RECOIL_DETAILED_TRACY_ZONE;
	if (unit->GetTransporter() == nullptr)
		return unit->heading;
	if (dynamic_cast<CHoverAirMoveType*>(moveType) == nullptr)
		return unit->heading;
	if (dynamic_cast<const CBuilding*>(unit) == nullptr)
		return unit->heading;

	// transported structures want to face a cardinal direction
	return (GetHeadingFromFacing(unit->buildFacing));
}

/******************************************************************************/
/******************************************************************************/

CR_BIND_DERIVED_POOL(CUnit, CSolidObject, , unitMemPool.allocMem, unitMemPool.freeMem)
CR_REG_METADATA(CUnit, (
	CR_MEMBER(unitDef),
	CR_MEMBER(shieldWeapon),
	CR_MEMBER(stockpileWeapon),
	CR_MEMBER(selfdExpDamages),
	CR_MEMBER(deathExpDamages),

	CR_MEMBER(featureDefID),

	CR_MEMBER(power),

	CR_MEMBER(paralyzeDamage),
	CR_MEMBER(captureProgress),
	CR_MEMBER(experience),
	CR_MEMBER(limExperience),

	CR_MEMBER(neutral),
	CR_MEMBER(beingBuilt),
	CR_MEMBER(upright),

	CR_MEMBER(lastAttackFrame),
	CR_MEMBER(lastFireWeapon),
	CR_MEMBER(lastFlareDrop),
	CR_MEMBER(lastNanoAdd),

	CR_MEMBER(soloBuilder),
	CR_MEMBER(lastAttacker),
	CR_MEMBER(transporter),

	CR_MEMBER(fpsControlPlayer),

	CR_MEMBER(moveType),
	CR_MEMBER(prevMoveType),

	CR_MEMBER(commandAI),
	CR_MEMBER(script),

	CR_IGNORED( usMemBuffer),
	CR_IGNORED(amtMemBuffer),
	CR_IGNORED(smtMemBuffer),
	CR_IGNORED(caiMemBuffer),

	CR_MEMBER(weapons),
	CR_IGNORED(los),
	CR_MEMBER(losStatus),
	CR_MEMBER(posErrorMask),
	CR_MEMBER(quads),


	CR_MEMBER(loadingTransportId),
	CR_MEMBER(unloadingTransportId),
	CR_MEMBER(requestRemoveUnloadTransportId),
	CR_MEMBER(transportCapacityUsed),
	CR_MEMBER(transportMassUsed),

	CR_MEMBER(buildProgress),
	CR_MEMBER(groundLevelled),
	CR_MEMBER(terraformLeft),
	CR_MEMBER(repairAmount),

	CR_MEMBER(realLosRadius),
	CR_MEMBER(realAirLosRadius),

	CR_MEMBER(inBuildStance),
	CR_MEMBER(useHighTrajectory),
	CR_MEMBER(onTempHoldFire),

	CR_MEMBER(forceUseWeapons),
	CR_MEMBER(allowUseWeapons),

	CR_MEMBER(deathScriptFinished),
	CR_MEMBER(delayedWreckLevel),

	CR_MEMBER(restTime),

	CR_MEMBER(reloadSpeed),
	CR_MEMBER(maxRange),
	CR_MEMBER(lastMuzzleFlameSize),

	CR_MEMBER(lastMuzzleFlameDir),
	CR_MEMBER(flankingBonusDir),

	CR_MEMBER(armorType),
	CR_MEMBER(category),

	CR_MEMBER(mapSquare),

	CR_MEMBER(losRadius),
	CR_MEMBER(airLosRadius),

	CR_MEMBER(radarRadius),
	CR_MEMBER(sonarRadius),
	CR_MEMBER(jammerRadius),
	CR_MEMBER(sonarJamRadius),
	CR_MEMBER(seismicRadius),
	CR_MEMBER(seismicSignature),
	CR_MEMBER(stealth),
	CR_MEMBER(sonarStealth),

	CR_MEMBER(curTarget),

	CR_MEMBER(resourcesCondUse),
	CR_MEMBER(resourcesCondMake),
	CR_MEMBER(resourcesUncondUse),
	CR_MEMBER(resourcesUncondMake),

	CR_MEMBER(resourcesUse),
	CR_MEMBER(resourcesMake),

	CR_MEMBER(resourcesUseI),
	CR_MEMBER(resourcesMakeI),
	CR_MEMBER(resourcesUseOld),
	CR_MEMBER(resourcesMakeOld),

	CR_MEMBER(storage),

	CR_MEMBER(harvestStorage),
	CR_MEMBER(harvested),

	CR_MEMBER(metalExtract),

	CR_MEMBER(cost),
	CR_MEMBER(buildTime),

	CR_MEMBER(recentDamage),

	CR_MEMBER(fireState),
	CR_MEMBER(moveState),

	CR_MEMBER(activated),

	CR_MEMBER(isDead),
	CR_MEMBER(fallSpeed),

	CR_MEMBER(flankingBonusMode),
	CR_MEMBER(flankingBonusMobility),
	CR_MEMBER(flankingBonusMobilityAdd),
	CR_MEMBER(flankingBonusAvgDamage),
	CR_MEMBER(flankingBonusDifDamage),

	CR_MEMBER(armoredState),
	CR_MEMBER(armoredMultiple),
	CR_MEMBER(curArmorMultiple),

	CR_MEMBER(posErrorVector),
	CR_MEMBER(posErrorDelta),

	CR_MEMBER(nextPosErrorUpdate),

	CR_MEMBER(wantCloak),
	CR_MEMBER(isCloaked),
	CR_MEMBER(decloakDistance),

	CR_MEMBER(lastTerrainType),
	CR_MEMBER(curTerrainType),

	CR_MEMBER(selfDCountdown),

	CR_MEMBER_UN(myIcon),
	CR_MEMBER_UN(drawIcon),

	CR_MEMBER(transportedUnits),
	CR_MEMBER(incomingMissiles),

	CR_MEMBER(cegDamage),

	CR_MEMBER_UN(noMinimap),
	CR_MEMBER_UN(leaveTracks),

	CR_MEMBER_UN(isSelected),
	CR_MEMBER(iconRadius),

	CR_MEMBER(stunned),
	CR_MEMBER_UN(noGroup),

//	CR_MEMBER(expMultiplier),
//	CR_MEMBER(expPowerScale),
//	CR_MEMBER(expHealthScale),
//	CR_MEMBER(expReloadScale),
//	CR_MEMBER(expGrade),

//	CR_MEMBER(empDecline),

	CR_POSTLOAD(PostLoad)
))

CR_BIND(CUnit::TransportedUnit,)

CR_REG_METADATA_SUB(CUnit, TransportedUnit, (
	CR_MEMBER(unit),
	CR_MEMBER(piece)
))

CR_BIND(GlobalUnitParams, )
CR_REG_METADATA(GlobalUnitParams, (
	CR_MEMBER(empDeclineRate),
	CR_MEMBER(expMultiplier	),
	CR_MEMBER(expPowerScale	),
	CR_MEMBER(expHealthScale),
	CR_MEMBER(expReloadScale),
	CR_MEMBER(expGrade      )
))
