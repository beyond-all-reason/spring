/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveDefHandler.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "MoveMath/MoveMath.h"
#include "Sim/Misc/YardmapStatusEffectsMap.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Units/Unit.h"
#include "System/creg/STL_Map.h"
#include "System/Exceptions.h"
#include "System/CRC.h"
#include "System/SpringMath.h"
#include "System/StringHash.h"
#include "System/StringUtil.h"

#include "System/Misc/TracyDefs.h"

CR_BIND(MoveDef, ())
CR_BIND(MoveDefHandler, )

CR_REG_METADATA(MoveDef, (
	CR_MEMBER(name),

	CR_MEMBER(speedModClass),
	CR_MEMBER(terrainClass),

	CR_MEMBER(pathType),

	CR_MEMBER(xsize),
	CR_MEMBER(xsizeh),
	CR_MEMBER(zsize),
	CR_MEMBER(zsizeh),

	CR_MEMBER(depth),
	CR_MEMBER(depthModParams),
	CR_MEMBER(maxSlope),
	CR_MEMBER(slopeMod),
	CR_MEMBER(crushStrength),
	CR_MEMBER(speedModMults),

	CR_MEMBER(heatMod),
	CR_MEMBER(flowMod),
	CR_MEMBER(heatProduced),

	CR_MEMBER(followGround),
	CR_MEMBER(isSubmarine),
	CR_MEMBER(isSubmersible),

	CR_MEMBER(avoidMobilesOnPath),
	CR_MEMBER(allowTerrainCollisions),
	CR_MEMBER(allowRawMovement),

	CR_MEMBER(heatMapping),
	CR_MEMBER(flowMapping)
))

CR_REG_METADATA(MoveDefHandler, (
	CR_MEMBER(moveDefs),
	CR_MEMBER(nameMap),
	CR_MEMBER(mdCounter),
	CR_MEMBER(mdChecksum)
))


MoveDefHandler moveDefHandler;

// FIXME: do something with these magic numbers
static constexpr float MAX_ALLOWED_WATER_DAMAGE_GMM = 1e3f;
static constexpr float MAX_ALLOWED_WATER_DAMAGE_HMM = 1e4f;

static float DegreesToMaxSlope(float degrees)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Prevent MSVC from inlining stuff that would break the
	// PE checksum compatibility between debug and release
	static constexpr float degToRad = math::DEG_TO_RAD;

	const float deg = std::clamp(degrees, 0.0f, 60.0f) * 1.5f;
	const float rad = deg * degToRad;

	return (1.0f - math::cos(rad));
}

static MoveDef::SpeedModClass ParseSpeedModClass(const std::string& moveDefName, const LuaTable& moveDefTable)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int speedModClass = moveDefTable.GetInt("speedModClass", -1);

	if (speedModClass != -1)
		return std::clamp(MoveDef::SpeedModClass(speedModClass), MoveDef::Tank, MoveDef::Ship);

	// name-based fallbacks
	if (moveDefName.find( "boat") != string::npos)
		return MoveDef::Ship;
	if (moveDefName.find( "ship") != string::npos)
		return MoveDef::Ship;
	if (moveDefName.find("hover") != string::npos)
		return MoveDef::Hover;
	if (moveDefName.find( "tank") != string::npos)
		return MoveDef::Tank;

	return MoveDef::KBot;
}



void MoveDefHandler::Init(LuaParser* defsParser)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const LuaTable& rootTable = defsParser->GetRoot().SubTable("MoveDefs");

	if (!rootTable.IsValid())
		throw content_error("[MoveDefHandler] error loading MoveDef entries");
	if (rootTable.GetLength() > moveDefs.size())
		throw content_error("[MoveDefHandler] too many MoveDef entries");

	CRC crc;

	for (const CMapInfo::TerrainType& terrType: mapInfo->terrainTypes) {
		crc << terrType.tankSpeed << terrType.kbotSpeed;
		crc << terrType.hoverSpeed << terrType.shipSpeed;
	}


	nameMap.clear();
	nameMap.reserve(rootTable.GetLength());

	for (unsigned int moveDefID = 0; /* no test */; moveDefID++) {
		const LuaTable& moveDefTable = rootTable.SubTable(moveDefID + 1);

		if (!moveDefTable.IsValid())
			break;

		moveDefs[mdCounter] = {moveDefTable};
		nameMap[hashString(moveDefs[mdCounter].name.c_str())] = (moveDefs[mdCounter].pathType = moveDefID);

		largestSize = std::max(largestSize, moveDefs[mdCounter].xsize);
		largestSize = std::max(largestSize, moveDefs[mdCounter].zsize);
		largestSizeH = std::max(largestSizeH, moveDefs[mdCounter].xsizeh);
		largestSizeH = std::max(largestSizeH, moveDefs[mdCounter].zsizeh);

		crc << moveDefs[mdCounter++].CalcCheckSum();
	}

	CMoveMath::noHoverWaterMove = (mapInfo->water.damage >= MAX_ALLOWED_WATER_DAMAGE_HMM);
	CMoveMath::waterDamageCost = (mapInfo->water.damage >= MAX_ALLOWED_WATER_DAMAGE_GMM)?
		0.0f: (1.0f / (1.0f + mapInfo->water.damage * 0.1f));

	crc << CMoveMath::waterDamageCost;
	crc << CMoveMath::noHoverWaterMove;

	mdChecksum = crc.GetDigest();
}


void MoveDefHandler::PostSimInit() {
	// Pathing system is initialized later and the movedefs may need to adjust their behaviour to align with
	// capabilities of the pathing system.
	std::for_each(moveDefs.begin(), moveDefs.end(), [](MoveDef& md){
		md.allowDirectionalPathing &= pathManager->AllowDirectionalPathing();
		md.preferShortestPath &= pathManager->AllowShortestPath();
	});
}


MoveDef* MoveDefHandler::GetMoveDefByName(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = nameMap.find(hashString(name.c_str()));

	if (it == nameMap.end())
		return nullptr;

	return &moveDefs[it->second];
}



MoveDef::MoveDef()
{
	RECOIL_DETAILED_TRACY_ZONE;
	depthModParams[DEPTHMOD_MIN_HEIGHT] = 0.0f;
	depthModParams[DEPTHMOD_MAX_HEIGHT] = std::numeric_limits<float>::max();
	depthModParams[DEPTHMOD_MAX_SCALE ] = std::numeric_limits<float>::max();
	depthModParams[DEPTHMOD_QUA_COEFF ] = 0.0f;
	depthModParams[DEPTHMOD_LIN_COEFF ] = 0.1f;
	depthModParams[DEPTHMOD_CON_COEFF ] = 1.0f;

	speedModMults[SPEEDMOD_MOBILE_BUSY_MULT] = 0.10f;
	speedModMults[SPEEDMOD_MOBILE_IDLE_MULT] = 0.35f;
	speedModMults[SPEEDMOD_MOBILE_MOVE_MULT] = 0.65f;
	speedModMults[SPEEDMOD_MOBILE_NUM_MULTS] = 0.0f;
}

MoveDef::MoveDef(const LuaTable& moveDefTable): MoveDef() {
	RECOIL_DETAILED_TRACY_ZONE;
	name          = StringToLower(moveDefTable.GetString("name", ""));
	crushStrength = moveDefTable.GetFloat("crushStrength", 10.0f);

	const LuaTable& depthModTable = moveDefTable.SubTable("depthModParams");
	const LuaTable& speedModMultsTable = moveDefTable.SubTable("speedModMults");

	const float minWaterDepth = moveDefTable.GetFloat("minWaterDepth", GetDefaultMinWaterDepth());
	const float maxWaterDepth = moveDefTable.GetFloat("maxWaterDepth", GetDefaultMaxWaterDepth());

	switch ((speedModClass = ParseSpeedModClass(name, moveDefTable))) {
		case MoveDef::Tank: // fall-through
		case MoveDef::KBot: {
			depthModParams[DEPTHMOD_MIN_HEIGHT] = std::max(0.00f, depthModTable.GetFloat("minHeight",                                        0.0f ));
			depthModParams[DEPTHMOD_MAX_HEIGHT] =         (       depthModTable.GetFloat("maxHeight",           std::numeric_limits<float>::max() ));
			depthModParams[DEPTHMOD_MAX_SCALE ] = std::max(0.01f, depthModTable.GetFloat("maxScale",            std::numeric_limits<float>::max() ));
			depthModParams[DEPTHMOD_QUA_COEFF ] = std::max(0.00f, depthModTable.GetFloat("quadraticCoeff",                                   0.0f ));
			depthModParams[DEPTHMOD_LIN_COEFF ] = std::max(0.00f, depthModTable.GetFloat("linearCoeff",    moveDefTable.GetFloat("depthMod", 0.1f)));
			depthModParams[DEPTHMOD_CON_COEFF ] = std::max(0.00f, depthModTable.GetFloat("constantCoeff",                                    1.0f ));

			// ensure [depthModMinHeight, depthModMaxHeight] is a valid range
			depthModParams[DEPTHMOD_MAX_HEIGHT] = std::max(depthModParams[DEPTHMOD_MIN_HEIGHT], depthModParams[DEPTHMOD_MAX_HEIGHT]);

			depth    = maxWaterDepth;
			maxSlope = DegreesToMaxSlope(moveDefTable.GetFloat("maxSlope", 60.0f));
		} break;

		case MoveDef::Hover: {
			depth    = maxWaterDepth;
			maxSlope = DegreesToMaxSlope(moveDefTable.GetFloat("maxSlope", 15.0f));
		} break;

		case MoveDef::Ship: {
			depth    = minWaterDepth;
			// maxSlope = "n/a";

			isSubmarine = moveDefTable.GetBool("subMarine", false);
		} break;
	}

	speedModMults[SPEEDMOD_MOBILE_BUSY_MULT] = std::max(0.01f, speedModMultsTable.GetFloat("mobileBusyMult", 1.0f /*0.10f*/));
	speedModMults[SPEEDMOD_MOBILE_IDLE_MULT] = std::max(0.01f, speedModMultsTable.GetFloat("mobileIdleMult", 1.0f /*0.35f*/));
	speedModMults[SPEEDMOD_MOBILE_MOVE_MULT] = std::max(0.01f, speedModMultsTable.GetFloat("mobileMoveMult", 1.0f /*0.65f*/));

	avoidMobilesOnPath = moveDefTable.GetBool("avoidMobilesOnPath", true);
	allowTerrainCollisions = moveDefTable.GetBool("allowTerrainCollisions", true);
	allowRawMovement = moveDefTable.GetBool("allowRawMovement", false);

	heatMapping = moveDefTable.GetBool("heatMapping", false);
	flowMapping = moveDefTable.GetBool("flowMapping", true);

	heatMod = moveDefTable.GetFloat("heatMod", (1.0f / (GAME_SPEED * 2)) * 0.25f);
	flowMod = moveDefTable.GetFloat("flowMod", 1.0f);

	// by default heat decays to zero after N=2 seconds
	//
	// the cost contribution to a square from heat must
	// be on the same order as its normal movement cost
	// PER FRAME, i.e. such that heatMod * heatProduced
	// ~= O(1 / (GAME_SPEED * N)) because unit behavior
	// in groups quickly becomes FUBAR if heatMod >>> 1
	//
	heatProduced = moveDefTable.GetInt("heatProduced", GAME_SPEED * 2);

	//  <maxSlope> ranges from 0.0 to 60 * 1.5 degrees, ie. from 0.0 to
	//  0.5 * PI radians, ie. from 1.0 - cos(0.0) to 1.0 - cos(0.5 * PI)
	//  = [0, 1] --> DEFAULT <slopeMod> values range from (4 / 0.001) to
	//  (4 / 1.001) = [4000.0, 3.996]
	//
	// speedMod values for a terrain-square slope in [0, 1] are given by
	// (1.0 / (1.0 + slope * slopeMod)) and therefore have a MAXIMUM at
	// <slope=0, slopeMod=...> and a MINIMUM at <slope=1, slopeMod=4000>
	// (of 1.0 / (1.0 + 0.0 * ...) = 1.0 and 1.0 / (1.0 + 1.0 * 4000.0)
	// = 0.00025 respectively)
	//
	slopeMod = moveDefTable.GetFloat("slopeMod", 4.0f / (maxSlope + 0.001f));

	// ground units hug the ocean floor when in water,
	// ships stay at a "fixed" level (their waterline)
	followGround = (speedModClass == MoveDef::Tank || speedModClass == MoveDef::KBot);

	// TODO:
	//   remove terrainClass, not used anywhere
	//   and only AI's MIGHT have benefit from it
	//
	// tank or bot that cannot get its threads / feet
	// wet, or hovercraft (which doesn't touch ground
	// or water)
	if ((followGround && maxWaterDepth <= 0.0f) || speedModClass == MoveDef::Hover)
		terrainClass = MoveDef::Land;
	// ship (or sub) that cannot crawl onto shore, OR tank
	// or bot restricted to snorkling (strange but possible)
	if ((speedModClass == MoveDef::Ship && minWaterDepth > 0.0f) || (followGround && minWaterDepth > 0.0f))
		terrainClass = MoveDef::Water;
	// tank or kbot that CAN go skinny-dipping (amph.),
	// or ship that CAN sprout legs when at the beach
	if ((followGround && maxWaterDepth > 0.0f) || (speedModClass == MoveDef::Ship && minWaterDepth < 0.0f))
		terrainClass = MoveDef::Mixed;

	const int xsizeDef = std::max(1, moveDefTable.GetInt("footprintX",        1));
	const int zsizeDef = std::max(1, moveDefTable.GetInt("footprintZ", xsizeDef));

	// make all mobile footprints point-symmetric in heightmap space
	// (meaning that only non-even dimensions are possible and each
	// footprint always has a unique center square)
	xsize = xsizeDef * SPRING_FOOTPRINT_SCALE;
	zsize = zsizeDef * SPRING_FOOTPRINT_SCALE;
	xsize -= ((xsize & 1)? 0: 1);
	zsize -= ((zsize & 1)? 0: 1);
	// precalculated data for MoveMath
	xsizeh = xsize >> 1;
	zsizeh = zsize >> 1;
	assert((xsize & 1) == 1);
	assert((zsize & 1) == 1);

	int defaultHeight = xsize * SQUARE_SIZE;
	if (speedModClass != MoveDef::KBot) {
		defaultHeight >>= 1;
	}

	if (this->FloatOnWater()) {
		int defaultWaterline = 0;
		if (isSubmarine) {
			defaultWaterline = xsize * SQUARE_SIZE; 
		} else if (speedModClass == MoveDef::Ship) {
			defaultWaterline = 1;
		}
		waterline = std::abs(moveDefTable.GetInt("waterline", defaultWaterline));
		overrideUnitWaterline = moveDefTable.GetBool("overrideUnitWaterline", overrideUnitWaterline);
	} else {
		waterline = std::numeric_limits<int>::max();
	}

	height = std::max(1, moveDefTable.GetInt("height", defaultHeight));
	isSubmersible = (isSubmarine || (followGround && depth > height));
	allowDirectionalPathing = moveDefTable.GetBool("allowDirectionalPathing", allowDirectionalPathing);
	preferShortestPath = moveDefTable.GetBool("preferShortestPath", preferShortestPath);
}

bool MoveDef::DoRawSearch(
	const CSolidObject* collider,
	const MoveDef* md,
	const float3 startPos,
	const float3 endPos,
	float goalRadius,
	bool testTerrain,
	bool testObjects,
	bool centerOnly,
	float* minSpeedModPtr,
	int* maxBlockBitPtr,
	int2* nearestSquare,
	int thread
) const {
	ZoneScoped;
	assert(testTerrain || testObjects);

	// if the endPos sits on a cross section, shift the pos slightly to pick the most appropriate
	// block. If perfectly aligned with starPos then don't shift because the blocks will align.
	const float upDir    = (startPos.z == endPos.z) ? 0 : 1 - float(startPos.z < endPos.z) * 2.f;
	const float rightDir = (startPos.x == endPos.x) ? 0 : 1 - float(startPos.x < endPos.x) * 2.f;
	const float goalRadiusSq = Square(goalRadius);

	const int2 startBlock
		( std::clamp(int(startPos.x / SQUARE_SIZE), 0, mapDims.mapxm1)
		, std::clamp(int(startPos.z / SQUARE_SIZE), 0, mapDims.mapym1)
		);
	const int2 endBlock
		( std::clamp(int((endPos.x + rightDir) / SQUARE_SIZE), 0, mapDims.mapxm1)
		, std::clamp(int((endPos.z + upDir) / SQUARE_SIZE), 0, mapDims.mapym1)
		);
	const int2 diffBlk = {std::abs(endBlock.x - startBlock.x), std::abs(endBlock.y - startBlock.y)};
	const float speedModThreshold = modInfo.pfRawMoveSpeedThreshold;

	const/*expr*/ auto StepFunc = [](const int2& dir, const int2& dif, int2& pos, int2& err) {
		pos.x += (dir.x * (err.y >= 0));
		pos.y += (dir.y * (err.y <= 0));
		err.x -= (dif.y * (err.y >= 0));
		err.x += (dif.x * (err.y <= 0));
	};

	if (nearestSquare != nullptr)
		*nearestSquare = endBlock;

	auto walkPath = [startBlock, endBlock, diffBlk, &StepFunc, goalRadiusSq, nearestSquare](auto& f) -> bool {
		bool result = false;

		const int2 fwdStepDir = int2{(endBlock.x > startBlock.x), (endBlock.y > startBlock.y)} * 2 - int2{1, 1};
		const int2 revStepDir = int2{(startBlock.x > endBlock.x), (startBlock.y > endBlock.y)} * 2 - int2{1, 1};

		int2 blkStepCtr = {diffBlk.x + diffBlk.y, diffBlk.x + diffBlk.y};
		int2 fwdStepErr = {diffBlk.x - diffBlk.y, diffBlk.x - diffBlk.y};
		int2 revStepErr = fwdStepErr;
		int2 fwdTestBlk = startBlock;
		int2 revTestBlk = endBlock;

		// int2 prevFwdTestBlk = {-1, -1};
		// int2 prevRevTestBlk = {-1, -1};

		int2 walkedNearestSqr(-1,-1);

		for (blkStepCtr += int2{1, 1}; (blkStepCtr.x > 0 && blkStepCtr.y > 0); blkStepCtr -= int2{1, 1}) {
			// Check forward search.
			result = f(fwdTestBlk.x, fwdTestBlk.y);
			if (result) {
				// Forward search has got close enough, drop out early.
				if (float((endBlock).DistanceSq(fwdTestBlk)*Square(SQUARE_SIZE)) < goalRadiusSq) {
					if (nearestSquare != nullptr)
						walkedNearestSqr = fwdTestBlk;
					break;
				}

				// Now check reverse search.
				result = f(revTestBlk.x, revTestBlk.y);

				// Keep record of the current reverse test block to correctly report it as the nearest node.
				int2 curRevTestBlk = revTestBlk;

				// Need to step the reverse forward early to assess whether the next test will be within the goal
				// radius if the current block test fails. As long as there's at least one more reverse block inside
				// the goal radius, then a failure in the reverse search doesn't mean the walk is a failure.
				StepFunc(revStepDir, diffBlk * 2, revTestBlk, revStepErr);
				if (result) {
					if (walkedNearestSqr.x == -1)
						walkedNearestSqr = curRevTestBlk;
				} else {
					walkedNearestSqr = int2(-1, -1);

					// Allow reverse search to continue if it is still within the goal radius.
					result = ( float((endBlock).DistanceSq(revTestBlk)*Square(SQUARE_SIZE)) < goalRadiusSq );
				}
			}
			if (!result) { break; }

			// NOTE: for odd-length paths, center square is tested twice
			if ((std::abs(fwdTestBlk.x - revTestBlk.x) <= 1) && (std::abs(fwdTestBlk.y - revTestBlk.y) <= 1)) {
				// This is to catch the case where the reverse never finds an open square but was always in the goal
				// radius, and the forward search never reached the goal radius. I.e. they stopped on the squares
				// that bordered either side of the goal radius.
				result = (walkedNearestSqr.x != -1);
				break;
			}

			// prevFwdTestBlk = fwdTestBlk;
			// prevRevTestBlk = revTestBlk;

			StepFunc(fwdStepDir, diffBlk * 2, fwdTestBlk, fwdStepErr);
			// StepFunc(revStepDir, diffBlk * 2, revTestBlk, revStepErr); - done earlier now.

			// skip if exactly crossing a vertex (in either direction)
			blkStepCtr.x -= (fwdStepErr.y == 0);
			blkStepCtr.y -= (revStepErr.y == 0);
			fwdStepErr.y  = fwdStepErr.x;
			revStepErr.y  = revStepErr.x;
		}

		if (result && nearestSquare != nullptr)
			*nearestSquare = walkedNearestSqr;

		return result;
	};

	auto walkPathFwdOnly = [startBlock, endBlock, diffBlk, &StepFunc, goalRadiusSq, nearestSquare](auto& f) -> bool {
		bool result = false;

		const int2 fwdStepDir = int2{(endBlock.x > startBlock.x), (endBlock.y > startBlock.y)} * 2 - int2{1, 1};

		int blkStepCtr = diffBlk.x + diffBlk.y;
		int2 fwdStepErr = {diffBlk.x - diffBlk.y, diffBlk.x - diffBlk.y};
		int2 fwdTestBlk = startBlock;

		for (blkStepCtr += 1; blkStepCtr > 0; blkStepCtr -= 1) {
			// Check forward search.
			result = f(fwdTestBlk.x, fwdTestBlk.y);
			if (!result) {
				break;
			} else {
				// Forward search has got close enough, drop out early.
				float dist = float((endBlock).DistanceSq(fwdTestBlk)*Square(SQUARE_SIZE));
				if (dist < goalRadiusSq) {
					if (nearestSquare != nullptr) {
						// Move the nearest square further out if the unit would hit an exit only zone sooner
						float origDist = float((endBlock).DistanceSq(*nearestSquare)*Square(SQUARE_SIZE));
						if (dist > origDist)
							*nearestSquare = fwdTestBlk;
					}
					break;
				}
			}

			StepFunc(fwdStepDir, diffBlk * 2, fwdTestBlk, fwdStepErr);

			// skip if exactly crossing a vertex (in either direction)
			blkStepCtr -= (fwdStepErr.y == 0);
			fwdStepErr.y  = fwdStepErr.x;
		}

		return result;
	};

	float minSpeedMod = std::numeric_limits<float>::max();
	int   maxBlockBit = CMoveMath::BLOCK_NONE;

	std::function<float(int x, int z)> getPosSpeedMod;
	if (md->allowDirectionalPathing) {
		float3 testMoveDir = startPos - endPos;
		getPosSpeedMod = [this, testMoveDir](int x, int z){ return CMoveMath::GetPosSpeedMod(*this, x, z, testMoveDir); };
	} else {
		getPosSpeedMod = [&](int x, int z){ return CMoveMath::GetPosSpeedMod(*this, x, z); };
	}

	auto terrainTest = [this, &minSpeedMod, speedModThreshold, &getPosSpeedMod](int x, int z) -> bool {
		if (x >= mapDims.mapx || x < 0 || z >= mapDims.mapy || z < 0) { return true; }

		const float speedMod = getPosSpeedMod(x, z);
		minSpeedMod = std::min(minSpeedMod, speedMod);

		return (speedMod > speedModThreshold);
	};

	// GetPosSpeedMod only checks *one* square of terrain
	// (heightmap/slopemap/typemap), not the blocking-map
	int tempNum = gs->GetMtTempNum(thread);

	MoveTypes::CheckCollisionQuery virtualObject = (collider != nullptr)
			? MoveTypes::CheckCollisionQuery(collider)
			: MoveTypes::CheckCollisionQuery(md);
	MoveDefs::CollisionQueryStateTrack queryState;
	md->UpdateCheckCollisionQuery(virtualObject, queryState, startBlock);
	const bool isSubmersible = md->IsComplexSubmersible();

	if (!isSubmersible)
		virtualObject.DisableHeightChecks();

	auto objectsTest = [this, &maxBlockBit, thread, centerOnly, &tempNum, md, isSubmersible, &virtualObject, &queryState]
			(int x, int z) -> bool {
		const int xmin = std::max(x - xsizeh * (1 - centerOnly), 0);
		const int zmin = std::max(z - zsizeh * (1 - centerOnly), 0);
		const int xmax = std::min(x + xsizeh * (1 - centerOnly), mapDims.mapxm1);
		const int zmax = std::min(z + zsizeh * (1 - centerOnly), mapDims.mapym1);

		// Height affects whether units in water collide or not, so the new y positions need
		// to be considered or else we will get incorrect results.
		if (isSubmersible){
			UpdateCheckCollisionQuery(virtualObject, queryState, {x, z});
			if (queryState.refreshCollisionCache)
				tempNum = gs->GetMtTempNum(thread);
		}

		const CMoveMath::BlockType blockBits = CMoveMath::RangeIsBlockedHashedMt(xmin, xmax, zmin, zmax, &virtualObject, tempNum, thread);
		maxBlockBit = blockBits;
		return ((blockBits & CMoveMath::BLOCK_STRUCTURE) == 0);
	};
	
	// Keep track of the exit only status on the path. A unit starting in exit only is allowed to leave, but it is not
	// permitted to enter an exit only square if the path has left an exit-only zone or never started in exit-only.
	bool curExitOnlyState = true;
	auto exitOnlytest = [this, &curExitOnlyState](int x, int z) -> bool {
		bool canProceed = true;
		bool nextExitOnlyState = IsInExitOnly(x, z);
		if (!curExitOnlyState)
			canProceed = !nextExitOnlyState;

		curExitOnlyState = nextExitOnlyState;
		return (canProceed);
	};

	auto test = [this, testTerrain, testObjects, &terrainTest, &objectsTest](int x, int z) {
		return (!testTerrain || terrainTest(x, z)) && (!testObjects || objectsTest(x, z));
	};
	const bool retTestMove = walkPath(test) && walkPathFwdOnly(exitOnlytest);

	// don't use std::min or |= because the ptr values might be garbage
	if (minSpeedModPtr != nullptr) *minSpeedModPtr = minSpeedMod;
	if (maxBlockBitPtr != nullptr) *maxBlockBitPtr = maxBlockBit;

	assert(nearestSquare == nullptr || nearestSquare->x != -1);

	return retTestMove;
}

void MoveDef::UpdateCheckCollisionQuery
	( MoveTypes::CheckCollisionQuery& collider
	, MoveDefs::CollisionQueryStateTrack& state
	, const int2 pos
) const {
	RECOIL_DETAILED_TRACY_ZONE;
	state.refreshCollisionCache = false;

	const MoveDef* md = collider.moveDef;
	const float mapHeight = readMap->GetMaxHeightMapSynced()[pos.y * mapDims.mapx + pos.x];
	collider.pos.y = std::max(mapHeight, -collider.moveDef->waterline);

	const bool waterCollisions = ((mapHeight + md->height) < 0.f || mapHeight < -md->waterline);
	if (waterCollisions || waterCollisions != state.lastWaterCollisions) {
		if (mapHeight != state.lastPosY) {
			state.refreshCollisionCache = true;
			state.lastPosY = mapHeight;
		}
		state.lastWaterCollisions = waterCollisions;
	}

	bool inWater = (collider.pos.y < 0.f);
	if (state.lastInWater != inWater) {
		if (inWater)
			collider.SetPhysicalStateBit(CSolidObject::PhysicalState::PSTATE_BIT_INWATER);
		else
			collider.ClearPhysicalStateBit(CSolidObject::PhysicalState::PSTATE_BIT_INWATER);

		state.lastInWater = inWater;
	}
}

bool MoveDef::TestMoveSquareRange(
	const MoveTypes::CheckCollisionQuery& collider,
	const float3 rangeMins,
	const float3 rangeMaxs,
	const float3 testMoveDir,
	bool testTerrain,
	bool testObjects,
	bool centerOnly,
	float* minSpeedModPtr,
	int* maxBlockBitPtr,
	int thread
) const {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(testTerrain || testObjects);

	const int xmid = int(rangeMins.x / SQUARE_SIZE);
	const int zmid = int(rangeMins.z / SQUARE_SIZE);

	const int xmin = xmid - xsizeh * (1 - centerOnly);
	const int zmin = zmid - zsizeh * (1 - centerOnly);
	const int xmax = xmid + xsizeh * (1 - centerOnly);
	const int zmax = zmid + zsizeh * (1 - centerOnly);

	float minSpeedMod = std::numeric_limits<float>::max();
	int   maxBlockBit = CMoveMath::BLOCK_NONE;

	bool retTestMove = true;

	if (testTerrain) {
		std::function<float(int x, int z)> getPosSpeedMod;
		if (collider.moveDef->allowDirectionalPathing) {
			getPosSpeedMod = [&](int x, int z){ return CMoveMath::GetPosSpeedMod(*this, x, z, testMoveDir); };
		} else {
			getPosSpeedMod = [&](int x, int z){ return CMoveMath::GetPosSpeedMod(*this, x, z); };
		}

		for (int z = zmin; retTestMove && z <= zmax; ++z) {
			for (int x = xmin; retTestMove && x <= xmax; ++x) {
				const float speedMod = getPosSpeedMod(x, z);

				minSpeedMod = std::min(minSpeedMod, speedMod);
				retTestMove = (speedMod > 0.0f);
			}
		}
	}

	// GetPosSpeedMod only checks *one* square of terrain
	// (heightmap/slopemap/typemap), not the blocking-map
	if (testObjects && retTestMove) {
		const CMoveMath::BlockType blockBits = CMoveMath::RangeIsBlocked(xmin, xmax, zmin, zmax, &collider, thread);

		maxBlockBit = blockBits;
		retTestMove = ((blockBits & CMoveMath::BLOCK_STRUCTURE) == 0);
	}

	// don't use std::min or |= because the ptr values might be garbage
	if (minSpeedModPtr != nullptr) *minSpeedModPtr = minSpeedMod;
	if (maxBlockBitPtr != nullptr) *maxBlockBitPtr = maxBlockBit;
	return retTestMove;
}

bool MoveDef::TestMovePositionForObjects(
	const MoveTypes::CheckCollisionQuery* collider,
	const float3 testMovePos,
	int magicNum,
	int thread
) const {
	RECOIL_DETAILED_TRACY_ZONE;
	const int xmid = int(testMovePos.x / SQUARE_SIZE);
	const int zmid = int(testMovePos.z / SQUARE_SIZE);

	const int xmin = xmid - xsizeh;
	const int zmin = zmid - zsizeh;
	const int xmax = xmid + xsizeh;
	const int zmax = zmid + zsizeh;

	const CMoveMath::BlockType blockBits = CMoveMath::RangeIsBlockedTempNum(xmin, xmax, zmin, zmax, collider, magicNum, thread);
	if (blockBits & CMoveMath::BLOCK_STRUCTURE)
		return false;

	if (!collider->inExitOnlyZone) {
		auto createHelper = [&](const MoveTypes::CheckCollisionQuery* collider) {
			if (collider->pos.y != MoveTypes::CheckCollisionQuery::POS_Y_UNAVAILABLE)
				return ObjectCollisionMapHelper(*this, collider->pos.y);
			return ObjectCollisionMapHelper(*this);
		};
		const ObjectCollisionMapHelper object = createHelper(collider);
		return !CMoveMath::RangeHasExitOnly(xmin, xmax, zmin, zmax, object);
	}

	return true;
	// return ((blockBits & CMoveMath::BLOCK_STRUCTURE) == 0);
}

bool MoveDef::IsInExitOnly(const float3 testMovePos) const {
	const int xmid = int(testMovePos.x / SQUARE_SIZE);
	const int zmid = int(testMovePos.z / SQUARE_SIZE);

	return IsInExitOnly(xmid, zmid);
}

bool MoveDef::IsInExitOnly(int xmid, int zmid) const {
	const int xmin = std::max(xmid - xsizeh, 0);
	const int zmin = std::max(zmid - zsizeh, 0);
	const int xmax = std::min(xmid + xsizeh, mapDims.mapxm1);
	const int zmax = std::min(zmid + zsizeh, mapDims.mapym1);

	const ObjectCollisionMapHelper object;
	return CMoveMath::RangeHasExitOnly(xmin, xmax, zmin, zmax, object);
}


float MoveDef::CalcFootPrintMinExteriorRadius(float scale) const { return ((math::sqrt((xsize * xsize + zsize * zsize)) * 0.5f * SQUARE_SIZE) * scale); }
float MoveDef::CalcFootPrintMaxInteriorRadius(float scale) const { return ((std::max(xsize, zsize) * 0.5f * SQUARE_SIZE) * scale); }
float MoveDef::CalcFootPrintAxisStretchFactor() const
{
	return (std::abs(xsize - zsize) * 1.0f / (xsize + zsize));
}


float MoveDef::GetDepthMod(float height) const {
	RECOIL_DETAILED_TRACY_ZONE;
	// [DEPTHMOD_{MIN, MAX}_HEIGHT] are always >= 0,
	// so we return early for positive height values
	// only negative heights ("depths") are allowed
	if (height > -depthModParams[DEPTHMOD_MIN_HEIGHT])
		return 1.0f;
	if (height < -depthModParams[DEPTHMOD_MAX_HEIGHT])
		return 0.0f;

	const float a = depthModParams[DEPTHMOD_QUA_COEFF];
	const float b = depthModParams[DEPTHMOD_LIN_COEFF];
	const float c = depthModParams[DEPTHMOD_CON_COEFF];

	const float minScale = 0.01f;
	const float maxScale = depthModParams[DEPTHMOD_MAX_SCALE];

	const float depth = -height;
	const float scale = std::clamp((a * depth * depth + b * depth + c), minScale, maxScale);

	// NOTE:
	//   <maxScale> is guaranteed to be >= 0.01, so the
	//   depth-mod range is [1.0 / 0.01, 1.0 / +infinity]
	//
	//   if minScale <= scale <       1.0, speedup
	//   if      1.0 <  scale <= maxScale, slowdown
	return (1.0f / scale);
}

unsigned int MoveDef::CalcCheckSum() const {
	RECOIL_DETAILED_TRACY_ZONE;
	unsigned int sum = 0;

	const unsigned char* minByte = reinterpret_cast<const unsigned char*>(&speedModClass);
	const unsigned char* maxByte = reinterpret_cast<const unsigned char*>(&flowMapping) + sizeof(flowMapping);

	assert(minByte < maxByte);

	// NOTE:
	//   safe so long as MoveDef has no virtuals and we
	//   make sure we do not checksum any padding bytes
	for (const unsigned char* byte = minByte; byte != maxByte; byte++) {
		sum ^= ((((byte + 1) - minByte) << 8) * (*byte));
	}

	return sum;
}

