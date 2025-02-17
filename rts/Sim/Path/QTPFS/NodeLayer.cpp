/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include <limits>

#if defined(_MSC_VER)
#include <intrin.h>
// visual c
inline int __bsfd (int mask)
{
	unsigned long index;
	_BitScanForward(&index, mask);
	return index;
}
#elif defined(__GNUC__)
#include <x86intrin.h>
#else
#error no bsfd intrinsic currently set
#endif

#include "NodeLayer.h"
#include "PathManager.h"
#include "Node.h"

#include "Map/MapInfo.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Objects/SolidObject.h"
#include "System/SpringMath.h"

#include "System/Misc/TracyDefs.h"

unsigned int QTPFS::NodeLayer::NUM_SPEEDMOD_BINS;
float        QTPFS::NodeLayer::MIN_SPEEDMOD_VALUE;
float        QTPFS::NodeLayer::MAX_SPEEDMOD_VALUE;

void QTPFS::NodeLayer::InitStatic() {
	RECOIL_DETAILED_TRACY_ZONE;
	NUM_SPEEDMOD_BINS  = std::max(  1u, mapInfo->pfs.qtpfs_constants.numSpeedModBins);
	MIN_SPEEDMOD_VALUE = std::max(0.0f, mapInfo->pfs.qtpfs_constants.minSpeedModVal);
	MAX_SPEEDMOD_VALUE = std::min(8.0f, mapInfo->pfs.qtpfs_constants.maxSpeedModVal);

	LOG("%s: NUM_SPEEDMOD_BINS=%d, MIN_SPEEDMOD_VALUE=%f, MAX_SPEEDMOD_VALUE=%f", __func__
			, NUM_SPEEDMOD_BINS, MIN_SPEEDMOD_VALUE, MAX_SPEEDMOD_VALUE);
}


void QTPFS::NodeLayer::Init(unsigned int layerNum) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert((QTPFS::NodeLayer::NUM_SPEEDMOD_BINS + 1) <= MaxSpeedBinTypeValue());

	constexpr size_t initialNodeReserve = 256;
	openNodes.reserve(initialNodeReserve);
	selectedNodes.reserve(initialNodeReserve);

	// pre-count the root
	numLeafNodes = 1;
	layerNumber = layerNum;

	xsize = mapDims.mapx;
	zsize = mapDims.mapy;

	{
		// chunks are reserved OTF
		nodeIndcs.clear();
		nodeIndcs.resize(POOL_TOTAL_SIZE);

		std::for_each(nodeIndcs.begin(), nodeIndcs.end(), [&](const unsigned int& i) { nodeIndcs[&i - &nodeIndcs[0]] = &i - &nodeIndcs[0]; assert((size_t)(&i - &nodeIndcs[0]) < nodeIndcs.size()); });
		std::reverse(nodeIndcs.begin(), nodeIndcs.end());
	}

	curSpeedMods.resize(QTPFS_MAX_NODE_SIZE*QTPFS_MAX_NODE_SIZE,  0);
	curSpeedBins.resize(QTPFS_MAX_NODE_SIZE*QTPFS_MAX_NODE_SIZE, -1);

	MoveDef* md = moveDefHandler.GetMoveDefByPathType(layerNum);
	useShortestPath = md->preferShortestPath;
}

void QTPFS::NodeLayer::Clear() {
	RECOIL_DETAILED_TRACY_ZONE;
	curSpeedMods.clear();
	curSpeedBins.clear();
}


bool QTPFS::NodeLayer::Update(UpdateThreadData& threadData) {
	RECOIL_DETAILED_TRACY_ZONE;
	// assert((luSpeedMods == nullptr && luBlockBits == nullptr) || (luSpeedMods != nullptr && luBlockBits != nullptr));

	unsigned int numClosedSquares = 0;

	// areaUpdated is always 16x16 squares. areaRelinkedInner could be bigger; this doesn't happen as often and once the
	// nodes have divided they will never again grow larger than 16x16 squares in a match.
	const SRectangle& r = threadData.areaRelinkedInner;
	const MoveDef* md = threadData.moveDef;

	auto &blockRect = threadData.areaMaxBlockBits;
	auto &blockBits = threadData.maxBlockBits;

	int tempNum = gs->GetMtTempNum(threadData.threadId);

	MoveTypes::CheckCollisionQuery virtualObject(md);
	MoveDefs::CollisionQueryStateTrack queryState;

	const bool isSubmersible = md->IsComplexSubmersible();
	if (!isSubmersible) {
		CMoveMath::FloodFillRangeIsBlocked(*md, nullptr, threadData.areaMaxBlockBits, threadData.maxBlockBits, threadData.threadId);
	}

	auto rangeIsBlocked = [&blockRect, &blockBits](const MoveDef& md, int chmx, int chmz){
		const int xmin = (chmx - md.xsizeh) - blockRect.x1;
		const int zmin = (chmz - md.zsizeh) - blockRect.z1;
		const int xmax = (chmx + md.xsizeh) - blockRect.x1;
		const int zmax = (chmz + md.zsizeh) - blockRect.z1;
		const int blockRectWidth = blockRect.GetWidth();
		int ret = 0;
		
		// footprints are point-symmetric around <xSquare, zSquare>
		for (int z = zmin; z <= zmax; z += 2/*FOOTPRINT_ZSTEP*/) {
			for (int x = xmin; x <= xmax; x += 2/*FOOTPRINT_XSTEP*/) {
				ret |= blockBits[z * blockRectWidth + x];
				if ((ret & CMoveMath::BLOCK_STRUCTURE) != 0)
					return ret;
			}
		}

		return ret;
	};

	// Collisions around above/below water units is messy in the engine and trying to pass flags
	// down through the multiple function calls will make it even messier. Creating a querying
	// based on a virtual object, which can then be moved per query may be the simplest approach.
	// We can't flood fill collision data like normal because the squares that collide can change
	// as the unit's centre square's height changes.
	auto submersibleRangeIsBlocked = [this, &virtualObject, &queryState, &tempNum, &threadData](const MoveDef& md, int chmx, int chmz){
		const int xmin = (chmx - md.xsizeh);
		const int zmin = (chmz - md.zsizeh);
		const int xmax = (chmx + md.xsizeh);
		const int zmax = (chmz + md.zsizeh);

		md.UpdateCheckCollisionQuery(virtualObject, queryState, {chmx, chmz});
		if (queryState.refreshCollisionCache)
			tempNum = gs->GetMtTempNum(threadData.threadId);
		
		return CMoveMath::RangeIsBlockedHashedMt(xmin, xmax, zmin, zmax, &virtualObject, tempNum, threadData.threadId);
	};

	// divide speed-modifiers into bins
	for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
		for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
			const unsigned int recIdx = (hmz - r.z1) * r.GetWidth() + (hmx - r.x1);

			// don't tesselate map edges when footprint extends across them in IsBlocked*
			const int chmx = std::clamp(int(hmx), md->xsizeh, mapDims.mapxm1 + (-md->xsizeh));
			const int chmz = std::clamp(int(hmz), md->zsizeh, mapDims.mapym1 + (-md->zsizeh));

			int maxBlockBit = (!isSubmersible) ? rangeIsBlocked(*md, chmx, chmz)
											  : submersibleRangeIsBlocked(*md, chmx, chmz);

			// NOTE:
			//   movetype code checks ONLY the *CENTER* square of a unit's footprint
			//   to get the current speedmod affecting it, and the default pathfinder
			//   only takes the entire footprint into account for STRUCTURE-blocking
			//   tests --> do the same here because full-footprint checking for both
			//   structures AND terrain is much slower (and if not handled correctly
			//   units will get stuck everywhere)
			// NOTE:
			//   IsBlockedNoSpeedModCheck works at HALF-heightmap resolution (as does
			//   the default pathfinder for DETAILED_DISTANCE searches!), so this can
			//   generate false negatives!
			//   
			// const int maxBlockBit = (luBlockBits == NULL)? CMoveMath::SquareIsBlocked(*md, hmx, hmz, NULL): (*luBlockBits)[recIdx];

			float newAbsSpeedMod = 0.f;

			#define NL QTPFS::NodeLayer
			if ((maxBlockBit & CMoveMath::BLOCK_STRUCTURE) == 0) {
				const float minSpeedMod = CMoveMath::GetPosSpeedMod(*md, hmx, hmz);
				newAbsSpeedMod = std::clamp(minSpeedMod, NL::MIN_SPEEDMOD_VALUE, NL::MAX_SPEEDMOD_VALUE);
			}
			const float newRelSpeedMod = std::clamp((newAbsSpeedMod - NL::MIN_SPEEDMOD_VALUE) / (NL::MAX_SPEEDMOD_VALUE - NL::MIN_SPEEDMOD_VALUE), 0.0f, 1.0f);
			#undef NL

			const SpeedBinType newSpeedModBin = GetSpeedModBin(newAbsSpeedMod, newRelSpeedMod);
			numClosedSquares += int(newSpeedModBin == QTPFS::NodeLayer::NUM_SPEEDMOD_BINS);

			// need to keep track of these for Tesselate
			curSpeedMods[recIdx] = newRelSpeedMod * float(MaxSpeedModTypeValue());
			curSpeedBins[recIdx] = newSpeedModBin;
		}
	}

	return true;
}


QTPFS::SpeedBinType QTPFS::NodeLayer::GetSpeedModBin(float absSpeedMod, float relSpeedMod) const {
	RECOIL_DETAILED_TRACY_ZONE;
	// NOTE:
	//     bins N and N+1 are reserved for modifiers <= min and >= max
	//     respectively; blocked squares MUST be in their own category
	const SpeedBinType defBin = NUM_SPEEDMOD_BINS * relSpeedMod;
	const SpeedBinType maxBin = NUM_SPEEDMOD_BINS - 1;

	SpeedBinType speedModBin = std::clamp(defBin, static_cast<SpeedBinType>(0), maxBin);

	if (absSpeedMod <= MIN_SPEEDMOD_VALUE) { speedModBin = NUM_SPEEDMOD_BINS + 0; }
	if (absSpeedMod >= MAX_SPEEDMOD_VALUE) { speedModBin = NUM_SPEEDMOD_BINS + 1; }

	return speedModBin;
}


void QTPFS::NodeLayer::ExecNodeNeighborCacheUpdates(const SRectangle& ur, UpdateThreadData& threadData) {
	RECOIL_DETAILED_TRACY_ZONE;
	// account for the rim of nodes around the bounding box
	// (whose neighbors also changed during re-tesselation)
	const int xmin = std::max(ur.x1 - 1, 0), xmax = std::min(ur.x2 + 1, mapDims.mapx);
	const int zmin = std::max(ur.z1 - 1, 0), zmax = std::min(ur.z2 + 1, mapDims.mapy);

	// ------------------------------------------------------
	// find all leaf nodes
	// go through each level - check for children in area.
	SRectangle searchArea(xmin, zmin, xmax, zmax);
	GetNodesInArea(searchArea, selectedNodes);

	threadData.relinkNodeGrid.clear();
	threadData.relinkNodeGrid.resize(threadData.areaRelinked.GetArea(), nullptr);

	// Build grid with selected nodes.
	std::for_each(selectedNodes.begin(), selectedNodes.end(), [&threadData](INode *curNode){
		SRectangle& r = threadData.areaRelinked;
		SRectangle nodeArea(curNode->xmin(), curNode->zmin(), curNode->xmax(), curNode->zmax());
		nodeArea.ClampIn(r);
		int width = r.GetWidth();
		int zlast = nodeArea.z2 - 1;
		for (int z = nodeArea.z1; z < nodeArea.z2; ++z) {
			int zoff = (z - r.z1) * width;
			if (z == nodeArea.z1 || z == zlast){
				for (int x = nodeArea.x1; x < nodeArea.x2; ++x) {
					unsigned int index = zoff + (x - r.x1);
					assert(index < threadData.relinkNodeGrid.size());
					threadData.relinkNodeGrid[index] = curNode;
				}
			} else {
				// only fill edges, inner body is never consulted.
				{
					int x = nodeArea.x1;
					unsigned int index = zoff + (x - r.x1);
					assert(index < threadData.relinkNodeGrid.size());
					threadData.relinkNodeGrid[index] = curNode;
				}
				{
					int x = nodeArea.x2 - 1;
					unsigned int index = zoff + (x - r.x1);
					assert(index < threadData.relinkNodeGrid.size());
					threadData.relinkNodeGrid[index] = curNode;
				}
			}
		}
	});

	// now update the selected nodes
	std::for_each(selectedNodes.begin(), selectedNodes.end(), [this, &threadData](INode* curNode){
		curNode->UpdateNeighborCache(*this, threadData);
	});
}


void QTPFS::NodeLayer::GetNodesInArea(const SRectangle& areaToSearch, std::vector<INode*>& nodesFound) {
	RECOIL_DETAILED_TRACY_ZONE;
	openNodes.clear();
	nodesFound.clear();

	const int xmin = areaToSearch.x1, xmax = areaToSearch.x2;
	const int zmin = areaToSearch.z1, zmax = areaToSearch.z2;

	SRectangle rootNodes
		( xmin / rootNodeSize
		, zmin / rootNodeSize
		, (xmax-1) / rootNodeSize
		, (zmax-1) / rootNodeSize
	);

	for (int z = rootNodes.z1; z <= rootNodes.z2; ++z) {
		int i = z * xRootNodes;
		for (int x = rootNodes.x1; x <= rootNodes.x2; ++x) {
			INode* curNode = GetPoolNode(i + x);
			openNodes.emplace_back(curNode);
		}
	}

	while (!openNodes.empty()) {
		INode* curNode = openNodes.back();
		openNodes.pop_back();

		if (curNode->IsLeaf()) {
			nodesFound.emplace_back(curNode);
			continue;
		}

		for (int i = 0; i < QTNODE_CHILD_COUNT; ++i) {
			int childIndex = curNode->GetChildBaseIndex() + i;
			INode* childNode = GetPoolNode(childIndex);

			if (xmax <= childNode->xmin()) { continue; }
			if (xmin >= childNode->xmax()) { continue; }
			if (zmax <= childNode->zmin()) { continue; }
			if (zmin >= childNode->zmax()) { continue; }

			openNodes.emplace_back(childNode);
		}
	}
}

QTPFS::INode* QTPFS::NodeLayer::GetNearestNodeInArea
		( const SRectangle& areaToSearch
		, int2 referencePoint
		, std::vector<INode*>& tmpNodes
		) {
	RECOIL_DETAILED_TRACY_ZONE;
	tmpNodes.clear();
	INode* bestNode = nullptr;
	uint64_t bestDistScore = std::numeric_limits<uint64_t>::max();

	const int xmin = areaToSearch.x1, xmax = areaToSearch.x2;
	const int zmin = areaToSearch.z1, zmax = areaToSearch.z2;

	// The xmin (0), xmax (1), zmin (2), zmax (3) can be accessed by (index.)
	// The these are the indices needed to make the 4 corners of a quad.
	constexpr int2 cornerPoints[] =
		{ {0, 2}, {1, 2}
		, {0, 3}, {1, 3}
		};

	SRectangle rootNodes
		( xmin / rootNodeSize
		, zmin / rootNodeSize
		, (xmax-1) / rootNodeSize
		, (zmax-1) / rootNodeSize
	);

	for (int z = rootNodes.z1; z <= rootNodes.z2; ++z) {
		int i = z * xRootNodes;
		for (int x = rootNodes.x1; x <= rootNodes.x2; ++x) {
			INode* curNode = GetPoolNode(i + x);
			tmpNodes.emplace_back(curNode);
		}
	}

	auto getNodeScore = [referencePoint, &cornerPoints](const INode* curNode) -> uint64_t {
		int2 midRef(curNode->xmid(), curNode->zmid());
		int midDist = referencePoint.DistanceSq(midRef);
		int closestPointDist = midDist;
		int bestIndex = 0;
		for (int i = 0; i < 4; ++i) {
			int2 ref(curNode->point(cornerPoints[i].x), curNode->point(cornerPoints[i].y));
			int dist = referencePoint.DistanceSq(ref);
			if (dist < closestPointDist) {
				closestPointDist = dist;
				bestIndex = i;
			}
		}
		return ((uint64_t)closestPointDist << 32) + ((uint64_t)midDist << 2) + (uint64_t)bestIndex;
	};

	while (!tmpNodes.empty()) {
		INode* curNode = tmpNodes.back();
		tmpNodes.pop_back();

		if (curNode->IsLeaf()) {
			// Lowest score is better
			uint64_t curDistScore = getNodeScore(curNode);
			if (curDistScore < bestDistScore) {
				bestDistScore = curDistScore;
				bestNode = curNode;
			}
			continue;
		}

		for (int i = 0; i < QTNODE_CHILD_COUNT; ++i) {
			int childIndex = curNode->GetChildBaseIndex() + i;
			INode* childNode = GetPoolNode(childIndex);

			if (xmax <= childNode->xmin()) { continue; }
			if (xmin >= childNode->xmax()) { continue; }
			if (zmax <= childNode->zmin()) { continue; }
			if (zmin >= childNode->zmax()) { continue; }
			if (childNode->AllSquaresImpassable()) { continue; }
			if (childNode->IsExitOnly()) { continue; }

			tmpNodes.emplace_back(childNode);
		}
	}

	return bestNode;
}

QTPFS::INode* QTPFS::NodeLayer::GetNodeThatEncasesPowerOfTwoArea(const SRectangle& areaToEncase) {
	RECOIL_DETAILED_TRACY_ZONE;
	INode* selectedNode = nullptr;
	int length = rootNodeSize;
	int iz = (areaToEncase.z1 / length) * xRootNodes;
	int ix = (areaToEncase.x1 / length);
	int i = iz + ix;
	INode* curNode = GetPoolNode(i);

	while (curNode->RectIsInside(areaToEncase)) {
		selectedNode = curNode;
		if (curNode->IsLeaf()) { break; }
		
		bool isRight = areaToEncase.x1 >= curNode->xmid();
		bool isDown = areaToEncase.z1 >= curNode->zmid();
		int offset = 1*(isRight) + 2*(isDown);
		int nextIndex = curNode->GetChildBaseIndex() + offset;
		curNode = GetPoolNode(nextIndex);
	}
	assert(selectedNode != nullptr);
	return selectedNode;
}
