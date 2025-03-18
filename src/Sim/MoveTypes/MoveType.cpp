/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>


#include "MoveType.h"
#include "Map/Ground.h"
#include "Components/MoveTypesComponents.h"
#include "Sim/Ecs/Registry.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/SpringMath.h"
#include "System/SpringHash.h"

#include "System/TimeProfiler.h"

#include "System/Misc/TracyDefs.h"

using namespace MoveTypes;

CR_BIND_DERIVED_INTERFACE(AMoveType, CObject)
CR_REG_METADATA(AMoveType, (
	CR_MEMBER(owner),
	CR_MEMBER(goalPos),
	CR_MEMBER(oldPos),
	CR_MEMBER(oldSlowUpdatePos),
	CR_MEMBER(oldCollisionUpdatePos),

	CR_MEMBER(progressState),

	CR_MEMBER(maxSpeed),
	CR_MEMBER(maxSpeedDef),
	CR_MEMBER(maxWantedSpeed),
	CR_MEMBER(maneuverLeash),
	CR_MEMBER(waterline),

	CR_MEMBER(useHeading),
	CR_MEMBER(useWantedSpeed)
))



#define MEMBER_CHARPTR_HASH(memberName) spring::LiteHash(memberName, strlen(memberName),     0)
#define MEMBER_LITERAL_HASH(memberName) spring::LiteHash(memberName, sizeof(memberName) - 1, 0)

static const unsigned int BOOL_MEMBER_HASHES[] = {
	MEMBER_LITERAL_HASH("useWantedSpeed[0]"), // individual
	MEMBER_LITERAL_HASH("useWantedSpeed[1]"), // formation
};
static const unsigned int FLOAT_MEMBER_HASHES[] = {
	MEMBER_LITERAL_HASH(         "maxSpeed"),
	MEMBER_LITERAL_HASH(   "maxWantedSpeed"),
	MEMBER_LITERAL_HASH(    "maneuverLeash"),
	MEMBER_LITERAL_HASH(        "waterline"),
};

#undef MEMBER_CHARPTR_HASH
#undef MEMBER_LITERAL_HASH



AMoveType::AMoveType(CUnit* owner):
	owner(owner),

	goalPos((owner != nullptr)? owner->pos: ZeroVector),
	oldPos((owner != nullptr)? owner->pos: ZeroVector),
	oldSlowUpdatePos(oldPos),

	maxSpeed((owner != nullptr)? owner->unitDef->speed / GAME_SPEED : 0.0f),
	maxSpeedDef((owner != nullptr)? owner->unitDef->speed / GAME_SPEED : 0.0f),
	maxWantedSpeed((owner != nullptr)? owner->unitDef->speed / GAME_SPEED : 0.0f),

	maneuverLeash(500.0f),
	waterline((owner != nullptr)? owner->unitDef->waterline: 0.0f)
{
}

void AMoveType::SlowUpdate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	UpdateGroundBlockMap();
}

void AMoveType::UpdateCollisionMap(bool force)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!force && ((gs->frameNum + owner->id) % modInfo.unitQuadPositionUpdateRate))
		return;

	if (owner->pos != oldCollisionUpdatePos){
		oldCollisionUpdatePos = owner->pos;
		quadField.MovedUnit(owner);
	}
}

void AMoveType::UpdateGroundBlockMap() {
	RECOIL_DETAILED_TRACY_ZONE;
	if (owner->pos != oldSlowUpdatePos) {
		const int newMapSquare = CGround::GetSquare(oldSlowUpdatePos = owner->pos);

		if (newMapSquare != owner->mapSquare) {
			owner->mapSquare = newMapSquare;

			if (!owner->UsingScriptMoveType()) {
				if ((owner->IsOnGround() || owner->IsInWater()) && owner->unitDef->IsGroundUnit()) {
					// always (re-)add us to occupation map if we moved
					// (since our last SlowUpdate) and are on the ground
					// NOTE: ships are ground units but not on the ground
					owner->Block();
				}
			}
		}
	}
}

void AMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive)
{
	RECOIL_DETAILED_TRACY_ZONE;
	KeepPointingTo(float3(unit->pos), distance, aggressive);
}

float AMoveType::CalcStaticTurnRadius() const {
	RECOIL_DETAILED_TRACY_ZONE;
	// calculate a rough turn radius (not based on current speed)
	const float turnFrames = SPRING_CIRCLE_DIVS / std::max(owner->unitDef->turnRate, 1.0f);
	const float turnRadius = (maxSpeedDef * turnFrames) / math::TWOPI;

	return turnRadius;
}



bool AMoveType::SetMemberValue(unsigned int memberHash, void* memberValue) {
	RECOIL_DETAILED_TRACY_ZONE;
	#define          MAXSPEED_MEMBER_IDX 0
	#define    MAXWANTEDSPEED_MEMBER_IDX 1
	#define     MANEUVERLEASH_MEMBER_IDX 2
	#define         WATERLINE_MEMBER_IDX 3

	bool* boolMemberPtrs[] = {
		&useWantedSpeed[false],
		&useWantedSpeed[ true],
	};

	#if 0
	// unordered_map etc. perform dynallocs, so KISS here
	float* floatMemberPtrs[] = {
		&maxSpeed,
		&maxWantedSpeed,
	};
	#endif

	for (size_t n = 0; n < sizeof(boolMemberPtrs) / sizeof(boolMemberPtrs[0]); n++) {
		if (memberHash == BOOL_MEMBER_HASHES[n]) {
			*(boolMemberPtrs[n]) = *(reinterpret_cast<bool*>(memberValue));
			return true;
		}
	}

	// special cases
	if (memberHash == FLOAT_MEMBER_HASHES[MAXSPEED_MEMBER_IDX]) {
		SetMaxSpeed((*reinterpret_cast<float*>(memberValue)) / GAME_SPEED);
		return true;
	}
	if (memberHash == FLOAT_MEMBER_HASHES[MAXWANTEDSPEED_MEMBER_IDX]) {
		SetWantedMaxSpeed((*reinterpret_cast<float*>(memberValue)) / GAME_SPEED);
		return true;
	}
	if (memberHash == FLOAT_MEMBER_HASHES[MANEUVERLEASH_MEMBER_IDX]) {
		SetManeuverLeash(*reinterpret_cast<float*>(memberValue));
		return true;
	}
	if (memberHash == FLOAT_MEMBER_HASHES[WATERLINE_MEMBER_IDX]) {
		SetWaterline(*reinterpret_cast<float*>(memberValue));
		return true;
	}

	return false;
}

void AMoveType::Connect() {
	RECOIL_DETAILED_TRACY_ZONE;
	Sim::registry.emplace_or_replace<GeneralMoveType>(owner->entityReference, owner->id);
}

void AMoveType::Disconnect() {
	RECOIL_DETAILED_TRACY_ZONE;
	Sim::registry.remove<GeneralMoveType>(owner->entityReference);
}
