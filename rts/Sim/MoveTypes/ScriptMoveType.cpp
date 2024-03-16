/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ScriptMoveType.h"

#include "Components/MoveTypesComponents.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Sim/Ecs/Registry.h"
#include "Sim/Misc/Wind.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"

#include <tracy/Tracy.hpp>

using namespace MoveTypes;

CR_BIND_DERIVED(CScriptMoveType, AMoveType, (nullptr))
CR_REG_METADATA(CScriptMoveType, (
	CR_MEMBER(velVec),
	CR_MEMBER(relVel),
	CR_MEMBER(rot),
	CR_MEMBER(rotVel),
	CR_MEMBER(mins),
	CR_MEMBER(maxs),

	CR_MEMBER(tag),

	CR_MEMBER(drag),
	CR_MEMBER(groundOffset),
	CR_MEMBER(gravityFactor),
	CR_MEMBER(windFactor),

	CR_MEMBER(extrapolate),
	CR_MEMBER(useRelVel),
	CR_MEMBER(useRotVel),

	CR_MEMBER(trackSlope),
	CR_MEMBER(trackGround),
	CR_MEMBER(trackLimits),

	CR_MEMBER(noBlocking), // copy of CSolidObject::PSTATE_BIT_BLOCKING

	CR_MEMBER(groundStop),
	CR_MEMBER(limitsStop),

	CR_MEMBER(scriptNotify),

	CR_PREALLOC(GetPreallocContainer)
))


CScriptMoveType::CScriptMoveType(CUnit* unit): AMoveType(unit)
{
	//ZoneScoped;
	// use the transformation matrix instead of heading
	UseHeading(false);

	// creg
	if (unit == nullptr)
		return;

	Sim::registry.emplace_or_replace<GeneralMoveType>(owner->entityReference, owner->id);
}


CScriptMoveType::~CScriptMoveType()
{
	//ZoneScoped;
	// clean up if noBlocking was made true at
	// some point during this script's lifetime
	// and not reset
	owner->UnBlock();

	Sim::registry.remove<GeneralMoveType>(owner->entityReference);
}


void CScriptMoveType::CheckNotify()
{
	//ZoneScoped;
	if (scriptNotify == HitNothing)
		return;

	if (eventHandler.MoveCtrlNotify(owner, scriptNotify)) {
		// NOTE: deletes \<this\>
		owner->DisableScriptMoveType();
	} else {
		scriptNotify = HitNothing;
	}
}


bool CScriptMoveType::Update()
{
	//ZoneScoped;
	if (useRotVel)
		owner->SetDirVectorsEuler(rot += rotVel);

	if (extrapolate) {
		// NOTE: only gravitational acc. is allowed to build up velocity
		// NOTE: strong wind plus low gravity can cause substantial drift
		const float3 gravVec = UpVector * (mapInfo->map.gravity * gravityFactor);
		const float3 windVec =            (envResHandler.GetCurrentWindVec() * windFactor);
		const float3 unitVec = useRelVel?
			(owner->frontdir *  relVel.z) +
			(owner->updir    *  relVel.y) +
			(owner->rightdir * -relVel.x):
			ZeroVector;

		owner->Move(gravVec + velVec, true);
		owner->Move(windVec,          true);
		owner->Move(unitVec,          true);

		// quadratic drag does not work well here
		velVec += gravVec;
		velVec *= (1.0f - drag);

		owner->SetVelocityAndSpeed(velVec);
	}

	if (trackGround) {
		const float gndMin = CGround::GetHeightReal(owner->pos.x, owner->pos.z) + groundOffset;

		if (owner->pos.y <= gndMin) {
			owner->Move(UpVector * (gndMin - owner->pos.y), true);
			owner->speed.y = 0.0f;

			if (groundStop) {
				velVec = ZeroVector;
				relVel = ZeroVector;
				rotVel = ZeroVector;
				scriptNotify = HitGround;
			}
		}
	}

	// positional clamps
	CheckLimits();

	if (trackSlope) {
		owner->UpdateDirVectors(owner->IsOnGround(), owner->IsInAir(), owner->unitDef->upDirSmoothing);
		owner->UpdateMidAndAimPos();
	}

	// don't need the rest if the pos hasn't changed
	if (oldPos == owner->pos) {
		CheckNotify();
		return false;
	}

	oldPos = owner->pos;

	if (!noBlocking)
		owner->Block();

	CheckNotify();
	return true;
}


void CScriptMoveType::CheckLimits()
{
	//ZoneScoped;
	float3 pos = owner->pos;
	float4 vel = owner->speed;

	if (pos.x < mins.x) { pos.x = mins.x; vel.x = 0.0f; }
	if (pos.x > maxs.x) { pos.x = maxs.x; vel.x = 0.0f; }
	if (pos.y < mins.y) { pos.y = mins.y; vel.y = 0.0f; }
	if (pos.y > maxs.y) { pos.y = maxs.y; vel.y = 0.0f; }
	if (pos.z < mins.z) { pos.z = mins.z; vel.z = 0.0f; }
	if (pos.z > maxs.z) { pos.z = maxs.z; vel.z = 0.0f; }

	// bail if still within limits
	if (pos == owner->pos)
		return;


	if (limitsStop) {
		owner->Move(pos, false);
		owner->SetVelocityAndSpeed(vel);
	}

	if (trackLimits) {
		scriptNotify = HitLimit;
		CheckNotify();
	}
}


void CScriptMoveType::SetPhysics(const float3& _pos, const float3& _vel, const float3& _rot)
{
	//ZoneScoped;
	SetPosition(_pos);
	SetVelocity(_vel);
	SetRotation(_rot);
}


void CScriptMoveType::SetPosition(const float3& _pos) { owner->Move(_pos, false); }
void CScriptMoveType::SetVelocity(const float3& _vel) { owner->SetVelocityAndSpeed(velVec = _vel); }


void CScriptMoveType::SetRelativeVelocity(const float3& _relVel) { useRelVel = ((relVel = _relVel) != ZeroVector); }
void CScriptMoveType::SetRotationVelocity(const float3& _rotVel) { useRotVel = ((rotVel = _rotVel) != ZeroVector); }


void CScriptMoveType::SetRotation(const float3& _rot)
{
	//ZoneScoped;
	owner->SetDirVectorsEuler(rot = _rot);
}

void CScriptMoveType::SetHeading(short heading)
{
	//ZoneScoped;
	owner->SetHeading(heading, trackSlope, false, 0.0f);
}


void CScriptMoveType::SetNoBlocking(bool state)
{
	//ZoneScoped;
	// if false, forces blocking-map updates
	if ((noBlocking = state)) {
		owner->UnBlock();
	} else {
		owner->Block();
	}
}
