/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveMath.h"

#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Units/Unit.h"
#include "System/Platform/Threading.h"

#include <tracy/Tracy.hpp>

bool CMoveMath::noHoverWaterMove = false;
float CMoveMath::waterDamageCost = 0.0f;

static constexpr int FOOTPRINT_XSTEP = 2;
static constexpr int FOOTPRINT_ZSTEP = 2;


float CMoveMath::yLevel(const MoveDef& moveDef, int xSqr, int zSqr)
{
	//ZoneScoped;
	switch (moveDef.speedModClass) {
		case MoveDef::Tank: // fall-through
		case MoveDef::KBot:  { return (CGround::GetHeightReal      (xSqr * SQUARE_SIZE, zSqr * SQUARE_SIZE) + 10.0f); } break;
		case MoveDef::Hover: { return (CGround::GetHeightAboveWater(xSqr * SQUARE_SIZE, zSqr * SQUARE_SIZE) + 10.0f); } break;
		case MoveDef::Ship:  { return (                                                                        0.0f); } break;
	}

	return 0.0f;
}

float CMoveMath::yLevel(const MoveDef& moveDef, const float3& pos)
{
	//ZoneScoped;
	switch (moveDef.speedModClass) {
		case MoveDef::Tank: // fall-through
		case MoveDef::KBot:  { return (CGround::GetHeightReal      (pos.x, pos.z) + 10.0f); } break;
		case MoveDef::Hover: { return (CGround::GetHeightAboveWater(pos.x, pos.z) + 10.0f); } break;
		case MoveDef::Ship:  { return (                                              0.0f); } break;
	}

	return 0.0f;
}



/* calculate the local speed-modifier for this MoveDef */
float CMoveMath::GetPosSpeedMod(const MoveDef& moveDef, unsigned xSquare, unsigned zSquare)
{
	//ZoneScoped;
	if (xSquare >= mapDims.mapx || zSquare >= mapDims.mapy)
		return 0.0f;

	const int accurateSquare = xSquare + (zSquare * mapDims.mapx);
	const int square = (xSquare >> 1) + ((zSquare >> 1) * mapDims.hmapx);
	const int squareTerrType = readMap->GetTypeMapSynced()[square];

	const float height = readMap->GetMaxHeightMapSynced()[accurateSquare];
	const float slope   = readMap->GetSlopeMapSynced()[square];

	const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[squareTerrType];

	switch (moveDef.speedModClass) {
		case MoveDef::Tank:  { return (GroundSpeedMod(moveDef, height, slope) * tt.tankSpeed ); } break;
		case MoveDef::KBot:  { return (GroundSpeedMod(moveDef, height, slope) * tt.kbotSpeed ); } break;
		case MoveDef::Hover: { return ( HoverSpeedMod(moveDef, height, slope) * tt.hoverSpeed); } break;
		case MoveDef::Ship:  { return (  ShipSpeedMod(moveDef, height, slope) * tt.shipSpeed ); } break;
		default: {} break;
	}

	return 0.0f;
}

float CMoveMath::GetPosSpeedMod(const MoveDef& moveDef, unsigned xSquare, unsigned zSquare, float3 moveDir)
{
	//ZoneScoped;
	if (xSquare >= mapDims.mapx || zSquare >= mapDims.mapy)
		return 0.0f;

	const int accurateSquare = xSquare + (zSquare * mapDims.mapx);
	const int square = (xSquare >> 1) + ((zSquare >> 1) * mapDims.hmapx);
	const int squareTerrType = readMap->GetTypeMapSynced()[square];

	const float height = readMap->GetMaxHeightMapSynced()[accurateSquare];
	const float slope  = readMap->GetSlopeMapSynced()[square];

	const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[squareTerrType];

	const float3 sqrNormal = readMap->GetCenterNormals2DSynced()[xSquare + zSquare * mapDims.mapx];

	// with a flat normal, only consider the normalized xz-direction
	// (the actual steepness is represented by the "slope" variable)
	// we verify that it was normalized in advance
	assert(float3(moveDir).SafeNormalize2D() == moveDir);

	// note: moveDir is (or should be) a unit vector in the xz-plane, y=0
	// scale is negative for "downhill" slopes, positive for "uphill" ones
	const float dirSlopeMod = -moveDir.dot(sqrNormal);

	switch (moveDef.speedModClass) {
		case MoveDef::Tank:  { return (GroundSpeedMod(moveDef, height, slope, dirSlopeMod) * tt.tankSpeed ); } break;
		case MoveDef::KBot:  { return (GroundSpeedMod(moveDef, height, slope, dirSlopeMod) * tt.kbotSpeed ); } break;
		case MoveDef::Hover: { return ( HoverSpeedMod(moveDef, height, slope, dirSlopeMod) * tt.hoverSpeed); } break;
		case MoveDef::Ship:  { return (  ShipSpeedMod(moveDef, height, slope, dirSlopeMod) * tt.shipSpeed ); } break;
		default: {} break;
	}

	return 0.0f;
}

/* Check if a given square-position is accessable by the MoveDef footprint. */
CMoveMath::BlockType CMoveMath::IsBlockedNoSpeedModCheck(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	//ZoneScoped;
	const int xmin = std::max(xSquare - moveDef.xsizeh,                0);
	const int zmin = std::max(zSquare - moveDef.zsizeh,                0);
	const int xmax = std::min(xSquare + moveDef.xsizeh, mapDims.mapx - 1);
	const int zmax = std::min(zSquare + moveDef.zsizeh, mapDims.mapy - 1);

	BlockType ret = BLOCK_NONE;

	// footprints are point-symmetric around <xSquare, zSquare>
	// same as RangeIsBlocked but without anti-duplication test
	for (int z = zmin; z <= zmax; z += FOOTPRINT_ZSTEP) {
		const int zOffset = z * mapDims.mapx;

		for (int x = xmin; x <= xmax; x += FOOTPRINT_XSTEP) {
			const CGroundBlockingObjectMap::BlockingMapCell& cell = groundBlockingObjectMap.GetCellUnsafeConst(zOffset + x);

			for (size_t i = 0, n = cell.size(); i < n; i++) {
				const CSolidObject* collidee = cell[i];

				if (((ret |= ObjectBlockType(moveDef, collidee, collider)) & BLOCK_STRUCTURE) == 0)
					continue;

				return ret;
			}
		}
	}

	return ret;
}

/* Check if a given square-position is accessable by the MoveDef footprint. */
CMoveMath::BlockType CMoveMath::IsBlockedNoSpeedModCheckDiff(const MoveDef& moveDef, int2 prevSqr, int2 newSqr, const CSolidObject* collider, int thread)
{
	//ZoneScoped;
	// prev allows {-1, -1} so that the first check is always treated as a full test
	const int prev_xmin = std::max(prevSqr.x - moveDef.xsizeh,               -1);
	const int prev_zmin = std::max(prevSqr.y - moveDef.zsizeh,               -1);
	const int prev_xmax = std::min(prevSqr.x + moveDef.xsizeh, mapDims.mapx - 1);
	const int prev_zmax = std::min(prevSqr.y + moveDef.zsizeh, mapDims.mapy - 1);

	const int xmin = std::max(newSqr.x - moveDef.xsizeh,                0);
	const int zmin = std::max(newSqr.y - moveDef.zsizeh,                0);
	const int xmax = std::min(newSqr.x + moveDef.xsizeh, mapDims.mapx - 1);
	const int zmax = std::min(newSqr.y + moveDef.zsizeh, mapDims.mapy - 1);

	BlockType ret = BLOCK_NONE;

	const int tempNum = gs->GetMtTempNum(thread);

	// footprints are point-symmetric around <xSquare, zSquare>
	for (int z = zmin; z <= zmax; z += FOOTPRINT_ZSTEP) {
		const int zOffset = z * mapDims.mapx;

		for (int x = xmin; x <= xmax; x += FOOTPRINT_XSTEP) {

			if (		z <= prev_zmax && z >= prev_zmin
			 		&& 	x <= prev_xmax && x >= prev_xmin)
				continue;

			const CGroundBlockingObjectMap::BlockingMapCell& cell = groundBlockingObjectMap.GetCellUnsafeConst(zOffset + x);

			for (size_t i = 0, n = cell.size(); i < n; i++) {
				CSolidObject* collidee = cell[i];

				if (collidee->mtTempNum[thread] == tempNum)
					continue;

				collidee->mtTempNum[thread] = tempNum;

				if (((ret |= ObjectBlockType(moveDef, collidee, collider)) & BLOCK_STRUCTURE) == 0)
					continue;

				return ret;
			}
		}
	}

	return ret;
}

CMoveMath::BlockType CMoveMath::IsBlockedNoSpeedModCheckThreadUnsafe(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	//ZoneScoped;
	assert(Threading::IsMainThread() || Threading::IsGameLoadThread());
	return RangeIsBlocked(moveDef, xSquare - moveDef.xsizeh, xSquare + moveDef.xsizeh, zSquare - moveDef.zsizeh, zSquare + moveDef.zsizeh, collider);
}


bool CMoveMath::CrushResistant(const MoveDef& colliderMD, const CSolidObject* collidee)
{
	//ZoneScoped;
	if (!collidee->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
		return false;
	if (!collidee->crushable)
		return true;

	return (collidee->crushResistance > colliderMD.crushStrength);
}

bool CMoveMath::IsNonBlocking(const CSolidObject* collidee, const MoveTypes::CheckCollisionQuery* collider)
{
	//ZoneScoped;
	if (collider->unit == collidee)
		return true;
	if (!collidee->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
		return true;
	// if obstacle is out of map bounds, it cannot block us
	if (!collidee->pos.IsInBounds())
		return true;
	// same if obstacle is not currently marked on blocking-map
	if (!collidee->IsBlocking())
		return true;

	// remaining conditions under which obstacle does NOT block unit
	// only reachable from stand-alone PE invocations or GameHelper
	//   1.
	//      unit is a submarine, obstacle sticks out above-water
	//      (and not itself flagged as a submarine) *OR* unit is
	//      not a submarine and obstacle is (fully under-water or
	//      flagged as a submarine)
	//
	//      NOTE:
	//        do we want to allow submarines to pass underneath
	//        any obstacle even if it is 99% submerged already?
	//
	//        will cause stacking for submarines that are *not*
	//        explicitly flagged as such in their MoveDefs
	//
	// note that these condition(s) can lead to a certain degree of
	// clipping: for full 3D accuracy the height of the MoveDef's
	// owner would need to be accessible, but the path-estimator
	// defs are not tied to any collider instances
	//
	if (collider->pos.y == MoveTypes::CheckCollisionQuery::POS_Y_UNAVAILABLE) {
		const bool colliderIsSub = collider->moveDef->isSubmarine;
		const bool collideeIsSub = collidee->moveDef != nullptr && collidee->moveDef->isSubmarine;

		if (colliderIsSub)
			return (!collidee->IsUnderWater() && !collideeIsSub);

		// we don't have height information here so everything above and below water is going to be
		// considered blocking when the unit moveDef is amphibious.
		if (collider->moveDef->followGround)
			return false;

		return (collidee->IsUnderWater() || collideeIsSub);
	}

	// simple case: if unit and obstacle have non-zero
	// vertical separation as measured by their (model)
	// heights, unit can in theory always pass obstacle
	//
	// this allows (units marked as) submarines to both
	// *pass* and *short-range path* underneath floating
	// DT, or ships to P&SRP over underwater structures
	//
	// specifically restricted to units *inside* water
	// because it can have the unwanted side-effect of
	// enabling the PFS to generate paths for units on
	// steep slopes *through* obstacles, either higher
	// up or lower down
	//
	if (collider->IsInWater() && collidee->IsInWater()) {
		float colliderHeight = (collider->moveDef != nullptr) ? collider->moveDef->height : math::fabs(collider->unit->height);
		if ((collider->pos.y + colliderHeight) < collidee->pos.y)
			return true;

		float collideeHeight = (collidee->moveDef != nullptr) ? collidee->moveDef->height : math::fabs(collidee->height);
		if ((collidee->pos.y + collideeHeight) < collider->pos.y)
			return true;
	}
	return false;
}

CMoveMath::BlockType CMoveMath::ObjectBlockType(const CSolidObject* collidee, const MoveTypes::CheckCollisionQuery* collider)
{
	//ZoneScoped;
	if (IsNonBlocking(collidee, collider))
		return BLOCK_NONE;

	if (collidee->immobile)
		return ((CrushResistant(*(collider->moveDef), collidee))? BLOCK_STRUCTURE: BLOCK_NONE);

	// mobile obstacle, must be a unit
	const CUnit* u = static_cast<const CUnit*>(collidee);
	const AMoveType* mt = u->moveType;

	// if moving, unit is probably following a path
	if (u->IsMoving())
		return BLOCK_MOVING;

	// not moving and not pushable, treat as blocking
	if (mt->IsPushResistant())
		return BLOCK_STRUCTURE;

	// otherwise, unit is idling (no orders) or busy with a command
	// being-built units never count as idle, but should perhaps be
	// considered BLOCK_STRUCTURE
	return ((u->IsIdle())? BLOCK_MOBILE: BLOCK_MOBILE_BUSY);
}

CMoveMath::BlockType CMoveMath::SquareIsBlocked(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	//ZoneScoped;
	if (static_cast<unsigned>(xSquare) >= mapDims.mapx || static_cast<unsigned>(zSquare) >= mapDims.mapy)
		return BLOCK_IMPASSABLE;

	BlockType r = BLOCK_NONE;

	const CGroundBlockingObjectMap::BlockingMapCell& cell = groundBlockingObjectMap.GetCellUnsafeConst(zSquare * mapDims.mapx + xSquare);

	for (size_t i = 0, n = cell.size(); i < n; i++) {
		r |= ObjectBlockType(moveDef, cell[i], collider);
	}

	return r;
}

CMoveMath::BlockType CMoveMath::RangeIsBlocked(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int thread)
{
	//ZoneScoped;
	xmin = std::max(xmin,                0);
	zmin = std::max(zmin,                0);
	xmax = std::min(xmax, mapDims.mapx - 1);
	zmax = std::min(zmax, mapDims.mapy - 1);

	BlockType ret = BLOCK_NONE;
	if (ThreadPool::inMultiThreadedSection) {
		const int tempNum = gs->GetMtTempNum(thread);
		ret = CMoveMath::RangeIsBlockedMt(xmin, xmax, zmin, zmax, collider, thread, tempNum);
	} else {
		const int tempNum = gs->GetTempNum();
		ret = CMoveMath::RangeIsBlockedSt(xmin, xmax, zmin, zmax, collider, tempNum);
	}

	return ret;
}

CMoveMath::BlockType CMoveMath::RangeIsBlockedTempNum(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int tempNum, int thread)
{
	//ZoneScoped;
	xmin = std::max(xmin,                0);
	zmin = std::max(zmin,                0);
	xmax = std::min(xmax, mapDims.mapx - 1);
	zmax = std::min(zmax, mapDims.mapy - 1);

	BlockType ret = BLOCK_NONE;
	if (ThreadPool::inMultiThreadedSection) {
		const int tempNum = gs->GetMtTempNum(thread);
		ret = CMoveMath::RangeIsBlockedHashedMt(xmin, xmax, zmin, zmax, collider, tempNum, thread);
	} else {
		const int tempNum = gs->GetTempNum();
		ret = CMoveMath::RangeIsBlockedHashedSt(xmin, xmax, zmin, zmax, collider, tempNum);
	}

	return ret;
}

CMoveMath::BlockType CMoveMath::RangeIsBlockedSt(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int tempNum)
{
	//ZoneScoped;
	BlockType ret = BLOCK_NONE;

	// footprints are point-symmetric around <xSquare, zSquare>
	for (int z = zmin; z <= zmax; z += FOOTPRINT_ZSTEP) {
		const int zOffset = z * mapDims.mapx;

		for (int x = xmin; x <= xmax; x += FOOTPRINT_XSTEP) {
			const CGroundBlockingObjectMap::BlockingMapCell& cell = groundBlockingObjectMap.GetCellUnsafeConst(zOffset + x);

			for (size_t i = 0, n = cell.size(); i < n; i++) {
				CSolidObject* collidee = cell[i];

				if (collidee->tempNum == tempNum)
					continue;

				collidee->tempNum = tempNum;

				if (((ret |= ObjectBlockType(collidee, collider)) & BLOCK_STRUCTURE) == 0)
					continue;

				return ret;
			}
		}
	}

	return ret;
}


CMoveMath::BlockType CMoveMath::RangeIsBlockedMt(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int thread, int tempNum)
{
	//ZoneScoped;
	BlockType ret = BLOCK_NONE;

	// footprints are point-symmetric around <xSquare, zSquare>
	for (int z = zmin; z <= zmax; z += FOOTPRINT_ZSTEP) {
		const int zOffset = z * mapDims.mapx;

		for (int x = xmin; x <= xmax; x += FOOTPRINT_XSTEP) {
			const CGroundBlockingObjectMap::BlockingMapCell& cell = groundBlockingObjectMap.GetCellUnsafeConst(zOffset + x);

			for (size_t i = 0, n = cell.size(); i < n; i++) {
				CSolidObject* collidee = cell[i];

				if (collidee->mtTempNum[thread] == tempNum)
					continue;

				collidee->mtTempNum[thread] = tempNum;

				if (((ret |= ObjectBlockType(collidee, collider)) & BLOCK_STRUCTURE) == 0)
					continue;

				return ret;
			}
		}
	}

	return ret;
}

CMoveMath::BlockType CMoveMath::RangeIsBlockedHashedSt(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int tempNum)
{
	//ZoneScoped;
	BlockType ret = BLOCK_NONE;

	static spring::unordered_map<CSolidObject*, CMoveMath::BlockType> blockMap(10);
	static int lastTempNum = -1;

	if (lastTempNum != tempNum){
		blockMap.clear();
		lastTempNum = tempNum;
	}

	// footprints are point-symmetric around <xSquare, zSquare>
	for (int z = zmin; z <= zmax; z += FOOTPRINT_ZSTEP) {
		const int zOffset = z * mapDims.mapx;

		for (int x = xmin; x <= xmax; x += FOOTPRINT_XSTEP) {
			const CGroundBlockingObjectMap::BlockingMapCell& cell = groundBlockingObjectMap.GetCellUnsafeConst(zOffset + x);

			for (size_t i = 0, n = cell.size(); i < n; i++) {
				CSolidObject* collidee = cell[i];

				auto blockMapResult = blockMap.find(collidee);
				if (blockMapResult == blockMap.end()) {
					blockMapResult = blockMap.emplace(collidee, ObjectBlockType(collidee, collider)).first;
				}

				ret |= blockMapResult->second;

				if ((ret & BLOCK_STRUCTURE) == 0)
					continue;

				return ret;
			}
		}
	}

	return ret;
}

static std::array<spring::unordered_map<CSolidObject*, CMoveMath::BlockType>, ThreadPool::MAX_THREADS> blockMaps;
static std::array<int, ThreadPool::MAX_THREADS> lastTempNums;

// Called by GeneralMoveSystem::Init()
void CMoveMath::InitRangeIsBlockedHashes() {
	for (auto& blockMap : blockMaps) {
		blockMap.reserve(10);
	}
	for (auto& lastTempNum : lastTempNums) {
		lastTempNum = -1;
	}
}

CMoveMath::BlockType CMoveMath::RangeIsBlockedHashedMt(int xmin, int xmax, int zmin, int zmax, const MoveTypes::CheckCollisionQuery* collider, int tempNum, int thread)
{
	//ZoneScoped;
	BlockType ret = BLOCK_NONE;

	spring::unordered_map<CSolidObject*, CMoveMath::BlockType>& blockMap = blockMaps[thread];
	int& lastTempNum = lastTempNums[thread];

	if (lastTempNum != tempNum){
		blockMap.clear();
		lastTempNum = tempNum;
	}

	// footprints are point-symmetric around <xSquare, zSquare>
	for (int z = zmin; z <= zmax; z += FOOTPRINT_ZSTEP) {
		const int zOffset = z * mapDims.mapx;

		for (int x = xmin; x <= xmax; x += FOOTPRINT_XSTEP) {
			const CGroundBlockingObjectMap::BlockingMapCell& cell = groundBlockingObjectMap.GetCellUnsafeConst(zOffset + x);

			for (size_t i = 0, n = cell.size(); i < n; i++) {
				CSolidObject* collidee = cell[i];

				auto blockMapResult = blockMap.find(collidee);
				if (blockMapResult == blockMap.end()) {
					blockMapResult = blockMap.emplace(collidee, ObjectBlockType(collidee, collider)).first;
				}

				ret |= blockMapResult->second;

				if ((ret & BLOCK_STRUCTURE) == 0)
					continue;

				return ret;
			}
		}
	}

	return ret;
}

void CMoveMath::FloodFillRangeIsBlocked(const MoveDef& moveDef, const CSolidObject* collider, const SRectangle& areaToSample, std::vector<std::uint8_t>& results, int thread)
{
	//ZoneScoped;
	spring::unordered_map<CSolidObject*, CMoveMath::BlockType>& blockMap = blockMaps[thread];
	blockMap.clear();

	results.clear();
	results.reserve(areaToSample.GetArea());

	for (int z = areaToSample.z1; z < areaToSample.z2; ++z) {
		const int zOffset = z * mapDims.mapx;

		for (int x = areaToSample.x1; x < areaToSample.x2; ++x) {
			const CGroundBlockingObjectMap::BlockingMapCell& cell = groundBlockingObjectMap.GetCellUnsafeConst(zOffset + x);
			BlockType ret = BLOCK_NONE;

			for (size_t i = 0, n = cell.size(); i < n; i++) {
				CSolidObject* collidee = cell[i];

				auto blockMapResult = blockMap.find(collidee);
				if (blockMapResult == blockMap.end()) {
					blockMapResult = blockMap.emplace(collidee, ObjectBlockType(moveDef, collidee, collider)).first;
				}

				ret |= blockMapResult->second;

				if ((ret & BLOCK_STRUCTURE) != 0)
					break;
			}
			results.emplace_back(ret);
		}
	}
}

