/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "GroundMoveType.h"
#include "MoveDefHandler.h"
#include "Components/MoveTypesComponents.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/Player.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "MoveMath/MoveMath.h"
#include "Sim/Ecs/Registry.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/creg/STL_Tuple.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/FastMath.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/type2.h"
#include "System/Sound/ISoundChannels.h"
#include "System/SpringHash.h"
#include "Utils/UnitTrapCheckUtils.h"

#include "System/Misc/TracyDefs.h"

// #define PATHING_DEBUG

#ifdef PATHING_DEBUG
#include <sim/Path/HAPFS/PathGlobal.h>
#include "System/Threading/ThreadPool.h"
#endif

#if 1
#include "Rendering/IPathDrawer.h"
#define DEBUG_DRAWING_ENABLED ((gs->cheatEnabled || gu->spectatingFullView) && pathDrawer->IsEnabled())
spring::spinlock geometryLock;
#else
#define DEBUG_DRAWING_ENABLED false
#endif

using namespace MoveTypes;

#define LOG_SECTION_GMT "GroundMoveType"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_GMT)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_GMT


// speeds near (MAX_UNIT_SPEED * 1e1) elmos / frame can be caused by explosion impulses
// CUnitHandler removes units with speeds > MAX_UNIT_SPEED as soon as they exit the map,
// so the assertion can be less strict
#define ASSERT_SANE_OWNER_SPEED(v) assert(v.SqLength() < (MAX_UNIT_SPEED * MAX_UNIT_SPEED * 1e2));

// magic number to reduce damage taken from collisions
// between a very heavy and a very light CSolidObject
#define COLLISION_DAMAGE_MULT    0.02f

#define MAX_IDLING_SLOWUPDATES     16
#define IGNORE_OBSTACLES            0
#define WAIT_FOR_PATH               0
#define MODEL_TURN_INERTIA          1

#define UNIT_EVENTS_RESERVE			8

#define UNIT_CMD_QUE_SIZE(u) (u->commandAI->commandQue.size())
// Not using IsMoveCommand on purpose, as the following is changing the effective goalRadius
#define UNIT_HAS_MOVE_CMD(u) (u->commandAI->commandQue.empty() || u->commandAI->commandQue[0].GetID() == CMD_MOVE || u->commandAI->commandQue[0].GetID() == CMD_FIGHT)

#define WAYPOINT_RADIUS (1.25f * SQUARE_SIZE)

#define MAXREVERSESPEED_MEMBER_IDX 7

#define MEMBER_CHARPTR_HASH(memberName) spring::LiteHash(memberName, strlen(memberName),     0)
#define MEMBER_LITERAL_HASH(memberName) spring::LiteHash(memberName, sizeof(memberName) - 1, 0)

CR_BIND_DERIVED(CGroundMoveType, AMoveType, (nullptr))
CR_REG_METADATA(CGroundMoveType, (
	CR_IGNORED(pathController),

	CR_MEMBER(currWayPoint),
	CR_MEMBER(nextWayPoint),

	CR_MEMBER(earlyCurrWayPoint),
	CR_MEMBER(earlyNextWayPoint),

	CR_MEMBER(waypointDir),
	CR_MEMBER(flatFrontDir),
	CR_MEMBER(lastAvoidanceDir),
	CR_MEMBER(mainHeadingPos),
	CR_MEMBER(skidRotVector),

	CR_MEMBER(turnRate),
	CR_MEMBER(turnSpeed),
	CR_MEMBER(turnAccel),
	CR_MEMBER(accRate),
	CR_MEMBER(decRate),
	CR_MEMBER(myGravity),

	CR_MEMBER(maxReverseDist),
	CR_MEMBER(minReverseAngle),
	CR_MEMBER(maxReverseSpeed),
	CR_MEMBER(sqSkidSpeedMult),

	CR_MEMBER(wantedSpeed),
	CR_MEMBER(currentSpeed),
	CR_MEMBER(deltaSpeed),

	CR_MEMBER(currWayPointDist),
	CR_MEMBER(prevWayPointDist),
	CR_MEMBER(goalRadius),
	CR_MEMBER(ownerRadius),
	CR_MEMBER(extraRadius),

	CR_MEMBER(skidRotSpeed),
	CR_MEMBER(skidRotAccel),

	CR_MEMBER(resultantForces),
	CR_MEMBER(forceFromMovingCollidees),
	CR_MEMBER(forceFromStaticCollidees),

	CR_MEMBER(pathID),

	CR_MEMBER(numIdlingUpdates),
	CR_MEMBER(numIdlingSlowUpdates),

	CR_MEMBER(wantedHeading),
	CR_MEMBER(minScriptChangeHeading),

	CR_MEMBER(atGoal),
	CR_MEMBER(atEndOfPath),
	CR_MEMBER(wantRepath),

	CR_MEMBER(reversing),
	CR_MEMBER(idling),
	CR_MEMBER(pushResistant),
	CR_MEMBER(canReverse),
	CR_MEMBER(useMainHeading),
	CR_MEMBER(useRawMovement),
	CR_MEMBER(pathingFailed),
	CR_MEMBER(pathingArrived),
	CR_MEMBER(positionStuck),
	CR_MEMBER(forceStaticObjectCheck),
	CR_MEMBER(avoidingUnits),
	CR_MEMBER(setHeading),
	CR_MEMBER(setHeadingDir),

	CR_POSTLOAD(PostLoad),
	CR_PREALLOC(GetPreallocContainer)
))



static CGroundMoveType::MemberData gmtMemberData = {
	{{
		std::pair<unsigned int,  bool*>{MEMBER_LITERAL_HASH(       "atGoal"), nullptr},
		std::pair<unsigned int,  bool*>{MEMBER_LITERAL_HASH(  "atEndOfPath"), nullptr},
		std::pair<unsigned int,  bool*>{MEMBER_LITERAL_HASH("pushResistant"), nullptr},
	}},
	{{
		std::pair<unsigned int, short*>{MEMBER_LITERAL_HASH("minScriptChangeHeading"), nullptr},
	}},
	{{
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH(       "turnRate"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH(      "turnAccel"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH(        "accRate"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH(        "decRate"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH(      "myGravity"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH( "maxReverseDist"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH("minReverseAngle"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH("maxReverseSpeed"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH("sqSkidSpeedMult"), nullptr},
	}},
};




namespace SAT {
	static float CalcSeparatingDist(
		const float3& axis,
		const float3& zdir,
		const float3& xdir,
		const float3& sepv,
		const float2& size
	) {
		const float axisDist = math::fabs(axis.dot(sepv)         );
		const float xdirDist = math::fabs(axis.dot(xdir) * size.x);
		const float zdirDist = math::fabs(axis.dot(zdir) * size.y);

		return (axisDist - zdirDist - xdirDist);
	}

	static bool HaveSeparatingAxis(
		const CSolidObject* collider,
		const CSolidObject* collidee,
		const MoveDef* colliderMD,
		const MoveDef* collideeMD,
		const float3& separationVec
	) {
		// const float2 colliderSize = (colliderMD != nullptr)? colliderMD->GetFootPrint(0.5f * SQUARE_SIZE): collider->GetFootPrint(0.5f * SQUARE_SIZE);
		const float2 colliderSize =                          colliderMD->GetFootPrint(0.5f * SQUARE_SIZE)                                            ;
		const float2 collideeSize = (collideeMD != nullptr)? collideeMD->GetFootPrint(0.5f * SQUARE_SIZE): collidee->GetFootPrint(0.5f * SQUARE_SIZE);

		// true if no overlap on at least one axis
		bool haveAxis = false;

		haveAxis = haveAxis || (CalcSeparatingDist(collider->frontdir,  collidee->frontdir, collidee->rightdir,  separationVec, collideeSize) > colliderSize.y);
		haveAxis = haveAxis || (CalcSeparatingDist(collider->rightdir,  collidee->frontdir, collidee->rightdir,  separationVec, collideeSize) > colliderSize.x);
		haveAxis = haveAxis || (CalcSeparatingDist(collidee->frontdir,  collider->frontdir, collider->rightdir,  separationVec, colliderSize) > collideeSize.y);
		haveAxis = haveAxis || (CalcSeparatingDist(collidee->rightdir,  collider->frontdir, collider->rightdir,  separationVec, colliderSize) > collideeSize.x);
		return haveAxis;
	}
};


static bool CheckCollisionExclSAT(
	const float4& separationVec,
	const CSolidObject* collider = nullptr,
	const CSolidObject* collidee = nullptr,
	const MoveDef* colliderMD = nullptr,
	const MoveDef* collideeMD = nullptr
) {
	return ((separationVec.SqLength() - separationVec.w) <= 0.01f);
}

static bool CheckCollisionInclSAT(
	const float4& separationVec,
	const CSolidObject* collider = nullptr,
	const CSolidObject* collidee = nullptr,
	const MoveDef* colliderMD = nullptr,
	const MoveDef* collideeMD = nullptr
) {
	assert(collider   != nullptr);
	assert(collidee   != nullptr);
	assert(colliderMD != nullptr);

	return (CheckCollisionExclSAT(separationVec) && !SAT::HaveSeparatingAxis(collider, collidee, colliderMD, collideeMD, separationVec));
}



static void HandleUnitCollisionsAux(
	const CUnit* collider,
	const CUnit* collidee,
	CGroundMoveType* gmtCollider,
	CGroundMoveType* gmtCollidee
) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!collider->IsMoving() || gmtCollider->progressState != AMoveType::Active)
		return;

	// if collidee shares our goal position and is no longer
	// moving along its path, trigger Arrived() to kill long
	// pushing contests
	//
	// check the progress-states so collisions with units which
	// failed to reach goalPos for whatever reason do not count
	// (or those that still have orders)
	//
	// CFactory applies random jitter to otherwise equal goal
	// positions of at most TWOPI elmos, use half as threshold
	// (simply bail if distance between collider and collidee
	// goal-positions exceeds PI)
	// if (!gmtCollider->IsAtGoalPos(gmtCollidee->goalPos, math::PI))
	// 	return;

	switch (gmtCollidee->progressState) {
		case AMoveType::Done: {
			if (collidee->IsMoving() || UNIT_CMD_QUE_SIZE(collidee) != 0)
				return;

			float separationDist = std::max(collider->unitDef->separationDistance, collidee->unitDef->separationDistance);
			const bool triggerArrived = gmtCollider->IsAtGoalPos(collidee->pos, gmtCollidee->GetOwnerRadius() + separationDist);
			if (triggerArrived) {
				gmtCollider->TriggerCallArrived();
			} else {
				// if the collidee is touching the waypoint, then also switch to the next waypoint.
				const float3& currWaypoint = gmtCollider->GetCurrWayPoint();
				const float collideeToCurrDistSq = currWaypoint.SqDistance2D(collidee->pos);
				const float collideeGoalRadius = gmtCollidee->GetOwnerRadius();

				if (collideeToCurrDistSq <= Square(collideeGoalRadius+separationDist)) {
					gmtCollider->TriggerSkipWayPoint();
					return;
				}
			}
		} break;

		case AMoveType::Active: {
			// collider and collidee are both actively moving and share the same goal position
			// (i.e. a traffic jam) so ignore current waypoint and go directly to the next one
			// or just make collider give up if already within footprint radius.
			if (gmtCollidee->GetCurrWayPoint() == gmtCollider->GetNextWayPoint()) {
				// const float3& currWaypoint = gmtCollider->GetCurrWayPoint();
				// const float3& nextWaypoint = gmtCollider->GetNextWayPoint();

				// const float unitToNextDistSq = nextWaypoint.SqDistance2D(collider->pos);
				// const float currToNextDistSq = nextWaypoint.SqDistance2D(currWaypoint);

				// // Switch waypoints if the current waypoint is effectively sending us in the
				// // wrong direction. This can happen as units push each other around. This check
				// // is important to prevent units in a long line back-propagating the next
				// // waypoint, which could cause units to try and cut corners, which could cause
				// // them to be unable to path around obstacles.
				// if (unitToNextDistSq <= currToNextDistSq) {
					gmtCollider->TriggerSkipWayPoint();
					return;
				// }
			}

			// Several large units can end up surrounding a waypoint and block each other from touching it, because it
			// sits diagonally from all of them and outside their radius.
			// Adding an extra diagonal distance compensation will allow such large units to "touch" the waypoint.
			constexpr float adjustForDiagonal = 1.45f;
			float separationDist = std::max(collider->unitDef->separationDistance, collidee->unitDef->separationDistance);
			const bool triggerArrived = (gmtCollider->IsAtGoalPos(collider->pos, (gmtCollider->GetOwnerRadius() + separationDist)*adjustForDiagonal)
										|| gmtCollider->IsAtGoalPos(collidee->pos, (gmtCollidee->GetOwnerRadius() + separationDist)*adjustForDiagonal));
			if (triggerArrived) {
				gmtCollider->TriggerCallArrived();
			} else {
				// if the collidee is touching the waypoint, then also switch to the next
				// waypoint.
				const float3& currWaypoint = gmtCollider->GetCurrWayPoint();
				const float collideeToCurrDistSq = currWaypoint.SqDistance2D(collidee->pos);
				const float collideeGoalRadius = gmtCollidee->GetOwnerRadius();
				if (collideeToCurrDistSq <= Square((collideeGoalRadius+separationDist)*adjustForDiagonal)) {
					gmtCollider->TriggerSkipWayPoint();
					return;
				}
			}

			// if (!gmtCollider->IsAtGoalPos(collider->pos, gmtCollider->GetOwnerRadius()))
			// 	return;

			// gmtCollider->TriggerCallArrived();
		} break;

		default: {
		} break;
	}
}


static float3 CalcSpeedVectorInclGravity(const CUnit* owner, const CGroundMoveType* mt, float hAcc, float vAcc) {
	RECOIL_DETAILED_TRACY_ZONE;
	float3 newSpeedVector;

	// NOTE:
	//   the drag terms ensure speed-vector always decays if
	//   wantedSpeed and deltaSpeed are 0 (needed because we
	//   do not call GetDragAccelerationVect while a unit is
	//   moving under its own power)
	const float dragCoeff = mix(0.99f, 0.9999f, owner->IsInAir());
	const float slipCoeff = mix(0.95f, 0.9999f, owner->IsInAir());

	const float3& ownerPos = owner->pos;
	const float3& ownerSpd = owner->speed;

	// use terrain-tangent vector because it does not
	// depend on UnitDef::upright (unlike o->frontdir)
	const float3& gndNormVec = mt->GetGroundNormal(ownerPos);
	const float3  gndTangVec = gndNormVec.cross(owner->rightdir);
	const float3& flatFrontDir = mt->GetFlatFrontDir();

	const int dirSign = Sign(flatFrontDir.dot(ownerSpd));
	const int revSign = Sign(int(!mt->IsReversing()));

	const float3 horSpeed = ownerSpd * XZVector * dirSign * revSign;
	const float3 verSpeed = UpVector * ownerSpd.y;

	if (!modInfo.allowHoverUnitStrafing || owner->moveDef->speedModClass != MoveDef::Hover) {
		const float3 accelVec = (gndTangVec * hAcc) + (UpVector * vAcc);
		const float3 speedVec = (horSpeed + verSpeed) + accelVec;

		newSpeedVector += (flatFrontDir * speedVec.dot(flatFrontDir)) * dragCoeff;
		newSpeedVector += (    UpVector * speedVec.dot(    UpVector));
	} else {
		// TODO: also apply to non-hovercraft on low-gravity maps?
		newSpeedVector += (              gndTangVec * (  std::max(0.0f,   ownerSpd.dot(gndTangVec) + hAcc * 1.0f))) * dragCoeff;
		newSpeedVector += (   horSpeed - gndTangVec * (/*std::max(0.0f,*/ ownerSpd.dot(gndTangVec) - hAcc * 0.0f )) * slipCoeff;
		newSpeedVector += (UpVector * UpVector.dot(ownerSpd + UpVector * vAcc));
	}

	// never drop below terrain while following tangent
	// (SPEED must be adjusted so that it does not keep
	// building up when the unit is on the ground or is
	// within one frame of hitting it)
	const float oldGroundHeight = mt->GetGroundHeight(ownerPos                 );
	const float newGroundHeight = mt->GetGroundHeight(ownerPos + newSpeedVector);

	if ((ownerPos.y + newSpeedVector.y) <= newGroundHeight)
		newSpeedVector.y = std::min(newGroundHeight - ownerPos.y, math::fabs(newGroundHeight - oldGroundHeight));

	return newSpeedVector;
}

static float3 CalcSpeedVectorExclGravity(const CUnit* owner, const CGroundMoveType* mt, float hAcc, float vAcc) {
	RECOIL_DETAILED_TRACY_ZONE;
	// LuaSyncedCtrl::SetUnitVelocity directly assigns
	// to owner->speed which gets overridden below, so
	// need to calculate hSpeedScale from it (not from
	// currentSpeed) directly

	if (mt->GetWantedSpeed() == 0.f && math::fabs(owner->speed.w) - hAcc <= 0.01f)
		return ZeroVector;
	else {
		float vel = owner->speed.w;
		float maxSpeed = owner->moveType->GetMaxSpeed();
		if (vel > maxSpeed) {
			// Once a unit is travelling faster than their maximum speed, their engine power is no longer sufficient to counteract
			// the drag from air and rolling resistance. So reduce their velocity by these forces until a return to maximum speed.
			float rollingResistanceCoeff = owner->unitDef->rollingResistanceCoefficient;
			vel = std::max(maxSpeed,
				(owner->speed +
				owner->GetDragAccelerationVec(
					mapInfo->atmosphere.fluidDensity,
					mapInfo->water.fluidDensity,
					owner->unitDef->atmosphericDragCoefficient,
					rollingResistanceCoeff
				)).Length()
			);
		}
		return (owner->frontdir * (vel * Sign(int(!mt->IsReversing())) + hAcc));
	}
}



static constexpr decltype(&CheckCollisionInclSAT) checkCollisionFuncs[] = {
	CheckCollisionExclSAT,
	CheckCollisionInclSAT,
};

static constexpr decltype(&CalcSpeedVectorInclGravity) calcSpeedVectorFuncs[] = {
	CalcSpeedVectorExclGravity,
	CalcSpeedVectorInclGravity,
};




CGroundMoveType::CGroundMoveType(CUnit* owner):
	AMoveType(owner),
	pathController(owner),

	currWayPoint(ZeroVector),
	nextWayPoint(ZeroVector),

	flatFrontDir(FwdVector),
	lastAvoidanceDir(ZeroVector),
	mainHeadingPos(ZeroVector),
	skidRotVector(UpVector),

	wantedHeading(0),
	minScriptChangeHeading((SPRING_CIRCLE_DIVS - 1) >> 1),

	pushResistant((owner != nullptr) && owner->unitDef->pushResistant),
	canReverse((owner != nullptr) && (owner->unitDef->rSpeed > 0.0f))
{
	// creg
	if (owner == nullptr)
		return;

	const UnitDef* ud = owner->unitDef;
	const MoveDef* md = owner->moveDef;

	assert(ud != nullptr);
	assert(md != nullptr);

	// maxSpeed is set in AMoveType's ctor
	maxReverseSpeed = ud->rSpeed / GAME_SPEED;

	// SPRING_CIRCLE_DIVS is 65536, but turnRate can be at most
	// 32767 since it is converted to (signed) shorts in places
	turnRate = std::clamp(ud->turnRate, 1.0f, SPRING_CIRCLE_DIVS * 0.5f - 1.0f);
	turnAccel = turnRate * mix(0.333f, 0.033f, md->speedModClass == MoveDef::Ship);

	accRate = std::max(0.01f, ud->maxAcc);
	decRate = std::max(0.01f, ud->maxDec);

	// unit-gravity must always be negative
	myGravity = mix(-math::fabs(ud->myGravity), mapInfo->map.gravity, ud->myGravity == 0.0f);

	ownerRadius = md->CalcFootPrintMinExteriorRadius();

	forceStaticObjectCheck = true;

	flatFrontDir = (owner->frontdir * XZVector).Normalize();

	// Override the unit size, it should match the MoveDef's to avoid conflicts elsewhere in the code.
	owner->xsize = md->xsize;
	owner->zsize = md->zsize;

	Connect();
}

CGroundMoveType::~CGroundMoveType()
{
	Disconnect();

	if (nextPathId != 0) {
		pathManager->DeletePath(nextPathId, true);
	}

	if (pathID != 0) {
		pathManager->DeletePath(pathID, true);
	}

	if (deletePathId != 0) {
		pathManager->DeletePath(deletePathId);
	}
}

void CGroundMoveType::PostLoad()
{
	RECOIL_DETAILED_TRACY_ZONE;
	pathController = GMTDefaultPathController(owner);

	// If the active moveType is not set to default ground move (i.e. is on scripted move type) then skip.
	if ((uint8_t *)owner->moveType != owner->amtMemBuffer) {
		// Safety measure to clear the path id.
		pathID = 0;
		return;
	}

	Connect();

	// HACK: re-initialize path after load
	if (pathID == 0)
		return;

	// There isn't a path to clear (we've just loaded a saved game), so we must now clear pathID
	// before requesting our new path; otherwise, a valid path for another unit could be deleted.
	pathID = 0;
	pathID = pathManager->RequestPath(owner, owner->moveDef, owner->pos, goalPos, goalRadius + extraRadius, true);
}

bool CGroundMoveType::OwnerMoved(const short oldHeading, const float3& posDif, const float3& cmpEps) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (posDif.equals(ZeroVector, cmpEps)) {
		// note: the float3::== test is not exact, so even if this
		// evaluates to true the unit might still have an epsilon
		// speed vector --> nullify it to prevent apparent visual
		// micro-stuttering (speed is used to extrapolate drawPos)
		owner->SetVelocityAndSpeed(ZeroVector);

		// negative y-coordinates indicate temporary waypoints that
		// only exist while we are still waiting for the pathfinder
		// (so we want to avoid being considered "idle", since that
		// will cause our path to be re-requested and again give us
		// a temporary waypoint, etc.)
		// NOTE: this is only relevant for QTPFS (at present)
		// if the unit is just turning in-place over several frames
		// (eg. to maneuver around an obstacle), do not consider it
		// as "idling"
		idling = true;
		idling &= !atGoal;
		idling &= (std::abs(owner->heading - oldHeading) < turnRate);

		return false;
	}

	// note: HandleObjectCollisions() may have negated the position set
	// by UpdateOwnerPos() (so that owner->pos is again equal to oldPos)
	// note: the idling-check can only succeed if we are oriented in the
	// direction of our waypoint, which compensates for the fact distance
	// decreases much less quickly when moving orthogonal to <waypointDir>
	oldPos = owner->pos;

	const float3 ffd = flatFrontDir * posDif.SqLength() * 0.5f;
	const float3 wpd = waypointDir * ((int(!reversing) * 2) - 1);

	// too many false negatives: speed is unreliable if stuck behind an obstacle
	//   idling = (Square(owner->speed.w) < (accRate * accRate));
	//   idling &= (Square(currWayPointDist - prevWayPointDist) <= (accRate * accRate));
	// too many false positives: waypoint-distance delta and speed vary too much
	//   idling = (Square(currWayPointDist - prevWayPointDist) < Square(owner->speed.w));
	// too many false positives: many slow units cannot even manage 1 elmo/frame
	//   idling = (Square(currWayPointDist - prevWayPointDist) < 1.0f);
	idling = true;
	idling &= (math::fabs(posDif.y) < math::fabs(cmpEps.y * owner->pos.y));
	idling &= (Square(currWayPointDist - prevWayPointDist) < ffd.dot(wpd));
	idling &= (posDif.SqLength() < Square(owner->speed.w * 0.5f));

	return true;
}

void CGroundMoveType::UpdatePreCollisions()
{
	RECOIL_DETAILED_TRACY_ZONE;
 	ASSERT_SYNCED(owner->pos);
	ASSERT_SYNCED(owner->heading);
 	ASSERT_SYNCED(currWayPoint);
 	ASSERT_SYNCED(nextWayPoint);

	if (resultantForces.SqLength() > 0.f)
		owner->Move(resultantForces, true);

	SyncWaypoints();

	// The mt section may have noticed the new path was ready and switched over to it. If so then
	// delete the old path, which has to be done in an ST section.
	if (deletePathId != 0) {
		pathManager->DeletePath(deletePathId);
		deletePathId = 0;
	}

	if (pathingArrived) {
		Arrived(false);
		pathingArrived = false;
	}

 	if (pathingFailed) {
 		Fail(false);
 		pathingFailed = false;
 	}

	pathManager->UpdatePath(owner, pathID);

	if (owner->GetTransporter() != nullptr) return;

	owner->UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING, owner->IsSkidding() || OnSlope(1.0f));

	if (owner->IsSkidding()) {
		UpdateSkid();
		return;
	}

	// set drop height when we start to drop
	if (owner->IsFalling()) {
		UpdateControlledDrop();
		return;
	}

	reversing = UpdateOwnerSpeed(math::fabs(oldSpeed), math::fabs(newSpeed), newSpeed);
	oldSpeed = newSpeed = 0.f;
}

void CGroundMoveType::UpdateUnitPosition() {
	resultantForces = ZeroVector;

	if (owner->IsSkidding()) return;

 	switch (setHeading) {
 		case HEADING_CHANGED_MOVE:
 			// ChangeHeading(setHeadingDir);
			ChangeSpeed(maxWantedSpeed, WantReverse(waypointDir, flatFrontDir));
 			setHeading = HEADING_CHANGED_NONE;
 			break;
		case HEADING_CHANGED_STOP:
	// 		SetMainHeading();
			ChangeSpeed(0.0f, false);
			setHeading = HEADING_CHANGED_NONE;
			break;
		case HEADING_CHANGED_STUN:
			ChangeSpeed(0.0f, false);
			setHeading = HEADING_CHANGED_NONE;
			break;
	}

	if (owner->GetTransporter() != nullptr) return;

 	if (owner->UnderFirstPersonControl())
 		UpdateDirectControl();

	UpdateOwnerPos(owner->speed, calcSpeedVectorFuncs[modInfo.allowGroundUnitGravity](owner, this, deltaSpeed, myGravity));
}

void CGroundMoveType::UpdateCollisionDetections() {
	RECOIL_DETAILED_TRACY_ZONE;

	if (owner->GetTransporter() != nullptr) return;
	if (owner->IsSkidding()) return;
	if (owner->IsFalling()) return;

	HandleObjectCollisions();
}

bool CGroundMoveType::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (owner->requestRemoveUnloadTransportId) {
		owner->unloadingTransportId = -1;
		owner->requestRemoveUnloadTransportId = false;
	}

	// Collisions can change waypoint states (y); though they won't move them (xz).
	SyncWaypoints();

	// do nothing at all if we are inside a transport
	if (owner->GetTransporter() != nullptr) return false;
	if (owner->IsSkidding()) return false;
	if (owner->IsFalling()) return false;

	if (resultantForces.SqLength() > 0.f)
		owner->Move(resultantForces, true);

	AdjustPosToWaterLine();

	ASSERT_SANE_OWNER_SPEED(owner->speed);

	// <dif> is normally equal to owner->speed (if no collisions)
	// we need more precision (less tolerance) in the y-dimension
	// for all-terrain units that are slowed down a lot on cliffs
	return (OwnerMoved(owner->heading, owner->pos - oldPos, float3(float3::cmp_eps(), float3::cmp_eps() * 1e-2f, float3::cmp_eps())));
}

void CGroundMoveType::UpdateOwnerAccelAndHeading()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (owner->IsStunned() || owner->beingBuilt) {
		setHeading = HEADING_CHANGED_STUN;
		return;
	}

	if (owner->UnderFirstPersonControl()) return;

	// either follow user control input or pathfinder
	// waypoints; change speed and heading as required
	// if (owner->UnderFirstPersonControl()) {
	// 	UpdateDirectControl();
	// } else {
		FollowPath(ThreadPool::GetThreadNum());
	// }
}

void CGroundMoveType::SlowUpdate()
{
	RECOIL_DETAILED_TRACY_ZONE;

	// bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
	// 	&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
	// if (printMoveInfo) {
	// 	LOG("%s: unit selected=%d pathType=%d atEndOfPath=%d atGoal=%d currWayPoint=(%f,%f,%f) nextWayPoint=(%f,%f,%f) goal=(%f,%f,%f) pos=(%f,%f,%f)"
	// 			, __func__
	// 			, owner->id, owner->moveDef->pathType, int(atEndOfPath), int(atGoal)
	// 			, float(currWayPoint.x), float(currWayPoint.y), float(currWayPoint.z)
	// 			, float(nextWayPoint.x), float(nextWayPoint.y), float(nextWayPoint.z)
	// 			, goalPos.x, goalPos.y, goalPos.z
	// 			, owner->pos.x, owner->pos.y, owner->pos.z);
	// }

	if (owner->GetTransporter() != nullptr) {
		if (progressState == Active)
			StopEngine(false);

	} else {
		if (progressState == Active) {
			if (pathID != 0) {
				if (idling) {
					numIdlingSlowUpdates = std::min(MAX_IDLING_SLOWUPDATES, int(numIdlingSlowUpdates + 1));
				} else {
					numIdlingSlowUpdates = std::max(0, int(numIdlingSlowUpdates - 1));
				}

				if (numIdlingUpdates > (SPRING_MAX_HEADING / turnRate)) {
					// case A: we have a path but are not moving
					LOG_L(L_DEBUG, "[%s] unit %i has pathID %i but %i ETA failures", __func__, owner->id, pathID, numIdlingUpdates);

					if (numIdlingSlowUpdates < MAX_IDLING_SLOWUPDATES) {
						// avoid spamming rerequest paths if the unit is making progress.
						if (idling) {
							// Unit may have got stuck in
							// 1) a wreck that has spawned
							// 2) a push-resistant unit that stopped moving
							// 3) an amphibious unit has just emerged from water right underneath a structure.
							// 4) a push-resistant unit/building was spawned or teleported on top of us via lua
							// 5) a stopped unit we were inside of was newly made push-resistant via lua
							// 6) our movedef changed into one of a different size / crushStrength via lua
							forceStaticObjectCheck = true;
							ReRequestPath(true);
						}
					} else {
						// unit probably ended up on a non-traversable
						// square, or got stuck in a non-moving crowd
						Fail(false);
						// bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
						// 	&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
						// if (printMoveInfo) {
						// 	LOG("%s: failed by idling too long.", __func__);
						// }
					}
				}
			} else {
				// case B: we want to be moving but don't have a path
				LOG_L(L_DEBUG, "[%s] unit %i has no path", __func__, owner->id);
				ReRequestPath(true);
			}

			if (wantRepath) {
				// When repaths are requested, they are pre-emptive and are made without
				// confirmation that it is really necessary. Give the unit a chance to
				// make progress: for example, when it got pushed against a building, but
				// is otherwise moving on. Pathing is expensive so we really want to keep
				// repathing to a minimum.
				// Resolution distance checks kept to 1/10th of an Elmo to reduce the
				// amount of time a unit can spend making insignificant progress, every
				// SlowUpdate.
				float curDist = math::floorf(currWayPoint.distance2D(owner->pos) * 10.f) / 10.f;
				if (curDist < bestLastWaypointDist) {
					bestLastWaypointDist = curDist;
					wantRepathFrame = gs->frameNum;
				}

				// lastWaypoint typically retries a repath and most likely won't get closer, so
				// in this case, don't wait around making the unit try to run into an obstacle for
				// longer than absolutely necessary.
				bool timeForRepath = gs->frameNum >= wantRepathFrame + modInfo.pfRepathDelayInFrames
									&& (gs->frameNum >= lastRepathFrame + modInfo.pfRepathMaxRateInFrames || lastWaypoint);

				// can't request a new path while the unit is stuck in terrain/static objects
				if (timeForRepath){
					if (lastWaypoint) {
						bestLastWaypointDist *= (1.f / SQUARE_SIZE);
						if (bestLastWaypointDist < bestReattemptedLastWaypointDist) {
							// Give a bad path another try, in case we can get closer.
							lastWaypoint = false;
							bestReattemptedLastWaypointDist = bestLastWaypointDist;
						}
						else {
							bestReattemptedLastWaypointDist = std::numeric_limits<decltype(bestReattemptedLastWaypointDist)>::infinity();
						}
					}
					if (!lastWaypoint) {
						ReRequestPath(true);
					} else {
						// This is here to stop units from trying forever to reach an goal they can't reach.
						Fail(false);
						// bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
						// 	&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
						// if (printMoveInfo) {
							// LOG("%s: failed to reach final waypoint", __func__);
						// }
					}
				}
			}
		}

		// move us into the map, and update <oldPos>
		// to prevent any extreme changes in <speed>
		if (!owner->IsFlying() && !owner->pos.IsInBounds())
			owner->Move(oldPos = owner->pos.cClampInBounds(), false);
	}

	AMoveType::SlowUpdate();
}


void CGroundMoveType::StartMovingRaw(const float3 moveGoalPos, float moveGoalRadius) {
	RECOIL_DETAILED_TRACY_ZONE;
	const float deltaRadius = std::max(0.0f, ownerRadius - moveGoalRadius);

	#ifdef PATHING_DEBUG
	if (DEBUG_DRAWING_ENABLED) {
		bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
			&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
		if (printMoveInfo) {
			LOG("%s unit goal (%f,%f,%f) [%f]", __func__
				, static_cast<float>(moveGoalPos.x)
				, static_cast<float>(moveGoalPos.y)
				, static_cast<float>(moveGoalPos.z)
				, moveGoalRadius);
		}
	}
	#endif

	goalPos = moveGoalPos * XZVector;
	goalRadius = moveGoalRadius;

	MoveTypes::CheckCollisionQuery collisionQuery(owner->moveDef, moveGoalPos);
	extraRadius = deltaRadius * (1 - owner->moveDef->TestMoveSquare(collisionQuery, moveGoalPos, ZeroVector, true, true));

	earlyCurrWayPoint = currWayPoint = goalPos;
	earlyNextWayPoint = nextWayPoint = goalPos;

	atGoal = (moveGoalPos.SqDistance2D(owner->pos) < Square(goalRadius + extraRadius));
	atEndOfPath = false;
	lastWaypoint = false;

	useMainHeading = false;
	useRawMovement = true;

	progressState = Active;

	numIdlingUpdates = 0;
	numIdlingSlowUpdates = 0;

	currWayPointDist = 0.0f;
	prevWayPointDist = 0.0f;

	pathingArrived = false;
	pathingFailed = false;
}

void CGroundMoveType::StartMoving(float3 moveGoalPos, float moveGoalRadius) {
	RECOIL_DETAILED_TRACY_ZONE;
	// add the footprint radius if moving onto goalPos would cause it to overlap impassable squares
	// (otherwise repeated coldet push-jittering can ensue if allowTerrainCollision is not disabled)
	// not needed if goalRadius actually exceeds ownerRadius, e.g. for builders
	const float deltaRadius = std::max(0.0f, ownerRadius - moveGoalRadius);

	#ifdef PATHING_DEBUG
	if (DEBUG_DRAWING_ENABLED) {
		bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
			&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
		if (printMoveInfo) {
			LOG("%s unit goal (%f,%f,%f) [%f]", __func__
				, static_cast<float>(moveGoalPos.x)
				, static_cast<float>(moveGoalPos.y)
				, static_cast<float>(moveGoalPos.z)
				, moveGoalRadius);
		}
	}
	#endif

	// set the new goal
	goalPos = moveGoalPos * XZVector;

	float mapx = mapDims.mapxm1 * SQUARE_SIZE;
	float mapz = mapDims.mapym1 * SQUARE_SIZE;

	// Sanitize the move command.
	if (goalPos.x < 0.f)  { goalPos.x = 0.f; }
	if (goalPos.z < 0.f)  { goalPos.z = 0.f; }
	if (goalPos.x > mapx) { goalPos.x = mapx; }
	if (goalPos.z > mapz) { goalPos.z = mapz; }

	goalRadius = moveGoalRadius;
	MoveTypes::CheckCollisionQuery collisionQuery(owner->moveDef, goalPos);
	extraRadius = deltaRadius * (1 - owner->moveDef->TestMoveSquare(collisionQuery, goalPos, ZeroVector, true, true));

	atGoal = (goalPos.SqDistance2D(owner->pos) < Square(goalRadius + extraRadius));
	atEndOfPath = false;
	lastWaypoint = false;

	useMainHeading = false;
	useRawMovement = false;

	progressState = Active;

	numIdlingUpdates = 0;
	numIdlingSlowUpdates = 0;

	currWayPointDist = 0.0f;
	prevWayPointDist = 0.0f;

	pathingArrived = false;
	pathingFailed = false;

	LOG_L(L_DEBUG, "[%s] starting engine for unit %i", __func__, owner->id);

	if (atGoal)
		return;

	// silently free previous path if unit already had one
	//
	// units passing intermediate waypoints will TYPICALLY not cause any
	// script->{Start,Stop}Moving calls now (even when turnInPlace=true)
	// unless they come to a full stop first
	ReRequestPath(true);

	bestReattemptedLastWaypointDist = std::numeric_limits<decltype(bestReattemptedLastWaypointDist)>::infinity();

	if (owner->team == gu->myTeam)
		Channels::General->PlayRandomSample(owner->unitDef->sounds.activate, owner);
}

void CGroundMoveType::StopMoving(bool callScript, bool hardStop, bool cancelRaw) {
	RECOIL_DETAILED_TRACY_ZONE;
	LOG_L(L_DEBUG, "[%s] stopping engine for unit %i", __func__, owner->id);

	if (!atGoal)
		earlyCurrWayPoint = (goalPos = (currWayPoint = Here()));

	// this gets called under a variety of conditions (see MobileCAI)
	// the most common case is a CMD_STOP being issued which means no
	// StartMoving-->StartEngine will follow
	StopEngine(callScript, hardStop);

	// force goal to be set to prevent obscure conditions triggering a
	// stationary unit to try moving due to obstacle collisions.
	atGoal = true;

	// force WantToStop to return true when useRawMovement is enabled
	atEndOfPath |= useRawMovement;
	// only a new StartMoving call can normally reset this
	useRawMovement &= (!cancelRaw);
	useMainHeading = false;

	progressState = Done;
}

void CGroundMoveType::UpdateTraversalPlan() {
	RECOIL_DETAILED_TRACY_ZONE;

	earlyCurrWayPoint = currWayPoint;
	earlyNextWayPoint = nextWayPoint;

	// Check whether the new path is ready.
	if (nextPathId != 0) {
		float3 tempWaypoint = pathManager->NextWayPoint(owner, nextPathId, 0,   owner->pos, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);

		// a non-temp answer tells us that the new path is ready to be used.
		if (tempWaypoint.y != (-1.f)) {
			// if the unit has switched to a raw move since the new path was requested then don't
			// try to redirect onto the new path.
			if (!useRawMovement) {
				// switch straight over to the new path
				earlyCurrWayPoint = tempWaypoint;
				earlyNextWayPoint = pathManager->NextWayPoint(owner, nextPathId, 0, earlyCurrWayPoint, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);
				lastWaypoint = false;
				wantRepath = false;
				atEndOfPath = false;
			}

			// can't delete the path in an MT section
			deletePathId = pathID;
			pathID = nextPathId;
			nextPathId = 0;
		}
	}

	if (owner->GetTransporter() != nullptr) return;
	if (owner->IsSkidding()) return;
	if (owner->IsFalling()) return;

	UpdateObstacleAvoidance();
	UpdateOwnerAccelAndHeading();
}

void CGroundMoveType::UpdateObstacleAvoidance() {
	if (owner->IsStunned() || owner->beingBuilt)
		return;

	if (owner->UnderFirstPersonControl())
		return;

	const float3&  ffd = flatFrontDir;
	auto wantReverse = WantReverse(waypointDir, ffd);
	const float3  rawWantedDir = waypointDir * Sign(int(!wantReverse));

	GetObstacleAvoidanceDir(mix(ffd, rawWantedDir, !atGoal));
}

bool CGroundMoveType::FollowPath(int thread)
{
	RECOIL_DETAILED_TRACY_ZONE;
	bool wantReverse = false;

	if (WantToStop()) {
		earlyCurrWayPoint.y = -1.0f;
		earlyNextWayPoint.y = -1.0f;

		setHeading = HEADING_CHANGED_STOP;
		auto& event = Sim::registry.get<ChangeMainHeadingEvent>(owner->entityReference);
		event.changed = true;
	} else {
		#ifdef PATHING_DEBUG
		if (DEBUG_DRAWING_ENABLED) {
			bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
				&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
			if (printMoveInfo) {
				LOG("%s unit origin (%f,%f,%f)", __func__
					, static_cast<float>(owner->pos.x)
					, static_cast<float>(owner->pos.y)
					, static_cast<float>(owner->pos.z));

				LOG("%s currWayPoint (%f,%f,%f)", __func__
					, static_cast<float>(currWayPoint.x)
					, static_cast<float>(currWayPoint.y)
					, static_cast<float>(currWayPoint.z));

				LOG("%s nextWayPoint (%f,%f,%f)", __func__
					, static_cast<float>(nextWayPoint.x)
					, static_cast<float>(nextWayPoint.y)
					, static_cast<float>(nextWayPoint.z));
			}
		}
		#endif

		const float3& opos = owner->pos;
		const float3& ovel = owner->speed;
		const float3&  ffd = flatFrontDir;
		const float3&  cwp = earlyCurrWayPoint;

		prevWayPointDist = currWayPointDist;
		currWayPointDist = earlyCurrWayPoint.distance2D(opos);

		{
			// NOTE:
			//   uses owner->pos instead of currWayPoint (ie. not the same as atEndOfPath)
			//
			//   if our first command is a build-order, then goal-radius is set to our build-range
			//   and we cannot increase tolerance safely (otherwise the unit might stop when still
			//   outside its range and fail to start construction)
			//
			//   units moving faster than <minGoalDist> elmos per frame might overshoot their goal
			//   the last two atGoal conditions will just cause flatFrontDir to be selected as the
			//   "wanted" direction when this happens
			const float curGoalDistSq = (opos - goalPos).SqLength2D();
			const float minGoalDistSq = (UNIT_HAS_MOVE_CMD(owner))?
				Square((goalRadius + extraRadius) * (numIdlingSlowUpdates + 1)):
				Square((goalRadius + extraRadius)                             );
			const float spdGoalDistSq = Square(currentSpeed * 1.05f);

			atGoal |= (curGoalDistSq <= minGoalDistSq);
			atGoal |= ((curGoalDistSq <= spdGoalDistSq) && !reversing && (ffd.dot(goalPos - opos) > 0.0f && ffd.dot(goalPos - (opos + ovel)) <= 0.0f));
			atGoal |= ((curGoalDistSq <= spdGoalDistSq) &&  reversing && (ffd.dot(goalPos - opos) < 0.0f && ffd.dot(goalPos - (opos + ovel)) >= 0.0f));

			atEndOfPath |= atGoal;

			#ifdef PATHING_DEBUG
			if (DEBUG_DRAWING_ENABLED) {
				bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
					&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
				if (printMoveInfo) {
					LOG("%s opos (%f,%f,%f)", __func__
						, static_cast<float>(opos.x)
						, static_cast<float>(opos.y)
						, static_cast<float>(opos.z));

					LOG("%s cwp (%f,%f,%f)", __func__
						, static_cast<float>(cwp.x)
						, static_cast<float>(cwp.y)
						, static_cast<float>(cwp.z));

					LOG("%s goalpos (%f,%f,%f)", __func__
						, static_cast<float>(goalPos.x)
						, static_cast<float>(goalPos.y)
						, static_cast<float>(goalPos.z));

					LOG("%s curGoalDistSq(%f) <= minGoalDistSq(%f)", __func__, curGoalDistSq, minGoalDistSq);
					LOG("%s (ffd.dot(goalPos - opos)(%f) ffd.dot(goalPos - (opos + ovel))(%f)", __func__, ffd.dot(goalPos - opos), ffd.dot(goalPos - (opos + ovel)));
					LOG("%s atGoal(%d?) reversing(%d?)", __func__, (int)atGoal, (int)reversing);
				}
			}
			#endif
		}

		if (!atGoal) {
			numIdlingUpdates -= ((numIdlingUpdates >                  0) * (1 - idling));
			numIdlingUpdates += ((numIdlingUpdates < SPRING_MAX_HEADING) *      idling );
		}

		// An updated path must be re-evaluated.
		if (atEndOfPath && !atGoal)
			atEndOfPath = !pathManager->PathUpdated(pathID);

		// atEndOfPath never becomes true when useRawMovement, except via StopMoving
		if (!atEndOfPath && !useRawMovement) {
			SetNextWayPoint(thread);
		} else {
			if (atGoal){
				#ifdef PATHING_DEBUG
				if (DEBUG_DRAWING_ENABLED) {
					bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
						&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
					if (printMoveInfo) {
						LOG("%s arrival signaled", __func__);
					}
				}
				#endif
				pathingArrived = true;
			}
		}

		// set direction to waypoint AFTER requesting it; should not be a null-vector
		// do not compare y-components since these usually differ and only x&z matter
		SetWaypointDir(cwp, opos);

		//ASSERT_SYNCED(waypointVec);
		//ASSERT_SYNCED(waypointDir);

		wantReverse = WantReverse(waypointDir, ffd);

		// apply obstacle avoidance (steering), prevent unit from chasing its own tail if already at goal
		// const float3  rawWantedDir = waypointDir * Sign(int(!wantReverse));
		// const float3& modWantedDir = GetObstacleAvoidanceDir(mix(ffd, rawWantedDir, !atGoal));

		// const float3& modWantedDir = GetObstacleAvoidanceDir(mix(ffd, rawWantedDir, (!atGoal) && (wpProjDists.x > wpProjDists.y || wpProjDists.z < 0.995f)));

		const float3& modWantedDir = lastAvoidanceDir;

		// ChangeHeading(GetHeadingFromVector(modWantedDir.x, modWantedDir.z));
		// ChangeSpeed(maxWantedSpeed, wantReverse);
		setHeading = HEADING_CHANGED_MOVE;
		auto& event = Sim::registry.get<ChangeHeadingEvent>(owner->entityReference);
		event.deltaHeading = GetHeadingFromVector(modWantedDir.x, modWantedDir.z);
		event.changed = true;
		// setHeadingDir = GetHeadingFromVector(modWantedDir.x, modWantedDir.z);


		#ifdef PATHING_DEBUG
		if (DEBUG_DRAWING_ENABLED) {
			bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
				&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
			if (printMoveInfo) {
				LOG("%s waypointDir (%f,%f,%f)", __func__
					, static_cast<float>(waypointDir.x)
					, static_cast<float>(waypointDir.y)
					, static_cast<float>(waypointDir.z));

				LOG("%s ffd (%f,%f,%f)", __func__
					, static_cast<float>(ffd.x)
					, static_cast<float>(ffd.y)
					, static_cast<float>(ffd.z));

				LOG("%s change heading (%f, %f) [%d]", __func__
						, modWantedDir.x, modWantedDir.z
						, GetHeadingFromVector(modWantedDir.x, modWantedDir.z));
				LOG("%s change speed (%f, %d?)", __func__, maxWantedSpeed, (int)wantReverse);
			}
		}
		#endif
	}

	//pathManager->UpdatePath(owner, pathID);
	return wantReverse;
}

void CGroundMoveType::SetWaypointDir(const float3& cwp, const float3 &opos) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!epscmp(cwp.x, opos.x, float3::cmp_eps()) || !epscmp(cwp.z, opos.z, float3::cmp_eps())) {
		float3 waypointVec = (cwp - opos) * XZVector;
		waypointDir = waypointVec / waypointVec.Length();
		// wpProjDists = {math::fabs(waypointVec.dot(ffd)), 1.0f, math::fabs(waypointDir.dot(ffd))};
	}
}

void CGroundMoveType::ChangeSpeed(float newWantedSpeed, bool wantReverse, bool fpsMode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// shortcut to specify acceleration to bring to a stop.
	if ((wantedSpeed = newWantedSpeed) <= 0.0f && currentSpeed < 0.01f) {
		deltaSpeed = -currentSpeed;
		return;
	}

	// first calculate the "unrestricted" speed and acceleration
	float targetSpeed = mix(maxSpeed, maxReverseSpeed, wantReverse);

	#if (WAIT_FOR_PATH == 1)
	// don't move until we have an actual path, trying to hide queuing
	// lag is too dangerous since units can blindly drive into objects,
	// cliffs, etc. (requires the QTPFS idle-check in Update)
	if (currWayPoint.y == -1.0f && nextWayPoint.y == -1.0f) {
		targetSpeed = 0.0f;
	} else
	#endif
	{
		if (wantedSpeed > 0.0f) {
			const UnitDef* ud = owner->unitDef;
			const MoveDef* md = owner->moveDef;

			// the pathfinders do NOT check the entire footprint to determine
			// passability wrt. terrain (only wrt. structures), so we look at
			// the center square ONLY for our current speedmod
			float groundSpeedMod = CMoveMath::GetPosSpeedMod(*md, owner->pos, flatFrontDir);

			// the pathfinders don't check the speedmod of the square our unit is currently on
			// so if we got stuck on a nonpassable square and can't move try to see if we're
			// trying to release ourselves towards a passable square
			if (groundSpeedMod == 0.0f)
				groundSpeedMod = CMoveMath::GetPosSpeedMod(*md, owner->pos + flatFrontDir * SQUARE_SIZE, flatFrontDir);

			const float curGoalDistSq = (owner->pos - goalPos).SqLength2D();
			const float minGoalDistSq = Square(BrakingDistance(currentSpeed, decRate));

			const float3& waypointDifFwd = waypointDir;
			const float3  waypointDifRev = -waypointDifFwd;

			const float3& waypointDif = mix(waypointDifFwd, waypointDifRev, reversing);
			const short turnDeltaHeading = owner->heading - GetHeadingFromVector(waypointDif.x, waypointDif.z);

			const bool startBraking = (UNIT_CMD_QUE_SIZE(owner) <= 1 && curGoalDistSq <= minGoalDistSq && !fpsMode);

			float maxSpeedToMakeTurn = std::numeric_limits<float>::infinity();

			if (!fpsMode && turnDeltaHeading != 0) {
				// only auto-adjust speed for turns when not in FPS mode
				const float reqTurnAngle = math::fabs(180.0f * short(owner->heading - wantedHeading) / SPRING_MAX_HEADING);
				const float maxTurnAngle = (turnRate / SPRING_CIRCLE_DIVS) * 360.0f;

				const float turnMaxSpeed = mix(maxSpeed, maxReverseSpeed, reversing);
				      float turnModSpeed = turnMaxSpeed;

				if (reqTurnAngle != 0.0f)
					turnModSpeed *= std::clamp(maxTurnAngle / reqTurnAngle, 0.1f, 1.0f);

				if (waypointDir.SqLength() > 0.1f) {
					if (!ud->turnInPlace) {
						// never let speed drop below TIPSL, but limit TIPSL itself to turnMaxSpeed
						targetSpeed = std::clamp(turnModSpeed, std::min(ud->turnInPlaceSpeedLimit, turnMaxSpeed), turnMaxSpeed);
					} else {
						targetSpeed = mix(targetSpeed, turnModSpeed, reqTurnAngle > ud->turnInPlaceAngleLimit);
					}
				}

				if (atEndOfPath) {
					// at this point, Update() will no longer call SetNextWayPoint()
					// and we must slow down to prevent entering an infinite circle
					// base ftt on maximum turning speed
					const float absTurnSpeed = turnRate;
					const float framesToTurn = SPRING_CIRCLE_DIVS / absTurnSpeed;

					targetSpeed = std::min(targetSpeed, (currWayPointDist * math::PI) / framesToTurn);
				}
			}

			// When a waypoint is moved, it is because of a collision with a building. This can
			// result in the unit having to loop back around, but we need to make sure it doesn't
			// go too fast looping back around that it crashes right back into the same building
			// and trigger a looping behaviour.
			if (limitSpeedForTurning > 0) {
				const int dirSign = Sign(int(!reversing));
				short idealHeading = GetHeadingFromVector(waypointDir.x, waypointDir.z);
				short offset = idealHeading - owner->heading;

				const float absTurnSpeed = std::max(0.0001f, math::fabs(turnRate));
				const float framesToTurn = std::max(0.0001f, math::fabs(offset / absTurnSpeed));

				maxSpeedToMakeTurn = std::max(0.01f, (currWayPointDist / framesToTurn) * (.95f));

				// bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
				// 	&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
				// if (printMoveInfo) {
				// 	LOG("%s: dirSign %d, waypointDir=(%f,%f,%f), idealHeading=%d, offset=%d, absTurnSpeed=%f, framesToTurn=%f, maxSpeedToMakeTurn=%f"
				// 			, __func__
				// 			, dirSign, waypointDir.x, waypointDir.y, waypointDir.z, idealHeading, offset
				// 			, absTurnSpeed, framesToTurn, maxSpeedToMakeTurn);
				// }
			}

			// now apply the terrain and command restrictions
			// NOTE:
			//   if wantedSpeed > targetSpeed, the unit will
			//   not accelerate to speed > targetSpeed unless
			//   its actual max{Reverse}Speed is also changed
			//
			//   raise wantedSpeed iff the terrain-modifier is
			//   larger than 1 (so units still get their speed
			//   bonus correctly), otherwise leave it untouched
			//
			//   disallow changing speed (except to zero) without
			//   a path if not in FPS mode (FIXME: legacy PFS can
			//   return path when none should exist, mantis3720)
			wantedSpeed *= std::max(groundSpeedMod, 1.0f);
			targetSpeed *= groundSpeedMod;
			targetSpeed *= (1 - startBraking);
			targetSpeed *= ((1 - WantToStop()) || fpsMode);
			targetSpeed = std::min(targetSpeed, wantedSpeed);
			targetSpeed = std::min(targetSpeed, maxSpeedToMakeTurn);
		} else {
			targetSpeed = 0.0f;
		}
	}

	deltaSpeed = pathController.GetDeltaSpeed(
		pathID,
		targetSpeed,
		currentSpeed,
		accRate,
		decRate,
		wantReverse,
		reversing
	);
}

/*
 * Changes the heading of the owner.
 * Also updates world position of aim points, and orientation if walking over terrain slopes.
 * FIXME near-duplicate of HoverAirMoveType::UpdateHeading
 */
void CGroundMoveType::ChangeHeading(short newHeading) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (owner->IsFlying())
		return;
	if (owner->GetTransporter() != nullptr)
		return;

	wantedHeading = newHeading;
	if (owner->heading == wantedHeading) {
		owner->UpdateDirVectors(!owner->upright && owner->IsOnGround(), owner->IsInAir(), owner->unitDef->upDirSmoothing);
		return;
	}

	#if (MODEL_TURN_INERTIA == 0)
	const short rawDeltaHeading = pathController.GetDeltaHeading(pathID, wantedHeading, owner->heading, turnRate);
	#else
	// model rotational inertia (more realistic for ships)
	const short rawDeltaHeading = pathController.GetDeltaHeading(pathID, wantedHeading, owner->heading, turnRate, turnAccel, BrakingDistance(turnSpeed, turnAccel), &turnSpeed);
	#endif
	const short absDeltaHeading = rawDeltaHeading * Sign(rawDeltaHeading);

	if (absDeltaHeading >= minScriptChangeHeading)
		owner->script->ChangeHeading(rawDeltaHeading);

	owner->AddHeading(rawDeltaHeading, !owner->upright && owner->IsOnGround(), owner->IsInAir(), owner->unitDef->upDirSmoothing);

	flatFrontDir = (owner->frontdir * XZVector).Normalize();
}


bool CGroundMoveType::CanApplyImpulse(const float3& impulse)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// NOTE: ships must be able to receive impulse too (for collision handling)
	if (owner->beingBuilt)
		return false;
	// will be applied to transporter instead
	if (owner->GetTransporter() != nullptr)
		return false;
	if (impulse.SqLength() <= 0.01f)
		return false;

	UseHeading(false);

	skidRotSpeed = 0.0f;
	skidRotAccel = 0.0f;

	float3 newSpeed = owner->speed + impulse;
	float3 skidDir = mix(float3(owner->frontdir), owner->frontdir * -1, reversing);

	// NOTE:
	//   we no longer delay the skidding-state until owner has "accumulated" an
	//   arbitrary hardcoded amount of impulse (possibly across several frames),
	//   but enter it on any vector that causes speed to become misaligned with
	//   frontdir
	// TODO
	//   there should probably be a configurable minimum-impulse below which the
	//   unit does not react at all but also does NOT "store" the impulse like a
	//   small-charge capacitor, or a more physically-based approach (|N*m*g*cf|
	//   > |impulse/dt|) could be used
	//
	const bool startSkidding = StartSkidding(newSpeed, skidDir);
	const bool startFlying = StartFlying(newSpeed, CGround::GetNormal(owner->pos.x, owner->pos.z));

	if (startSkidding)
		owner->script->StartSkidding(newSpeed);

	if (newSpeed.SqLength2D() >= 0.01f)
		skidDir = newSpeed.Normalize2D();

	skidRotVector = skidDir.cross(UpVector) * startSkidding;
	skidRotAccel = ((gsRNG.NextFloat() - 0.5f) * 0.04f) * startFlying;

	owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING * (startSkidding | startFlying));
	owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING * startFlying);

	// indicate we want to react to the impulse
	return true;
}

void CGroundMoveType::UpdateSkid()
{
	RECOIL_DETAILED_TRACY_ZONE;
	ASSERT_SYNCED(owner->midPos);

	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	const float minCollSpeed = owner->unitDef->minCollisionSpeed;
	const float groundHeight = GetGroundHeight(pos);
	const float negAltitude = groundHeight - pos.y;

	owner->SetVelocity(
		spd +
		owner->GetDragAccelerationVec(
			mapInfo->atmosphere.fluidDensity,
			mapInfo->water.fluidDensity,
			owner->unitDef->atmosphericDragCoefficient,
			owner->unitDef->groundFrictionCoefficient
		)
	);

	if (owner->IsFlying()) {
		const float collImpactSpeed = pos.IsInBounds()?
			-spd.dot(CGround::GetNormal(pos.x, pos.z)):
			-spd.dot(UpVector);
		const float impactDamageMul = collImpactSpeed * owner->mass * COLLISION_DAMAGE_MULT;

		if (negAltitude > 0.0f) {
			// ground impact, stop flying
			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			owner->Move(UpVector * negAltitude, true);

			// deal ground impact damage
			// TODO:
			//   bouncing behaves too much like a rubber-ball,
			//   most impact energy needs to go into the ground
			if (modInfo.allowUnitCollisionDamage && collImpactSpeed > minCollSpeed && minCollSpeed >= 0.0f)
				owner->DoDamage(DamageArray(impactDamageMul), ZeroVector, nullptr, -CSolidObject::DAMAGE_COLLISION_GROUND, -1);

			skidRotSpeed = 0.0f;
			// skidRotAccel = 0.0f;
		} else {
			owner->SetVelocity(spd + (UpVector * mapInfo->map.gravity));
		}
	} else {
		// *assume* this means the unit is still on the ground
		// (Lua gadgetry can interfere with our "physics" logic)
		float skidRotSpd = 0.0f;

		const bool onSlope = OnSlope(0.0f);
		const bool stopSkid = StopSkidding(spd, owner->frontdir);

		if (!onSlope && stopSkid) {
			skidRotSpd = math::floor(skidRotSpeed + skidRotAccel + 0.5f);
			skidRotAccel = (skidRotSpd - skidRotSpeed) * 0.5f;
			skidRotAccel *= math::DEG_TO_RAD;

			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);
			owner->script->StopSkidding();

			UseHeading(true);
			// update wanted-heading after coming to a stop
			ChangeHeading(owner->heading);
		} else {
			constexpr float speedReduction = 0.35f;

			// number of frames until rotational speed would drop to 0
			const float speedScale = owner->SetSpeed(spd);
			const float rotRemTime = std::max(1.0f, speedScale / speedReduction);

			if (onSlope) {
				const float3& normalVector = CGround::GetNormal(pos.x, pos.z);
				const float3 normalForce = normalVector * normalVector.dot(UpVector * mapInfo->map.gravity);
				const float3 newForce = UpVector * mapInfo->map.gravity - normalForce;

				owner->SetVelocity(spd + newForce);
				owner->SetVelocity(spd * (1.0f - (0.1f * normalVector.y)));
			} else {
				// RHS is clamped to 0..1
				owner->SetVelocity(spd * (1.0f - std::min(1.0f, speedReduction / speedScale)));
			}

			skidRotSpd = math::floor(skidRotSpeed + skidRotAccel * (rotRemTime - 1.0f) + 0.5f);
			skidRotAccel = (skidRotSpd - skidRotSpeed) / rotRemTime;
			skidRotAccel *= math::DEG_TO_RAD;

			if (math::floor(skidRotSpeed) != math::floor(skidRotSpeed + skidRotAccel)) {
				skidRotSpeed = 0.0f;
				skidRotAccel = 0.0f;
			}
		}

		if (negAltitude < (spd.y + mapInfo->map.gravity)) {
			owner->SetVelocity(spd + (UpVector * mapInfo->map.gravity));

			// flying requires skidding and relies on CalcSkidRot
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);

			UseHeading(false);
		} else if (negAltitude > spd.y) {
			// LHS is always negative, so this becomes true when the
			// unit is falling back down and will impact the ground
			// in one frame
			const float3& gndNormal = (pos.IsInBounds())? CGround::GetNormal(pos.x, pos.z): UpVector;
			const float projSpeed = spd.dot(gndNormal);

			if (projSpeed > 0.0f) {
				// not possible without lateral movement
				owner->SetVelocity(spd * 0.95f);
			} else {
				owner->SetVelocity(spd + (gndNormal * (math::fabs(projSpeed) + 0.1f)));
				owner->SetVelocity(spd * 0.8f);
			}
		}
	}

	// finally update speed.w
	owner->SetSpeed(spd);
	// translate before rotate, match terrain normal if not in air
	owner->Move(spd, true);
	owner->UpdateDirVectors(!owner->upright && owner->IsOnGround(), owner->IsInAir(), owner->unitDef->upDirSmoothing);

	if (owner->IsSkidding()) {
		CalcSkidRot();
		CheckCollisionSkid();
	} else {
		// do this here since ::Update returns early if it calls us
		// HandleObjectCollisions(); // No longer the case
	}

	AdjustPosToWaterLine();

	// always update <oldPos> here so that <speed> does not make
	// extreme jumps when the unit transitions from skidding back
	// to non-skidding
	oldPos = owner->pos;

	ASSERT_SANE_OWNER_SPEED(spd);
	ASSERT_SYNCED(owner->midPos);
}

void CGroundMoveType::UpdateControlledDrop()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;
	const float3  acc = UpVector * std::min(mapInfo->map.gravity * owner->fallSpeed, 0.0f);
	const float   alt = GetGroundHeight(pos) - pos.y;

	owner->SetVelocity(spd + acc);
	owner->SetVelocity(
		spd +
		owner->GetDragAccelerationVec(
			mapInfo->atmosphere.fluidDensity,
			mapInfo->water.fluidDensity,
			owner->unitDef->atmosphericDragCoefficient,
			owner->unitDef->groundFrictionCoefficient * 10
		)
	);
	owner->SetSpeed(spd);
	owner->Move(spd, true);

	if (alt <= 0.0f)
		return;

	// ground impact, stop parachute animation
	owner->Move(UpVector * alt, true);
	owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FALLING);
	owner->script->Landed();
}

void CGroundMoveType::CheckCollisionSkid()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* collider = owner;

	// NOTE:
	//   the QuadField::Get* functions check o->midPos,
	//   but the quad(s) that objects are stored in are
	//   derived from o->pos (!)
	const float3& pos = collider->pos;

	const float colliderMinCollSpeed = collider->unitDef->minCollisionSpeed;
	      float collideeMinCollSpeed = 0.0f;

	// copy on purpose, since the below can call Lua
	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos, collider->radius);
	quadField.GetFeaturesExact(qfQuery, pos, collider->radius);

	for (CUnit* collidee: *qfQuery.units) {
		if (!collidee->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
			continue;

		const UnitDef* collideeUD = collidee->unitDef;

		const float sqDist = (pos - collidee->pos).SqLength();
		const float totRad = collider->radius + collidee->radius;

		if ((sqDist >= totRad * totRad) || (sqDist <= 0.01f))
			continue;

		const float3 collSeparationDir = (pos - collidee->pos).SafeNormalize();

		if (collideeUD->IsImmobileUnit()) {
			const float collImpactSpeed = -collider->speed.dot(collSeparationDir);
			const float impactDamageMul = std::min(collImpactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);

			if (collImpactSpeed <= 0.0f)
				continue;

			// damage the collider, no added impulse
			if (modInfo.allowUnitCollisionDamage && collImpactSpeed > colliderMinCollSpeed && colliderMinCollSpeed >= 0.0f)
				collider->DoDamage(DamageArray(impactDamageMul), ZeroVector, collidee, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

			// damage the (static) collidee based on collider's mass, no added impulse
			if (modInfo.allowUnitCollisionDamage && collImpactSpeed > (collideeMinCollSpeed = collideeUD->minCollisionSpeed) && collideeMinCollSpeed >= 0.0f)
				collidee->DoDamage(DamageArray(impactDamageMul), ZeroVector, collider, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

			collider->Move(collSeparationDir * collImpactSpeed, true);
			collider->SetVelocity(collider->speed + ((collSeparationDir * collImpactSpeed) * 1.8f));
		} else {
			assert(collider->mass > 0.0f && collidee->mass > 0.0f);

			// don't conserve momentum (impact speed is halved, so impulses are too)
			// --> collisions are neither truly elastic nor truly inelastic to prevent
			// the simulation from blowing up from impulses applied to tight groups of
			// units
			const float collImpactSpeed = (collidee->speed - collider->speed).dot(collSeparationDir) * 0.5f;
			const float colliderRelMass = (collider->mass / (collider->mass + collidee->mass));
			const float colliderRelImpactSpeed = collImpactSpeed * (1.0f - colliderRelMass);
			const float collideeRelImpactSpeed = collImpactSpeed * (       colliderRelMass);

			const float  colliderImpactDmgMult = std::min(colliderRelImpactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);
			const float  collideeImpactDmgMult = std::min(collideeRelImpactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);
			const float3 colliderImpactImpulse = collSeparationDir * colliderRelImpactSpeed;
			const float3 collideeImpactImpulse = collSeparationDir * collideeRelImpactSpeed;

			if (collImpactSpeed <= 0.0f)
				continue;

			// damage the collider
			if (modInfo.allowUnitCollisionDamage && collImpactSpeed > colliderMinCollSpeed && colliderMinCollSpeed >= 0.0f)
				collider->DoDamage(DamageArray(colliderImpactDmgMult), collSeparationDir * colliderImpactDmgMult, collidee, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

			// damage the collidee
			if (modInfo.allowUnitCollisionDamage && collImpactSpeed > (collideeMinCollSpeed = collideeUD->minCollisionSpeed) && collideeMinCollSpeed >= 0.0f)
				collidee->DoDamage(DamageArray(collideeImpactDmgMult), collSeparationDir * -collideeImpactDmgMult, collider, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

			collider->Move( colliderImpactImpulse, true);
			collidee->Move(-collideeImpactImpulse, true);
			collider->SetVelocity        (collider->speed + colliderImpactImpulse);
			collidee->SetVelocityAndSpeed(collidee->speed - collideeImpactImpulse);
		}
	}

	for (CFeature* collidee: *qfQuery.features) {
		if (!collidee->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
			continue;

		const float sqDist = (pos - collidee->pos).SqLength();
		const float totRad = collider->radius + collidee->radius;

		if ((sqDist >= totRad * totRad) || (sqDist <= 0.01f))
			continue;

		const float3 collSeparationDir = (pos - collidee->pos).SafeNormalize();
		const float collImpactSpeed = -collider->speed.dot(collSeparationDir);
		const float impactDamageMul = std::min(collImpactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);
		const float3 impactImpulse = collSeparationDir * collImpactSpeed;

		if (collImpactSpeed <= 0.0f)
			continue;

		// damage the collider, no added impulse (!)
		// the collidee feature can not be passed along to the collider as attacker
		// yet, keep symmetry and do not pass collider along to the collidee either
		if (modInfo.allowUnitCollisionDamage && collImpactSpeed > colliderMinCollSpeed && colliderMinCollSpeed >= 0.0f)
			collider->DoDamage(DamageArray(impactDamageMul), ZeroVector, nullptr, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

		// damage the collidee feature based on collider's mass
		collidee->DoDamage(DamageArray(impactDamageMul), -impactImpulse, nullptr, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

		collider->Move(impactImpulse, true);
		collider->SetVelocity(collider->speed + (impactImpulse * 1.8f));
	}

	// finally update speed.w
	collider->SetSpeed(collider->speed);

	ASSERT_SANE_OWNER_SPEED(collider->speed);
}

void CGroundMoveType::CalcSkidRot()
{
	RECOIL_DETAILED_TRACY_ZONE;
	skidRotSpeed += skidRotAccel;
	skidRotSpeed *= 0.999f;
	skidRotAccel *= 0.95f;

	const float angle = (skidRotSpeed / GAME_SPEED) * math::TWOPI;
	const float cosp = math::cos(angle);
	const float sinp = math::sin(angle);

	float3 f1 = skidRotVector * skidRotVector.dot(owner->frontdir);
	float3 f2 = owner->frontdir - f1;

	float3 r1 = skidRotVector * skidRotVector.dot(owner->rightdir);
	float3 r2 = owner->rightdir - r1;

	float3 u1 = skidRotVector * skidRotVector.dot(owner->updir);
	float3 u2 = owner->updir - u1;

	f2 = f2 * cosp + f2.cross(skidRotVector) * sinp;
	r2 = r2 * cosp + r2.cross(skidRotVector) * sinp;
	u2 = u2 * cosp + u2.cross(skidRotVector) * sinp;

	owner->frontdir = f1 + f2;
	owner->rightdir = r1 + r2;
	owner->updir    = u1 + u2;

	owner->UpdateMidAndAimPos();
}




/*
 * Dynamic obstacle avoidance, helps the unit to
 * follow the path even when it's not perfect.
 */
float3 CGroundMoveType::GetObstacleAvoidanceDir(const float3& desiredDir) {
	#if (IGNORE_OBSTACLES == 1)
	return desiredDir;
	#endif

	// obstacle-avoidance only needs to run if the unit wants to move
	if (WantToStop())
		return lastAvoidanceDir = flatFrontDir;

	// Speed-optimizer. Reduces the times this system is run.
	if ((gs->frameNum + owner->id) % modInfo.groundUnitCollisionAvoidanceUpdateRate) {
		if (!avoidingUnits)
			lastAvoidanceDir = desiredDir;

		return lastAvoidanceDir;
	}

	float3 avoidanceVec = ZeroVector;
	float3 avoidanceDir = desiredDir;

	if (avoidingUnits)
		avoidingUnits = false;

	lastAvoidanceDir = desiredDir;

	CUnit* avoider = owner;

	// const UnitDef* avoiderUD = avoider->unitDef;
	const MoveDef* avoiderMD = avoider->moveDef;

	// degenerate case: if facing anti-parallel to desired direction,
	// do not actively avoid obstacles since that can interfere with
	// normal waypoint steering (if the final avoidanceDir demands a
	// turn in the opposite direction of desiredDir)
	if (avoider->frontdir.dot(desiredDir) < 0.0f)
		return lastAvoidanceDir;

	static constexpr float AVOIDER_DIR_WEIGHT = 1.0f;
	static constexpr float DESIRED_DIR_WEIGHT = 0.5f;
	static constexpr float LAST_DIR_MIX_ALPHA = 0.7f;
	static const     float MAX_AVOIDEE_COSINE = math::cosf(120.0f * math::DEG_TO_RAD);

	// now we do the obstacle avoidance proper
	// avoider always uses its never-rotated MoveDef footprint
	// note: should increase radius for smaller turnAccel values
	const float avoidanceRadius = std::max(currentSpeed, 1.0f) * (avoider->radius * 2.0f);
	const float avoiderRadius = avoiderMD->CalcFootPrintMinExteriorRadius();

	MoveTypes::CheckCollisionQuery avoiderInfo(avoider);

	QuadFieldQuery qfQuery;
	qfQuery.threadOwner = ThreadPool::GetThreadNum();
	quadField.GetSolidsExact(qfQuery, avoider->pos, avoidanceRadius, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);

	for (const CSolidObject* avoidee: *qfQuery.solids) {
		const MoveDef* avoideeMD = avoidee->moveDef;
		const UnitDef* avoideeUD = dynamic_cast<const UnitDef*>(avoidee->GetDef());

		// cases in which there is no need to avoid this obstacle
		if (avoidee == owner)
			continue;
		// do not avoid statics (it interferes too much with PFS)
		if (avoideeMD == nullptr)
			continue;
		// ignore aircraft (or flying ground units)
		if (avoidee->IsInAir() || avoidee->IsFlying())
			continue;
		if (CMoveMath::IsNonBlocking(avoidee, &avoiderInfo))
			continue;
		if (!CMoveMath::CrushResistant(*avoiderMD, avoidee))
			continue;

		const bool avoideeMobile  = (avoideeMD != nullptr);
		const bool avoideeMovable = (avoideeUD != nullptr && !static_cast<const CUnit*>(avoidee)->moveType->IsPushResistant());

		const float3 avoideeVector = (avoider->pos + avoider->speed) - (avoidee->pos + avoidee->speed);

		// use the avoidee's MoveDef footprint as radius if it is mobile
		// use the avoidee's Unit (not UnitDef) footprint as radius otherwise
		const float avoideeRadius = avoideeMobile?
			avoideeMD->CalcFootPrintMinExteriorRadius():
			avoidee->CalcFootPrintMinExteriorRadius();
		const float avoidanceRadiusSum = avoiderRadius + avoideeRadius;
		const float avoidanceMassSum = avoider->mass + avoidee->mass;
		const float avoideeMassScale = avoideeMobile? (avoidee->mass / avoidanceMassSum): 1.0f;
		const float avoideeDistSq = avoideeVector.SqLength();
		const float avoideeDist   = math::sqrt(avoideeDistSq) + 0.01f;

		// do not bother steering around idling MOBILE objects
		// (since collision handling will just push them aside)
		if (avoideeMobile && avoideeMovable) {
			if (!avoiderMD->avoidMobilesOnPath || (!avoidee->IsMoving() && avoidee->allyteam == avoider->allyteam))
				continue;
		}

		// ignore objects that are more than this many degrees off-center from us
		// NOTE:
		//   if MAX_AVOIDEE_COSINE is too small, then this condition can be true
		//   one frame and false the next (after avoider has turned) causing the
		//   avoidance vector to oscillate --> units with turnInPlace = true will
		//   slow to a crawl as a result
		if (avoider->frontdir.dot(-(avoideeVector / avoideeDist)) < MAX_AVOIDEE_COSINE)
			continue;

		if (avoideeDistSq >= Square(std::max(currentSpeed, 1.0f) * GAME_SPEED + avoidanceRadiusSum))
			continue;
		if (avoideeDistSq >= avoider->pos.SqDistance2D(goalPos))
			continue;

		// if object and unit in relative motion are closing in on one another
		// (or not yet fully apart), then the object is on the path of the unit
		// and they are not collided
		if (DEBUG_DRAWING_ENABLED) {
			if (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end()){
				geometryLock.lock();
				geometricObjects->AddLine(avoider->pos + (UpVector * 20.0f), avoidee->pos + (UpVector * 20.0f), 3, 1, 4);
				geometryLock.unlock();
			}
		}

		float avoiderTurnSign = -Sign(avoidee->pos.dot(avoider->rightdir) - avoider->pos.dot(avoider->rightdir));
		float avoideeTurnSign = -Sign(avoider->pos.dot(avoidee->rightdir) - avoidee->pos.dot(avoidee->rightdir));

		// for mobile units, avoidance-response is modulated by angle
		// between avoidee's and avoider's frontdir such that maximal
		// avoidance occurs when they are anti-parallel
		const float avoidanceCosAngle = std::clamp(avoider->frontdir.dot(avoidee->frontdir), -1.0f, 1.0f);
		const float avoidanceResponse = (1.0f - avoidanceCosAngle * int(avoideeMobile)) + 0.1f;
		const float avoidanceFallOff  = (1.0f - std::min(1.0f, avoideeDist / (5.0f * avoidanceRadiusSum)));

		// if parties are anti-parallel, it is always more efficient for
		// both to turn in the same local-space direction (either R/R or
		// L/L depending on relative object positions) but there exists
		// a range of orientations for which the signs are not equal
		//
		// (this is also true for the parallel situation, but there the
		// degeneracy only occurs when one of the parties is behind the
		// other and can be ignored)
		if (avoidanceCosAngle < 0.0f)
			avoiderTurnSign = std::max(avoiderTurnSign, avoideeTurnSign);

		avoidanceDir = avoider->rightdir * AVOIDER_DIR_WEIGHT * avoiderTurnSign;
		avoidanceVec += (avoidanceDir * avoidanceResponse * avoidanceFallOff * avoideeMassScale);

		if (!avoidingUnits)
			avoidingUnits = true;
	}

	// use a weighted combination of the desired- and the avoidance-directions
	// also linearly smooth it using the vector calculated the previous frame
	avoidanceDir = (mix(desiredDir, avoidanceVec, DESIRED_DIR_WEIGHT)).SafeNormalize();
	avoidanceDir = (mix(avoidanceDir, lastAvoidanceDir, LAST_DIR_MIX_ALPHA)).SafeNormalize();

	if (DEBUG_DRAWING_ENABLED) {
		if (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end()) {
			const float3 p0 = owner->pos + (    UpVector * 20.0f);
			const float3 p1 =         p0 + (avoidanceVec * 40.0f);
			const float3 p2 =         p0 + (avoidanceDir * 40.0f);

			geometryLock.lock();
			const int avFigGroupID = geometricObjects->AddLine(p0, p1, 8.0f, 1, 4);
			const int adFigGroupID = geometricObjects->AddLine(p0, p2, 8.0f, 1, 4);

			geometricObjects->SetColor(avFigGroupID, 1, 0.3f, 0.3f, 0.6f);
			geometricObjects->SetColor(adFigGroupID, 1, 0.3f, 0.3f, 0.6f);
			geometryLock.unlock();
		}
	}

	return (lastAvoidanceDir = avoidanceDir);
}



#if 0
// Calculates an approximation of the physical 2D-distance between given two objects.
// Old, no longer used since all separation tests are based on FOOTPRINT_RADIUS now.
float CGroundMoveType::Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal)
{
	// calculate the distance in (x,z) depending
	// on the shape of the object footprints
	float dist2D = 0.0f;

	const float xs = ((object1->xsize + object2->xsize) * (SQUARE_SIZE >> 1));
	const float zs = ((object1->zsize + object2->zsize) * (SQUARE_SIZE >> 1));

	if (object1->xsize == object1->zsize || object2->xsize == object2->zsize) {
		// use xsize as a cylindrical radius.
		const float3 distVec = object1->midPos - object2->midPos;

		dist2D = distVec.Length2D() - xs + 2.0f * marginal;
	} else {
		// Pytagorean sum of the x and z distance.
		float3 distVec;

		const float xdiff = math::fabs(object1->midPos.x - object2->midPos.x);
		const float zdiff = math::fabs(object1->midPos.z - object2->midPos.z);

		distVec.x = xdiff - xs + 2.0f * marginal;
		distVec.z = zdiff - zs + 2.0f * marginal;

		if (distVec.x > 0.0f && distVec.z > 0.0f) {
			dist2D = distVec.Length2D();
		} else if (distVec.x < 0.0f && distVec.z < 0.0f) {
			dist2D = -distVec.Length2D();
		} else if (distVec.x > 0.0f) {
			dist2D = distVec.x;
		} else {
			dist2D = distVec.z;
		}
	}

	return dist2D;
}
#endif

// Creates a path to the goal.
unsigned int CGroundMoveType::GetNewPath()
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!ThreadPool::inMultiThreadedSection);
	unsigned int newPathID = 0;

	#ifdef PATHING_DEBUG
	if (DEBUG_DRAWING_ENABLED) {
		bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
			&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
		if (printMoveInfo) {
			LOG("%s useRawMovement (%d)", __func__, useRawMovement);
		}
	}
	#endif

	if (useRawMovement)
		return newPathID;

	#ifdef PATHING_DEBUG
	if (DEBUG_DRAWING_ENABLED) {
		bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
			&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
		if (printMoveInfo) {
			LOG("%s Dist Away (%f) Goal Radius (%f)", __func__
					, (owner->pos - goalPos).SqLength2D()
					, Square(goalRadius + extraRadius));
		}
	}
	#endif

	// avoid frivolous requests if called from outside StartMoving*()
	if ((owner->pos - goalPos).SqLength2D() <= Square(goalRadius + extraRadius))
		return newPathID;

	if ((newPathID = pathManager->RequestPath(owner, owner->moveDef, owner->pos, goalPos, goalRadius + extraRadius, true)) != 0) {
		atGoal = false;
		atEndOfPath = false;
		lastWaypoint = false;

		earlyCurrWayPoint = currWayPoint = pathManager->NextWayPoint(owner, newPathID, 0,   owner->pos, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);
		earlyNextWayPoint = nextWayPoint = pathManager->NextWayPoint(owner, newPathID, 0, currWayPoint, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);

		pathController.SetRealGoalPosition(newPathID, goalPos);
		pathController.SetTempGoalPosition(newPathID, currWayPoint);
	} else {
		Fail(false);
	}

	return newPathID;
}

void CGroundMoveType::ReRequestPath(bool forceRequest) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (forceRequest) {
		assert(!ThreadPool::inMultiThreadedSection);
		// StopEngine(false);
		StartEngine(false);
		wantRepath = false;
		lastRepathFrame = gs->frameNum;
		return;
	}

	if (!wantRepath) {
		wantRepath = true;
		wantRepathFrame = gs->frameNum;
		bestLastWaypointDist = std::numeric_limits<float>::infinity();
	}
}

bool CGroundMoveType::CanSetNextWayPoint(int thread) {
	ZoneScoped;

	if (pathID == 0)
		return false;
	if (!pathController.AllowSetTempGoalPosition(pathID, nextWayPoint))
		return false;
	if (atEndOfPath)
		return false;

	const float3& pos = owner->pos;
		  float3& cwp = earlyCurrWayPoint;
		  float3& nwp = earlyNextWayPoint;

	// QTPFS ONLY PATH
	if (pathManager->PathUpdated(pathID)) {
		// path changed while we were following it (eg. due
		// to terrain deformation) in between two waypoints
		// but still has the same ID; in this case (which is
		// specific to QTPFS) we don't go through GetNewPath
		//
		cwp = pathManager->NextWayPoint(owner, pathID, 0, pos, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);
		nwp = pathManager->NextWayPoint(owner, pathID, 0, cwp, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);

		currWayPointDist = cwp.distance2D(pos);
		SetWaypointDir(cwp, pos);
		wantRepath = false;
	
		pathManager->ClearPathUpdated(pathID);
	}

	if (earlyCurrWayPoint.y == -1.0f || earlyNextWayPoint.y == -1.0f)
		return true;

	if (DEBUG_DRAWING_ENABLED) {
		if (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end()) {
			// plot the vectors to {curr, next}WayPoint

			geometryLock.lock();
			const int cwpFigGroupID = geometricObjects->AddLine(pos + (UpVector * 20.0f), cwp + (UpVector * (pos.y + 20.0f)), 8.0f, 1, 4);
			const int nwpFigGroupID = geometricObjects->AddLine(pos + (UpVector * 20.0f), nwp + (UpVector * (pos.y + 20.0f)), 8.0f, 1, 4);

			geometricObjects->SetColor(cwpFigGroupID, 1, 0.3f, 0.3f, 0.6f);
			geometricObjects->SetColor(nwpFigGroupID, 1, 0.3f, 0.3f, 0.6f);
			geometryLock.unlock();
		}
	}

	float cwpDistSq = cwp.SqDistance2D(pos);
	// -1 to avoid units checking for corners to rotate slightly and fail to escape when at max
	// distance for allowSkip. The slide/corner checks are done upto 8 elmos.
	const bool allowSkip = (cwpDistSq < Square(SQUARE_SIZE - 1));
	if (!allowSkip) {
		const bool skipRequested = (earlyCurrWayPoint.y == -2.0f || earlyNextWayPoint.y == -2.0f);
		if (!skipRequested) {
			// perform a turn-radius check: if the waypoint lies outside
			// our turning circle, do not skip since we can steer toward
			// this waypoint and pass it without slowing down
			// note that the DIAMETER of the turning circle is calculated
			// to prevent sine-like "snaking" trajectories; units capable
			// of instant turns *and* high speeds also need special care
			const int dirSign = Sign(int(!reversing));

			const float absTurnSpeed = turnRate;
			const float framesToTurn = SPRING_CIRCLE_DIVS / absTurnSpeed;

			const float turnRadius = std::max((currentSpeed * framesToTurn) * math::INVPI2, currentSpeed * 1.05f) * 2.f;
			const float waypointDot = std::clamp(waypointDir.dot(flatFrontDir * dirSign), -1.0f, 1.0f);

			#if 1

			// #ifdef PATHING_DEBUG
			// if (DEBUG_DRAWING_ENABLED)
			// {
			// 	bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
			// 		&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
			// 	if (printMoveInfo) {
			// 		LOG("%s absTurnSpeed %f, cwpDistSq=%f, skip=%d", __func__, absTurnSpeed, cwpDistSq, int(allowSkip));
			// 		LOG("%s turnradius %f = max(%f*%f*%f=%f, %f)", __func__
			// 				, turnRadius, currentSpeed, framesToTurn, math::INVPI2
			// 				, (currentSpeed * framesToTurn) * math::INVPI2
			// 				, currentSpeed * 1.05f);
			// 		LOG("%s currWayPointDist %f, turn radius = %f", __func__, currWayPointDist, turnRadius);
			// 	}
			// }
			// #endif

			// wp outside turning circle
			if (currWayPointDist > turnRadius)
				return false;

			// #ifdef PATHING_DEBUG
			// if (DEBUG_DRAWING_ENABLED)
			// {
			// 	bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
			// 		&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
			// 	if (printMoveInfo) {
			// 		LOG("%s currWayPointDist %f, max=%f, wayDot=%f", __func__
			// 				, currWayPointDist
			// 				, std::max(SQUARE_SIZE * 1.0f, currentSpeed * 1.05f)
			// 				, waypointDot);
			// 	}
			// }
			// #endif

			// wp inside but ~straight ahead and not reached within one tick
			if (currWayPointDist > std::max(SQUARE_SIZE * 1.0f, currentSpeed * 1.05f) && waypointDot >= 0.995f)
				return false;

			#else

			if ((currWayPointDist > std::max(turnRadius * 2.0f, 1.0f * SQUARE_SIZE)) && (waypointDot >= 0.0f))
				return false;

			if ((currWayPointDist > std::max(turnRadius * 1.0f, 1.0f * SQUARE_SIZE)) && (waypointDot <  0.0f))
				return false;

			if (math::acosf(waypointDot) < ((turnRate / SPRING_CIRCLE_DIVS) * math::TWOPI))
				return false;
			#endif

		}

		// Check if the unit has overshot the current waypoint and on route to the next waypoint.
		// const float3& p0 = earlyCurrWayPoint, v0 = float3(p0.x - pos.x, 0.0f, p0.z - pos.z);
		// const float3& p1 = earlyNextWayPoint, v1 = float3(p1.x - pos.x, 0.0f, p1.z - pos.z);
		// bool unitIsBetweenWaypoints = (v0.dot(v1) <= -0.f);

		// The last waypoint on a bad path will never pass a range check so don't try.
		//bool doRangeCheck = !pathManager->NextWayPointIsUnreachable(pathID);
						// && !unitIsBetweenWaypoints;
		//if (doRangeCheck) {
			const float searchRadius = std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f);
			const float3 targetPos = nwp;

			// check the between pos and cwp for obstacles
			// if still further than SS elmos from waypoint, disallow skipping
			const bool rangeTest = owner->moveDef->DoRawSearch(owner, owner->moveDef, pos, targetPos, 0, true, true, false, nullptr, nullptr, nullptr, thread);

			// {
			// bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
			// 	&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
			// if (printMoveInfo)
			// 	LOG("%s: test move square (%f,%f)->(%f,%f) = %f (range=%d skip=%d)", __func__
			// 			, pos.x, pos.z, targetPos.x, targetPos.z, math::sqrtf(cwpDistSq)
			// 			, int(rangeTest), int(allowSkip));
			// }

			if (!rangeTest)
				return false;
		//}
	}

	{
		const float curGoalDistSq = (earlyCurrWayPoint - goalPos).SqLength2D();
		const float minGoalDistSq = (UNIT_HAS_MOVE_CMD(owner)) ?
			Square((goalRadius + extraRadius) * (numIdlingSlowUpdates + 1)):
			Square((goalRadius + extraRadius)                             );

		// trigger Arrived on the next Update (only if we have non-temporary waypoints)
		// note:
		//   coldet can (very rarely) interfere with this, causing it to remain false
		//   a unit would then keep moving along its final waypoint-direction forever
		//   if atGoal, so we require waypointDir to always be updated in FollowPath
		//   (checking curr == next is not perfect, becomes true a waypoint too early)
		//
		// atEndOfPath |= (currWayPoint == nextWayPoint);
		atEndOfPath |= (curGoalDistSq <= minGoalDistSq);

		if (!atEndOfPath) {
			lastWaypoint |= (earlyCurrWayPoint.same(earlyNextWayPoint)
						&& pathManager->CurrentWaypointIsUnreachable(pathID)
						&& (cwpDistSq <= minGoalDistSq));
			if (lastWaypoint) {
				// incomplete path and last valid waypoint has been reached.
				pathingFailed = true;
				return false;
			}
		}
	}

	if (atEndOfPath) {
		earlyCurrWayPoint = goalPos;
		earlyNextWayPoint = goalPos;
		return false;
	}

	return true;
}

void CGroundMoveType::SetNextWayPoint(int thread)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!useRawMovement);

	if (CanSetNextWayPoint(thread)) {
		#ifdef PATHING_DEBUG
		if (DEBUG_DRAWING_ENABLED) {
			bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
				&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
			if (printMoveInfo) {
				LOG("%s setting next waypoint (%d:%d)", __func__, owner->id, owner->moveDef->pathType);
			}
		}
		#endif
		// Not sure this actually does anything.
		pathController.SetTempGoalPosition(pathID, earlyNextWayPoint);

		int32_t update = 1;
		while (update-- > 0) {
			earlyCurrWayPoint = earlyNextWayPoint;
			earlyNextWayPoint = pathManager->NextWayPoint(owner, pathID, 0, earlyCurrWayPoint, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);
			update += (earlyCurrWayPoint.y == (-1.f) && earlyNextWayPoint.y != (-1.f));
		}

		if (limitSpeedForTurning > 0)
			--limitSpeedForTurning;

		// Prevent delay repaths because the waypoints have been updated.
		wantRepath = false;
	}

	if (earlyNextWayPoint.x == -1.0f && earlyNextWayPoint.z == -1.0f) {
		//Fail(false);
		pathingFailed = true;
		#ifdef PATHING_DEBUG
		if (DEBUG_DRAWING_ENABLED) {
			bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
				&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
			if (printMoveInfo) {
				LOG("%s path failed", __func__);
			}
		}
		#endif
		return;
	}

	MoveTypes::CheckCollisionQuery colliderInfo(owner);
	
	const auto CWP_BLOCK_MASK = CMoveMath::SquareIsBlocked(*owner->moveDef, earlyCurrWayPoint, &colliderInfo);
	const auto NWP_BLOCK_MASK = CMoveMath::SquareIsBlocked(*owner->moveDef, earlyNextWayPoint, &colliderInfo);

	if ((CWP_BLOCK_MASK & CMoveMath::BLOCK_STRUCTURE) == 0 && (NWP_BLOCK_MASK & CMoveMath::BLOCK_STRUCTURE) == 0){
		#ifdef PATHING_DEBUG
		if (DEBUG_DRAWING_ENABLED) {
			bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
				&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
			if (printMoveInfo) {
				LOG("%s path is clear", __func__);
			}
		}
		#endif
		return;
	}

	// this can happen if we crushed a non-blocking feature
	// and it spawned another feature which we cannot crush
	// (eg.) --> repath

	ReRequestPath(false);
	#ifdef PATHING_DEBUG
	if (DEBUG_DRAWING_ENABLED) {
		bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
			&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
		if (printMoveInfo) {
			LOG("%s requesting new path", __func__);
		}
	}
	#endif
}

/*
Gives the position this unit will end up at with full braking
from current velocity.
*/
float3 CGroundMoveType::Here() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float dist = BrakingDistance(currentSpeed, decRate);
	const int   sign = Sign(int(!reversing));

	const float3 pos2D = owner->pos * XZVector;
	const float3 dir2D = flatFrontDir * dist * sign;

	return (pos2D + dir2D);
}

void CGroundMoveType::StartEngine(bool callScript) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (pathID == 0)
		pathID = GetNewPath();
	else {
		if (nextPathId != 0) {
			pathManager->DeletePath(nextPathId);
		}
		nextPathId = GetNewPath();

		// This can happen if the current path has not been resolved yet and the pathing system has
		// decided to optimize by updating the existing search request.
		if (nextPathId == pathID) {
			nextPathId = 0;
		}
	}

	if (pathID != 0) {
		// pathManager->UpdatePath(owner, pathID);

		if (callScript) {
			// makes no sense to call this unless we have a new path
			owner->script->StartMoving(reversing);
		}
	}
}

void CGroundMoveType::StopEngine(bool callScript, bool hardStop) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!ThreadPool::inMultiThreadedSection);
	if (pathID != 0 || nextPathId != 0) {
		if (pathID != 0) {
			pathManager->DeletePath(pathID);
			pathID = 0;
		}
		if (nextPathId != 0) {
			pathManager->DeletePath(nextPathId);
			nextPathId = 0;
		}

		if (callScript)
			owner->script->StopMoving();
	}

	owner->SetVelocityAndSpeed(owner->speed * (1 - hardStop));

	currentSpeed *= (1 - hardStop);
	wantedSpeed = 0.0f;
	limitSpeedForTurning = 0;
	bestReattemptedLastWaypointDist = std::numeric_limits<decltype(bestReattemptedLastWaypointDist)>::infinity();
}

/* Called when the unit arrives at its goal. */
void CGroundMoveType::Arrived(bool callScript)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// can only "arrive" if the engine is active
	if (progressState == Active) {
		eventHandler.UnitArrivedAtGoal(owner);

		StopEngine(callScript);

		if (owner->team == gu->myTeam)
			Channels::General->PlayRandomSample(owner->unitDef->sounds.arrived, owner);

		// and the action is done
		progressState = Done;

		owner->commandAI->SlowUpdate();

		LOG_L(L_DEBUG, "[%s] unit %i arrived", __func__, owner->id);
	}
}

/*
Makes the unit fail this action.
No more trials will be done before a new goal is given.
*/
void CGroundMoveType::Fail(bool callScript)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!ThreadPool::inMultiThreadedSection);
	LOG_L(L_DEBUG, "[%s] unit %i failed", __func__, owner->id);

	StopEngine(callScript);

	// failure of finding a path means that
	// this action has failed to reach its goal.
	progressState = Failed;

	eventHandler.UnitMoveFailed(owner);
	eoh->UnitMoveFailed(*owner);
}




void CGroundMoveType::HandleObjectCollisions()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* collider = owner;
	auto curThread = ThreadPool::GetThreadNum();

	resultantForces = ZeroVector;

	// handle collisions for even-numbered objects on even-numbered frames and vv.
	// (temporal resolution is still high enough to not compromise accuracy much?)
	if (collider->beingBuilt)
		return;

	const UnitDef* colliderUD = collider->unitDef;
	const MoveDef* colliderMD = collider->moveDef;

	resultantForces = ZeroVector;
	forceFromMovingCollidees = ZeroVector;
	forceFromStaticCollidees = ZeroVector;

	// NOTE:
	//   use the collider's MoveDef footprint as radius since it is
	//   always mobile (its UnitDef footprint size may be different)
	const float colliderFootPrintRadius = colliderMD->CalcFootPrintMaxInteriorRadius();
	const float colliderAxisStretchFact = colliderMD->CalcFootPrintAxisStretchFactor();

	HandleUnitCollisions(collider, {collider->speed.w, colliderFootPrintRadius, colliderAxisStretchFact}, colliderUD, colliderMD, curThread);
	HandleFeatureCollisions(collider, {collider->speed.w, colliderFootPrintRadius, colliderAxisStretchFact}, colliderUD, colliderMD, curThread);

	if (forceStaticObjectCheck) {
		MoveTypes::CheckCollisionQuery colliderInfo(collider);
		positionStuck |= !colliderMD->TestMoveSquare(colliderInfo, owner->pos, owner->speed, true, false, true, nullptr, nullptr, curThread);
		positionStuck |= !colliderMD->TestMoveSquare(colliderInfo, owner->pos, owner->speed, false, true, false, nullptr, nullptr, curThread);
		forceStaticObjectCheck = false;
	}

	// auto canAssignForce = [colliderMD, collider, curThread](const float3& force) {
	// 	if (force.same(ZeroVector))
	// 		return false;

	// 	float3 pos = collider->pos + force;
	// 	return colliderMD->TestMoveSquare(collider, pos, force, true, false, true, nullptr, nullptr, curThread)
	// 			&& colliderMD->TestMoveSquare(collider, pos, force, false, true, false, nullptr, nullptr, curThread);
	// };

	// Try to apply all collision forces, but if that will collide with static parts of the map,
	// then only apply forces from static objects/terrain. This prevent units from pushing each
	// other into buildings far enough that the pathing systems can't get them out again.
	float3 tryForce = forceFromStaticCollidees + forceFromMovingCollidees;
	float maxPushForceSq = maxSpeed*maxSpeed*modInfo.maxCollisionPushMultiplier;
	if (tryForce.SqLength() > maxPushForceSq)
		(tryForce.Normalize()) *= maxSpeed;

	UpdatePos(owner, tryForce, resultantForces, curThread);

	if (resultantForces.same(ZeroVector) && positionStuck){
		resultantForces = forceFromStaticCollidees;
		if (resultantForces.SqLength() > maxPushForceSq)
			(resultantForces.Normalize()) *= maxSpeed;
	}
}

bool CGroundMoveType::HandleStaticObjectCollision(
	CUnit* collider,
	CSolidObject* collidee,
	const MoveDef* colliderMD,
	const float colliderRadius,
	const float collideeRadius,
	const float3& separationVector,
	bool canRequestPath,
	bool checkYardMap,
	bool checkTerrain,
	int curThread
) {
	RECOIL_DETAILED_TRACY_ZONE;
	// while being built, units that overlap their factory yardmap should not be moved at all
	assert(!collider->beingBuilt);

	if (checkTerrain && (!collider->IsMoving() || collider->IsInAir()))
		return false;

	// for factories, check if collidee's position is behind us (which means we are likely exiting)
	//
	// NOTE:
	//   allow units to move _through_ idle open factories by extending the collidee's footprint such
	//   that insideYardMap is true in a larger area (otherwise pathfinder and coldet would disagree)
	//   the transition from radius- to footprint-based handling is discontinuous --> cannot mix them
	// TODO:
	//   increase cost of squares inside open factories so PFS is less likely to path through them
	//
	#if 0
	const int xext = ((collidee->xsize >> 1) + std::max(1, colliderMD->xsizeh));
	const int zext = ((collidee->zsize >> 1) + std::max(1, colliderMD->zsizeh));

	const bool insideYardMap =
		(collider->pos.x >= (collidee->pos.x - xext * SQUARE_SIZE)) &&
		(collider->pos.x <= (collidee->pos.x + xext * SQUARE_SIZE)) &&
		(collider->pos.z >= (collidee->pos.z - zext * SQUARE_SIZE)) &&
		(collider->pos.z <= (collidee->pos.z + zext * SQUARE_SIZE));
	const bool exitingYardMap =
		((collider->frontdir.dot(separationVector) > 0.0f) &&
		 (collider->   speed.dot(separationVector) > 0.0f));
	#endif

	const float3& pos = collider->pos;
	const float3& vel = collider->speed;
	const float3& rgt = collider->rightdir;

	// RHS magic constant is the radius of a square (sqrt(2*(SQUARE_SIZE>>1)*(SQUARE_SIZE>>1)))
	constexpr float squareRadius = 5.656854249492381f;

	float3 strafeVec;
	float3 bounceVec;
	float3 summedVec;

	if (checkYardMap || checkTerrain) {
		float3 sqrSumPosition; // .y is always 0
		float2 sqrPenDistance; // .x = sum, .y = count

		float intersectDistance = 0.f;
		float intersectSqrCount = 0.f;
		float3 intersectSqrSumPosition;

		const float3 rightDir2D = (rgt * XZVector).SafeNormalize();
		const float3 speedDir2D = (vel * XZVector).SafeNormalize();

		const int xmid = (pos.x + vel.x) / SQUARE_SIZE;
		const int zmid = (pos.z + vel.z) / SQUARE_SIZE;

		const int xsquare = (pos.x / SQUARE_SIZE);
		const int zsquare = (pos.z / SQUARE_SIZE);

		const int realMinX = xsquare + (-colliderMD->xsizeh);
		const int realMinZ = zsquare + (-colliderMD->zsizeh);
		const int realMaxX = xsquare +   colliderMD->xsizeh ;
		const int realMaxZ = zsquare +   colliderMD->zsizeh ;

		// mantis{3614,4217}
		//   we cannot nicely bounce off terrain when checking only the center square
		//   however, testing more squares means CD can (sometimes) disagree with PFS
		//   in narrow passages --> still possible, but have to ensure we allow only
		//   lateral (non-obstructing) bounces
		const int xsh = colliderMD->xsizeh * (checkYardMap || (checkTerrain && colliderMD->allowTerrainCollisions));
		const int zsh = colliderMD->zsizeh * (checkYardMap || (checkTerrain && colliderMD->allowTerrainCollisions));
		const int intersectSize = colliderMD->xsize;

		const int xmin = std::min(-1, -xsh), xmax = std::max(1, xsh);
		const int zmin = std::min(-1, -zsh), zmax = std::max(1, zsh);

		if (DEBUG_DRAWING_ENABLED){
			geometryLock.lock();
			geometricObjects->AddLine(pos + (UpVector * 25.0f), pos + (UpVector * 100.0f), 3, 1, 4);
			geometryLock.unlock();
		}

		MoveTypes::CheckCollisionQuery colliderInfo(owner);

		// check for blocked squares inside collider's MoveDef footprint zone
		// interpret each square as a "collidee" and sum up separation vectors
		//
		// NOTE:
		//   assumes the collider's footprint is still always axis-aligned
		// NOTE:
		//   the pathfinders only care about the CENTER square wrt terrain!
		//   this means paths can come closer to impassable terrain than is
		//   allowed by collision detection (more probable if edges between
		//   passable and impassable areas are hard instead of gradients or
		//   if a unit is not affected by slopes) --> can be solved through
		//   smoothing the cost-function, eg. blurring heightmap before PFS
		//   sees it
		//
		for (int z = zmin; z <= zmax; z++) {
			for (int x = xmin; x <= xmax; x++) {
				const int xabs = xmid + x;
				const int zabs = zmid + z;

				if ( checkTerrain &&  (CMoveMath::GetPosSpeedMod(*colliderMD, xabs, zabs) > 0.f))
					continue;
				if ( checkYardMap && ((CMoveMath::SquareIsBlocked(*colliderMD, xabs, zabs, &colliderInfo) & CMoveMath::BLOCK_STRUCTURE) == 0))
					continue;

				const float3 squarePos = float3(xabs * SQUARE_SIZE + (SQUARE_SIZE >> 1), pos.y, zabs * SQUARE_SIZE + (SQUARE_SIZE >> 1));
				const float3 squareVec = pos - squarePos;


				const float  squareColRadiusSum = colliderRadius + squareRadius;
				const float   squareSepDistance = squareVec.Length2D() + 0.1f;
				const float   squarePenDistance = std::min(squareSepDistance - squareColRadiusSum, 0.0f);
				// const float  squareColSlideSign = -Sign(squarePos.dot(rightDir2D) - pos.dot(rightDir2D));

				if (x >= realMinX && x <= realMaxX && z >= realMinZ && z <= realMaxZ){
					if (intersectSize > 1) {
						intersectDistance = std::min(intersectDistance, squarePenDistance);
						intersectSqrSumPosition += (squarePos * XZVector);
						intersectSqrCount++;
					}
					if (checkYardMap && !positionStuck)
						positionStuck = true;
				}

				// ignore squares behind us (relative to velocity vector)
				if (squareVec.dot(vel) > 0.0f)
					continue;

				// this tends to cancel out too much on average
				// strafeVec += (rightDir2D * sqColSlideSign);
				bounceVec += (rightDir2D * (rightDir2D.dot(squareVec / squareSepDistance)));

				sqrPenDistance += float2(squarePenDistance, 1.0f);
				sqrSumPosition += (squarePos * XZVector);
			}
		}

		// This pushes units directly away from static objects so they don't intersect.
		if (intersectSqrCount > 0.f) {
			intersectSqrSumPosition *= (1.0f / intersectSqrCount);
			const float pushSpeed = std::min(-intersectDistance, maxSpeed);
			const float3 pushOutVec = ((pos - intersectSqrSumPosition) * XZVector).SafeNormalize() * pushSpeed;

			forceFromStaticCollidees += pushOutVec;
		}

		// This directs units to left/right around the static object.
		if (sqrPenDistance.y > 0.0f /*&& -deepestPenDistance <= squareRadius*2*/) {
			sqrSumPosition *= (1.0f / sqrPenDistance.y);
			sqrPenDistance *= (1.0f / sqrPenDistance.y);

			const float strafeSign = -Sign(sqrSumPosition.dot(rightDir2D) - pos.dot(rightDir2D));
			const float bounceSign =  Sign(rightDir2D.dot(bounceVec));
			const float strafeScale = std::min(std::max(currentSpeed*0.0f, maxSpeedDef), std::max(0.1f, -sqrPenDistance.x * 0.5f));
			const float bounceScale = std::min(std::max(currentSpeed*0.0f, maxSpeedDef), std::max(0.1f, -sqrPenDistance.x * 0.5f));

			// in FPS mode, normalize {strafe,bounce}Scale and multiply by maxSpeedDef
			// (otherwise it would be possible to slide along map edges at above-normal
			// speeds, etc.)
			const float fpsStrafeScale = (strafeScale / (strafeScale + bounceScale)) * maxSpeedDef;
			const float fpsBounceScale = (bounceScale / (strafeScale + bounceScale)) * maxSpeedDef;

			// bounceVec always points along rightDir by construction
			strafeVec = (rightDir2D * strafeSign) * mix(strafeScale, fpsStrafeScale, owner->UnderFirstPersonControl());
			bounceVec = (rightDir2D * bounceSign) * mix(bounceScale, fpsBounceScale, owner->UnderFirstPersonControl());
			summedVec = strafeVec + bounceVec;
			forceFromStaticCollidees += summedVec;
			limitSpeedForTurning = 2;
		}

		// note:
		//   in many cases this does not mean we should request a new path
		//   (and it can be counter-productive to do so since we might not
		//   even *get* one)
		return (canRequestPath && (summedVec != ZeroVector));
	}

	{
		const float  colRadiusSum = colliderRadius + collideeRadius;
		const float   sepDistance = separationVector.Length() + 0.1f;
		const float   penDistance = std::min(sepDistance - colRadiusSum, 0.0f);
		const float  colSlideSign = -Sign(collidee->pos.dot(rgt) - pos.dot(rgt));

		const float strafeScale = std::min(currentSpeed, std::max(0.0f, -penDistance * 0.5f)) * (1 - checkYardMap * false);
		const float bounceScale = std::min(currentSpeed, std::max(0.0f, -penDistance       )) * (1 - checkYardMap *  true);

		strafeVec = (             rgt * colSlideSign) * strafeScale;
		bounceVec = (separationVector /  sepDistance) * bounceScale;
		summedVec = strafeVec + bounceVec;
		forceFromStaticCollidees += summedVec;
		limitSpeedForTurning = 2;

		// same here
		return (canRequestPath && (penDistance < 0.0f));
	}
}

void CGroundMoveType::HandleUnitCollisions(
	CUnit* collider,
	const float3& colliderParams, // .x := speed, .y := radius, .z := fpstretch
	const UnitDef* colliderUD,
	const MoveDef* colliderMD,
	int curThread
) {
	RECOIL_DETAILED_TRACY_ZONE;
	// NOTE: probably too large for most units (eg. causes tree falling animations to be skipped)
	// const float3 crushImpulse = collider->speed * collider->mass * Sign(int(!reversing));

	const bool allowUCO = modInfo.allowUnitCollisionOverlap;
	const bool allowCAU = modInfo.allowCrushingAlliedUnits;
	const bool allowPEU = modInfo.allowPushingEnemyUnits;
	const bool allowSAT = modInfo.allowSepAxisCollisionTest;
	const bool forceSAT = (colliderParams.z > 0.1f);

	const float3 crushImpulse = owner->speed * owner->mass * Sign(int(!reversing));

	// Push resistant units when stopped impacting pathing and also cannot be pushed, so it is important that such
	// units are not going to prevent other units from moving around them if they are near narrow pathways.
	const float colliderSeparationDist = (pushResistant && pushResistanceBlockActive) ? 0.f : colliderUD->separationDistance;

	// Account for units that are larger than one's self.
	const float maxCollisionRadius = colliderParams.y + moveDefHandler.GetLargestFootPrintSizeH();
	const float searchRadius = colliderParams.x + maxCollisionRadius + colliderSeparationDist;

	MoveTypes::CheckCollisionQuery colliderInfo(collider);
	if ( !colliderMD->overrideUnitWaterline )
		colliderInfo.DisableHeightChecks();

	// copy on purpose, since the below can call Lua
	QuadFieldQuery qfQuery;
	qfQuery.threadOwner = curThread;
	quadField.GetUnitsExact(qfQuery, collider->pos, searchRadius);

	for (CUnit* collidee: *qfQuery.units) {
		if (collidee == collider) continue;
		if (collidee->IsSkidding()) continue;
		if (collidee->IsFlying()) continue;

		const UnitDef* collideeUD = collidee->unitDef;
		const MoveDef* collideeMD = collidee->moveDef;

		const bool colliderMobile = (colliderMD != nullptr); // always true
		const bool collideeMobile = (collideeMD != nullptr); // maybe true

		const bool unloadingCollidee = (collidee->unloadingTransportId == collider->id);
		const bool unloadingCollider = (collider->unloadingTransportId == collidee->id);

		if (unloadingCollider)
			collider->requestRemoveUnloadTransportId = true;

		// don't push/crush either party if the collidee does not block the collider (or vv.)
		if (colliderMobile && CMoveMath::IsNonBlocking(collidee, &colliderInfo))
			continue;

		// disable collisions between collider and collidee
		// if collidee is currently inside any transporter,
		// or if collider is being transported by collidee
		if (collider->GetTransporter() == collidee) continue;
		if (collidee->GetTransporter() != nullptr) continue;
		// also disable collisions if either party currently
		// has an order to load units (TODO: do we want this
		// for unloading as well?)
		if (collider->loadingTransportId == collidee->id) continue;
		if (collidee->loadingTransportId == collider->id) continue;

		const float collDist = (collideeMobile) ? collideeMD->CalcFootPrintMaxInteriorRadius() : collidee->CalcFootPrintMaxInteriorRadius();
		const float2 collideeParams = {collidee->speed.w, collDist};
		const float4 separationVect = {collider->pos - collidee->pos, Square(colliderParams.y + collideeParams.y)};

		const int collisionFunc = (allowSAT && (forceSAT || (collideeMobile && collideeMD->CalcFootPrintAxisStretchFactor() > 0.1f)));
		const bool isCollision = (checkCollisionFuncs[collisionFunc](separationVect, collider, collidee, colliderMD, collideeMD));

		// check for separation
		float separationDist = 0.f;
		if (!isCollision && collideeMobile) {
			const bool useCollideeSeparationDistance = !( collidee->moveType->IsPushResistant() && collidee->moveType->IsPushResitanceBlockActive() );
			const float collideeSeparationDist = (useCollideeSeparationDistance) ? colliderUD->separationDistance : 0.f;
			separationDist = std::max(colliderSeparationDist, collideeUD->separationDistance);
			const float separation = colliderParams.y + collideeParams.y + separationDist; 
			const bool isSeparation = static_cast<float3>(separationVect).SqLength2D() <= Square(separation);
			if (!isSeparation)
				continue;
		} else if (!isCollision)
			continue;

		if (unloadingCollider) {
			collider->requestRemoveUnloadTransportId = false;
			continue;
		}

		if (unloadingCollidee)
			continue;

		// NOTE:
		//   we exclude aircraft (which have NULL moveDef's) landed
		//   on the ground, since they would just stack when pushed
		bool pushCollider = colliderMobile;
		bool pushCollidee = collideeMobile;

		const bool alliedCollision =
			teamHandler.Ally(collider->allyteam, collidee->allyteam) &&
			teamHandler.Ally(collidee->allyteam, collider->allyteam);

		if (isCollision) {
			bool crushCollidee = false;

			// const bool collideeYields = (collider->IsMoving() && !collidee->IsMoving());
			// const bool ignoreCollidee = (collideeYields && alliedCollision);

			crushCollidee |= (!alliedCollision || allowCAU);
			crushCollidee &= ((colliderParams.x * collider->mass) > (collideeParams.x * collidee->mass));

			if (crushCollidee && !CMoveMath::CrushResistant(*colliderMD, collidee)) {
				auto& events = Sim::registry.get<UnitCrushEvents>(owner->entityReference);
				events.value.emplace_back(collider, collidee, crushImpulse);
			}

			// Only trigger this event once for each colliding pair of units.
			if (collider->id < collidee->id){
				auto& events = Sim::registry.get<UnitCollisionEvents>(owner->entityReference);
				events.value.emplace_back(collider, collidee);
			}
		}

		if (collideeMobile)
			HandleUnitCollisionsAux(collider, collidee, this, static_cast<CGroundMoveType*>(collidee->moveType));

		// NOTE:
		//   allowPushingEnemyUnits is (now) useless because alliances are bi-directional
		//   ie. if !alliedCollision, pushCollider and pushCollidee BOTH become false and
		//   the collision is treated normally --> not what we want here, but the desired
		//   behavior (making each party stop and block the other) has many corner-cases
		//   so instead have collider respond as though collidee is semi-static obstacle
		//   this also happens when both parties are pushResistant
		pushCollider = pushCollider && (alliedCollision || allowPEU || !collider->blockEnemyPushing);
		pushCollidee = pushCollidee && (alliedCollision || allowPEU || !collidee->blockEnemyPushing);
		pushCollider = pushCollider && (!collider->beingBuilt && !collider->UsingScriptMoveType() && !collider->moveType->IsPushResistant());
		pushCollidee = pushCollidee && (!collidee->beingBuilt && !collidee->UsingScriptMoveType() && !collidee->moveType->IsPushResistant());

		const bool isStatic = (!collideeMobile && !collideeUD->IsAirUnit()) || (!pushCollider && !pushCollidee);
		if (isCollision && isStatic) {
			// building (always axis-aligned, possibly has a yardmap)
			// or semi-static collidee that should be handled as such
			//
			// since all push-resistant units use the BLOCK_STRUCTURE
			// mask when stopped, avoid the yardmap || terrain branch
			// of HSOC which is not well suited to both parties moving
			// and can leave them inside stuck each other's footprints
			const bool allowNewPath = (!atEndOfPath && !atGoal);
			const bool checkYardMap = ((pushCollider || pushCollidee) || collideeUD->IsFactoryUnit());

			if (HandleStaticObjectCollision(collider, collidee, colliderMD,  colliderParams.y, collideeParams.y,  separationVect, allowNewPath, checkYardMap, false, curThread)) {
				ReRequestPath(false);
			}

			continue;
		}

		const bool moveCollider = ((pushCollider || !pushCollidee) && colliderMobile);
		if (moveCollider) {
			if (isCollision)
				forceFromMovingCollidees += CalculatePushVector(colliderParams, collideeParams, allowUCO, separationVect, collider, collidee);
			else {
				// push units away from each other though they are not colliding.
				const float3 colliderParams2 = {colliderParams.x, colliderParams.y + separationDist * 0.5f, colliderParams.z};
				const float2 collideeParams2 = {collidee->speed.w, collDist + separationDist * 0.5f};
				const float4 separationVect2 = {static_cast<float3>(separationVect), Square(colliderParams.y + collideeParams.y)};
				forceFromMovingCollidees += CalculatePushVector(colliderParams2, collideeParams2, allowUCO, separationVect2, collider, collidee);
			}
		}
	}
}

float3 CGroundMoveType::CalculatePushVector(const float3 &colliderParams, const float2 &collideeParams, const bool allowUCO, const float4 &separationVect, CUnit *collider, CUnit *collidee)
{
    const float colliderRelRadius = colliderParams.y / (colliderParams.y + collideeParams.y);
    const float collideeRelRadius = collideeParams.y / (colliderParams.y + collideeParams.y);
    const float collisionRadiusSum = allowUCO ? (colliderParams.y * colliderRelRadius + collideeParams.y * collideeRelRadius) : (colliderParams.y + collideeParams.y);

    const float sepDistance = separationVect.Length() + 0.1f;
    const float penDistance = std::max(collisionRadiusSum - sepDistance, 1.0f);
    const float sepResponse = std::min(SQUARE_SIZE * 2.0f, penDistance * 0.5f);

    const float3 sepDirection = separationVect / sepDistance;
    const float3 colResponseVec = sepDirection * XZVector * sepResponse;

    const float
        m1 = collider->mass,
        m2 = collidee->mass,
        v1 = std::max(1.0f, colliderParams.x),
        v2 = std::max(1.0f, collideeParams.x),
        c1 = 1.0f + (1.0f - math::fabs(collider->frontdir.dot(-sepDirection))) * 5.0f,
        c2 = 1.0f + (1.0f - math::fabs(collidee->frontdir.dot(sepDirection))) * 5.0f,
        // weighted momenta
        s1 = m1 * v1 * c1,
        s2 = m2 * v2 * c2,
        // relative momenta
        r1 = s1 / (s1 + s2 + 1.0f),
        r2 = s2 / (s1 + s2 + 1.0f);

    // far from a realistic treatment, but works
    const float colliderMassScale = std::clamp(1.0f - r1, 0.01f, 0.99f) * (allowUCO ? (1.0f / colliderRelRadius) : 1.0f);
    // const float collideeMassScale = std::clamp(1.0f - r2, 0.01f, 0.99f) * (allowUCO? (1.0f / collideeRelRadius): 1.0f);

    // try to prevent both parties from being pushed onto non-traversable
    // squares (without resetting their position which stops them dead in
    // their tracks and undoes previous legitimate pushes made this frame)
    //
    // if pushCollider and pushCollidee are both false (eg. if each party
    // is pushResistant), treat the collision as regular and push both to
    // avoid deadlocks
    const float colliderSlideSign = Sign(separationVect.dot(collider->rightdir));

    const float3 colliderPushVec = colResponseVec * colliderMassScale; // * int(!ignoreCollidee);
    const float3 colliderSlideVec = collider->rightdir * colliderSlideSign * (1.0f / penDistance) * r2;
    const float3 colliderMoveVec = colliderPushVec + colliderSlideVec;

    return colliderMoveVec;
}

void CGroundMoveType::HandleFeatureCollisions(
	CUnit* collider,
	const float3& colliderParams,
	const UnitDef* colliderUD,
	const MoveDef* colliderMD,
	int curThread
) {
	RECOIL_DETAILED_TRACY_ZONE;
	const bool allowSAT = modInfo.allowSepAxisCollisionTest;
	const bool forceSAT = (colliderParams.z > 0.1f);

	const float3 crushImpulse = owner->speed * owner->mass * Sign(int(!reversing));
	MoveTypes::CheckCollisionQuery colliderInfo(collider);

	// copy on purpose, since DoDamage below can call Lua
	QuadFieldQuery qfQuery;
	qfQuery.threadOwner = curThread;
	quadField.GetFeaturesExact(qfQuery, collider->pos, colliderParams.x + (colliderParams.y * 2.0f));

	for (CFeature* collidee: *qfQuery.features) {
		// const FeatureDef* collideeFD = collidee->def;

		// use the collidee's Feature (not FeatureDef) footprint as radius
		const float2 collideeParams = {0.0f, collidee->CalcFootPrintMaxInteriorRadius()};
		const float4 separationVect = {collider->pos - collidee->pos, Square(colliderParams.y + collideeParams.y)};

		if (!checkCollisionFuncs[allowSAT && (forceSAT || (collidee->CalcFootPrintAxisStretchFactor() > 0.1f))](separationVect, collider, collidee, colliderMD, nullptr))
			continue;


		if (CMoveMath::IsNonBlocking(collidee, &colliderInfo))
			continue;

		if (!CMoveMath::CrushResistant(*colliderMD, collidee)){
			auto& events = Sim::registry.get<FeatureCrushEvents>(owner->entityReference);
			events.value.emplace_back(collider, collidee, crushImpulse);
		}
		#if 0
		if (pathController.IgnoreCollision(collider, collidee))
			continue;
		#endif

		{
			auto& events = Sim::registry.get<FeatureCollisionEvents>(owner->entityReference);
			events.value.emplace_back(collider, collidee);
		}

		if (!collidee->IsMoving()) {
			if (HandleStaticObjectCollision(collider, collidee, colliderMD,  colliderParams.y, collideeParams.y,  separationVect, (!atEndOfPath && !atGoal), true, false, curThread)) {
				ReRequestPath(false);
			}

			continue;
		}

		const float  sepDistance    = separationVect.Length() + 0.1f;
		const float  penDistance    = std::max((colliderParams.y + collideeParams.y) - sepDistance, 1.0f);
		const float  sepResponse    = std::min(SQUARE_SIZE * 2.0f, penDistance * 0.5f);

		const float3 sepDirection   = separationVect / sepDistance;
		const float3 colResponseVec = sepDirection * XZVector * sepResponse;

		// multiply the collidee's mass by a large constant (so that heavy
		// features do not bounce light units away like jittering pinballs;
		// collideeMassScale ~= 0.01 suppresses large responses)
		const float
			m1 = collider->mass,
			m2 = collidee->mass * 10000.0f,
			v1 = std::max(1.0f, colliderParams.x),
			v2 = 1.0f,
			c1 = (1.0f - math::fabs( collider->frontdir.dot(-sepDirection))) * 5.0f,
			c2 = (1.0f - math::fabs(-collider->frontdir.dot( sepDirection))) * 5.0f,
			s1 = m1 * v1 * c1,
			s2 = m2 * v2 * c2,
 			r1 = s1 / (s1 + s2 + 1.0f),
 			r2 = s2 / (s1 + s2 + 1.0f);

		const float colliderMassScale = std::clamp(1.0f - r1, 0.01f, 0.99f);
		const float collideeMassScale = std::clamp(1.0f - r2, 0.01f, 0.99f);

		forceFromMovingCollidees += colResponseVec * colliderMassScale;

		{
			auto& events = Sim::registry.get<FeatureMoveEvents>(owner->entityReference);
			events.value.emplace_back(collider, collidee, -colResponseVec * collideeMassScale);
		}
	}
}



void CGroundMoveType::LeaveTransport()
{
	RECOIL_DETAILED_TRACY_ZONE;
	oldPos = owner->pos + UpVector * 0.001f;
}

void CGroundMoveType::Connect() {
	RECOIL_DETAILED_TRACY_ZONE;
	Sim::registry.emplace_or_replace<GroundMoveType>(owner->entityReference, owner->id);
	Sim::registry.emplace_or_replace<FeatureCollisionEvents>(owner->entityReference);
	Sim::registry.emplace_or_replace<UnitCollisionEvents>(owner->entityReference);
	Sim::registry.emplace_or_replace<FeatureCrushEvents>(owner->entityReference);
	Sim::registry.emplace_or_replace<UnitCrushEvents>(owner->entityReference);
	Sim::registry.emplace_or_replace<FeatureMoveEvents>(owner->entityReference);
	Sim::registry.emplace_or_replace<UnitMovedEvent>(owner->entityReference);
	Sim::registry.emplace_or_replace<ChangeHeadingEvent>(owner->entityReference, owner->id);
	Sim::registry.emplace_or_replace<ChangeMainHeadingEvent>(owner->entityReference, owner->id);
	// LOG("%s: loading %s as %d", __func__, owner->unitDef->name.c_str(), entt::to_integral(owner->entityReference));
}

void CGroundMoveType::Disconnect() {
	RECOIL_DETAILED_TRACY_ZONE;
	Sim::registry.remove<GroundMoveType>(owner->entityReference);
	Sim::registry.remove<FeatureCollisionEvents>(owner->entityReference);
	Sim::registry.remove<UnitCollisionEvents>(owner->entityReference);
	Sim::registry.remove<FeatureCrushEvents>(owner->entityReference);
	Sim::registry.remove<UnitCrushEvents>(owner->entityReference);
	Sim::registry.remove<FeatureMoveEvents>(owner->entityReference);
	Sim::registry.remove<UnitMovedEvent>(owner->entityReference);
	Sim::registry.remove<ChangeHeadingEvent>(owner->entityReference);
	Sim::registry.remove<ChangeMainHeadingEvent>(owner->entityReference);
}

void CGroundMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive) { KeepPointingTo(unit->pos, distance, aggressive); }
void CGroundMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive) {
	mainHeadingPos = pos;
	useMainHeading = aggressive;

	if (!useMainHeading)
		return;
	if (owner->weapons.empty())
		return;

	const CWeapon* frontWeapon = owner->weapons.front();

	if (!frontWeapon->weaponDef->waterweapon)
		mainHeadingPos.y = std::max(mainHeadingPos.y, 0.0f);

	float3 dir1 = frontWeapon->mainDir;
	float3 dir2 = mainHeadingPos - owner->pos;

	// in this case aligning is impossible
	if (dir1 == UpVector)
		return;

	dir1 = (dir1 * XZVector).SafeNormalize();
	dir2 = (dir2 * XZVector).SafeNormalize();

	if (dir2 == ZeroVector)
		return;

	const short heading =
		GetHeadingFromVector(dir2.x, dir2.z) -
		GetHeadingFromVector(dir1.x, dir1.z);

	if (owner->heading == heading)
		return;

	// NOTE:
	//   by changing the progress-state here (which seems redundant),
	//   SlowUpdate can suddenly request a new path for us even after
	//   StopMoving (which clears pathID; CAI often calls StopMoving
	//   before unit is at goalPos!)
	//   for this reason StopMoving always updates goalPos so internal
	//   GetNewPath's are no-ops (while CAI does not call StartMoving)
	if (frontWeapon->TestRange(mainHeadingPos, SWeaponTarget(mainHeadingPos, true)))
		return;

	progressState = Active;
}


/**
* @brief Orients owner so that weapon[0]'s arc includes mainHeadingPos
*/
void CGroundMoveType::SetMainHeading() {
	if (!useMainHeading || owner->weapons.empty()) {
		ChangeHeading(owner->heading);
		return;
	}

	const CWeapon* frontWeapon = owner->weapons.front();

	const float3 dir1 = ((       frontWeapon->mainDir) * XZVector).SafeNormalize();
	const float3 dir2 = ((mainHeadingPos - owner->pos) * XZVector).SafeNormalize();

	// ASSERT_SYNCED(dir1);
	// ASSERT_SYNCED(dir2);

	if (dir2 == ZeroVector)
		return;

	short newHeading =
		GetHeadingFromVector(dir2.x, dir2.z) -
		GetHeadingFromVector(dir1.x, dir1.z);

	// ASSERT_SYNCED(newHeading);

	if (progressState == Active) {
		if (owner->heading != newHeading) {
			// start or continue turning
			ChangeHeading(newHeading);
		} else {
			// stop turning
			progressState = Done;
		}

		return;
	}

	if (owner->heading == newHeading)
		return;

	if (frontWeapon->TestRange(mainHeadingPos, SWeaponTarget(mainHeadingPos, true)))
		return;

	progressState = Active;
}

bool CGroundMoveType::OnSlope(float minSlideTolerance) {
	RECOIL_DETAILED_TRACY_ZONE;
	const UnitDef* ud = owner->unitDef;
	const MoveDef* md = owner->moveDef;
	const float3& pos = owner->pos;

	if (ud->slideTolerance < minSlideTolerance)
		return false;
	if (owner->FloatOnWater() && owner->IsInWater())
		return false;
	if (!pos.IsInBounds())
		return false;

	// if minSlideTolerance is LEQ 0, do not multiply maxSlope by ud->slideTolerance
	// (otherwise the unit could stop on an invalid path location, and be teleported
	// back)
	const float slopeMul = mix(ud->slideTolerance, 1.0f, (minSlideTolerance <= 0.0f));
	const float curSlope = CGround::GetSlope(pos.x, pos.z);
	const float maxSlope = md->maxSlope * slopeMul;

	return (curSlope > maxSlope);
}



const float3& CGroundMoveType::GetGroundNormal(const float3& p) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// ship or hovercraft; return (CGround::GetNormalAboveWater(p));
	if (owner->IsInWater() && !owner->IsOnGround())
		return UpVector;

	return (CGround::GetNormal(p.x, p.z));
}

float CGroundMoveType::GetGroundHeight(const float3& p) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	MoveDef *md = owner->moveDef;

	// in [minHeight, maxHeight]
	const float gh = CGround::GetHeightReal(p.x, p.z);
	
	// in [-waterline, maxHeight], note that waterline
	// can be much deeper than ground in shallow water
	if (owner->FloatOnWater()) {
		MoveDef *md = owner->moveDef;
		const float wh = ((md->overrideUnitWaterline) ? -md->waterline : -owner->unitDef->waterline) * (gh <= 0.0f);

		return (std::max(gh, wh));
	}

	return gh;
}

void CGroundMoveType::AdjustPosToWaterLine()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (owner->IsFalling())
		return;
	if (owner->IsFlying())
		return;

	if (modInfo.allowGroundUnitGravity) {
		if (owner->FloatOnWater()) {
			MoveDef *md = owner->moveDef;
			owner->Move(UpVector * (std::max(CGround::GetHeightReal(owner->pos.x, owner->pos.z), -md->waterline) - owner->pos.y), true);
		} else {
			owner->Move(UpVector * (std::max(CGround::GetHeightReal(owner->pos.x, owner->pos.z), owner->pos.y) - owner->pos.y), true);
		}
	} else {
		owner->Move(UpVector * (GetGroundHeight(owner->pos) - owner->pos.y), true);
	}
}

bool CGroundMoveType::UpdateDirectControl()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const CPlayer* myPlayer = gu->GetMyPlayer();
	const FPSUnitController& selfCon = myPlayer->fpsController;
	const FPSUnitController& unitCon = owner->fpsControlPlayer->fpsController;
	const bool wantReverse = (unitCon.back && !unitCon.forward);

	float turnSign = 0.0f;

	currWayPoint = owner->frontdir * XZVector * mix(100.0f, -100.0f, wantReverse);
	earlyCurrWayPoint = currWayPoint = (owner->pos + currWayPoint).cClampInBounds();

	if (unitCon.forward || unitCon.back) {
		ChangeSpeed((maxSpeed * unitCon.forward) + (maxReverseSpeed * unitCon.back), wantReverse, true);
	} else {
		// not moving forward or backward, stop
		ChangeSpeed(0.0f, false, true);
	}

	if (unitCon.left ) { ChangeHeading(owner->heading + turnRate); turnSign =  1.0f; }
	if (unitCon.right) { ChangeHeading(owner->heading - turnRate); turnSign = -1.0f; }

	// local client is controlling us
	if (selfCon.GetControllee() == owner)
		camera->SetRotY(camera->GetRot().y + turnRate * turnSign * TAANG2RAD);

	return wantReverse;
}


void CGroundMoveType::UpdatePos(const CUnit* unit, const float3& moveDir, float3& resultantMove, int thread) const {
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 prevPos = unit->pos;
	const float3 newPos = unit->pos + moveDir;
	resultantMove = moveDir;

	// The series of tests done here will benefit from using the same cached results.
	MoveDef* md = unit->moveDef;
	int tempNum = gs->GetMtTempNum(thread);

	MoveTypes::CheckCollisionQuery virtualObject(unit);
	MoveDefs::CollisionQueryStateTrack queryState;

	const bool isSubmersible = md->IsComplexSubmersible();
	if (!isSubmersible)
		virtualObject.DisableHeightChecks();

	auto toMapSquare = [](float3 pos) {
		return int2({int(pos.x / SQUARE_SIZE), int(pos.z / SQUARE_SIZE)});
	};

	auto toSquareId = [](int2 square) {
		return (square.y * mapDims.mapx) + square.x;
	};

	auto isSquareOpen = [this, md, unit, &tempNum, thread, &toMapSquare, &virtualObject, &queryState, &isSubmersible](float3 pos) {
		int2 checkSquare = toMapSquare(pos);
		if ( checkSquare.x < 0
			|| checkSquare.y < 0
			|| checkSquare.x >= mapDims.mapx
			|| checkSquare.y >= mapDims.mapy) {
				return false;
			}
		
		if (isSubmersible){
			md->UpdateCheckCollisionQuery(virtualObject, queryState, checkSquare);
			if (queryState.refreshCollisionCache)
				tempNum = gs->GetMtTempNum(thread);
		}

		// separate calls because terrain is only checked for in the centre square, while
		// static objects are checked for in the whole footprint.
		bool result = ( pathController.IgnoreTerrain(*md, pos) ||
				 unit->moveDef->TestMoveSquare(virtualObject, pos, (pos - unit->pos), true, false, true, nullptr, nullptr, thread)
			   )
				&& unit->moveDef->TestMovePositionForObjects(&virtualObject, pos, tempNum, thread);

		return result;
	};

	auto toPosition = [](int2 square) {
		return float3({float(square.x * SQUARE_SIZE + 1), 0.f, float(square.y * SQUARE_SIZE + 1)});
	};

	const int2 prevSquare = toMapSquare(prevPos);
	const int2 newSquare = toMapSquare(newPos);
	const int newPosStartSquare = toSquareId(newSquare);
	if (!positionStuck && toSquareId(prevSquare) == newPosStartSquare) { return; }

	bool isSquareBlocked = !isSquareOpen(newPos);
	if (!isSquareBlocked) {
		const int2 fullDiffSquare = newSquare - prevSquare;
		if (fullDiffSquare.x != 0 && fullDiffSquare.y != 0) {

			const int2 diffSquare{1 - (2 * (fullDiffSquare.x < 0)), 1 - (2 * (fullDiffSquare.y < 0))};

			// We have a diagonal move. Make sure the unit cannot press through a corner.
			const int2 checkSqrX({newSquare.x - diffSquare.x, newSquare.y});
			const int2 checkSqrY({newSquare.x, newSquare.y - diffSquare.y});

			isSquareBlocked = !isSquareOpen(toPosition(checkSqrX)) && !isSquareOpen(toPosition(checkSqrY));
			if (isSquareBlocked)
				resultantMove = ZeroVector;
		}
	}
	else {
	// NOTE:
	//   does not check for structure blockage, coldet handles that
	//   entering of impassable terrain is *also* handled by coldet
	//
	//   the loop below tries to evade "corner" squares that would
	//   block us from initiating motion and is needed for when we
	//   are not *currently* moving but want to get underway to our
	//   first waypoint (HSOC coldet won't help then)
	//
	//   allowing movement through blocked squares when pathID != 0
	//   relies on assumption that PFS will not search if start-sqr
	//   is blocked, so too fragile
	//
	// TODO: look to move as much of this to MT to improve perf.

		bool updatePos = false;
		const float speed = moveDir.Length2D();

		auto tryToMove =
				[this, &isSquareOpen, &prevPos, &newPosStartSquare, &resultantMove]
				(const float3& newPos, float3 posOffset, float maxDisplacement = 0.f)
			{
			// units are moved in relation to their previous position.
			float3 offsetFromPrev = (newPos + posOffset) - prevPos;
			if ((maxDisplacement > 0.f) && offsetFromPrev.SqLength2D() > (maxDisplacement*maxDisplacement)) {
				offsetFromPrev.SafeNormalize2D() *= maxDisplacement;
			}
			float3 posToTest = prevPos + offsetFromPrev;
			int curSquare = int(posToTest.z / SQUARE_SIZE)*mapDims.mapx + int(posToTest.x / SQUARE_SIZE);
			if (curSquare != newPosStartSquare) {
				bool updatePos = isSquareOpen(posToTest);
				if (updatePos) {
					resultantMove = offsetFromPrev;
					return true;
				}
			}
			return false;
		};

		for (int n = 1; n <= SQUARE_SIZE; n++) {
			updatePos = tryToMove(newPos, unit->rightdir * n);
			if (updatePos) { break; }
			updatePos = tryToMove(newPos, unit->rightdir * -n);
			if (updatePos) { break; }
		}

		if (!updatePos)
			resultantMove = ZeroVector;
		else {
			const float3 openPos = prevPos + resultantMove;
			const int2 openSquare = toMapSquare(openPos);
			const int2 fullDiffSquare = openSquare - prevSquare;
			if (fullDiffSquare.x != 0 && fullDiffSquare.y != 0) {
				// axis-aligned slide to avoid clipping around corners and potentially into traps.
				const unsigned int facing = GetFacingFromHeading(unit->heading);
				constexpr float3 vecs[2] =
					{ { 0.f, 0.f, 1.f}
					, { 1.f, 0.f, 0.f}
				};
				const float3 aaSlideAxis = vecs[(facing - 1) % 2];

				const float displacement = (facing % 2 == 0) ? resultantMove.x : resultantMove.z;
				const float side = 1.f - (2.f * (displacement < 0.f));
				const float3 offset = aaSlideAxis * std::min(displacement*side, speed) * side;
				const float3 posToTest = prevPos + offset;

				updatePos = isSquareOpen(posToTest);

			// 	{bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
			// 		&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
			// {	bool printMoveInfo = (unit->id == 19432);
			// 	if (printMoveInfo) {
			// 		LOG("%s: unit %d: facing(%f,%f,%f) [%d:%d] right(%f,%f,%f) disp=%f"
			// 				, __func__, owner->id
			// 				, float(unit->frontdir.x), float(unit->frontdir.y), float(unit->frontdir.z), int(unit->heading), facing
			// 				, aaSlideAxis.x, aaSlideAxis.y, aaSlideAxis.z
			// 				, displacement);
			// 		LOG("%s: unit %d: resultantVec=(%f,%f,%f) prevPos=(%f,%f,%f) offset=(%f,%f,%f) posToTest=(%f,%f,%f) result=%d"
			// 				, __func__, owner->id
			// 				, resultantMove.x, resultantMove.y, resultantMove.z
			// 				, prevPos.x, prevPos.y, prevPos.z
			// 				, offset.x, offset.y, offset.z
			// 				, posToTest.x, posToTest.y, posToTest.z
			// 				, int(updatePos));
			// 	}}

				if (updatePos) {
					resultantMove = offset;
				} else {
					resultantMove = ZeroVector;
				}
			} else if (resultantMove.SqLength2D() > speed*speed) {
				updatePos = tryToMove(prevPos, resultantMove, speed);
				if (!updatePos)
					resultantMove = ZeroVector;
			}
		}
	}
}


void CGroundMoveType::UpdateOwnerPos(const float3& oldSpeedVector, const float3& newSpeedVector) {
	RECOIL_DETAILED_TRACY_ZONE;
	/*const float*/ oldSpeed = oldSpeedVector.dot(flatFrontDir);
	/*const float*/ newSpeed = newSpeedVector.dot(flatFrontDir);

	const float3 moveRequest = newSpeedVector;

	// if being built, the nanoframe might not be exactly on
	// the ground and would jitter from gravity acting on it
	// --> nanoframes can not move anyway, just return early
	// (units that become reverse-built will continue moving)
	if (owner->beingBuilt)
		return;

	if (!oldSpeedVector.same(newSpeedVector)) {
		owner->SetVelocityAndSpeed(newSpeedVector);
	}

	if (!moveRequest.same(ZeroVector)) {
		float3 resultantVel;
		bool limitDisplacment = true;
		float maxDisplacementSq = -1.f;

		UpdatePos(owner, moveRequest, resultantVel, ThreadPool::GetThreadNum());

		bool isMoveColliding = !resultantVel.same(moveRequest);
		if (isMoveColliding) {
			// Sometimes now regular collisions won't happen due to this code preventing that.
			// so units need to be able to get themselves out of stuck situations. So adding
			// a rerequest path check here.
			ReRequestPath(false);
		}

		bool isThereAnOpenSquare = !resultantVel.same(ZeroVector);
		if (isThereAnOpenSquare){
			resultantForces = resultantVel;
			// owner->Move(resultantVel, true);
			if (positionStuck) {
				positionStuck = false;
			}
		} else if (positionStuck) {
			// Unit is stuck an the an open square could not be found. Just allow the unit to move
			// so that it gets unstuck eventually. Hopefully this won't look silly in practice, but
			// it is better than a unit being permanently stuck on something.
			resultantForces = moveRequest;
			// owner->Move(moveRequest, true);
		}

		// NOTE:
		//   this can fail when gravity is allowed (a unit catching air
		//   can easily end up on an impassable square, especially when
		//   terrain contains micro-bumps) --> more likely at lower g's
		// assert(owner->moveDef->TestMoveSquare(owner, owner->pos, owner->speed, true, false, true));
	}

	// reversing = UpdateOwnerSpeed(math::fabs(oldSpeed), math::fabs(newSpeed), newSpeed);
}

bool CGroundMoveType::UpdateOwnerSpeed(float oldSpeedAbs, float newSpeedAbs, float newSpeedRaw)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const bool oldSpeedAbsGTZ = (oldSpeedAbs > 0.01f);
	const bool newSpeedAbsGTZ = (newSpeedAbs > 0.01f);
	const bool newSpeedRawLTZ = (newSpeedRaw < 0.0f );

	owner->UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING, newSpeedAbsGTZ);

	if (!oldSpeedAbsGTZ &&  newSpeedAbsGTZ)
		owner->script->StartMoving(newSpeedRawLTZ);
	if ( oldSpeedAbsGTZ && !newSpeedAbsGTZ)
		owner->script->StopMoving();

	// Push resistant units need special handling, when they start/stop.
	// Not much point while they are on the move because their squares get recognised as moving,
	// not structure, and are ignored by collision and pathing.
	bool changeInMotion = IsPushResistant() && oldSpeedAbsGTZ != newSpeedAbsGTZ;
	if (changeInMotion){
		if (newSpeedAbsGTZ) {
			owner->UnBlock();
			pushResistanceBlockActive = false;
		} else {
			owner->Block();
			pushResistanceBlockActive = true;
			RegisterUnitForUnitTrapCheck(owner);
		}

		// this has to be done manually because units don't trigger it with block commands
		const int bx = owner->mapPos.x, sx = owner->xsize;
		const int bz = owner->mapPos.y, sz = owner->zsize;
		const int xminSqr = bx, xmaxSqr = bx + sx;
		const int zminSqr = bz, zmaxSqr = bz + sz;

		pathManager->TerrainChange(xminSqr, zminSqr, xmaxSqr, zmaxSqr, TERRAINCHANGE_OBJECT_INSERTED);
	}

	currentSpeed = newSpeedAbs;
	deltaSpeed   = 0.0f;

	return newSpeedRawLTZ;
}


bool CGroundMoveType::WantReverse(const float3& wpDir, const float3& ffDir) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!canReverse)
		return false;

	// these values are normally non-0, but LuaMoveCtrl
	// can override them and we do not want any div0's
	if (maxReverseSpeed <= 0.0f)
		return false;
	if (maxSpeed <= 0.0f)
		return true;

	if (accRate <= 0.0f)
		return false;
	if (decRate <= 0.0f)
		return false;
	if (turnRate <= 0.0f)
		return false;

	if (wpDir.dot(ffDir) >= 0.0f)
		return false;

	const float goalDist   = (goalPos - owner->pos).Length2D();                  // use *final* WP for ETA calcs; in elmos
	const float goalFwdETA = (goalDist / maxSpeed);                              // in frames (simplistic)
	const float goalRevETA = (goalDist / maxReverseSpeed);                       // in frames (simplistic)

	const float waypointAngle = std::clamp(wpDir.dot(owner->frontdir), -1.0f, 0.0f);  // clamp to prevent NaN's; [-1, 0]
	const float turnAngleDeg  = math::acosf(waypointAngle) * math::RAD_TO_DEG;   // in degrees; [90.0, 180.0]
	const float fwdTurnAngle  = (turnAngleDeg / 360.0f) * SPRING_CIRCLE_DIVS;    // in "headings"
	const float revTurnAngle  = SPRING_MAX_HEADING - fwdTurnAngle;               // 180 deg - angle

	// values <= 0 preserve default behavior
	if (maxReverseDist > 0.0f && minReverseAngle > 0.0f)
		return (currWayPointDist <= maxReverseDist && turnAngleDeg >= minReverseAngle);

	// units start accelerating before finishing the turn, so subtract something
	const float turnTimeMod      = 5.0f;
	const float fwdTurnAngleTime = std::max(0.0f, (fwdTurnAngle / turnRate) - turnTimeMod); // in frames
	const float revTurnAngleTime = std::max(0.0f, (revTurnAngle / turnRate) - turnTimeMod);

	const float apxFwdSpdAfterTurn = std::max(0.0f, currentSpeed - 0.125f * (fwdTurnAngleTime * decRate));
	const float apxRevSpdAfterTurn = std::max(0.0f, currentSpeed - 0.125f * (revTurnAngleTime * decRate));

	const float fwdDecTime = ( reversing * apxFwdSpdAfterTurn) / decRate;
	const float revDecTime = (!reversing * apxRevSpdAfterTurn) / decRate;
	const float fwdAccTime = (maxSpeed        - !reversing * apxFwdSpdAfterTurn) / accRate;
	const float revAccTime = (maxReverseSpeed -  reversing * apxRevSpdAfterTurn) / accRate;

	const float fwdETA = goalFwdETA + fwdTurnAngleTime + fwdAccTime + fwdDecTime;
	const float revETA = goalRevETA + revTurnAngleTime + revDecTime + revAccTime;

	return (fwdETA > revETA);
}



void CGroundMoveType::InitMemberPtrs(MemberData* memberData)
{
	RECOIL_DETAILED_TRACY_ZONE;
	memberData->bools[0].second = &atGoal;
	memberData->bools[1].second = &atEndOfPath;
	memberData->bools[2].second = &pushResistant;

	memberData->shorts[0].second = &minScriptChangeHeading;

	memberData->floats[0].second = &turnRate;
	memberData->floats[1].second = &turnAccel;
	memberData->floats[2].second = &accRate;
	memberData->floats[3].second = &decRate;
	memberData->floats[4].second = &myGravity;
	memberData->floats[5].second = &maxReverseDist,
	memberData->floats[6].second = &minReverseAngle;
	memberData->floats[7].second = &maxReverseSpeed;
	memberData->floats[8].second = &sqSkidSpeedMult;
}

bool CGroundMoveType::SetMemberValue(unsigned int memberHash, void* memberValue)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// try the generic members first
	if (AMoveType::SetMemberValue(memberHash, memberValue))
		return true;

	// set pointers for this GMT instance
	InitMemberPtrs(&gmtMemberData);

	// special cases
	if (memberHash == gmtMemberData.floats[MAXREVERSESPEED_MEMBER_IDX].first) {
		*(gmtMemberData.floats[MAXREVERSESPEED_MEMBER_IDX].second) = *(reinterpret_cast<float*>(memberValue)) / GAME_SPEED;
		return true;
	}

	// note: <memberHash> should be calculated via HsiehHash
	// todo: use template lambdas in C++14
	{
		const auto pred = [memberHash](const std::pair<unsigned int, bool*>& p) { return (memberHash == p.first); };
		const auto iter = std::find_if(gmtMemberData.bools.begin(), gmtMemberData.bools.end(), pred);
		if (iter != gmtMemberData.bools.end()) {
			*(iter->second) = *(reinterpret_cast<bool*>(memberValue));
			return true;
		}
	}
	{
		const auto pred = [memberHash](const std::pair<unsigned int, short*>& p) { return (memberHash == p.first); };
		const auto iter = std::find_if(gmtMemberData.shorts.begin(), gmtMemberData.shorts.end(), pred);
		if (iter != gmtMemberData.shorts.end()) {
			*(iter->second) = short(*(reinterpret_cast<float*>(memberValue))); // sic (see SetMoveTypeValue)
			return true;
		}
	}
	{
		const auto pred = [memberHash](const std::pair<unsigned int, float*>& p) { return (memberHash == p.first); };
		const auto iter = std::find_if(gmtMemberData.floats.begin(), gmtMemberData.floats.end(), pred);
		if (iter != gmtMemberData.floats.end()) {
			*(iter->second) = *(reinterpret_cast<float*>(memberValue));
			return true;
		}
	}

	return false;
}

