/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVEMATH_H
#define MOVEMATH_H

#include "Map/ReadMap.h"
#include "Sim/Objects/SolidObject.h"
#include "System/float3.h"
#include "System/Misc/BitwiseEnum.h"

struct MoveDef;
class CSolidObject;

namespace MoveTypes {
	struct CheckCollisionQuery {
		static constexpr float POS_Y_UNAVAILABLE = std::numeric_limits<float>::infinity();

		CheckCollisionQuery(const CSolidObject* ref)
			: unit(ref)
			, moveDef(ref->moveDef)
			, pos(ref->pos)
			, physicalState(ref->physicalState)
		{}

		CheckCollisionQuery(const MoveDef* refMoveDef)
			: moveDef(refMoveDef)
		{}

		bool    HasPhysicalStateBit(unsigned int bit) const { return ((physicalState & bit) != 0); }
		void    SetPhysicalStateBit(unsigned int bit) { unsigned int ps = physicalState; ps |= ( bit); physicalState = static_cast<CSolidObject::PhysicalState>(ps); }
		void  ClearPhysicalStateBit(unsigned int bit) { unsigned int ps = physicalState; ps &= (~bit); physicalState = static_cast<CSolidObject::PhysicalState>(ps); }
		bool IsInWater() const { return (HasPhysicalStateBit(CSolidObject::PhysicalState::PSTATE_BIT_INWATER)); }
		void DisableHeightChecks() { pos.y = POS_Y_UNAVAILABLE; }

		const CSolidObject* unit = nullptr;
		const MoveDef* moveDef = nullptr;
		float3 pos = {0.f, POS_Y_UNAVAILABLE, 0.f};
		CSolidObject::PhysicalState physicalState = CSolidObject::PhysicalState(CSolidObject::PhysicalState::PSTATE_BIT_ONGROUND);
	};
}


class CMoveMath {
	CR_DECLARE(CMoveMath)

protected:
	static float GroundSpeedMod(const MoveDef& moveDef, float height, float slope);
	static float GroundSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeMod);
	static float HoverSpeedMod(const MoveDef& moveDef, float height, float slope);
	static float HoverSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeMod);
	static float ShipSpeedMod(const MoveDef& moveDef, float height, float slope);
	static float ShipSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeMod);

public:
	// gives the y-coordinate the unit will "stand on"
	static float yLevel(const MoveDef& moveDef, const float3& pos);
	static float yLevel(const MoveDef& moveDef, int xSquare, int zSquare);

public:
	enum BlockTypes {
		BLOCK_NONE        = 0,
		BLOCK_MOVING      = 1,
		BLOCK_MOBILE      = 2,
		BLOCK_MOBILE_BUSY = 4,
		BLOCK_STRUCTURE   = 8,
		BLOCK_IMPASSABLE  = 24 // := 16 | BLOCK_STRUCTURE;
	};
	typedef Bitwise::BitwiseEnum<BlockTypes> BlockType;


	// returns a speed-multiplier for given position or data
	static float GetPosSpeedMod(const MoveDef& moveDef, unsigned xSquare, unsigned zSquare);
	static float GetPosSpeedMod(const MoveDef& moveDef, unsigned xSquare, unsigned zSquare, float3 moveDir);
	static float GetPosSpeedMod(const MoveDef& moveDef, const float3& pos)
	{
		return (GetPosSpeedMod(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE));
	}
	static float GetPosSpeedMod(const MoveDef& moveDef, const float3& pos, const float3& moveDir)
	{
		return (GetPosSpeedMod(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE, moveDir));
	}
	static float GetPosSpeedMod(const MoveDef& moveDef, unsigned squareIndex);

	// tells whether a position is blocked (inaccessable for a given object's MoveDef)
	static inline BlockType IsBlocked(const MoveDef& moveDef, const float3& pos, const CSolidObject* collider);
	static inline BlockType IsBlocked(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);
	static BlockType IsBlockedNoSpeedModCheck(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);
	static BlockType IsBlockedNoSpeedModCheckDiff(const MoveDef& moveDef, int2 prevSqr, int2 newSqr, const CSolidObject* collider, int thread = 0);
	static BlockType IsBlockedNoSpeedModCheckThreadUnsafe(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);
	static inline BlockType IsBlockedStructure(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);

	// checks whether an object (collidee) is non-crushable by the given MoveDef
	static bool CrushResistant(const MoveDef& colliderMD, const CSolidObject* collidee);
	// checks whether an object (collidee) is non-blocking for the given MoveDef
	// (eg. would return true for a submarine's moveDef vs. a surface ship object)
	static bool IsNonBlocking(const CSolidObject* collidee, const MoveTypes::CheckCollisionQuery* collider);
	static bool IsNonBlocking(const MoveDef& colliderMD, const CSolidObject* collidee, const CSolidObject* collider) {
		if (collider != nullptr) {
			MoveTypes::CheckCollisionQuery colliderInfo(collider);
			return (IsNonBlocking(collidee, &colliderInfo));
		} else {
			MoveTypes::CheckCollisionQuery colliderInfo(&colliderMD);
			return (IsNonBlocking(collidee, &colliderInfo));
		}
	};
	//static bool IsNonBlocking(const CSolidObject* collidee, const CSolidObject* collider);

	// check how this unit blocks its squares
	static BlockType ObjectBlockType(const CSolidObject* collidee, const MoveTypes::CheckCollisionQuery* collider);
	static BlockType ObjectBlockType(const MoveDef& moveDef, const CSolidObject* collidee, const CSolidObject* collider) {
		if (collider != nullptr) {
			MoveTypes::CheckCollisionQuery colliderInfo(collider);
			return (ObjectBlockType(collidee, &colliderInfo));
		} else {
			MoveTypes::CheckCollisionQuery colliderInfo(&moveDef);
			return (ObjectBlockType(collidee, &colliderInfo));
		}
	}; // TODO: EXPECT THIS FUNCTION CAN BE REMOVED

	// checks if a single square is accessable for any object which uses the given MoveDef
	static BlockType SquareIsBlocked(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);
	static BlockType SquareIsBlocked(const MoveDef& moveDef, const float3& pos, const CSolidObject* collider) {
		return (SquareIsBlocked(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE, collider));
	}
	static BlockType RangeIsBlocked(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int thread = 0);
	static BlockType RangeIsBlocked(const MoveDef& moveDef, int xmin, int xmax, int zmin, int zmax, const CSolidObject* collider, int thread = 0) {
		if (collider != nullptr) {
			MoveTypes::CheckCollisionQuery colliderInfo(collider);
			return (RangeIsBlocked(xmin, xmax, zmin, zmax, &colliderInfo, thread));
		} else {
			MoveTypes::CheckCollisionQuery colliderInfo(&moveDef);
			return (RangeIsBlocked(xmin, xmax, zmin, zmax, &colliderInfo, thread));
		}
	};
	static BlockType RangeIsBlockedTempNum(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int tempNum, int thread);

	static BlockType RangeIsBlockedSt(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int magicNumber);
	static BlockType RangeIsBlockedMt(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int thread, int magicNumber);

	static void InitRangeIsBlockedHashes();
	static BlockType RangeIsBlockedHashedSt(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int magicNumber);
	static BlockType RangeIsBlockedHashedMt(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int magicNumber, int thread);

	static void FloodFillRangeIsBlocked(const MoveDef& moveDef, const CSolidObject* collider, const SRectangle& areaToSample, std::vector<std::uint8_t>& results, int thread);

public:
	static bool noHoverWaterMove;
	static float waterDamageCost;
};


/* Check if a given square-position is accessable by the MoveDef footprint. */
inline CMoveMath::BlockType CMoveMath::IsBlocked(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	if (GetPosSpeedMod(moveDef, xSquare, zSquare) == 0.0f)
		return BLOCK_IMPASSABLE;

	return (IsBlockedNoSpeedModCheck(moveDef, xSquare, zSquare, collider));
}

/* Converts a point-request into a square-positional request. */
inline CMoveMath::BlockType CMoveMath::IsBlocked(const MoveDef& moveDef, const float3& pos, const CSolidObject* collider)
{
	return (IsBlocked(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE, collider));
}

inline CMoveMath::BlockType CMoveMath::IsBlockedStructure(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider) {
	return (IsBlockedNoSpeedModCheck(moveDef, xSquare, zSquare, collider) & BLOCK_STRUCTURE);
}

#endif

