/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>
#include <algorithm>
#include "Builder.h"
#include "Building.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "System/SpringMath.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/TerraformTask.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/Sound/ISoundChannels.h"

CR_BIND_DERIVED(CBuilder, CUnit, )
CR_REG_METADATA(CBuilder, (
	CR_MEMBER(range3D),
	CR_MEMBER(buildDistance),
	CR_MEMBER(buildSpeed),
	CR_MEMBER(repairSpeed),
	CR_MEMBER(reclaimSpeed),
	CR_MEMBER(resurrectSpeed),
	CR_MEMBER(captureSpeed),
	CR_MEMBER(terraformSpeed),
	CR_MEMBER(curResurrect),
	CR_MEMBER(lastResurrected),
	CR_MEMBER(curBuild),
	CR_MEMBER(curCapture),
	CR_MEMBER(curReclaim),
	CR_MEMBER(reclaimingUnit),
	CR_MEMBER(terraformTaskToken),
	CR_MEMBER(nanoPieceCache)
))


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBuilder::CBuilder():
	CUnit(),
	range3D(true),
	buildDistance(16),
	buildSpeed(100),
	repairSpeed(100),
	reclaimSpeed(100),
	resurrectSpeed(100),
	captureSpeed(100),
	terraformSpeed(100),
	curResurrect(0),
	lastResurrected(0),
	curBuild(0),
	curCapture(0),
	curReclaim(0),
	reclaimingUnit(false),
	terraformTaskToken(nullptr)
{}

void CBuilder::PreInit(const UnitLoadParams& params)
{
	unitDef = params.unitDef;
	range3D = unitDef->buildRange3D;
	buildDistance = (params.unitDef)->buildDistance;

	const float scale = (1.0f / TEAM_SLOWUPDATE_RATE);

	buildSpeed     = scale * unitDef->buildSpeed;
	repairSpeed    = scale * unitDef->repairSpeed;
	reclaimSpeed   = scale * unitDef->reclaimSpeed;
	resurrectSpeed = scale * unitDef->resurrectSpeed;
	captureSpeed   = scale * unitDef->captureSpeed;
	terraformSpeed = scale * unitDef->terraformSpeed;

	CUnit::PreInit(params);
}


bool CBuilder::CanAssistUnit(const CUnit* u, const UnitDef* def) const
{
	if (!unitDef->canAssist)
		return false;

	return ((def == nullptr || u->unitDef == def) && u->beingBuilt && (u->buildProgress < 1.0f) && (u->soloBuilder == nullptr || u->soloBuilder == this));
}


bool CBuilder::CanRepairUnit(const CUnit* u) const
{
	if (!unitDef->canRepair)
		return false;
	if (u->beingBuilt)
		return false;
	if (u->health >= u->maxHealth)
		return false;

	return (u->unitDef->repairable);
}

bool CBuilder::TerraformingForBuilding(TerraformTask* tt)
{
	return (tt != nullptr && tt->buildee != nullptr);
}

float3 CBuilder::TerraformCenter(TerraformTask* tt)
{
	return tt != nullptr ? tt->GetCenterPos() : float3{};
}

float CBuilder::TerraformRadius(TerraformTask* tt)
{
	return tt != nullptr ? tt->GetRadius() : 0.0f;
}

SRectangle CBuilder::GetBuildingRectangle(const CUnit& unit)
{
	const int tx1 = static_cast<int>((unit.pos.x - (unit.xsize * 0.5f * SQUARE_SIZE)) / SQUARE_SIZE);
	const int tz1 = static_cast<int>((unit.pos.z - (unit.zsize * 0.5f * SQUARE_SIZE)) / SQUARE_SIZE);
	const int tx2 = tx1 + unit.xsize;
	const int tz2 = tz1 + unit.zsize;
	return SRectangle{ tx1, tz1, tx2, tz2 };
}


bool CBuilder::UpdateTerraform(const Command&)
{
	if (!GetTerraformTask() || !inBuildStance)
		return false;

	assert(!mapDamage->Disabled());
	auto* tt = GetTerraformTask();
	assert(curBuild == tt->buildee);

	if (curBuild) {
		// prevent building from timing out while terraforming for it
		curBuild->AddBuildPower(this, 0.0f);
	}

	if (tt->TerraformComplete()) {
		if (curBuild) {
			if (eventHandler.TerraformComplete(this, curBuild)) {
				StopBuild();
			}
		}
		else {
			StopBuild();
		}
		terraformTaskToken = nullptr;
	}
	else {
		tt->AddTerraformSpeed(terraformSpeed);
	}

	if (curBuild)
		ScriptDecloak(curBuild, nullptr);

	if (tt != nullptr)
		CreateNanoParticle(TerraformCenter(tt), TerraformRadius(tt) * 0.5f, false);

	return tt != nullptr;
}

bool CBuilder::UpdateBuild(const Command& fCommand)
{
	CUnit* curBuildee = curBuild;
	CBuilderCAI* cai = static_cast<CBuilderCAI*>(commandAI);

	if (curBuildee == nullptr || !cai->IsInBuildRange(curBuildee))
		return false;

	if (fCommand.GetID() == CMD_WAIT) {
		if (curBuildee->buildProgress < 1.0f) {
			// prevent buildee from decaying (we cannot call StopBuild here)
			curBuildee->AddBuildPower(this, 0.0f);
		} else {
			// stop repairing (FIXME: should be much cleaner to let BuilderCAI
			// call this instead when a wait command is given?)
			StopBuild();
		}

		return true;
	}

	if (curBuildee->soloBuilder != nullptr && (curBuildee->soloBuilder != this)) {
		StopBuild();
		return true;
	}

	// NOTE:
	//   technically this block of code should be guarded by
	//   "if (inBuildStance)", but doing so can create zombie
	//   guarders because scripts might not set inBuildStance
	//   to true when guard or repair orders are executed and
	//   SetRepairTarget does not check for it
	//
	//   StartBuild *does* ensure construction will not start
	//   until inBuildStance is set to true by the builder's
	//   script, and there are no cases during construction
	//   when inBuildStance can become false yet the buildee
	//   should be kept from decaying, so this is free from
	//   serious side-effects (when repairing, a builder might
	//   start adding build-power before having fully finished
	//   its opening animation)
	if (!(inBuildStance || true))
		return true;

	ScriptDecloak(curBuildee, nullptr);

	// adjusted build-speed: use repair-speed on units with
	// progress >= 1 rather than raw build-speed on buildees
	// with progress < 1
	float adjBuildSpeed = buildSpeed;

	if (curBuildee->buildProgress >= 1.0f)
		adjBuildSpeed = std::min(repairSpeed, unitDef->maxRepairSpeed * 0.5f - curBuildee->repairAmount); // repair

	if (adjBuildSpeed > 0.0f && curBuildee->AddBuildPower(this, adjBuildSpeed)) {
		CreateNanoParticle(curBuildee->midPos, curBuildee->radius * 0.5f, false);
		return true;
	}

	// check if buildee finished construction
	if (curBuildee->beingBuilt || curBuildee->health < curBuildee->maxHealth)
		return true;

	StopBuild();
	return true;
}

bool CBuilder::UpdateReclaim(const Command& fCommand)
{
	// AddBuildPower can invoke StopBuild indirectly even if returns true
	// and reset curReclaim to null (which would crash CreateNanoParticle)
	CSolidObject* curReclaimee = curReclaim;

	if (curReclaimee == nullptr || f3SqDist(curReclaimee->pos, pos) >= Square(buildDistance + curReclaimee->buildeeRadius) || !inBuildStance)
		return false;

	if (fCommand.GetID() == CMD_WAIT) {
		StopBuild();
		return true;
	}

	ScriptDecloak(curReclaimee, nullptr);

	if (!curReclaimee->AddBuildPower(this, -reclaimSpeed))
		return true;

	CreateNanoParticle(curReclaimee->midPos, curReclaimee->radius * 0.7f, true, (reclaimingUnit && curReclaimee->team != team));
	return true;
}

bool CBuilder::UpdateResurrect(const Command& fCommand)
{
	CBuilderCAI* cai = static_cast<CBuilderCAI*>(commandAI);
	CFeature* curResurrectee = curResurrect;

	if (curResurrectee == nullptr || f3SqDist(curResurrectee->pos, pos) >= Square(buildDistance + curResurrectee->buildeeRadius) || !inBuildStance)
		return false;

	if (fCommand.GetID() == CMD_WAIT) {
		StopBuild();
		return true;
	}

	if (curResurrectee->udef == nullptr) {
		StopBuild(true);
		return true;
	}

	if ((modInfo.reclaimMethod != 1) && (curResurrectee->reclaimLeft < 1)) {
		// this corpse has been reclaimed a little, need to restore
		// its resources before we can let the player resurrect it
		curResurrectee->AddBuildPower(this, resurrectSpeed);
		return true;
	}

	const UnitDef* resurrecteeDef = curResurrectee->udef;

	// corpse has been restored, begin resurrection
	const float step = resurrectSpeed / resurrecteeDef->buildTime;

	const bool resurrectAllowed = eventHandler.AllowFeatureBuildStep(this, curResurrectee, step);
	const bool canExecResurrect = (resurrectAllowed && UseEnergy(resurrecteeDef->energy * step * modInfo.resurrectEnergyCostFactor));

	if (canExecResurrect) {
		curResurrectee->resurrectProgress += step;
		curResurrectee->resurrectProgress = std::min(curResurrectee->resurrectProgress, 1.0f);

		CreateNanoParticle(curResurrectee->midPos, curResurrectee->radius * 0.7f, gsRNG.NextInt(2));
	}

	if (curResurrectee->resurrectProgress < 1.0f)
		return true;

	if (!curResurrectee->deleteMe) {
		// resurrect finished and we are the first
		curResurrectee->UnBlock();

		UnitLoadParams resurrecteeParams = {resurrecteeDef, this, curResurrectee->pos, ZeroVector, -1, team, curResurrectee->buildFacing, false, false};
		CUnit* resurrectee = unitLoader->LoadUnit(resurrecteeParams);

		assert(resurrecteeDef == resurrectee->unitDef);
		resurrectee->SetSoloBuilder(this, resurrecteeDef);
		resurrectee->SetHeading(curResurrectee->heading, !resurrectee->upright && resurrectee->IsOnGround(), false, 0.0f);

		for (const int resurrecterID: cai->resurrecters) {
			CBuilder* resurrecter = static_cast<CBuilder*>(unitHandler.GetUnit(resurrecterID));
			CCommandAI* resurrecterCAI = resurrecter->commandAI;

			if (resurrecterCAI->commandQue.empty())
				continue;

			Command& c = resurrecterCAI->commandQue.front();

			if (c.GetID() != CMD_RESURRECT || c.GetNumParams() != 1)
				continue;

			if ((c.GetParam(0) - unitHandler.MaxUnits()) != curResurrectee->id)
				continue;

			if (!teamHandler.Ally(allyteam, resurrecter->allyteam))
				continue;

			// all units that were rezzing shall assist the repair too
			resurrecter->lastResurrected = resurrectee->id;

			// prevent FinishCommand from removing this command when the
			// feature is deleted, since it is needed to start the repair
			// (WTF!)
			c.SetParam(0, INT_MAX / 2);
		}

		// this takes one simframe to do the deletion
		featureHandler.DeleteFeature(curResurrectee);
	}

	StopBuild(true);
	return true;
}

bool CBuilder::UpdateCapture(const Command& fCommand)
{
	CUnit* curCapturee = curCapture;

	if (curCapturee == nullptr || f3SqDist(curCapturee->pos, pos) >= Square(buildDistance + curCapturee->buildeeRadius) || !inBuildStance)
		return false;

	if (fCommand.GetID() == CMD_WAIT) {
		StopBuild();
		return true;
	}

	if (curCapturee->team == team) {
		StopBuild(true);
		return true;
	}

	const float captureMagicNumber = (150.0f + (curCapturee->buildTime / captureSpeed) * (curCapturee->health + curCapturee->maxHealth) / curCapturee->maxHealth * 0.4f);
	const float captureProgressStep = 1.0f / captureMagicNumber;
	const float captureProgressTemp = std::min(curCapturee->captureProgress + captureProgressStep, 1.0f);

	const float captureFraction = captureProgressTemp - curCapturee->captureProgress;
	const float energyUseScaled = curCapturee->cost.energy * captureFraction * modInfo.captureEnergyCostFactor;

	const bool buildStepAllowed = (eventHandler.AllowUnitBuildStep(this, curCapturee, captureProgressStep));
	const bool captureStepAllowed = (eventHandler.AllowUnitCaptureStep(this, curCapturee, captureProgressStep));
	const bool canExecCapture = (buildStepAllowed && captureStepAllowed && UseEnergy(energyUseScaled));

	if (!canExecCapture)
		return true;

	curCapturee->captureProgress += captureProgressStep;
	curCapturee->captureProgress = std::min(curCapturee->captureProgress, 1.0f);

	CreateNanoParticle(curCapturee->midPos, curCapturee->radius * 0.7f, false, true);

	if (curCapturee->captureProgress < 1.0f)
		return true;

	if (!curCapturee->ChangeTeam(team, CUnit::ChangeCaptured)) {
		// capture failed
		if (team == gu->myTeam) {
			LOG_L(L_WARNING, "%s: Capture failed, unit type limit reached", unitDef->humanName.c_str());
			eventHandler.LastMessagePosition(pos);
		}
	}

	curCapturee->captureProgress = 0.0f;
	StopBuild(true);
	return true;
}



void CBuilder::Update()
{
	const CBuilderCAI* cai = static_cast<CBuilderCAI*>(commandAI);

	const CCommandQueue& cQueue = cai->commandQue;
	const Command& fCommand = (!cQueue.empty())? cQueue.front(): Command(CMD_STOP);

	bool updated = false;

	nanoPieceCache.Update();

	if (!beingBuilt && !IsStunned()) {
		updated = updated || UpdateTerraform(fCommand);
		updated = updated || UpdateBuild(fCommand);
		updated = updated || UpdateReclaim(fCommand);
		updated = updated || UpdateResurrect(fCommand);
		updated = updated || UpdateCapture(fCommand);
	}

	CUnit::Update();
}

void CBuilder::SetRepairTarget(CUnit* target)
{
	if (target == curBuild)
		return;

	StopBuild(false);
	TempHoldFire(CMD_REPAIR);

	curBuild = target;
	AddDeathDependence(curBuild, DEPENDENCE_BUILD);

	if (!target->groundLevelled) {
		// resume levelling the ground
		terraformTaskToken = terraformTaskHandler.AddTerraformTask(
			GetBuildingRectangle(*target), target
		);
	}

	ScriptStartBuilding(target->pos, false);
}


void CBuilder::SetReclaimTarget(CSolidObject* target)
{
	if (dynamic_cast<CFeature*>(target) != nullptr && !static_cast<CFeature*>(target)->def->reclaimable)
		return;

	CUnit* recUnit = dynamic_cast<CUnit*>(target);

	if (recUnit != nullptr && !recUnit->unitDef->reclaimable)
		return;

	if (curReclaim == target || this == target)
		return;

	StopBuild(false);
	TempHoldFire(CMD_RECLAIM);

	reclaimingUnit = (recUnit != nullptr);
	curReclaim = target;

	AddDeathDependence(curReclaim, DEPENDENCE_RECLAIM);
	ScriptStartBuilding(target->pos, false);
}


void CBuilder::SetResurrectTarget(CFeature* target)
{
	if (curResurrect == target || target->udef == nullptr)
		return;

	StopBuild(false);
	TempHoldFire(CMD_RESURRECT);

	curResurrect = target;

	AddDeathDependence(curResurrect, DEPENDENCE_RESURRECT);
	ScriptStartBuilding(target->pos, false);
}


void CBuilder::SetCaptureTarget(CUnit* target)
{
	if (target == curCapture)
		return;

	StopBuild(false);
	TempHoldFire(CMD_CAPTURE);

	curCapture = target;

	AddDeathDependence(curCapture, DEPENDENCE_CAPTURE);
	ScriptStartBuilding(target->pos, false);
}


void CBuilder::StartRestore(float3 centerPos, float radius)
{
	StopBuild(false);
	TempHoldFire(CMD_RESTORE);

	terraformTaskToken = terraformTaskHandler.AddTerraformTask(
		SRectangle(
			static_cast<int>((centerPos.x - radius) / SQUARE_SIZE),
			static_cast<int>((centerPos.z - radius) / SQUARE_SIZE),
			static_cast<int>((centerPos.x + radius) / SQUARE_SIZE),
			static_cast<int>((centerPos.z + radius) / SQUARE_SIZE)
		), nullptr
	);

	ScriptStartBuilding(centerPos, false);
}


void CBuilder::StopBuild(bool callScript)
{
	if (curBuild != nullptr)
		DeleteDeathDependence(curBuild, DEPENDENCE_BUILD);
	if (curReclaim != nullptr)
		DeleteDeathDependence(curReclaim, DEPENDENCE_RECLAIM);
	if (curResurrect != nullptr)
		DeleteDeathDependence(curResurrect, DEPENDENCE_RESURRECT);
	if (curCapture != nullptr)
		DeleteDeathDependence(curCapture, DEPENDENCE_CAPTURE);

	curBuild = nullptr;
	curReclaim = nullptr;
	curResurrect = nullptr;
	curCapture = nullptr;

	terraformTaskToken = nullptr;

	if (callScript)
		script->StopBuilding();

	SetHoldFire(false);
}


bool CBuilder::StartBuild(BuildInfo& buildInfo, CFeature*& feature, bool& inWaitStance, bool& limitReached)
{
	const CUnit* prvBuild = curBuild;

	StopBuild(false);
	TempHoldFire(-1);

	buildInfo.pos = CGameHelper::Pos2BuildPos(buildInfo, true);

	// Pass -1 as allyteam to behave like we have maphack.
	// This is needed to prevent building on top of cloaked stuff.
	const CGameHelper::BuildSquareStatus tbs = CGameHelper::TestUnitBuildSquare(buildInfo, feature, -1, true);

	switch (tbs) {
		case CGameHelper::BUILDSQUARE_OPEN:
			break;

		case CGameHelper::BUILDSQUARE_BLOCKED:
		case CGameHelper::BUILDSQUARE_OCCUPIED: {
			const CUnit* u = nullptr;

			const int2 mins = CSolidObject::GetMapPosStatic(buildInfo.pos, buildInfo.GetXSize(), buildInfo.GetZSize());
			const int2 maxs = mins + int2(buildInfo.GetXSize(), buildInfo.GetZSize());

			for (int z = mins.y; z < maxs.y; ++z) {
				for (int x = mins.x; x < maxs.x; ++x) {
					const CGroundBlockingObjectMap::BlockingMapCell& cell = groundBlockingObjectMap.GetCellUnsafeConst(float3{
						static_cast<float>(x * SQUARE_SIZE),
						0.0f,
						static_cast<float>(z * SQUARE_SIZE) }
					);

					// look for any blocking assistable buildee at build.pos
					for (size_t i = 0, n = cell.size(); i < n; i++) {
						const CUnit* cu = dynamic_cast<const CUnit*>(cell[i]);

						if (cu == nullptr)
							continue;
						if (allyteam != cu->allyteam)
							return false; // Enemy units that block always block the cell
						if (!CanAssistUnit(cu, buildInfo.def))
							continue;

						u = cu;
						goto out; //lol
					}
				}
			}

			out:
			// <pos> might map to a non-blocking portion
			// of the buildee's yardmap, fallback check
			if (u == nullptr)
				u = CGameHelper::GetClosestFriendlyUnit(nullptr, buildInfo.pos, buildDistance, allyteam);

			if (u != nullptr) {
				if (CanAssistUnit(u, buildInfo.def)) {
					if (u != prvBuild) {
						terraformTaskToken = nullptr;
					}

					AddDeathDependence(curBuild = const_cast<CUnit*>(u), DEPENDENCE_BUILD);
					ScriptStartBuilding(u->pos, false);
					return true;
				}

				// let BuggerOff handle this case (TODO: non-landed aircraft should not count)
				if (buildInfo.FootPrintOverlap(u->pos, u->GetFootPrint(SQUARE_SIZE * 0.5f)))
					return false;
			}
		} return false;

		case CGameHelper::BUILDSQUARE_RECLAIMABLE:
			// caller should handle this
			return false;
	}

	// at this point we know the builder is going to create a new unit, bail if at the limit
	if ((limitReached = (unitHandler.NumUnitsByTeamAndDef(team, buildInfo.def->id) >= buildInfo.def->maxThisUnit)))
		return false;

	if ((inWaitStance = !ScriptStartBuilding(buildInfo.pos, true)))
		return false;

	const UnitDef* buildeeDef = buildInfo.def;
	const UnitLoadParams buildeeParams = {buildeeDef, this, buildInfo.pos, ZeroVector, -1, team, buildInfo.buildFacing, true, false};

	CUnit* buildee = unitLoader->LoadUnit(buildeeParams);

	// floating structures don't terraform the seabed
	const bool buildeeOnWater = (buildee->FloatOnWater() && buildee->IsInWater());
	const bool allowTerraform = (!mapDamage->Disabled() && buildeeDef->levelGround);
	const bool  skipTerraform = (buildeeOnWater || buildeeDef->IsAirUnit() || !buildeeDef->IsImmobileUnit());

	if (!allowTerraform || skipTerraform) {
		// skip the terraforming job
		buildee->groundLevelled = true;
	} else {
		buildee->groundLevelled = false;

		terraformTaskToken = terraformTaskHandler.AddTerraformTask(GetBuildingRectangle(*buildee), buildee);
	}

	// pass the *builder*'s udef for checking canBeAssisted; if buildee
	// happens to be a non-assistable factory then it would also become
	// impossible to *construct* with multiple builders
	buildee->SetSoloBuilder(this, this->unitDef);
	AddDeathDependence(curBuild = buildee, DEPENDENCE_BUILD);

	// if the ground is not going to be terraformed the buildee would
	// 'pop' to the correct height over the (un-flattened) terrain on
	// completion, so put it there to begin with
	curBuild->moveType->SlowUpdate();
	return true;
}

void CBuilder::DependentDied(CObject* o)
{
	if (o == curBuild) {
		curBuild = nullptr;
		StopBuild();
	}
	if (o == curReclaim) {
		curReclaim = nullptr;
		StopBuild();
	}
	if (o == curResurrect) {
		curResurrect = nullptr;
		StopBuild();
	}
	if (o == curCapture) {
		curCapture = nullptr;
		StopBuild();
	}
	/*
	if (o == helpTerraform) {
		helpTerraform = nullptr;
		StopBuild();
	}
	*/
	CUnit::DependentDied(o);
}


bool CBuilder::ScriptStartBuilding(float3 pos, bool silent)
{
	if (script->HasStartBuilding()) {
		const float3 wantedDir = (pos - midPos).Normalize();
		const float h = GetHeadingFromVectorF(wantedDir.x, wantedDir.z);
		const float p = math::asin(wantedDir.dot(updir));
		const float pitch = math::asin(frontdir.dot(updir));

		// clamping p - pitch not needed, range of asin is -PI/2..PI/2,
		// so max difference between two asin calls is PI.
		// FIXME: convert CSolidObject::heading to radians too.
		script->StartBuilding(ClampRad(h - heading * TAANG2RAD), p - pitch);
	}

	if ((!silent || inBuildStance) && IsInLosForAllyTeam(gu->myAllyTeam))
		Channels::General->PlayRandomSample(unitDef->sounds.build, pos);

	return inBuildStance;
}

void CBuilder::CreateNanoParticle(const float3& goal, float radius, bool inverse, bool highPriority)
{
	const int modelNanoPiece = nanoPieceCache.GetNanoPiece(script);

	if (!localModel.Initialized() || !localModel.HasPiece(modelNanoPiece))
		return;

	const float3 relNanoFirePos = localModel.GetRawPiecePos(modelNanoPiece);
	const float3 nanoPos = this->GetObjectSpacePos(relNanoFirePos);

	// unsynced
	projectileHandler.AddNanoParticle(nanoPos, goal, unitDef, team, radius, inverse, highPriority);
}
