/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathEstimator.h"
// #include "PathFinder.h"
#include "PathFinderDef.h"
#include "PathLog.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"

#include "System/Misc/TracyDefs.h"

// #include "Game/SelectedUnitsHandler.h"
// #include "PathGlobal.h"
// #include "System/Threading/ThreadPool.h"

//#define ENABLE_NETLOG_CHECKSUM 1

namespace HAPFS {

// PCMemPool pcMemPool;
// PEMemPool peMemPool;


void CPathEstimator::Init(IPathFinder* pf, unsigned int BLOCK_SIZE, PathingState* ps)
{
	RECOIL_DETAILED_TRACY_ZONE;
	IPathFinder::Init(BLOCK_SIZE);

	{
		BLOCKS_TO_UPDATE = SQUARES_TO_UPDATE / (BLOCK_SIZE * BLOCK_SIZE) + 1;

		nextOffsetMessageIdx = 0;
		//nextCostMessageIdx = 0;

		pathChecksum = 0;
		fileHashCode = CalcHash(__func__);

		offsetBlockNum = {nbrOfBlocks.x * nbrOfBlocks.y};
		costBlockNum = {nbrOfBlocks.x * nbrOfBlocks.y};

		parentPathFinder = pf;
		//nextPathEstimator = nullptr;

		pathingState = ps;
		assert(pathingState != nullptr);

		psBlockStates = &pathingState->blockStates;
		assert(psBlockStates != nullptr);
	}

	CPathEstimator*  childPE = this;
	CPathEstimator* parentPE = dynamic_cast<CPathEstimator*>(pf);

	//if (parentPE != nullptr)
	//	parentPE->nextPathEstimator = childPE;

	// load precalculated data if it exists
	InitEstimator();
}


void CPathEstimator::Kill()
{
}


void CPathEstimator::InitEstimator()
{
}


const CPathCache::CacheItem& CPathEstimator::GetCache(const int2 strtBlock, const int2 goalBlock, float goalRadius, int pathType, const bool synced) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	tempCacheItem = pathingState->GetCache(strtBlock, goalBlock, goalRadius, pathType, synced);
	return tempCacheItem;
}


void CPathEstimator::AddCache(const IPath::Path* path, const IPath::SearchResult result, const int2 strtBlock, const int2 goalBlock, float goalRadius, int pathType, const bool synced)
{
	RECOIL_DETAILED_TRACY_ZONE;
	pathingState->AddCache(path, result, strtBlock, goalBlock, goalRadius, pathType, synced);
}

bool CPathEstimator::SetStartBlock(
	const MoveDef& moveDef,
	const CPathFinderDef& peDef,
	const CSolidObject* owner,
	float3 startPos
)
{
	RECOIL_DETAILED_TRACY_ZONE;
	/* Set this to determine how many of the nearest blocks to attempt to path to.
	 *
	 * The first nerest block is the one that the startPos is in, but the reference point may not
	 * be reachable. If not then the eight blocks around the start block should be checked in
	 * order of proximitimity to the startPos.
	 * 
	 * [ ][ ][ ][ ][ ]
	 * [ ][x][x][x][ ]
	 * [ ][x][x][x][ ]
	 * [ ][x][x][x][ ]
	 * [ ][ ][ ][ ][ ]
	 * 
	 * If maxBlocksToCheck is changed, it will affect the number of nearest blocks checked before
	 * giving up. Using more than 9 won't work because searches are constrained (for performance
	 * reasons) to just the starting and ending block areas.
	 * 
	 * Fewer can be used, but makes it more likely for a unit to get stuck.
	 */
	constexpr size_t maxBlocksToCheck = PATH_DIRECTIONS + 1;
	size_t blocksToCheck = 0;

	const float3 centreOffset(BLOCK_PIXEL_SIZE*.5f, 0, BLOCK_PIXEL_SIZE*.5f);

	int2 nearestBlock;
	nearestBlock.x = startPos.x / BLOCK_PIXEL_SIZE;
	nearestBlock.y = startPos.z / BLOCK_PIXEL_SIZE;

	// support peDef.skipSubSearches - just take nearest point
	if (peDef.skipSubSearches || !peDef.useVerifiedStartBlock) {
		mStartBlock = nearestBlock;
		mStartBlockIdx = BlockPosToIdx(nearestBlock);
		mGoalBlockIdx = mStartBlockIdx;
		return true;
	}

	// {bool printMoveInfo = (owner != nullptr) && (selectedUnitsHandler.selectedUnits.size() == 1)
    //     && (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
    // if (printMoveInfo) {
    //     LOG("%s: nearest block (%d, %d) start(%f, %f) owner(%f, %f)", __func__, nearestBlock.x, nearestBlock.y
	// 		, startPos.x, startPos.z, owner->pos.x, owner->pos.z);
    // }}

	auto buildBlock = [this, startPos, owner, centreOffset](int2 blockPos) {
			float3 realBlockPos = float3(blockPos.x, 0.f, blockPos.y) * BLOCK_PIXEL_SIZE + centreOffset;
			float distSq = startPos.SqDistance2D(realBlockPos);

			// {bool printMoveInfo = (owner != nullptr) && (selectedUnitsHandler.selectedUnits.size() == 1)
			// 	&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
			// if (printMoveInfo) {
			// 	LOG("%s: adding block (%d, %d) [%d]: %f realblockpos(%f, %f)", __func__, blockPos.x, blockPos.y
			// 		, BlockPosToIdx(blockPos), distSq, realBlockPos.x, realBlockPos.z);
			// }}

			return std::make_tuple(BlockPosToIdx(blockPos), distSq);
		};
	std::array< std::tuple<int, float>, maxBlocksToCheck > blockIdsByDist;
	blockIdsByDist.fill( std::make_tuple(std::numeric_limits<int>::max(), std::numeric_limits<float>::infinity()) );

	blockIdsByDist[blocksToCheck++] = buildBlock(nearestBlock);

	for (std::uint32_t i = 0; i < PATH_DIRECTIONS; ++i){
		int2 blockPos(nearestBlock + PE_DIRECTION_VECTORS[i]);
		bool inBounds = (blockPos.x >= 0) & (blockPos.x < nbrOfBlocks.x)
					  & (blockPos.y >= 0) & (blockPos.y < nbrOfBlocks.y);
		if (inBounds)
			blockIdsByDist[blocksToCheck++] = buildBlock(blockPos);
	}

	auto sortBlocksByDist = [](const std::tuple<int, float>& lhv, const std::tuple<int, float>& rhv)
			{ return std::get<1>(lhv) < std::get<1>(rhv); };
	std::stable_sort(blockIdsByDist.begin(), blockIdsByDist.end(), sortBlocksByDist);

	// std::for_each(blockIdsByDist.begin(), blockIdsByDist.end(), [this, owner](const std::tuple<int, float>& blockByDist) {
	// 			{bool printMoveInfo = (owner != nullptr) && (selectedUnitsHandler.selectedUnits.size() == 1)
	// 			&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
	// 		if (printMoveInfo) {
	// 			LOG("%s: block added (%d) %f", __func__, std::get<0>(blockByDist), std::get<1>(blockByDist));
	// 		}}
	// });

	auto unitCanReachBlock = [this, &moveDef = std::as_const(moveDef), &peDef = std::as_const(peDef), owner]
		(const std::tuple<int, float>& blockByDist)
			{ return (std::get<1>(blockByDist) < std::numeric_limits<float>::infinity())
					&& TestBlockReachability(moveDef, peDef, owner, std::get<0>(blockByDist)); };

	auto result = std::find_if(blockIdsByDist.begin(), blockIdsByDist.end(), unitCanReachBlock);
	if (result != std::end(blockIdsByDist)){
		mStartBlockIdx = std::get<0>(*result);
		mGoalBlockIdx = mStartBlockIdx;
		mStartBlock = BlockIdxToPos(mStartBlockIdx);
		return true;
	}

	return false;
}



float CPathEstimator::GetHeuristic(const MoveDef& moveDef, const CPathFinderDef& pfDef, const int2& square) const {
	RECOIL_DETAILED_TRACY_ZONE;
	return pfDef.Heuristic(square.x, square.y, BLOCK_SIZE) * pathingState->GetMaxSpeedMod(moveDef.pathType);
}



IPath::SearchResult CPathEstimator::DoBlockSearch(
	const CSolidObject* owner,
	const MoveDef& moveDef,
	const int2 s,
	const int2 g
) {
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 sw = float3(s.x * SQUARE_SIZE, 0, s.y * SQUARE_SIZE);
	const float3 gw = float3(g.x * SQUARE_SIZE, 0, g.y * SQUARE_SIZE);

	return (DoBlockSearch(owner, moveDef, sw, gw));
}


IPath::SearchResult CPathEstimator::DoBlockSearch(
	const CSolidObject* owner,
	const MoveDef& moveDef,
	const float3 sw,
	const float3 gw
) {
	RECOIL_DETAILED_TRACY_ZONE;
	// always use max-res (in addition to raw) search for this
	IPathFinder* pf = (BLOCK_SIZE == 32)? parentPathFinder->GetParent(): parentPathFinder;
	CRectangularSearchConstraint pfDef = CRectangularSearchConstraint(sw, gw, 8.0f, BLOCK_SIZE); // sets goalSquare{X,Z}
	IPath::Path path;

	pfDef.testMobile     = false;
	pfDef.needPath       = false;
	pfDef.exactPath      = true;
	pfDef.allowRawPath   = true;
	pfDef.dirIndependent = true;

	// search within the rectangle defined by sw and gw, corners snapped to the PE grid
	return (pf->GetPath(moveDef, pfDef, owner, sw, path, MAX_SEARCHED_NODES_PF >> 3));
}


/**
 * Performs the actual search.
 */
IPath::SearchResult CPathEstimator::DoSearch(const MoveDef& moveDef, const CPathFinderDef& peDef, const CSolidObject* owner)
{
	RECOIL_DETAILED_TRACY_ZONE;
	bool foundGoal = false;

	// get the goal square offset
	const int2 goalSqrOffset = peDef.GoalSquareOffset(BLOCK_SIZE);
	const float maxSpeedMod = pathingState->GetMaxSpeedMod(moveDef.pathType);

	// {bool printMoveInfo = (owner != nullptr) && (selectedUnitsHandler.selectedUnits.size() == 1)
    //     && (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
    // if (printMoveInfo) {
    //     LOG("%s Block Size [%d] search started", __func__, BLOCK_SIZE);
    // }}

	// if (debugLoggingActive == ThreadPool::GetThreadNum()){

	// 	const PathNode* ob = openBlocks.top();
	// 	int blockIdx = ob->nodeNum;

	// 	int limitX = 0;
	// 	int x = 0;
	// 	int limitY = 0;
	// 	int y = 0;

	// 	for (; x<limitX; ++x) {
	// 		for (;y<limitY; ++y){
	// 			const unsigned int pathOpt = blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS;
	// 			const unsigned int pathDir = PathOpt2PathDir(pathOpt);

	// 	// 		blockIdx  = BlockPosToIdx(BlockIdxToPos(blockIdx) - PE_DIRECTION_VECTORS[pathDir]);
	// 	// 		numNodes += 1;
	// 		}
	// 	}
	// }

	while (!openBlocks.empty() && (openBlockBuffer.GetSize() < maxBlocksToBeSearched)) {
		// get the open block with lowest cost
		const PathNode* ob = openBlocks.top();
		openBlocks.pop();

		// check if the block has been marked as unaccessible during its time in the queue
		if (blockStates.nodeMask[ob->nodeNum] & (PATHOPT_BLOCKED | PATHOPT_CLOSED))
			continue;

		// no, check if the goal is already reached
		const int2 bSquare = (*psBlockStates).peNodeOffsets[moveDef.pathType][ob->nodeNum];
		const int2 gSquare = ob->nodePos * BLOCK_SIZE + goalSqrOffset;

		bool runBlkSearch = false;
		bool canReachGoal =  true;

		// NOTE:
		//   this is a radius-based check, so gSquare can be considered the goal
		//   even if it can not be reached (via a maximum-res path) from bSquare
		//   basically the same condition as in TestBlock, except needed for the
		//   case that <ob> neighbors the goal but no actual edge leads to it
		if (peDef.IsGoal(bSquare.x, bSquare.y) || (runBlkSearch = peDef.IsGoal(gSquare.x, gSquare.y))) {
			if (runBlkSearch)
				canReachGoal = (DoBlockSearch(owner, moveDef, bSquare, gSquare) == IPath::Ok);

			if (canReachGoal) {
				mGoalBlockIdx = ob->nodeNum;
				mGoalHeuristic = 0.0f;
				foundGoal = true;
				break;
			}
		}

		// no, test the 8 surrounding blocks
		// NOTE:
		//   each of these calls increments openBlockBuffer.idx by (at most) 1, so
		//   maxBlocksToBeSearched is always less than <MAX_SEARCHED_NODES_PE - 8>
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_LEFT,       PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_LEFT_UP,    PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_UP,         PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_RIGHT_UP,   PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_RIGHT,      PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_RIGHT_DOWN, PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_DOWN,       PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_LEFT_DOWN,  PATHOPT_OPEN, maxSpeedMod);

		// mark this block as closed
		blockStates.nodeMask[ob->nodeNum] |= PATHOPT_CLOSED;
	}

	// {bool printMoveInfo = (owner != nullptr) && (selectedUnitsHandler.selectedUnits.size() == 1)
    //     && (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
    // if (printMoveInfo) {
    //     LOG("%s Block Size [%d] search finished", __func__, BLOCK_SIZE);
    // }}

	// we found our goal
	if (foundGoal)
		return IPath::Ok;

	// we could not reach the goal
	if (openBlockBuffer.GetSize() >= maxBlocksToBeSearched)
		return IPath::GoalOutOfRange;

	// search could not reach the goal due to the unit being locked in
	if (openBlocks.empty())
		return IPath::GoalOutOfRange;

	// should never happen
	LOG_L(L_ERROR, "%s - Unhandled end of search!", __func__);
	return IPath::Error;
}


/**
 * Test the accessibility of a block and its value,
 * possibly also add it to the open-blocks pqueue.
 */
bool CPathEstimator::TestBlock(
	const MoveDef& moveDef,
	const CPathFinderDef& peDef,
	const PathNode* parentOpenBlock,
	const CSolidObject* owner,
	const unsigned int pathDir,
	const unsigned int /*blockStatus*/,
	float maxSpeedMod
) {
	RECOIL_DETAILED_TRACY_ZONE;
	testedBlocks++;

	// step from parent to child block (e.g. PATHDIR_LEFT_TO_RIGHT=<+1,0>)
	const int2 openBlockPos = parentOpenBlock->nodePos;
	const int2 testBlockPos = openBlockPos + PE_DIRECTION_VECTORS[pathDir];
	const int2 goalBlockPos = {int(peDef.goalSquareX / BLOCK_SIZE), int(peDef.goalSquareZ / BLOCK_SIZE)};

	// bounds-check
	if (static_cast<unsigned int>(testBlockPos.x) >= nbrOfBlocks.x)
		return false;
	if (static_cast<unsigned int>(testBlockPos.y) >= nbrOfBlocks.y)
		return false;

	// read precached vertex costs
	const unsigned int openBlockIdx = BlockPosToIdx(openBlockPos);
	const unsigned int testBlockIdx = BlockPosToIdx(testBlockPos);

	// check if the block is unavailable
	if (blockStates.nodeMask[testBlockIdx] & (PATHOPT_BLOCKED | PATHOPT_CLOSED))
		return false;

	const unsigned int vertexBaseIdx = moveDef.pathType * nbrOfBlocks.x * nbrOfBlocks.y * PATH_DIRECTION_VERTICES;
	const unsigned int vertexCostIdx =
		vertexBaseIdx +
		openBlockIdx * PATH_DIRECTION_VERTICES +
		GetBlockVertexOffset(moveDef, pathDir, nbrOfBlocks.x);

	assert(testBlockIdx < (*psBlockStates).peNodeOffsets[moveDef.pathType].size());
	//assert(vertexCostIdx < vertexCosts.size());

	// best accessible heightmap-coordinate within tested block
	// [DBG] const int2 openBlockSquare = blockStates.peNodeOffsets[moveDef.pathType][openBlockIdx];
	const int2 testBlockSquare = (*psBlockStates).peNodeOffsets[moveDef.pathType][testBlockIdx];

	// transition-cost from parent to tested child
	float testVertexCost = pathingState->GetVertexCost(vertexCostIdx);


	// inf-cost means we can not get from the parent VERTEX to the child
	// but the latter might still be reachable from peDef.wsStartPos (if
	// it is one of the first 8 expanded)
	// regular edges within the base-set are only valid to expand iff end
	// is reachable from wsStartPos, which can disagree with reachability
	// from openBlockSquare
	const bool infCostVertex = (testVertexCost >= PATHCOST_INFINITY);
	const bool baseSetVertex = (!peDef.useVerifiedStartBlock) && (testedBlocks <= 8);
	const bool blockedSearch = (!baseSetVertex || peDef.skipSubSearches);

	// {bool printMoveInfo = (owner != nullptr) && (selectedUnitsHandler.selectedUnits.size() == 1)
	// 	&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
	// if (printMoveInfo) {
	// if (owner != nullptr && selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end()){
	// 	LOG("%s Block Size [%d] infCostVertex = %d,  blockedSearch = %d, peDef.useVerifiedStartBlock = %d, testedBlocks = %d"
	// 		, __func__, BLOCK_SIZE, (int)infCostVertex, (int)blockedSearch, (int)peDef.useVerifiedStartBlock, testedBlocks);
	// }

	if (infCostVertex) {
		// warning: we cannot naively set PATHOPT_BLOCKED here;
		// vertexCosts[] depends on the direction and nodeMask
		// does not
		// we would have to save the direction via PATHOPT_LEFT
		// etc. in the nodeMask but that is complicated and not
		// worth it: would just save the vertexCosts[] lookup
		//
		// blockStates.nodeMask[testBlockIdx] |= (PathDir2PathOpt(pathDir) | PATHOPT_BLOCKED);
		// dirtyBlocks.push_back(testBlockIdx);
		if (blockedSearch || DoBlockSearch(owner, moveDef, peDef.wsStartPos, SquareToFloat3(testBlockSquare)) != IPath::Ok)
			return false;

		testVertexCost = peDef.Heuristic(testBlockSquare.x, testBlockSquare.y, peDef.startSquareX, peDef.startSquareZ, BLOCK_SIZE);
	} else {
		if (!blockedSearch && DoBlockSearch(owner, moveDef, peDef.wsStartPos, SquareToFloat3(testBlockSquare)) != IPath::Ok)
			return false;
	}

	// check if the block is outside constraints
	if (!peDef.WithinConstraints(testBlockSquare)) {
		blockStates.nodeMask[testBlockIdx] |= PATHOPT_BLOCKED;
		dirtyBlocks.push_back(testBlockIdx);
		return false;
	}

	if (!peDef.skipSubSearches) {
		#if 0
		if (infCostVertex && baseSetVertex && DoBlockSearch(owner, moveDef, peDef.wsStartPos, SquareToFloat3(testBlockSquare.x, testBlockSquare.y)) != IPath::Ok)
			return false;
		#endif

		if (testBlockPos == goalBlockPos) {
			// must skip goal sub-searches during CalcVertexPathCosts
			//
			// if we have expanded the goal-block, check if a valid
			// max-resolution path exists (from where we entered it
			// to the actual goal position) since there might still
			// be impassable terrain in between
			//
			// const float3 gWorldPos = {testBlockPos.x * BLOCK_PIXEL_SIZE * 1.0f, 0.0f, testBlockPos.y * BLOCK_PIXEL_SIZE * 1.0f};
			// const float3 sWorldPos = {openBlockPos.x * BLOCK_PIXEL_SIZE * 1.0f, 0.0f, openBlockPos.y * BLOCK_PIXEL_SIZE * 1.0f};
			const float3 sWorldPos = SquareToFloat3(testBlockSquare.x, testBlockSquare.y);
			const float3 gWorldPos = peDef.wsGoalPos;

			if (sWorldPos.SqDistance2D(gWorldPos) > peDef.sqGoalRadius) {

				// {bool printMoveInfo = (owner != nullptr) && (selectedUnitsHandler.selectedUnits.size() == 1)
				// 	&& (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end());
				// if (printMoveInfo) {
				// 	LOG("%s Block Size [%d] Check goalpos of "
				// 		, __func__, BLOCK_SIZE);
				// }}

				if (DoBlockSearch(owner, moveDef, sWorldPos, gWorldPos) != IPath::Ok) {
					// we cannot set PATHOPT_BLOCKED here either, result
					// depends on direction of entry from the parent node
					//
					// blockStates.nodeMask[testBlockIdx] |= PATHOPT_BLOCKED;
					// dirtyBlocks.push_back(testBlockIdx);
					return false;
				}
			}
		}
	}


	// evaluate this node (NOTE the max-resolution indexing for {flow,extra}Cost)
	//
	// nodeCost incorporates speed-modifiers, but the heuristic estimate does not
	// this causes an overestimation (and hence sub-optimal paths) if the average
	// modifier is greater than 1, which can only be fixed by choosing a constant
	// multiplier smaller than 1
	// however, since that would increase the number of explored nodes on "normal"
	// maps where the average is ~1, it is better to divide hCost by the (initial)
	// maximum modifier value
	//
	// const float  flowCost = (peDef.testMobile) ? (PathFlowMap::GetInstance())->GetFlowCost(testBlockSquare.x, testBlockSquare.y, moveDef, PathDir2PathOpt(pathDir)) : 0.0f;
	const float extraCost = blockStates.GetNodeExtraCost(testBlockSquare.x, testBlockSquare.y, peDef.synced);
	const float  nodeCost = testVertexCost + extraCost;

	const float gCost = parentOpenBlock->gCost + nodeCost;
	const float hCost = peDef.Heuristic(testBlockSquare.x, testBlockSquare.y, BLOCK_SIZE) * maxSpeedMod;
	const float fCost = gCost + hCost;

	// already in the open set?
	if (blockStates.nodeMask[testBlockIdx] & PATHOPT_OPEN) {
		// check if new found path is better or worse than the old one
		if (blockStates.fCost[testBlockIdx] <= fCost)
			return true;

		// no, clear old path data
		blockStates.nodeMask[testBlockIdx] &= ~PATHOPT_CARDINALS;
	}

	// look for improvements
	if (hCost < mGoalHeuristic) {
		mGoalBlockIdx = testBlockIdx;
		mGoalHeuristic = hCost;
	}

	// store this block as open
	openBlockBuffer.SetSize(openBlockBuffer.GetSize() + 1);
	assert(openBlockBuffer.GetSize() < MAX_SEARCHED_NODES_PE);

	PathNode* ob = openBlockBuffer.GetNode(openBlockBuffer.GetSize());
		ob->fCost   = fCost;
		ob->gCost   = gCost;
		ob->nodePos = testBlockPos;
		ob->nodeNum = testBlockIdx;
	openBlocks.push(ob);

	blockStates.SetMaxCost(NODE_COST_F, std::max(blockStates.GetMaxCost(NODE_COST_F), fCost));
	blockStates.SetMaxCost(NODE_COST_G, std::max(blockStates.GetMaxCost(NODE_COST_G), gCost));

	// mark this block as open
	blockStates.fCost[testBlockIdx] = fCost;
	blockStates.gCost[testBlockIdx] = gCost;
	blockStates.nodeMask[testBlockIdx] |= (PathDir2PathOpt(pathDir) | PATHOPT_OPEN);

	dirtyBlocks.push_back(testBlockIdx);
	return true;
}



/**
 * Test the accessibility of a block and its value,
 * possibly also add it to the open-blocks pqueue.
 */
bool CPathEstimator::TestBlockReachability(
	const MoveDef& moveDef,
	const CPathFinderDef& peDef,
	const CSolidObject* owner,
	const unsigned int testBlockIdx
) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(testBlockIdx < (*psBlockStates).peNodeOffsets[moveDef.pathType].size());
	const int2 testBlockSquare = (*psBlockStates).peNodeOffsets[moveDef.pathType][testBlockIdx];

	if (!peDef.WithinConstraints(testBlockSquare))
		return false;

	return (DoBlockSearch(owner, moveDef, peDef.wsStartPos, SquareToFloat3(testBlockSquare)) == IPath::Ok);
}



/**
 * Recreate the path taken to the goal
 */
void CPathEstimator::FinishSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, IPath::Path& foundPath) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (pfDef.needPath) {
		unsigned int blockIdx = mGoalBlockIdx;
		unsigned int numNodes = 0;

		{
			#if 1
			while (blockIdx != mStartBlockIdx) {
				const unsigned int pathOpt = blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS;
				const unsigned int pathDir = PathOpt2PathDir(pathOpt);

				blockIdx  = BlockPosToIdx(BlockIdxToPos(blockIdx) - PE_DIRECTION_VECTORS[pathDir]);
				numNodes += 1;
			}

			// PE's do not need the squares
			// foundPath.squares.reserve(numNodes);
			foundPath.path.reserve(numNodes);

			// reset
			blockIdx = mGoalBlockIdx;

			#else

			// more wasteful, but slightly faster for very long paths
			// foundPath.squares.reserve(1024 / BLOCK_SIZE);
			foundPath.path.reserve(1024 / BLOCK_SIZE);
			#endif
		}

		while (true) {
			// use offset defined by the block
			const int2 square = (*psBlockStates).peNodeOffsets[moveDef.pathType][blockIdx];

			// foundPath.squares.push_back(square);
			foundPath.path.emplace_back(square.x * SQUARE_SIZE, CMoveMath::yLevel(moveDef, square.x, square.y), square.y * SQUARE_SIZE);

			if (blockIdx == mStartBlockIdx)
				break;

			// next step backwards
			const unsigned int pathOpt = blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS;
			const unsigned int pathDir = PathOpt2PathDir(pathOpt);

			blockIdx = BlockPosToIdx(BlockIdxToPos(blockIdx) - PE_DIRECTION_VECTORS[pathDir]);
		}

		if (!foundPath.path.empty())
			foundPath.pathGoal = foundPath.path[0];
	}

	foundPath.pathCost = blockStates.fCost[mGoalBlockIdx] - mGoalHeuristic;
}


/**
 * Returns a hash-code identifying the dataset of this estimator.
 */
std::uint32_t CPathEstimator::CalcHash(const char* caller) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const unsigned int hmChecksum = readMap->CalcHeightmapChecksum();
	const unsigned int tmChecksum = readMap->CalcTypemapChecksum();
	const unsigned int mdChecksum = moveDefHandler.GetCheckSum();
	const unsigned int bmChecksum = groundBlockingObjectMap.CalcChecksum();
	const unsigned int peHashCode = (hmChecksum + tmChecksum + mdChecksum + bmChecksum + BLOCK_SIZE + PATHESTIMATOR_VERSION);

	LOG("[PathEstimator::%s][%s] BLOCK_SIZE=%u", __func__, caller, BLOCK_SIZE);
	LOG("[PathEstimator::%s][%s] PATHESTIMATOR_VERSION=%u", __func__, caller, PATHESTIMATOR_VERSION);
	LOG("[PathEstimator::%s][%s] heightMapChecksum=%x", __func__, caller, hmChecksum);
	LOG("[PathEstimator::%s][%s] typeMapChecksum=%x", __func__, caller, tmChecksum);
	LOG("[PathEstimator::%s][%s] moveDefChecksum=%x", __func__, caller, mdChecksum);
	LOG("[PathEstimator::%s][%s] blockMapChecksum=%x", __func__, caller, bmChecksum);
	LOG("[PathEstimator::%s][%s] estimatorHashCode=%x", __func__, caller, peHashCode);

	return peHashCode;
}

}