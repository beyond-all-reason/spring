/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathingState.h"

#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Path/Default/PathConstants.h"

#include "System/Threading/ThreadPool.h" // for_mt

namespace TKPFS {

void PathingState::Init(PathingState* parentState, unsigned int BLOCK_SIZE, const std::string& peFileName, const std::string& mapFileName)
{
	// {
	// 	BLOCKS_TO_UPDATE = SQUARES_TO_UPDATE / (BLOCK_SIZE * BLOCK_SIZE) + 1;

	// 	blockUpdatePenalty = 0;
	// 	nextOffsetMessageIdx = 0;
	// 	nextCostMessageIdx = 0;

	// 	pathChecksum = 0;
	// 	fileHashCode = CalcHash(__func__);

	// 	offsetBlockNum = {nbrOfBlocks.x * nbrOfBlocks.y};
	// 	costBlockNum = {nbrOfBlocks.x * nbrOfBlocks.y};

	// 	parentPathFinder = pf;
	// 	nextPathEstimator = nullptr;
	// }
	{
		// vertexCosts.clear();
		// vertexCosts.resize(moveDefHandler.GetNumMoveDefs() * blockStates.GetSize() * PATH_DIRECTION_VERTICES, PATHCOST_INFINITY);
		maxSpeedMods.clear();
		maxSpeedMods.resize(moveDefHandler.GetNumMoveDefs(), 0.001f);

		// updatedBlocks.clear();
		// consumedBlocks.clear();
		// offsetBlocksSortedByCost.clear();
	}

	PathingState*  childPE = this;
	PathingState* parentPE = parentState;

	if (parentPE != nullptr)
		parentPE->nextPathState = childPE;

	// precalc for FindBlockPosOffset()
	// {
	// 	offsetBlocksSortedByCost.reserve(BLOCK_SIZE * BLOCK_SIZE);
	// 	for (unsigned int z = 0; z < BLOCK_SIZE; ++z) {
	// 		for (unsigned int x = 0; x < BLOCK_SIZE; ++x) {
	// 			const float dx = x - (float)(BLOCK_SIZE - 1) * 0.5f;
	// 			const float dz = z - (float)(BLOCK_SIZE - 1) * 0.5f;
	// 			const float cost = (dx * dx + dz * dz);

	// 			offsetBlocksSortedByCost.emplace_back(cost, x, z);
	// 		}
	// 	}
	// 	std::stable_sort(offsetBlocksSortedByCost.begin(), offsetBlocksSortedByCost.end(), [](const SOffsetBlock& a, const SOffsetBlock& b) {
	// 		return (a.cost < b.cost);
	// 	});
	// }

	if (BLOCK_SIZE == LOWRES_PE_BLOCKSIZE) {
		assert(parentPE != nullptr);

		// calculate map-wide maximum positional speedmod for each MoveDef
		for_mt(0, moveDefHandler.GetNumMoveDefs(), [&](unsigned int i) {
			const MoveDef* md = moveDefHandler.GetMoveDefByPathType(i);

			for (int y = 0; y < mapDims.mapy; y++) {
				for (int x = 0; x < mapDims.mapx; x++) {
					childPE->maxSpeedMods[i] = std::max(childPE->maxSpeedMods[i], CMoveMath::GetPosSpeedMod(*md, x, y));
				}
			}
		});

		// calculate reciprocals, avoids divisions in TestBlock
		for (unsigned int i = 0; i < maxSpeedMods.size(); i++) {
			 childPE->maxSpeedMods[i] = 1.0f / childPE->maxSpeedMods[i];
			parentPE->maxSpeedMods[i] = childPE->maxSpeedMods[i];
		}
	}
}

}