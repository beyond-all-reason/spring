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
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/SpringMath.h"

#include <tracy/Tracy.hpp>

unsigned int QTPFS::NodeLayer::NUM_SPEEDMOD_BINS;
float        QTPFS::NodeLayer::MIN_SPEEDMOD_VALUE;
float        QTPFS::NodeLayer::MAX_SPEEDMOD_VALUE;



void QTPFS::NodeLayer::InitStatic() {
	NUM_SPEEDMOD_BINS  = 1u;// std::max(  1u, mapInfo->pfs.qtpfs_constants.numSpeedModBins);
	MIN_SPEEDMOD_VALUE = std::max(0.0f, mapInfo->pfs.qtpfs_constants.minSpeedModVal);
	MAX_SPEEDMOD_VALUE = std::min(8.0f, mapInfo->pfs.qtpfs_constants.maxSpeedModVal);

	LOG("%s: NUM_SPEEDMOD_BINS=%d, MIN_SPEEDMOD_VALUE=%f, MAX_SPEEDMOD_VALUE=%f", __func__
			, NUM_SPEEDMOD_BINS, MIN_SPEEDMOD_VALUE, MAX_SPEEDMOD_VALUE);
}


void QTPFS::NodeLayer::Init(unsigned int layerNum) {
	assert((QTPFS::NodeLayer::NUM_SPEEDMOD_BINS + 1) <= MaxSpeedBinTypeValue());

	openNodes.reserve(200); // TODO: remove magic numbers
	selectedNodes.reserve(200);

	// pre-count the root
	numLeafNodes = 1;
	layerNumber = layerNum;

	xsize = mapDims.mapx;
	zsize = mapDims.mapy;

	{
		// chunks are reserved OTF
		nodeIndcs.clear();
		nodeIndcs.resize(POOL_TOTAL_SIZE);

		std::for_each(nodeIndcs.begin(), nodeIndcs.end(), [&](const unsigned int& i) { nodeIndcs[&i - &nodeIndcs[0]] = &i - &nodeIndcs[0]; });
		std::reverse(nodeIndcs.begin(), nodeIndcs.end());
	}

	curSpeedMods.resize(xsize * zsize,  0);
	curSpeedBins.resize(xsize * zsize, -1);
}

void QTPFS::NodeLayer::Clear() {
	curSpeedMods.clear();
	curSpeedBins.clear();
}


bool QTPFS::NodeLayer::Update(UpdateThreadData& threadData) {
	// assert((luSpeedMods == nullptr && luBlockBits == nullptr) || (luSpeedMods != nullptr && luBlockBits != nullptr));

	// unsigned int numNewBinSquares = 0;
	unsigned int numClosedSquares = 0;
	const SRectangle& r = threadData.areaUpdated;
	const MoveDef* md = threadData.moveDef;

	// const bool globalUpdate =
	// 	((r.x1 == 0 && r.x2 == mapDims.mapx) &&
	// 	 (r.z1 == 0 && r.z2 == mapDims.mapy));

	// if (globalUpdate) {
	// 	maxRelSpeedMod = 0.0f;
	// 	avgRelSpeedMod = 0.0f;
	// }

	// threadData.curSpeedMods.resize(r.GetArea(), 0);
	// threadData.curSpeedBins.resize(r.GetArea(), -1);
	CMoveMath::FloodFillRangeIsBlocked(*md, nullptr, threadData.areaMaxBlockBits, threadData.maxBlockBits);

	// auto &curSpeedMods = threadData.curSpeedMods;
	// auto &curSpeedBins = threadData.curSpeedBins;
	auto &blockRect = threadData.areaMaxBlockBits;
	auto &blockBits = threadData.maxBlockBits;

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

	// divide speed-modifiers into bins
	for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
		for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
			// const unsigned int sqrIdx = hmz * xsize + hmx;
			// const unsigned int recIdx = (hmz - r.z1) * r.GetWidth() + (hmx - r.x1);
			const unsigned int recIdx = hmz * xsize + hmx;

			// don't tesselate map edges when footprint extends across them in IsBlocked*
			const int chmx = Clamp(int(hmx), md->xsizeh, r.x2 - md->xsizeh - 1);
			const int chmz = Clamp(int(hmz), md->zsizeh, r.z2 - md->zsizeh - 1);

			// const float minSpeedMod = (luSpeedMods == nullptr)? CMoveMath::GetPosSpeedMod(*md, hmx, hmz): (*luSpeedMods)[recIdx];
			// const   int maxBlockBit = (luBlockBits == nullptr)? CMoveMath::IsBlockedNoSpeedModCheck(*md, chmx, chmz, nullptr): (*luBlockBits)[recIdx];
			const float minSpeedMod = CMoveMath::GetPosSpeedMod(*md, hmx, hmz);
			// const   int maxBlockBit = CMoveMath::IsBlockedNoSpeedModCheck(*md, chmx, chmz, nullptr);
			const int maxBlockBit = rangeIsBlocked(*md, chmx, chmz);

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

			#define NL QTPFS::NodeLayer
			const float tmpAbsSpeedMod = Clamp(minSpeedMod, NL::MIN_SPEEDMOD_VALUE, NL::MAX_SPEEDMOD_VALUE);
			const float newAbsSpeedMod = tmpAbsSpeedMod * ((maxBlockBit & CMoveMath::BLOCK_STRUCTURE) == 0);
			const float newRelSpeedMod = Clamp((newAbsSpeedMod - NL::MIN_SPEEDMOD_VALUE) / (NL::MAX_SPEEDMOD_VALUE - NL::MIN_SPEEDMOD_VALUE), 0.0f, 1.0f);
			// const float curRelSpeedMod = Clamp(curSpeedMods[sqrIdx] / float(MaxSpeedModTypeValue()), 0.0f, 1.0f);
			#undef NL

			const SpeedBinType newSpeedModBin = GetSpeedModBin(newAbsSpeedMod, newRelSpeedMod);
			// const SpeedBinType curSpeedModBin = curSpeedBins[sqrIdx];

			// numNewBinSquares += int(newSpeedModBin != curSpeedModBin);
			numClosedSquares += int(newSpeedModBin == QTPFS::NodeLayer::NUM_SPEEDMOD_BINS);

			// need to keep track of these for Tesselate
			// oldSpeedMods[sqrIdx] = curRelSpeedMod * float(MaxSpeedModTypeValue());
			curSpeedMods[recIdx] = newRelSpeedMod * float(MaxSpeedModTypeValue());

			// oldSpeedBins[sqrIdx] = curSpeedModBin;
			curSpeedBins[recIdx] = newSpeedModBin;

			// if (globalUpdate && newRelSpeedMod > 0.0f) {
			// 	// only count open squares toward the maximum and average
			// 	maxRelSpeedMod  = std::max(maxRelSpeedMod, newRelSpeedMod);
			// 	avgRelSpeedMod += newRelSpeedMod;
			// }
		}
	}

	// if at least one open square, set the new average
	// if (globalUpdate && maxRelSpeedMod > 0.0f)
	// 	avgRelSpeedMod /= ((xsize * zsize) - numClosedSquares);

	// if at least one square changed bin, we need to re-tesselate
	// all nodes in the subtree of the deepest-level node that fully
	// contains <r>
	//
	// during initialization of the root this is true for ALL squares,
	// but we might NOT need to split it (ex. if the map is 100% flat)
	// if each square happened to change to the SAME bin
	//
	return true; //(numNewBinSquares > 0);
}

bool QTPFS::NodeLayer::UpdateCoarse(UpdateThreadData& threadData) {
	// Go and setup the Hierachy Nodes
	// get root node
	// request
	// open count/closed count

	return true;
}

static int2 cornerPoints[] =
	{ {0, 2}
	, {1, 2}
	, {0, 3}
	, {1, 3}
	};

QTPFS::NodeLayer::areaQueryResults QTPFS::NodeLayer::GetDataForArea(const SRectangle& areaToSearch) const {
	const int xmin = areaToSearch.x1, xmax = areaToSearch.x2;
	const int zmin = areaToSearch.z1, zmax = areaToSearch.z2;
	const int xcentre = (areaToSearch.x2 - areaToSearch.x1) >> 1;
	const int zcentre = (areaToSearch.z2 - areaToSearch.z1) >> 1;
	int2 centre(xcentre, zcentre);

	areaQueryResults areaQueryResults;
	areaQueryResults.bestNodeScore = std::numeric_limits<uint64_t>::max();
	areaQueryResults.centralLeafNode = nullptr;

	const int rootNodeId = (xmin / rootNodeSize) + (zmin / rootNodeSize)*xRootNodes;

	// call expects area to reside in a single root node.
	assert( rootNodeId == ((xmax - 1) / rootNodeSize) + ((zmax - 1) / rootNodeSize)*xRootNodes );
	assert(rootNodeSize*rootNodeSize*2 < (uint32_t)(-1)>>2);
	assert( rootNodeId < xRootNodes*zRootNodes );

	auto getNodeScore = [centre](const INode* curNode) -> uint64_t {
		int2 midRef(curNode->xmid(), curNode->zmid());
		int midDist = centre.distanceSq(midRef);
		int closestPointDist = midDist;
		int bestIndex = 0;
		for (int i = 0; i < 4; ++i) {
			int2 ref(curNode->point(cornerPoints[i].x), curNode->point(cornerPoints[i].y));
			int dist = centre.distanceSq(ref);
			if (dist < closestPointDist) {
				closestPointDist = dist;
				bestIndex = i;
			}
		}
		return ((uint64_t)closestPointDist << 32) + ((uint64_t)midDist << 2) + (uint64_t)bestIndex;
	};

	std::function<int(const INode*)> searchFunction;
	searchFunction = [this, &areaQueryResults, &searchFunction, getNodeScore, xmin, xmax, zmin, zmax](const INode* curNode) -> int {
		if (curNode->IsLeaf()) {
			// smallestNode = std::min(curNode->xsize(), smallestNode);
			bool nodeOpen = !curNode->AllSquaresImpassable();

			areaQueryResults.closedNodeCount += !nodeOpen;
			areaQueryResults.openNodeCount += nodeOpen;

			// use node corner closest to centre point.
			if (nodeOpen) {
				uint64_t curNodeScore = getNodeScore(curNode);
				if (curNodeScore < areaQueryResults.bestNodeScore) {
					areaQueryResults.bestNodeScore = curNodeScore;
					areaQueryResults.centralLeafNode = curNode;
				}
			}
			return 0;
		}

		for (int i = 0; i < QTNODE_CHILD_COUNT; ++i) {
			int childIndex = curNode->GetChildBaseIndex() + i;
			const INode* childNode = GetPoolNode(childIndex);

			if (xmax <= childNode->xmin()) { continue; }
			if (xmin >= childNode->xmax()) { continue; }
			if (zmax <= childNode->zmin()) { continue; }
			if (zmin >= childNode->zmax()) { continue; }

			searchFunction(childNode);
		}
		return 0;
	};

	searchFunction(GetPoolNode(rootNodeId));

	return areaQueryResults;
}


QTPFS::SpeedBinType QTPFS::NodeLayer::GetSpeedModBin(float absSpeedMod, float relSpeedMod) const {
	// NOTE:
	//     bins N and N+1 are reserved for modifiers <= min and >= max
	//     respectively; blocked squares MUST be in their own category
	const SpeedBinType defBin = NUM_SPEEDMOD_BINS * relSpeedMod;
	const SpeedBinType maxBin = NUM_SPEEDMOD_BINS - 1;

	SpeedBinType speedModBin = Clamp(defBin, static_cast<SpeedBinType>(0), maxBin);

	if (absSpeedMod <= MIN_SPEEDMOD_VALUE) { speedModBin = NUM_SPEEDMOD_BINS + 0; }
	if (absSpeedMod >= MAX_SPEEDMOD_VALUE) { speedModBin = NUM_SPEEDMOD_BINS + 1; }

	return speedModBin;
}


void QTPFS::NodeLayer::ExecNodeNeighborCacheUpdates(const SRectangle& ur, UpdateThreadData& threadData) {
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
		for (int z = nodeArea.z1; z < nodeArea.z2; ++z) {
			int zoff = (z - r.z1) * width;
			for (int x = nodeArea.x1; x < nodeArea.x2; ++x) {
				unsigned int index = zoff + (x - r.x1);
				assert(index < threadData.relinkNodeGrid.size());
				threadData.relinkNodeGrid[index] = curNode;
			}
		}
	});

	// if (GetNodelayer() == 2) {
	// 	LOG("Search Area [%d,%d:%d,%d]", searchArea.x1, searchArea.z1, searchArea.x2, searchArea.z2);
	// }

	// now update the selected nodes
	std::for_each(selectedNodes.begin(), selectedNodes.end(), [this, &threadData](INode* curNode){
		// const int xmin = std::max((int)curNode->xmin() - 1, 0), xmax = std::min((int)curNode->xmax() + 1, mapDims.mapx);
		// const int zmin = std::max((int)curNode->zmin() - 1, 0), zmax = std::min((int)curNode->zmax() + 1, mapDims.mapy);
		// curNode->SetMagicNumber(currMagicNum);
		curNode->UpdateNeighborCache(*this, threadData);
		// UpdateNeighborCache(curNode, 0xfff);
	});
}


void QTPFS::NodeLayer::GetNodesInArea(const SRectangle& areaToSearch, std::vector<INode*>& nodesFound) {
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

QTPFS::INode* QTPFS::NodeLayer::GetNearestNodeInArea(const SRectangle& areaToSearch, int2 referencePoint) {
	openNodes.clear();
	INode* bestNode = nullptr;
	uint64_t bestDistScore = std::numeric_limits<uint64_t>::max();

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

	auto getNodeScore = [referencePoint](const INode* curNode) -> uint64_t {
		int2 midRef(curNode->xmid(), curNode->zmid());
		int midDist = referencePoint.distanceSq(midRef);
		int closestPointDist = midDist;
		int bestIndex = 0;
		for (int i = 0; i < 4; ++i) {
			int2 ref(curNode->point(cornerPoints[i].x), curNode->point(cornerPoints[i].y));
			int dist = referencePoint.distanceSq(ref);
			if (dist < closestPointDist) {
				closestPointDist = dist;
				bestIndex = i;
			}
		}
		return ((uint64_t)closestPointDist << 32) + ((uint64_t)midDist << 2) + (uint64_t)bestIndex;
	};

	while (!openNodes.empty()) {
		INode* curNode = openNodes.back();
		openNodes.pop_back();

		if (curNode->IsLeaf()) {
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

			openNodes.emplace_back(childNode);
		}
	}

	return bestNode;
}

QTPFS::INode* QTPFS::NodeLayer::GetNodeThatEncasesPowerOfTwoArea(const SRectangle& areaToEncase) {
	INode* selectedNode = nullptr;
	// INode* curNode = GetPoolNode(0); // TODO: record width in layer directly !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!111
	// int length = curNode->xsize(); // width/height is forced to be the same.
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
