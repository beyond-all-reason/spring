/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include <cassert>
#include <limits>

#include "PathSearch.h"
#include "Path.h"
#include "PathCache.h"
#include "Map/MapInfo.h"
#include "NodeLayer.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/ModInfo.h"
#include "System/Log/ILog.h"
#include "Game/SelectedUnitsHandler.h"
#include "Sim/Objects/SolidObject.h"

#include "Components/PathSpeedModInfo.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"

#include "Map/ReadMap.h"

// #include "System/Log/ILog.h"

#ifdef QTPFS_TRACE_PATH_SEARCHES
#include "Sim/Misc/GlobalSynced.h"
#endif

#include "System/float3.h"

#include "System/Misc/TracyDefs.h"

int QTPFS::PathSearch::MAP_MAX_NODES_SEARCHED;
float QTPFS::PathSearch::MAP_RELATIVE_MAX_NODES_SEARCHED;

void QTPFS::PathSearch::InitStatic() {
	MAP_MAX_NODES_SEARCHED = std::max(0u, mapInfo->pfs.qtpfs_constants.maxNodesSearched);
	MAP_RELATIVE_MAX_NODES_SEARCHED = std::max(0.f, mapInfo->pfs.qtpfs_constants.maxRelativeNodesSearched);

	LOG("%s: MAP_MAX_NODES_SEARCHED=%i, MAP_RELATIVE_MAX_NODES_SEARCHED=%f", __func__
			, MAP_MAX_NODES_SEARCHED, MAP_RELATIVE_MAX_NODES_SEARCHED);
}


// The bit shift needed for the power of two number that is slightly bigger than the given number.
int GetNextBitShift(int n)
{
	RECOIL_DETAILED_TRACY_ZONE;
    // n = n - 1;
    // while (n & n - 1) {
    //     n = n & n - 1;
    // }
    // return n << 1;
    int c = 0;
	n = (n > 0) ? n - 1 : 0;
    while (n >>= 1) {
        c = c << 1;
    }
    return c + 1;
}

void CopyNodeBoundaries(QTPFS::SearchNode& dest, const QTPFS::INode& src) {
	RECOIL_DETAILED_TRACY_ZONE;
	dest.xmin = src.xmin();
	dest.zmin = src.zmin();
	dest.xmax = src.xmax();
	dest.zmax = src.zmax();
}

template<typename T>
void AssertPointIsOnNodeEdge(const float3& tmpPoint, const T* tmpNode) {
	#ifndef NDEBUG
	if (int(tmpPoint.x/SQUARE_SIZE) == tmpNode->xmin || int(tmpPoint.x/SQUARE_SIZE) == tmpNode->xmax)
		assert(int(tmpPoint.z/SQUARE_SIZE) >= tmpNode->zmin && int(tmpPoint.z/SQUARE_SIZE) <= tmpNode->zmax);
	else if (int(tmpPoint.z/SQUARE_SIZE) == tmpNode->zmin || int(tmpPoint.z/SQUARE_SIZE) == tmpNode->zmax)
		assert(int(tmpPoint.x/SQUARE_SIZE) >= tmpNode->xmin && int(tmpPoint.x/SQUARE_SIZE) <= tmpNode->xmax);
	else
		assert(false);
	#endif
}

void QTPFS::PathSearch::Initialize(
	NodeLayer* layer,
	const float3& sourcePoint,
	const float3& targetPoint,
	const CSolidObject* owner
) {
	RECOIL_DETAILED_TRACY_ZONE;
	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	fwd.srcPoint = sourcePoint; fwd.srcPoint.ClampInBounds(); fwd.srcPoint.y = 0.f;
	fwd.tgtPoint = targetPoint; fwd.tgtPoint.ClampInBounds(); fwd.tgtPoint.y = 0.f;

	goalPos = fwd.tgtPoint;

	assert(	fwd.srcPoint.x != 0.f || fwd.srcPoint.z != 0.f );

	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];
	bwd.srcPoint = fwd.tgtPoint;
	bwd.tgtPoint = fwd.srcPoint;

	pathOwner = owner;
	nodeLayer = layer;
	searchExec = nullptr;

	// if (/*(searchID == 7340095 || searchID == 10485810)*/ /*&&*/ pathOwner != nullptr && pathOwner->id == 30680){
	// 	LOG("%s: pathOwner=%d searchID=%d bwd.tgtPoint (%f, %f, %f) fwd.tgtPoint (%f, %f, %f)", __func__
	// 			, (pathOwner != nullptr) ? pathOwner->id : -1, searchID
	// 			, bwd.tgtPoint.x, bwd.tgtPoint.y, bwd.tgtPoint.z
	// 			, fwd.tgtPoint.x, fwd.tgtPoint.y, fwd.tgtPoint.z
	// 			);
	// }

	// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
	// 	LOG("%s: owner %d (%f,%f,%f)", __func__
	// 		, owner->id, owner->pos.x, owner->pos.y, owner->pos.z
	// 		);
	// }

	const uint32_t srcX = fwd.srcPoint.x / SQUARE_SIZE;
	const uint32_t srcZ = fwd.srcPoint.z / SQUARE_SIZE;
	const uint32_t tgtX = fwd.tgtPoint.x / SQUARE_SIZE;
	const uint32_t tgtZ = fwd.tgtPoint.z / SQUARE_SIZE;

	INode* srcNode = nodeLayer->GetNode(srcX, srcZ);
	INode* tgtNode = nodeLayer->GetNode(tgtX, tgtZ);

	assert(fwd.srcPoint.x / SQUARE_SIZE >= 0);
	assert(fwd.srcPoint.z / SQUARE_SIZE >= 0);
	assert(fwd.srcPoint.x / SQUARE_SIZE < mapDims.mapx);
	assert(fwd.srcPoint.z / SQUARE_SIZE < mapDims.mapy);

	pathSearchHash = GenerateHash(srcNode, tgtNode);
	pathPartialSearchHash = GenerateVirtualHash(srcNode, tgtNode);

	doPartialSearch = false;
	pathRequestWaiting = false;
	rejectPartialSearch = false;

	fwdNodesSearched = 0;
	bwdNodesSearched = 0;
}

// #pragma GCC push_options
// #pragma GCC optimize ("O0")

void QTPFS::PathSearch::InitializeThread(SearchThreadData* threadData) {
	ZoneScoped;
	searchThreadData = threadData;

	badGoal = false;

	// add 2 just in case the start and end nodes are closed. They can escape those nodes and check
	// all the open nodes. No more is required because nodes don't link themselves to closed nodes.
	searchThreadData->Init(nodeLayer->GetMaxNodesAlloced(), nodeLayer->GetNumOpenNodes() + 2);

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

	constexpr auto isRemainingPathLongEnoughForRetrigger = [](const QTPFS::IPath *pathToRepair) {
		const uint32_t firstCleanPointId = pathToRepair->GetFirstNodeIdOfCleanPath();
		const uint32_t pointCount = pathToRepair->GetGoodNodeCount() + 1;
		const float3& prevPoint = pathToRepair->GetPoint(firstCleanPointId);
		uint32_t pathLength = 0;
		for (auto i = firstCleanPointId + 1; i < pointCount; ++i) {
			const float3& nextPoint = pathToRepair->GetPoint(i);
			pathLength += nextPoint.distance2D(prevPoint);
			if (pathLength >= modInfo.qtRefreshPathMinDist)
				return true;
		}
		return false;
	};

	auto *pathToRepair = ( tryPathRepair && registry.valid(entt::entity(searchID)) )
			? registry.try_get<IPath>(entt::entity(searchID)) : nullptr;

	// Path repairs need only search to the point of finding the beginning of the renaming clean part of the old path.
	// Such searches are also restricted in the area they can search to avoid creating poor paths that would be better
	// off being recreated from scratch.
	doPathRepair = tryPathRepair
			&& (rawPathCheck == false)
			&& (pathToRepair != nullptr)
			&& (pathToRepair->IsRawPath() == false)
			&& (pathToRepair->GetFirstNodeIdOfCleanPath() > 0)
			&& (pathToRepair->GetFirstNodeIdOfCleanPath() < (pathToRepair->GetGoodNodeCount() - 1) )
			&& (pathToRepair->IsFullPath()
					|| pathToRepair->GetRepathTriggerIndex() == 0
					|| isRemainingPathLongEnoughForRetrigger(pathToRepair) );

	auto getTgtNode = [this, &fwd, pathToRepair](bool doPathRepair) {
		if (doPathRepair) {
			int firstCleanNodeId = pathToRepair->GetFirstNodeIdOfCleanPath();
			const QTPFS::IPath::PathNodeData& firstCleanNode = pathToRepair->GetNode(firstCleanNodeId);

			// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
			// if (pathOwner != nullptr && pathOwner->id == 1502) {
			// 	LOG("%s: goodRecord %d [%x] g=%d (%d,%d)-(%d,%d) [%f,%f] firstCleanNode=%d of %d", __func__
			// 		, firstCleanNode.nodeId, firstCleanNode.nodeNumber
			// 		, int(!firstCleanNode.IsNodeBad())
			// 		, firstCleanNode.xmin, firstCleanNode.zmin, firstCleanNode.xmax, firstCleanNode.zmax
			// 		, firstCleanNode.netPoint.x, firstCleanNode.netPoint.y
			// 		, pathToRepair->GetFirstNodeIdOfCleanPath()
			// 		, pathToRepair->GetGoodNodeCount()
			// 		);
			// }

			auto curNode = nodeLayer->GetPoolNode(firstCleanNode.nodeId);
			assert(curNode->xmin() == firstCleanNode.xmin);
			assert(curNode->xmax() == firstCleanNode.xmax);
			assert(curNode->zmin() == firstCleanNode.zmin);
			assert(curNode->zmax() == firstCleanNode.zmax);

			return curNode;
		}
		return nodeLayer->GetNode(fwd.tgtPoint.x / SQUARE_SIZE, fwd.tgtPoint.z / SQUARE_SIZE);
	};

	INode* srcNode = nodeLayer->GetNode(fwd.srcPoint.x / SQUARE_SIZE, fwd.srcPoint.z / SQUARE_SIZE);
	INode* tgtNode = getTgtNode(doPathRepair);

	// This shouldn't happen, but if it does for some reason, fall-back to full pathing.
	if (doPathRepair && tgtNode->AllSquaresImpassable()){
		assert(false);
		tgtNode = nodeLayer->GetNode(fwd.tgtPoint.x / SQUARE_SIZE, fwd.tgtPoint.z / SQUARE_SIZE);
		doPathRepair = false;
	}

	const bool moveTargetOutOfExitOnly = ( tgtNode->IsExitOnly() && (!srcNode->IsExitOnly()) );

	if (moveTargetOutOfExitOnly || tgtNode->AllSquaresImpassable()) {
		// find nearest acceptable node because this will otherwise trigger a full walk of every pathable node.
		int searchWidth = std::max(int(goalDistance)/SQUARE_SIZE, 16);
		INode* altTgtNode = nodeLayer->GetNearestNodeInArea
			( SRectangle
					( std::max(int(tgtNode->xmin()) - searchWidth, 0)
					, std::max(int(tgtNode->zmin()) - searchWidth, 0)
					, std::min(int(tgtNode->xmax()) + searchWidth, mapDims.mapx)
					, std::min(int(tgtNode->zmax()) + searchWidth, mapDims.mapy)
					)
			, int2(fwd.tgtPoint.x / SQUARE_SIZE, fwd.tgtPoint.z / SQUARE_SIZE)
			, searchThreadData->tmpNodesStore
		);
		if (altTgtNode != nullptr) {
			tgtNode = altTgtNode;
			badGoal = true;
		}
	}

	fwd.srcSearchNode = &searchThreadData->allSearchedNodes[SearchThreadData::SEARCH_FORWARD].InsertINode(srcNode->GetIndex());
	fwd.tgtSearchNode = &searchThreadData->allSearchedNodes[SearchThreadData::SEARCH_FORWARD].InsertINodeIfNotPresent(tgtNode->GetIndex());

	// note src and tgt are swapped when searching backwards.
	bwd.srcSearchNode = &searchThreadData->allSearchedNodes[SearchThreadData::SEARCH_BACKWARD].InsertINode(tgtNode->GetIndex());
	bwd.tgtSearchNode = &searchThreadData->allSearchedNodes[SearchThreadData::SEARCH_BACKWARD].InsertINodeIfNotPresent(srcNode->GetIndex());

	assert(fwd.srcSearchNode != nullptr);
	assert(bwd.srcSearchNode != nullptr);

	fwd.srcSearchNode->nodeNumber = srcNode->GetNodeNumber();
	fwd.tgtSearchNode->nodeNumber = tgtNode->GetNodeNumber();

	bwd.srcSearchNode->nodeNumber = tgtNode->GetNodeNumber();
	bwd.tgtSearchNode->nodeNumber = srcNode->GetNodeNumber();

	for (int i=0; i<SearchThreadData::SEARCH_DIRECTIONS; ++i) {
		auto& data = directionalSearchData[i];
		data.openNodes = &searchThreadData->openNodes[i];
		data.minSearchNode = data.srcSearchNode;

		while (!data.openNodes->empty())
			data.openNodes->pop();
	}

	// Set search boundaries for path repairs. If a repair cannot be made within the boundaries then the path is better
	// off being fully rebuilt.
	if (doPathRepair) {
		int firstCleanNodeId = pathToRepair->GetFirstNodeIdOfCleanPath();
		fwd.tgtPoint = pathToRepair->GetPoint(firstCleanNodeId);

		const float3 diff = (fwd.tgtPoint  - fwd.srcPoint) / 2.f;
		const float3 midPoint = fwd.srcPoint + diff;
		const float searchHalfWidth = std::max(abs(diff.x), abs(diff.z));

		searchLimitMins = midPoint - searchHalfWidth;
		searchLimitMaxs = midPoint + searchHalfWidth;

		// if ((searchID == 7340095 || searchID == 10485810))
		// LOG("%s: tgtPoint=(%f,%f,%f) fwd.srcPoint=(%f,%f,%f)", __func__
		// 		, fwd.tgtPoint.x, fwd.tgtPoint.y, fwd.tgtPoint.z
		// 		, fwd.srcPoint.x, fwd.srcPoint.y, fwd.srcPoint.z);
	}

	// LOG("%s: searchLimitMins=(%f,%f,%f) searchLimitMaxs=(%f,%f,%f)", __func__
	// 		, searchLimitMins.x, searchLimitMins.y, searchLimitMins.z
	// 		, searchLimitMaxs.x, searchLimitMaxs.y, searchLimitMaxs.z);

	// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
	// 	LOG("%s: revMinNode0 [%d] %d [%x] g=%d (%d,%d)-(%d,%d)", __func__, searchID
	// 		, bwd.minSearchNode->index, bwd.minSearchNode->nodeNumber
	// 		, int(!bwd.minSearchNode->isNodeBad())
	// 		, bwd.minSearchNode->xmin, bwd.minSearchNode->zmin, bwd.minSearchNode->xmax,bwd.minSearchNode->zmax
	// 		);
	// }

	// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
	// 	LOG("%s: srcNode (%d:%x) tgtNode (%d:%x)", __func__
	// 		, srcNode->GetIndex(), srcNode->GetNodeNumber()
	// 		, tgtNode->GetIndex(), tgtNode->GetNodeNumber()
	// 		);
	// }

	curSearchNode = nullptr;
	nextSearchNode = nullptr;
}

// #pragma GCC pop_options

// #pragma GCC push_options
// #pragma GCC optimize ("O0")

void QTPFS::PathSearch::PreLoadNode(uint32_t dir, uint32_t nodeId, uint32_t prevNodeId, const float2& netPoint, uint32_t stepIndex){
	auto& searchNodes = searchThreadData->allSearchedNodes[dir];
	auto& searchData = directionalSearchData[dir];

	SearchNode& searchNode = (nodeId != searchData.srcSearchNode->index && nodeId != searchData.tgtSearchNode->index)
				? searchNodes.InsertINode(nodeId)
				: searchNodes[nodeId];
	SearchNode* prevSearchNode = (prevNodeId != -1) ? &searchNodes.InsertINodeIfNotPresent(prevNodeId) : nullptr;
	// LOG("%s: prevNode %d = %p [%d]", __func__, prevNodeId, prevSearchNode, (prevSearchNode != nullptr) ? prevSearchNode->GetIndex() : 0);
	searchNode.SetPrevNode(prevSearchNode);
	searchNode.SetNeighborEdgeTransitionPoint(netPoint);
	searchNode.SetStepIndex(stepIndex);

	auto* curNode = nodeLayer->GetPoolNode(nodeId);
	CopyNodeBoundaries(searchNode, *curNode);
	searchNode.nodeNumber = curNode->GetNodeNumber();
	searchNode.badNode = (stepIndex == 999999);

	// if ((searchID == 7340095 || searchID == 10485810))
	// 	LOG("Add node in advance %d linked to %d : [%d,%d][%d,%d]"
	// 		, nodeId, prevNodeId, searchNode.xmin, searchNode.zmin, searchNode.xmax, searchNode.zmax);

	#ifndef NDEBUG
	if (prevNodeId != -1) {
		INode* nn0 = nodeLayer->GetPoolNode(prevNodeId);
		INode* nn1 = nodeLayer->GetPoolNode(nodeId);
		
		assert(nn1->GetNeighborRelation(nn0) != 0);
		assert(nn0->GetNeighborRelation(nn1) != 0);

		float3 tmpPoint(netPoint.x, 0.f, netPoint.y);
		AssertPointIsOnNodeEdge(tmpPoint, &searchNode);
		AssertPointIsOnNodeEdge(tmpPoint, prevSearchNode);
	}
	#endif
}

void QTPFS::PathSearch::LoadPartialPath(IPath* path) {
	ZoneScoped;
	auto& nodes = path->GetNodeList();

	assert(path->GetPathType() == pathType);

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	searchEarlyDrop = false;

	uint32_t badNodeCount = 0;
	{
		uint32_t stepIndex = 1;
		uint32_t prevNodeId = -1;
		uint32_t badPrevNodeId = -1;
		std::for_each(nodes.begin(), nodes.end(), [this, path, &prevNodeId, &badPrevNodeId, &stepIndex, &badNodeCount](const IPath::PathNodeData& node){
			#ifndef NDEBUG
			// a failure here is likely due to a path not being marked as dirty when quads in the path have been damaged.
			INode* nn1 = nodeLayer->GetPoolNode(node.nodeId);
			// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
			// 	LOG("%s: node bwd check %d [%x] g=%d (%d,%d)-(%d,%d) vs [%x] (%d,%d)-(%d,%d) [%f,%f]", __func__
			// 		, node.nodeId, node.nodeNumber, int(!node.IsNodeBad())
			// 		, node.xmin, node.zmin, node.xmax, node.zmax
			// 		, nn1->GetNodeNumber()
			// 		, nn1->xmin(), nn1->zmin(), nn1->xmax(), nn1->zmax()
			// 		, node.netPoint.x, node.netPoint.y
			// 		);
			// }
			assert(nn1->xmin() == node.xmin);
			assert(nn1->xmax() == node.xmax);
			assert(nn1->zmin() == node.zmin);
			assert(nn1->zmax() == node.zmax);
			#endif
			if (!node.IsNodeBad()) {
				PreLoadNode(SearchThreadData::SEARCH_FORWARD, node.nodeId, prevNodeId, node.netPoint, stepIndex++);
				prevNodeId = node.nodeId;
			} else {
				// mark node being on an incomplete route and won't link back to forward src.
				// used by reverse partial search to drop out early. Set stepindex to non-zero so
				// it can't be confused with a genuine search step by the forward searcher.
				PreLoadNode(SearchThreadData::SEARCH_FORWARD, node.nodeId, badPrevNodeId, node.netPoint, 999999);
				badPrevNodeId = node.nodeId;
				badNodeCount++;
			}
		});
	}
	{
		uint32_t stepIndex = nodes.size() - badNodeCount;
		float2 prevNetPoint;
		uint32_t prevNodeId = -1;
		std::for_each(nodes.rbegin(), nodes.rend(), [this, &prevNodeId, &prevNetPoint, &stepIndex](const IPath::PathNodeData& node){
			// INode* nn1 = nodeLayer->GetPoolNode(node.nodeId);
			// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
			// 	LOG("%s: node fwd check %d [%x] g=%d (%d,%d)-(%d,%d) vs [%x] (%d,%d)-(%d,%d) [%f,%f]", __func__
			// 		, node.nodeId, node.nodeNumber, int(!node.IsNodeBad())
			// 		, node.xmin, node.zmin, node.xmax, node.zmax
			// 		, nn1->GetNodeNumber()
			// 		, nn1->xmin(), nn1->zmin(), nn1->xmax(), nn1->zmax()
			// 		, node.netPoint.x, node.netPoint.y
			// 		);
			// }
			
			if (!node.IsNodeBad()) {
				PreLoadNode(SearchThreadData::SEARCH_BACKWARD, node.nodeId, prevNodeId, prevNetPoint, stepIndex--);
				prevNodeId = node.nodeId;
				prevNetPoint = node.netPoint;
			}
			// we don't need to set incomplete route step indices because the reverse path
			// explicitly doesn't capture the step index if it hits an early drop out.
		});
	}
	expectIncompletePartialSearch = (badNodeCount > 0);
}

void QTPFS::PathSearch::LoadRepairPath() {
	const auto *pathToRepair = registry.try_get<IPath>(entt::entity(searchID));

	const uint32_t firstCleanNodeId = pathToRepair->GetFirstNodeIdOfCleanPath();
	const uint32_t nodeCount = pathToRepair->GetGoodNodeCount();

	// The +1 is because the bwd.srcSearchNode will be the firstCleanNodeId node. So start from the next one on.
	uint32_t stepIndex = nodeCount - (firstCleanNodeId + 1);
	uint32_t prevNodeId = -1;
	float2 prevNetPoint = {goalPos.x, goalPos.z};

	// LOG("%s: nodeCount=%d firstCleanNodeId=%d", __func__, nodeCount, firstCleanNodeId);

	// Only preload the reverse search results. We don't want to preload in the forward search results because the
	// reverse search may think it has connected with the forward search and create a path loop.
	// The firstCleanNodeId node should have step 0 so as to avoid the reverse search refusing to search, due to the
	// repair-special check where it tries to avoid damaging any preloaded nodes.
	for (auto i = nodeCount - 1; i >= firstCleanNodeId; --i) {
		const QTPFS::IPath::PathNodeData& node = pathToRepair->GetNode(i);

			#ifndef NDEBUG
			// a failure here is likely due to a path not being marked as dirty when quads in the path have been damaged.
			INode* nn1 = nodeLayer->GetPoolNode(node.nodeId);
			// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
			// 	LOG("%s: node repair bwd check %d [%x] g=%d (%d,%d)-(%d,%d) vs [%x] (%d,%d)-(%d,%d) [%f,%f] prevNodeId=%d stepIndex=%d", __func__
			// 		, node.nodeId, node.nodeNumber, int(!node.IsNodeBad())
			// 		, node.xmin, node.zmin, node.xmax, node.zmax
			// 		, nn1->GetNodeNumber()
			// 		, nn1->xmin(), nn1->zmin(), nn1->xmax(), nn1->zmax()
			// 		, node.netPoint.x / SQUARE_SIZE, node.netPoint.y / SQUARE_SIZE
			// 		, prevNodeId
			// 		, stepIndex
			// 		);
			// }
			assert(nn1->xmin() == node.xmin);
			assert(nn1->xmax() == node.xmax);
			assert(nn1->zmin() == node.zmin);
			assert(nn1->zmax() == node.zmax);

			if (prevNodeId != -1) {
				INode* nn0 = nodeLayer->GetPoolNode(prevNodeId);
				assert(nn1->GetNeighborRelation(nn0) != 0);
				assert(nn0->GetNeighborRelation(nn1) != 0);
			}
			#endif

		PreLoadNode(SearchThreadData::SEARCH_BACKWARD, node.nodeId, prevNodeId, prevNetPoint, stepIndex--);
		prevNodeId = node.nodeId;
		prevNetPoint = node.netPoint;
	}

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	auto& fwdSearchNodes = searchThreadData->allSearchedNodes[SearchThreadData::SEARCH_FORWARD];
	
	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];
	auto& bwdSearchNodes = searchThreadData->allSearchedNodes[SearchThreadData::SEARCH_BACKWARD];

	int nodeId = pathToRepair->GetNode(nodeCount - 1).nodeId;
	assert( bwdSearchNodes.isSet(nodeId) );

	SearchNode& searchNode = bwdSearchNodes[nodeId];
	bwd.repairPathRealSrcSearchNode = &searchNode;

	// This should always be true.
	if (bwdSearchNodes.isSet(fwd.srcSearchNode->GetIndex())) {
		const QTPFS::SearchNode& bwdNode = bwdSearchNodes[fwd.srcSearchNode->index];

		// Check whether the repair is starting on the clean path.
		if (bwdNode.GetStepIndex() > 0) {
			const float2& edgePoint = bwdNode.GetNeighborEdgeTransitionPoint();
			const float3 newTgtPoint{edgePoint.x, 0.f, edgePoint.y};

			fwd.tgtPoint = goalPos;
			fwd.tgtSearchNode = fwd.srcSearchNode;
			bwd.tgtPoint = newTgtPoint;
			bwd.tgtSearchNode = bwdNode.prevNode;
			bwd.srcSearchNode = bwd.repairPathRealSrcSearchNode;

		// if ((searchID == 7340095 || searchID == 10485810))
		// LOG("%s: tgtPoint=(%f,%f,%f) fwd.srcPoint=(%f,%f,%f)", __func__
		// 		, fwd.tgtPoint.x, fwd.tgtPoint.y, fwd.tgtPoint.z
		// 		, fwd.srcPoint.x, fwd.srcPoint.y, fwd.srcPoint.z);
		}
	}
}

// #pragma GCC pop_options

bool QTPFS::PathSearch::Execute(unsigned int searchStateOffset) {
	RECOIL_DETAILED_TRACY_ZONE;
	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

	haveFullPath = (fwd.srcSearchNode == fwd.tgtSearchNode);
	havePartPath = false;

	// early-out
	if (haveFullPath) {
		// Ensure the node data is pulled
		if ( !rawPathCheck ) {
			{
			auto* curNode = nodeLayer->GetPoolNode(fwd.srcSearchNode->GetIndex());
			InitSearchNodeData(fwd.srcSearchNode, curNode);
			}
			{ // bwd node may not be the same as fwd if path repair is on
			auto* curNode = nodeLayer->GetPoolNode(bwd.srcSearchNode->GetIndex());
			InitSearchNodeData(bwd.srcSearchNode, curNode);
			}
		}
		return true;
	}

	if (rawPathCheck)
		return ExecuteRawSearch();

	return ExecutePathSearch();
}

void QTPFS::PathSearch::InitStartingSearchNodes() {
	RECOIL_DETAILED_TRACY_ZONE;
	fwdPathConnected = false;
	bwdPathConnected = false;

	// max nodes is split between forward and reverse search.
	if (synced){
		float relativeModifier = std::max(MAP_RELATIVE_MAX_NODES_SEARCHED, modInfo.qtMaxNodesSearchedRelativeToMapOpenNodes);
		int relativeLimit = nodeLayer->GetNumOpenNodes() * relativeModifier;
		int absoluteLimit = std::max(MAP_MAX_NODES_SEARCHED, modInfo.qtMaxNodesSearched);

		fwdNodeSearchLimit = std::max(absoluteLimit, relativeLimit) >> 1;
	} else {
		fwdNodeSearchLimit = std::numeric_limits<int>::max();
	}

	searchThreadData->ResetQueue();

	for (int i = 0; i < QTPFS::SEARCH_DIRS; ++i) {
		auto& data = directionalSearchData[i];

		// Path repair holds the reverse source node in a different place. We use a nearer node to speed up the search
		// so that we are only search for a path in the damaged area, rather than the whole map.
		auto* srcNode = data.srcSearchNode;

		// In a path repair the backwards start point is linked to the rest of the original path that we want to keep.
		// Don't clear the previous node otherwise the clean/kept path won't be unwind.
		bool keepPrevNode = (doPathRepair && i == SearchThreadData::SEARCH_BACKWARD);
		QTPFS::SearchNode *prevNode = (keepPrevNode) ? srcNode->GetPrevNode() : nullptr;
		float3 srcPoint = (keepPrevNode)
			? float3(srcNode->GetNeighborEdgeTransitionPoint().x, 0.f, srcNode->GetNeighborEdgeTransitionPoint().y)
			: data.srcPoint;

		ResetState(srcNode, data, srcPoint);
		UpdateNode(srcNode, prevNode, 0);

		auto* curNode = nodeLayer->GetPoolNode(data.srcSearchNode->GetIndex());
		CopyNodeBoundaries(*data.srcSearchNode, *curNode);
	}
}

void QTPFS::PathSearch::UpdateHcostMult() {
	RECOIL_DETAILED_TRACY_ZONE;
	auto& comp = systemGlobals.GetSystemComponent<PathSpeedModInfoSystemComponent>();

	// be as optimistic as possible: assume the remainder of our path will
	// cover only flat terrain with maximum speed-modifier between nxtPoint
	// and tgtPoint
	switch (searchType) {
		// This, by default, guarantees the best path, but underestimates distance costs considerabily.
		// Searching more quads than necessary is an impact to performance. So a slight increase to the
		// hcost can be applied to help. Care needs to be taken because if it goes too far, then
		// performance can drop further instead as the search shifts to best-first-search.
		case PATH_SEARCH_ASTAR:
			{
				const auto& speedModInfo = comp.relSpeedModinfos[nodeLayer->GetNodelayer()];
				hCostMult = 1.0f / speedModInfo.max;
			}
			break;
		case PATH_SEARCH_DIJKSTRA:
			hCostMult = 0.0f;
			break;
	}

	// Path repair is special in that we are fully linking paths, no early out is permitted.
	adjustedGoalDistance = (doPathRepair) ? -1.f : goalDistance * hCostMult;
}

void QTPFS::PathSearch::RemoveOutdatedOpenNodesFromQueue(int searchDir) {
	RECOIL_DETAILED_TRACY_ZONE;
	// Remove any out-of-date node entries in the queue.
	DirectionalSearchData& searchData = directionalSearchData[searchDir];

	while (!(*searchData.openNodes).empty()) {
		SearchQueueNode curOpenNode = (*searchData.openNodes).top();
		assert(searchThreadData->allSearchedNodes[searchDir].isSet(curOpenNode.nodeIndex));
		curSearchNode = &searchThreadData->allSearchedNodes[searchDir][curOpenNode.nodeIndex];
		
		// Check if this node entity is valid
		if (curOpenNode.heapPriority <= curSearchNode->GetHeapPriority())
			break;

		// remove the entry
		(*searchData.openNodes).pop();
	}
}

bool QTPFS::PathSearch::IsNodeActive(const SearchNode& curSearchNode) const {
	RECOIL_DETAILED_TRACY_ZONE;
	// prevNode catches all cases except the starting node.
	// Path H-cost will avoid reporting unwalked target node as a false positive, but catch starting node.
	// Step index check ensures even the initial node of a partial-share path is picked up. The first node fails the
	// first two checks.
	return (curSearchNode.GetPrevNode() != nullptr)
		|| (curSearchNode.GetPathCost(NODE_PATH_COST_H) != std::numeric_limits<float>::infinity())
		|| (curSearchNode.GetStepIndex() > 0);
}

static float CircularEaseOut(float t) {
	RECOIL_DETAILED_TRACY_ZONE;
	// Only using 0-1 range, the sqrt is too intense early on.
	return /*math::sqrt(*/ 1 - Square(t-1); //);
}

void QTPFS::PathSearch::SetForwardSearchLimit() {
	RECOIL_DETAILED_TRACY_ZONE;
	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

	/* These values have been chosen by testing and analysis. They give a reasonable starting point
	 * balancing performance gains against the chance that a poor path will result. I suspect these
	 * can be improved, but this will need further stress testing.
	 * 
	 * TODO: make into mod rules so that games can calibrate them.
	 */
	float dist = 0.f;

	if (hCostMult != 0.f) {
		dist = bwd.minSearchNode->GetPathCost(NODE_PATH_COST_H) / hCostMult;
	}
	else {
		dist = fwd.tgtPoint.distance2D(fwd.srcPoint);
	}

	constexpr int minNodesSearched = 256;

	const int limit = modInfo.qtMaxNodesSearched>>1;
	const float maxDist = modInfo.qtMaxNodesSearched;

	const float interp = std::clamp(dist / maxDist, 0.f, 1.f);
	fwdNodeSearchLimit = std::max(minNodesSearched, int(limit * CircularEaseOut(interp)));
}

// #pragma GCC push_options
// #pragma GCC optimize ("O0")

bool QTPFS::PathSearch::ExecutePathSearch() {
	ZoneScoped;

	#ifdef QTPFS_TRACE_PATH_SEARCHES
	searchExec = new PathSearchTrace::Execution(gs->frameNum);
	#endif

	UpdateHcostMult();
	InitStartingSearchNodes();

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	auto& fwdSearchNodes = searchThreadData->allSearchedNodes[SearchThreadData::SEARCH_FORWARD];
	
	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];
	auto& bwdSearchNodes = searchThreadData->allSearchedNodes[SearchThreadData::SEARCH_BACKWARD];

	bool reverseEarlyDrop = false;
	bool continueSearching = !(*fwd.openNodes).empty();

	bool isFullSearch = !doPartialSearch;
	int dirThatFinishedTheSearch = 0;

	disallowNodeRevisit = modInfo.qtLowerQualityPaths;

	auto nodeIsTemp = [](const SearchNode& curSearchNode) {
		return (curSearchNode.GetPathCost(NODE_PATH_COST_H) == std::numeric_limits<float>::infinity());
	};

	auto nodeIsEarlyDrop = [](const SearchNode& curSearchNode) {
		return (curSearchNode.isNodeBad());
	};

	auto reverseTrace = [this](int dir) {
		auto& fwd = directionalSearchData[dir];
		auto& fwdSearchNodes = searchThreadData->allSearchedNodes[dir];
		auto& revSearchNodes = searchThreadData->allSearchedNodes[1 - dir];
		auto* prevFwdNode = fwd.tgtSearchNode;
		// float2 nextTransitionPoint = fwd.tgtSearchNode->selectedNetpoint;
		QTPFS::SearchNode* fwdCurNode = fwd.tgtSearchNode;
		bool expectedNodeIsBad = ((dir == SearchThreadData::SEARCH_BACKWARD) & searchEarlyDrop);

		if (!revSearchNodes.isSet(fwd.tgtSearchNode->GetIndex()))
			return;

		SearchNode* revCurNode = &revSearchNodes[fwd.tgtSearchNode->GetIndex()];
		// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */ && dir == SearchThreadData::SEARCH_BACKWARD){
		// 	LOG("%s: revMinNode-- %d [%x] g=%d (%d,%d)-(%d,%d) [%f,%f]", __func__
		// 		, revCurNode->index, revCurNode->nodeNumber
		// 		, int(!revCurNode->isNodeBad())
		// 		, revCurNode->xmin, revCurNode->zmin, revCurNode->xmax, revCurNode->zmax
		// 		, fwd.tgtSearchNode->selectedNetpoint.x, fwd.tgtSearchNode->selectedNetpoint.y
		// 		);
		// }
		float2 nextTransitionPoint = revCurNode->selectedNetpoint;
		revCurNode = revCurNode->GetPrevNode();
		while (revCurNode != nullptr) {

			if (expectedNodeIsBad && (revCurNode->isNodeBad() == false)) {
				auto& bwd = directionalSearchData[dir - 1];

				// Surprise! We've found a route between forward and back routes! All the reverse route should have
				// been bad nodes, but we've just connected with a good node. Set the search results accordingly.

				fwd.tgtSearchNode = prevFwdNode;
				fwd.minSearchNode = fwd.tgtSearchNode;
				bwd.tgtSearchNode = revCurNode;
				bwd.minSearchNode = bwd.tgtSearchNode;
				const float2& searchTransitionPoint = nextTransitionPoint;

				// we know this is really the reverse search.
				fwd.tgtPoint = float3(searchTransitionPoint.x, 0.f, searchTransitionPoint.y);

				AssertPointIsOnNodeEdge(fwd.tgtPoint, prevFwdNode);
				AssertPointIsOnNodeEdge(fwd.tgtPoint, revCurNode);

				// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
				// 	LOG("%s: [%d] fwd.tgtPoint (%f, %f, %f)", __func__, dir, fwd.tgtPoint.x, fwd.tgtPoint.y, fwd.tgtPoint.z);
				// 	LOG("%s: [%d] bwd.tgtPoint (%f, %f, %f)", __func__, dir, bwd.tgtPoint.x, bwd.tgtPoint.y, bwd.tgtPoint.z);
				// }
				assert(fwd.tgtPoint.x >= 0.f);

				assert(prevFwdNode->index != revCurNode->index);

				haveFullPath = true;
				useFwdPathOnly = false;
				searchEarlyDrop = false;
				//if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */ && dir == SearchThreadData::SEARCH_BACKWARD){
					// LOG("true path found when all other hope is lost.");
				//}
				return;
			}

			assert(expectedNodeIsBad == revCurNode->isNodeBad());

			// The backwards search doesn't have the bad nodes data loaded (so as not to confuse the forward search.)
			// We should make sure that it is loaded if it will now be used.
			// Note: for the first iteration of this loop, this check will always be false, we're starting from a
			// searched node, which are always loaded.
			if (fwdSearchNodes.isSet(revCurNode->GetIndex()) == false) {
				// assert( nextTransitionPoint.x != 0.f );

				fwdCurNode = &fwdSearchNodes.InsertINode(revCurNode->GetIndex());
				fwdCurNode->CopyGeneralNodeData(*revCurNode);
				fwdCurNode->prevNode = prevFwdNode;
				fwdCurNode->selectedNetpoint = nextTransitionPoint;

				// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
				// 	LOG("%s: [%d] revMinNode added %d [%x] g=%d (%d,%d)-(%d,%d) [%f,%f]", __func__, dir
				// 		, revCurNode->index, revCurNode->nodeNumber
				// 		, int(!revCurNode->isNodeBad())
				// 		, revCurNode->xmin, revCurNode->zmin, revCurNode->xmax, revCurNode->zmax
				// 		, fwdCurNode->selectedNetpoint.x, fwdCurNode->selectedNetpoint.y
				// 		);
				// }
			} else {
				fwdCurNode = &fwdSearchNodes[revCurNode->GetIndex()];

				// Can happen for bad path loading - it is present on fwd. It it gets partially loaded on bwd due to
				// being a neighbour, but didn't get fully loaded because it wasn't visited in the bwd search.
				if (fwdCurNode->xmax == 0) {
					// assert( nextTransitionPoint.x != 0.f );
					fwdCurNode->CopyGeneralNodeData(*revCurNode);
					fwdCurNode->prevNode = prevFwdNode;
					fwdCurNode->selectedNetpoint = nextTransitionPoint;
				}

				// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */ && dir == SearchThreadData::SEARCH_BACKWARD){
				// 	LOG("Exists");
				// }
			}
			prevFwdNode = fwdCurNode;
			nextTransitionPoint = revCurNode->selectedNetpoint;

			// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
			// 	LOG("%s: [%d] revMinNode-- %d [%x] g=%d (%d,%d)-(%d,%d)", __func__, dir
			// 		, revCurNode->index, revCurNode->nodeNumber
			// 		, int(!revCurNode->isNodeBad())
			// 		, revCurNode->xmin, revCurNode->zmin, revCurNode->xmax, revCurNode->zmax
			// 		);
			// }

			revCurNode = revCurNode->GetPrevNode();
		}

		// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */ && dir == SearchThreadData::SEARCH_BACKWARD){
		// 	LOG("Switch to: %d", fwdCurNode->GetIndex());
		// }
		fwd.tgtSearchNode = fwdCurNode;//&fwdSearchNodes[revCurNode->GetIndex()];
		fwd.minSearchNode = fwd.tgtSearchNode;

		// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */ && dir == SearchThreadData::SEARCH_BACKWARD){
		// 	LOG("%s: revMinNode5 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
		// 		, fwd.minSearchNode->index, fwd.minSearchNode->nodeNumber
		// 		, int(!fwd.minSearchNode->isNodeBad())
		// 		, fwd.minSearchNode->xmin, fwd.minSearchNode->zmin, fwd.minSearchNode->xmax, fwd.minSearchNode->zmax
		// 		);
		// }
	};

	auto copyNodeBoundary = [this](SearchNode& dest) {
		if (!(dest.xmax > 0 || dest.zmax > 0)) {
			auto* curNode = nodeLayer->GetPoolNode(dest.GetIndex());
			CopyNodeBoundaries(dest, *curNode);
			dest.nodeNumber = curNode->GetNodeNumber();
		}
	};

	if (badGoal) {
		// Move the reverse search start point to the closest point to the goalPos.
		auto* curNode = nodeLayer->GetPoolNode(bwd.srcSearchNode->GetIndex());
		float2 tmpPoint
			{ float(curNode->xmid() * SQUARE_SIZE)
			, float(curNode->zmid() * SQUARE_SIZE)};

		bwd.srcSearchNode->SetNeighborEdgeTransitionPoint(tmpPoint);
		bwd.srcPoint = FindNearestPointOnNodeToGoal(*bwd.srcSearchNode, goalPos);
		bwd.srcSearchNode->SetNeighborEdgeTransitionPoint({bwd.srcPoint.x, bwd.srcPoint.z});
	}

	assert(fwd.srcSearchNode != nullptr);

	// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
	// 	LOG("%s: revTgtNode1 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
	// 		, bwd.tgtSearchNode->index, bwd.tgtSearchNode->nodeNumber
	// 		, int(!bwd.tgtSearchNode->isNodeBad())
	// 		, bwd.tgtSearchNode->xmin, bwd.tgtSearchNode->zmin, bwd.tgtSearchNode->xmax,bwd.tgtSearchNode->zmax
	// 		);
	// 	LOG("%s: revMinNode1 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
	// 		, bwd.minSearchNode->index, bwd.minSearchNode->nodeNumber
	// 		, int(!bwd.minSearchNode->isNodeBad())
	// 		, bwd.minSearchNode->xmin, bwd.minSearchNode->zmin, bwd.minSearchNode->xmax,bwd.minSearchNode->zmax
	// 		);
	// }

	while (continueSearching) {
		if (!(*fwd.openNodes).empty()) {
			fwdNodesSearched++;
			IterateNodes(SearchThreadData::SEARCH_FORWARD);

			// Search area limits are only used when the reverse search has determined that the
			// goal is not reachable. UPDATE: not anymore, they are now always in effect due to
			// several scenarios that impact performance.
			// 1. Maps with huge islands.
			// 2. Players wall off the map in PvE modes, create huge artificial islands.
			if (fwdNodesSearched >= fwdNodeSearchLimit)
				searchThreadData->ResetQueue(SearchThreadData::SEARCH_FORWARD);

			assert(curSearchNode->GetNeighborEdgeTransitionPoint().x != 0.f
				|| curSearchNode->GetNeighborEdgeTransitionPoint().y != 0.f);

			#ifdef QTPFS_TRACE_PATH_SEARCHES
			searchExec->AddIteration(searchIter);
			searchIter.Clear();
			#endif

			if (bwdSearchNodes.isSet(curSearchNode->GetIndex())) {
				SearchNode& bwdNode = bwdSearchNodes[curSearchNode->GetIndex()];
				fwdPathConnected = IsNodeActive(bwdNode);
				if (fwdPathConnected){
					fwdStepIndex = curSearchNode->GetStepIndex();
					copyNodeBoundary(bwdNode);

					// If step Index is 0, then a full path was found. This can happen for partial
					// searches, if the partial path isn't actually close enough.
					if (curSearchNode->GetStepIndex() == 0) { isFullSearch = true; }

					fwd.tgtSearchNode = curSearchNode;
					searchThreadData->ResetQueue(SearchThreadData::SEARCH_FORWARD);
				}

				haveFullPath = (isFullSearch) ? fwdPathConnected : bwdPathConnected & fwdPathConnected;
				if (haveFullPath){
					if (searchEarlyDrop) {
						// Turns out that there was a path after all.
						searchEarlyDrop = false;
					}

					if (curSearchNode->GetPrevNode() == nullptr) {
						// Can only happen with path repairs, because partial paths require both fwd and bwd to set
						// haveFullPath, which means the fwd going first can't get here if the starting node is on a
						// shared path.
						bwd.tgtSearchNode = bwdNode.prevNode;
						fwd.tgtSearchNode = fwd.minSearchNode = curSearchNode;

						const float2& searchTransitionPoint = bwdNode.GetNeighborEdgeTransitionPoint();
						bwd.tgtPoint = float3(searchTransitionPoint.x, 0.f, searchTransitionPoint.y);
					} else {
						fwd.tgtSearchNode = curSearchNode->GetPrevNode();
						bwd.tgtSearchNode = &bwdNode;

						// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
						// 	LOG("%s: revTgtNode5 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
						// 		, bwd.tgtSearchNode->index, bwd.tgtSearchNode->nodeNumber
						// 		, int(!bwd.tgtSearchNode->isNodeBad())
						// 		, bwd.tgtSearchNode->xmin, bwd.tgtSearchNode->zmin, bwd.tgtSearchNode->xmax,bwd.tgtSearchNode->zmax
						// 		);
						// }

						const float2& searchTransitionPoint = curSearchNode->GetNeighborEdgeTransitionPoint();
						bwd.tgtPoint = float3(searchTransitionPoint.x, 0.f, searchTransitionPoint.y);
					}

					AssertPointIsOnNodeEdge(bwd.tgtPoint, fwd.tgtSearchNode);
					AssertPointIsOnNodeEdge(bwd.tgtPoint, bwd.tgtSearchNode);

					searchThreadData->ResetQueue(SearchThreadData::SEARCH_BACKWARD);

					// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
					// 	LOG("%s: bwd.tgtPoint1 (%f, %f, %f)", __func__, bwd.tgtPoint.x, bwd.tgtPoint.y, bwd.tgtPoint.z);
					// }
				}
			} else if (curSearchNode->GetPathCost(NODE_PATH_COST_H) <= adjustedGoalDistance) {
				// if the forward search has got close enough to the goal, then we don't need the
				// reverse search.
				useFwdPathOnly = true;

				// Technically it is a full path.
				haveFullPath = true;
				fwd.tgtSearchNode = curSearchNode;

				// It turns out that the goal isn't bad any more.
				badGoal = false;

				searchThreadData->ResetQueue(SearchThreadData::SEARCH_FORWARD);
				searchThreadData->ResetQueue(SearchThreadData::SEARCH_BACKWARD);
			}
			RemoveOutdatedOpenNodesFromQueue(SearchThreadData::SEARCH_FORWARD);

			// We're done with the forward path and we expect the reverse path to fail so stop it right there.
			if ((*fwd.openNodes).empty() && expectIncompletePartialSearch){
				SetForwardSearchLimit();
			// 	searchEarlyDrop = true;
			// 	//bwd.tgtSearchNode = curSearchNode;
			// 	searchThreadData->ResetQueue(SearchThreadData::SEARCH_BACKWARD);
			}
		}

		if (!(*bwd.openNodes).empty()) {
			bwdNodesSearched++;
			IterateNodes(SearchThreadData::SEARCH_BACKWARD);

			// if (curSearchNode->GetIndex() == 20076 && (searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */ && gs->frameNum == 10213)
			// 	LOG("Whoops!");

			if (bwdNodesSearched >= fwdNodeSearchLimit)
				searchThreadData->ResetQueue(SearchThreadData::SEARCH_BACKWARD);

			assert(curSearchNode->GetNeighborEdgeTransitionPoint().x != 0.f
				|| curSearchNode->GetNeighborEdgeTransitionPoint().y != 0.f);

			#ifdef QTPFS_TRACE_PATH_SEARCHES
			searchExec->AddIteration(searchIter);
			searchIter.Clear();
			#endif

			if (fwdSearchNodes.isSet(curSearchNode->GetIndex())) {
				SearchNode& fwdNode = fwdSearchNodes[curSearchNode->GetIndex()];
				
				searchEarlyDrop = nodeIsEarlyDrop(fwdNode);
				if (!searchEarlyDrop) {
					bwdPathConnected = IsNodeActive(fwdNode);
				}
				if (bwdPathConnected) {
					bwdStepIndex = curSearchNode->GetStepIndex();
					copyNodeBoundary(fwdNode);

					// If step Index is 0, then a full path was found. This can happen for partial
					// searches, if the partial path isn't actually close enough.
					if (curSearchNode->GetStepIndex() == 0) { isFullSearch = true; }
				}
				if (bwdPathConnected || searchEarlyDrop) {
					bwd.tgtSearchNode = curSearchNode;
					searchThreadData->ResetQueue(SearchThreadData::SEARCH_BACKWARD);

					// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
					// 	LOG("%s: revTgtNode6 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
					// 		, bwd.tgtSearchNode->index, bwd.tgtSearchNode->nodeNumber
					// 		, int(!bwd.tgtSearchNode->isNodeBad())
					// 		, bwd.tgtSearchNode->xmin, bwd.tgtSearchNode->zmin, bwd.tgtSearchNode->xmax,bwd.tgtSearchNode->zmax
					// 		);
					// 	LOG("%s: revMinNode6 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
					// 		, bwd.minSearchNode->index, bwd.minSearchNode->nodeNumber
					// 		, int(!bwd.minSearchNode->isNodeBad())
					// 		, bwd.minSearchNode->xmin, bwd.minSearchNode->zmin, bwd.minSearchNode->xmax,bwd.minSearchNode->zmax
					// 		);
					// }
				}

				haveFullPath = (isFullSearch) ? bwdPathConnected : bwdPathConnected & fwdPathConnected;
				if (haveFullPath) {
					assert(!searchEarlyDrop);
					if (curSearchNode->GetPrevNode() == nullptr) {
						useFwdPathOnly = true;
						fwd.tgtSearchNode = &fwdNode;
						// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
						// 	LOG("%s: revTgtNode7 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
						// 		, bwd.tgtSearchNode->index, bwd.tgtSearchNode->nodeNumber
						// 		, int(!bwd.tgtSearchNode->isNodeBad())
						// 		, bwd.tgtSearchNode->xmin, bwd.tgtSearchNode->zmin, bwd.tgtSearchNode->xmax,bwd.tgtSearchNode->zmax
						// 		);
						// }
					} else {
						bwd.tgtSearchNode = curSearchNode->GetPrevNode();
						fwd.tgtSearchNode = &fwdNode;

						// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
						// 	LOG("%s: revTgtNode4 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
						// 		, bwd.tgtSearchNode->index, bwd.tgtSearchNode->nodeNumber
						// 		, int(!bwd.tgtSearchNode->isNodeBad())
						// 		, bwd.tgtSearchNode->xmin, bwd.tgtSearchNode->zmin, bwd.tgtSearchNode->xmax,bwd.tgtSearchNode->zmax
						// 		);
						// }

						const float2& searchTransitionPoint = curSearchNode->GetNeighborEdgeTransitionPoint();
						bwd.tgtPoint = float3(searchTransitionPoint.x, 0.f, searchTransitionPoint.y);

						AssertPointIsOnNodeEdge(bwd.tgtPoint, curSearchNode);
						AssertPointIsOnNodeEdge(bwd.tgtPoint, &fwdNode);

						// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
						// 	LOG("%s: bwd.tgtPoint2 (%f, %f, %f)", __func__, bwd.tgtPoint.x, bwd.tgtPoint.y, bwd.tgtPoint.z);
						// }
					}
					searchThreadData->ResetQueue(SearchThreadData::SEARCH_FORWARD);
				}
			}
			RemoveOutdatedOpenNodesFromQueue(SearchThreadData::SEARCH_BACKWARD);

			// Limit forward search to avoid excessively costly searches when the path cannot be
			// joined. This should be okay for partial searches as well.
			if ((*bwd.openNodes).empty() && !bwdPathConnected)
				SetForwardSearchLimit();
		}

		// stop if forward search is done, even if reverse search can continue. If forward search
		// is done, then no path can be found and we have the nearest node if one is present.
		// Reverse search will have to make do with what is has established for the sake of faster
		// earlier drop out checks in related partial-shared searches. We can't stop if reverse
		// search is done because we need the nearest node, which can only be found by the forward
		// search.
		if (isFullSearch)
			continueSearching = !(*fwd.openNodes).empty();
		else 
			continueSearching = (fwdPathConnected) ? !(*bwd.openNodes).empty() : !(*fwd.openNodes).empty();
	}

	assert(fwd.srcSearchNode != nullptr);

	if (doPathRepair) {
		// Move the backwards source node to where it would be if this was complete path search, rather than a repair.
		bwd.srcSearchNode = bwd.repairPathRealSrcSearchNode;
	
		if (!haveFullPath) {
			// Prevent this request from being processed further.
			haveFullPath = havePartPath = false;

			// This will have the path request rescheduled for the next frame.
			pathRequestWaiting = true;

			// If a complete repair path cannot be found, then abort this search and switch to a full repath.
			return false;
		}
	} 
	
	if (searchEarlyDrop) {
		// move forward only needed for incomplete partial searches,
		// but only if forward was able to find the partially shared path.
		if (fwdPathConnected) {
			reverseTrace(SearchThreadData::SEARCH_FORWARD);
		}
		reverseTrace(SearchThreadData::SEARCH_BACKWARD);
	} else if (doPartialSearch) {
		// Sanity check the path is properly connected. Partial searches can fail this check.
		// If a partial search found a full path instead of connecting to the partial path, then
		// isFullSearch is set to true and a check isn't needed.
		if (haveFullPath) {
			if (!isFullSearch) {
				if (fwdStepIndex > bwdStepIndex){
					haveFullPath = havePartPath = false;
					rejectPartialSearch = true;
					// LOG("%s: rejecting partial path 1 (search %x)", __func__, this->GetID());
					return false;
				}
			}
		} else {
			if (fwdPathConnected) {
				// if the partial path could not connect the reverse path, then we need to reject.
				if (!bwdPathConnected && !expectIncompletePartialSearch) {
					haveFullPath = havePartPath = false;
					rejectPartialSearch = true;
					// LOG("%s: rejecting partial path 2 (search %x)", __func__, this->GetID());
					return false;
				}

				// Connected part path needs to be walked. It is done if searchEarlyDrop == true,
				// but it also needs to be done if the backward search didn't connect.
				reverseTrace(SearchThreadData::SEARCH_FORWARD);
			}
		}
	}

	havePartPath = (fwd.minSearchNode != fwd.srcSearchNode)
				// Normally now, we would count this as a part path to avoid units smashing against
				// walls because elsewhere the pathing cannot fail, but as units can be trapped then
				// allow them to force their movement. The path manager forces paths incase the unit
				// is trapped in a wreck or something.
				|| (!nodeLayer->GetPoolNode(fwd.srcSearchNode->GetIndex())->AllSquaresImpassable());

	#ifdef QTPFS_SUPPORT_PARTIAL_SEARCHES
	// adjust the target-point if we only got a partial result
	// NOTE:
	//   movement code has handles special cases where the last waypoint isn't reachable so there
	//   is no need to change the target point anymore.
	if (!haveFullPath && havePartPath) {
		fwd.tgtSearchNode = fwd.minSearchNode;

		// used to trace a bad path to give partial searches a chance of an early out
		// in reverse searches.
		bwd.tgtSearchNode = bwd.minSearchNode;

		// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
		// 	LOG("%s: revTgtNode3 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
		// 		, bwd.tgtSearchNode->index, bwd.tgtSearchNode->nodeNumber
		// 		, int(!bwd.tgtSearchNode->isNodeBad())
		// 		, bwd.tgtSearchNode->xmin, bwd.tgtSearchNode->zmin, bwd.tgtSearchNode->xmax,bwd.tgtSearchNode->zmax
		// 		);
		// LOG("%s: revMinNode4 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
		// 	, bwd.minSearchNode->index, bwd.minSearchNode->nodeNumber
		// 	, int(!bwd.minSearchNode->isNodeBad())
		// 	, bwd.minSearchNode->xmin, bwd.minSearchNode->zmin, bwd.minSearchNode->xmax,bwd.minSearchNode->zmax
		// 	);
		// }

		// Final position needs to be the closest point on the last quad to the goal.
		fwd.tgtPoint = FindNearestPointOnNodeToGoal(*fwd.tgtSearchNode, goalPos);
	} else if (badGoal) {
		// reverse source point has already been setup as the closest point to the goalPos.
		fwd.tgtPoint = bwd.srcPoint;
	} else if (useFwdPathOnly) {
		fwd.tgtPoint = FindNearestPointOnNodeToGoal(*fwd.tgtSearchNode, goalPos);
	}
	#endif

	// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
	// 	LOG("%s: revTgtNode2 %d [%x] g=%d (%d,%d)-(%d,%d)", __func__
	// 		, bwd.tgtSearchNode->index, bwd.tgtSearchNode->nodeNumber
	// 		, int(!bwd.tgtSearchNode->isNodeBad())
	// 		, bwd.tgtSearchNode->xmin, bwd.tgtSearchNode->zmin, bwd.tgtSearchNode->xmax,bwd.tgtSearchNode->zmax
	// 		);
	// }

	// LOG("%s: search %x result(%d||%d) nodes searched (%d, %d) free nodes (%i)", __func__, this->GetID()
	// 		, int(haveFullPath), int(havePartPath), int(fwdNodesSearched), int(bwdNodesSearched)
	// 		, nodeLayer->GetNumOpenNodes());

	return (haveFullPath || havePartPath);
}

// #pragma GCC pop_options

bool QTPFS::PathSearch::ExecuteRawSearch() {
	ZoneScoped;
	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	int2 nearestSquare;
	haveFullPath = moveDefHandler.GetMoveDefByPathType(nodeLayer->GetNodelayer())
			->DoRawSearch( pathOwner, pathOwner->moveDef, fwd.srcPoint, fwd.tgtPoint, goalDistance
						 , true, true, false, nullptr, nullptr, &nearestSquare, searchThreadData->threadId);

	if (haveFullPath) {
		useFwdPathOnly = true;

		// Check whether the nearest point has to be moved.
		int2 pos2 = (nearestSquare * SQUARE_SIZE) + (SQUARE_SIZE/2);
		float3 pos(pos2.x, 0.f, pos2.y);

		// Slightly higher than cos(45)*8, but smaller than a square width.
		// Position is taken from the mid point of a square so we need a little tolerance to check whether it is the
		// same square.
		if (pos.SqDistance2D(fwd.tgtPoint) > Square((SQUARE_SIZE * 3) / 4))
			fwd.tgtPoint = pos;
	}

	return haveFullPath;
}


void QTPFS::PathSearch::ResetState(SearchNode* node, struct DirectionalSearchData& searchData, const float3& srcPoint) {
	RECOIL_DETAILED_TRACY_ZONE;
	// will be copied into srcNode by UpdateNode()
	netPoints[0] = {srcPoint.x, srcPoint.z};

	gDists[0] = 0.0f;
	hDists[0] = srcPoint.distance(searchData.tgtPoint);
	gCosts[0] = 0.0f;
	hCosts[0] = hDists[0] * hCostMult;

	for (unsigned int i = 1; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
		netPoints[i] = {0.0f, 0.0f};

		gDists[i] = 0.0f;
		hDists[i] = 0.0f;
		gCosts[i] = 0.0f;
		hCosts[i] = 0.0f;
	}

	(*searchData.openNodes).emplace(node->GetIndex(), 0.f);
}

void QTPFS::PathSearch::LocalUpdateNode(SearchNode* nextNode, SearchNode* prevNode, float gCost, float hCost, const float2& netPoint) {
	RECOIL_DETAILED_TRACY_ZONE;
	// NOTE:
	//   the heuristic must never over-estimate the distance,
	//   but this is *impossible* to achieve on a non-regular
	//   grid on which any node only has an average move-cost
	//   associated with it --> paths will be "nearly optimal"
	nextNode->SetPrevNode(prevNode);
	nextNode->SetPathCosts(gCost, hCost);
	nextNode->SetNeighborEdgeTransitionPoint(netPoint);

	#ifndef NDEBUG
	if (prevNode != nullptr) {
		auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
		auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

		if (fwd.srcSearchNode != nextNode && bwd.srcSearchNode != nextNode) {

			float3 tmpPoint = float3(netPoint.x, 0.f, netPoint.y);
			AssertPointIsOnNodeEdge(tmpPoint, prevNode);

			if (nextNode->xmax > 0) {
				AssertPointIsOnNodeEdge(tmpPoint, nextNode);
			}
		}
	}
	#endif
}

void QTPFS::PathSearch::UpdateNode(SearchNode* nextNode, SearchNode* prevNode, unsigned int netPointIdx) {
	RECOIL_DETAILED_TRACY_ZONE;
	// NOTE:
	//   the heuristic must never over-estimate the distance,
	//   but this is *impossible* to achieve on a non-regular
	//   grid on which any node only has an average move-cost
	//   associated with it --> paths will be "nearly optimal"
	nextNode->SetPrevNode(prevNode);
	nextNode->SetPathCosts(gCosts[netPointIdx], hCosts[netPointIdx]);
	nextNode->SetNeighborEdgeTransitionPoint(netPoints[netPointIdx]);

	#ifndef NDEBUG
	if (prevNode != nullptr) {
		auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
		auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

		if (fwd.srcSearchNode != nextNode && bwd.srcSearchNode != nextNode) {

			float3 tmpPoint = float3(netPoints[netPointIdx].x, 0.f, netPoints[netPointIdx].y);
			AssertPointIsOnNodeEdge(tmpPoint, prevNode);

			if (nextNode->xmax > 0) {
				AssertPointIsOnNodeEdge(tmpPoint, nextNode);
			}
		}
	}
	#endif
}

void QTPFS::PathSearch::IterateNodes(unsigned int searchDir) {
	ZoneScoped;
	DirectionalSearchData& searchData = directionalSearchData[searchDir];

	SearchQueueNode curOpenNode = (*searchData.openNodes).top();
	(*searchData.openNodes).pop();

	// if (searchID == 7340095 || searchID == 10485810)
	// 	LOG("%s: [%d] curNode=%d", __func__, searchDir, curOpenNode.nodeIndex);

	curSearchNode = &searchThreadData->allSearchedNodes[searchDir][curOpenNode.nodeIndex];

	#ifdef QTPFS_TRACE_PATH_SEARCHES
	{
	auto* curNode = nodeLayer->GetPoolNode(curOpenNode.nodeIndex);
	searchIter.SetPoppedNodeIdx(curNode->zmin() * mapDims.mapx + curNode->xmin());
	}
	#endif

	// LOG("%s: continuing search from %d to %d", __func__
	// 		, curSearchNode->GetIndex()
	// 		, tgtSearchNode->GetIndex()
	// 		);

	if (curSearchNode->xmax == 0) {
		auto* curNode = nodeLayer->GetPoolNode(curOpenNode.nodeIndex);
		InitSearchNodeData(curSearchNode, curNode);
	}

	// Path repairs are restricted to the area in which they can search.
	if (doPathRepair) {
		// LOG("%s:curSearchNode [%f,%f][%f,%f]", __func__
		// 		, float(curSearchNode->xmin*SQUARE_SIZE), float(curSearchNode->xmax*SQUARE_SIZE)
		// 		, float(curSearchNode->zmin*SQUARE_SIZE), float(curSearchNode->zmax*SQUARE_SIZE));

		if (curSearchNode->xmin*SQUARE_SIZE > searchLimitMaxs.x) { return; }
		if (curSearchNode->xmax*SQUARE_SIZE < searchLimitMins.x) { return; }
		if (curSearchNode->zmin*SQUARE_SIZE > searchLimitMaxs.z) { return; }
		if (curSearchNode->zmax*SQUARE_SIZE < searchLimitMins.z) { return; }
	}

	// Check if we've linked up with the other search
	auto& otherNodes = searchThreadData->allSearchedNodes[1 - searchDir];
	if (otherNodes.isSet(curSearchNode->GetIndex())){
		// Check whether it has been processed yet
		if (IsNodeActive(otherNodes[curSearchNode->GetIndex()])) {
			// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */ /*&& searchDir == SearchThreadData::SEARCH_BACKWARD*/){
			// 	LOG("%s: NodeActive [%d] %d [%x] g=%d (%d,%d)-(%d,%d)", __func__, searchDir
			// 		, searchData.minSearchNode->index, searchData.minSearchNode->nodeNumber
			// 		, int(!searchData.minSearchNode->isNodeBad())
			// 		, searchData.minSearchNode->xmin, searchData.minSearchNode->zmin, searchData.minSearchNode->xmax,searchData.minSearchNode->zmax
			// 		);
			// }
			return;
		}
	}

	#ifdef QTPFS_SUPPORT_PARTIAL_SEARCHES

	// remember the node with lowest h-cost in case the search fails to reach tgtNode
	if (curSearchNode->GetPathCost(NODE_PATH_COST_H) < searchData.minSearchNode->GetPathCost(NODE_PATH_COST_H))
		searchData.minSearchNode = curSearchNode;

	// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */ /*&& searchDir == SearchThreadData::SEARCH_BACKWARD*/){
	// 	LOG("%s: MinNode2  [%d] %d [%x] g=%d (%d,%d)-(%d,%d)", __func__, searchDir
	// 		, searchData.minSearchNode->index, searchData.minSearchNode->nodeNumber
	// 		, int(!searchData.minSearchNode->isNodeBad())
	// 		, searchData.minSearchNode->xmin, searchData.minSearchNode->zmin, searchData.minSearchNode->xmax,searchData.minSearchNode->zmax
	// 		);
	// }

	#endif

	if (curSearchNode->GetPathCost(NODE_PATH_COST_H) <= adjustedGoalDistance)
		return;

	assert(curSearchNode->GetIndex() == curOpenNode.nodeIndex);
	auto* curNode = nodeLayer->GetPoolNode(curOpenNode.nodeIndex);

	// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */ /*&& searchDir == SearchThreadData::SEARCH_BACKWARD*/){
	// 	LOG("%s: MinNode2. [%d] %d [%x] g=%d (%d,%d)-(%d,%d)", __func__, searchDir
	// 		, searchData.minSearchNode->index, searchData.minSearchNode->nodeNumber
	// 		, int(!searchData.minSearchNode->isNodeBad())
	// 		, searchData.minSearchNode->xmin, searchData.minSearchNode->zmin, searchData.minSearchNode->xmax,searchData.minSearchNode->zmax
	// 		);
	// }

	IterateNodeNeighbors(curNode, searchDir);
}

void QTPFS::PathSearch::IterateNodeNeighbors(const INode* curNode, unsigned int searchDir) {
	RECOIL_DETAILED_TRACY_ZONE;
	DirectionalSearchData& searchData = directionalSearchData[searchDir];

	const float2& curPoint2 = curSearchNode->GetNeighborEdgeTransitionPoint();

	// TODO: get rid of the need for this!!
	// Allow units to escape if starting in a closed node - a cost of infinity would prevent them escaping.
	const float curNodeSanitizedCost = curNode->AllSquaresImpassable() ? QTPFS_CLOSED_NODE_COST : curNode->GetMoveCost();

	const std::vector<INode::NeighbourPoints>& nxtNodes = curNode->GetNeighbours();
	for (unsigned int i = 0; i < nxtNodes.size(); i++) {
		// NOTE:
		//   this uses the actual distance that edges of the final path will cover,
		//   from <curPoint> (initialized to sourcePoint) to a position on the edge
		//   shared between <curNode> and <nxtNode>
		//   (each individual path-segment is weighted by the average move-cost of
		//   the node it crosses, which is the reciprocal of the average speed-mod)
		// NOTE:
		//   short paths that should have 3 points (2 nodes) can contain 4 (3 nodes);
		//   this happens when a path takes a "detour" through a corner neighbor of
		//   srcNode if the shared corner vertex is closer to the goal position than
		//   any transition-point on the edge between srcNode and tgtNode
		// NOTE:
		//   H needs to be of the same order as G, otherwise the search reduces to
		//   Dijkstra (if G dominates H) or becomes inadmissable (if H dominates G)
		//   in the first case we would explore many more nodes than necessary (CPU
		//   nightmare), while in the second we would get low-quality paths (player
		//   nightmare)
		int nxtNodesId = nxtNodes[i].nodeId;
		
		// LOG("%s: target node search from %d to %d", __func__
		// 		, curNode->GetIndex()
		// 		, nxtNode->GetIndex()
		// 		);

		nextSearchNode = &searchThreadData->allSearchedNodes[searchDir].InsertINodeIfNotPresent(nxtNodesId);

		// Don't process the node we just came from. We can't improve on the route by doubling back on ourselves.
		if (curSearchNode->GetPrevNode() == nextSearchNode)
			continue;

		// Forbid the reverse search from trampling on it's preloaded nodes.
		if ( (searchDir == SearchThreadData::SEARCH_BACKWARD) && (nextSearchNode->GetStepIndex() > 0) ) {
			continue;
		}

		// if (disallowNodeRevisit) {
		// 	bool alreadyVisited = (nextSearchNode->GetPathCost(NODE_PATH_COST_G) != QTPFS_POSITIVE_INFINITY);
		// 	constexpr bool improvingStart = false;

		// // 	// The start of a path should look good, a unit that starts off by moving in the wrong
		// // 	// direction will look weird and will break commands like guard, which regaularly ask
		// // 	// for path updates. This also cleans up the last bit around the goal due to the
		// // 	// reverse search, which is an added bonus.
		// // 	bool improvingStart = (nextSearchNode == searchData.srcSearchNode)
		// // 						|| (nextSearchNode->GetPrevNode() == searchData.srcSearchNode);
		// 	if (alreadyVisited && !improvingStart) {
		// 		continue;
		// 	}
		// }

		const bool isTarget = (nextSearchNode == searchData.tgtSearchNode);
		QTPFS::INode *nxtNode = nullptr;
		if (isTarget) {
			nxtNode = nodeLayer->GetPoolNode(nxtNodesId);
			assert(curNode->GetNeighborRelation(nxtNode) != 0);
			assert(nxtNode->GetNeighborRelation(curNode) != 0);
		}

		// #if (QTPFS_MAX_NETPOINTS_PER_NODE_EDGE == 1)
		// /*if (!IntersectEdge(curNode, nxtNode, tgtPoint - curPoint))*/ {
		// 	// if only one transition-point is allowed per edge,
		// 	// this will always be the edge's center --> no need
		// 	// to be fancy (note that this is not always the best
		// 	// option, it causes local and global sub-optimalities
		// 	// which SmoothPath can only partially address)
		const float2& netPoint = nxtNodes[i].netpoints[0];

		const float gDist = curPoint2.Distance(netPoint);
		const float hDist = searchData.tgtPoint.distance2D(netPoint);
		float gCost =
			curSearchNode->GetPathCost(NODE_PATH_COST_G) +
			curNodeSanitizedCost * gDist;
		const float hCost = hDist * hCostMult * float(!isTarget);

		if (isTarget) {
			gCost += nxtNode->GetMoveCost() * hDist;
		}

		// }
		// #else
		// examine a number of possible transition-points
		// along the edge between curNode and nxtNode and
		// pick the one that minimizes g+h
		// this fixes a few cases that path-smoothing can
		// not handle; more points means a greater degree
		// of non-cardinality (but gets expensive quickly)
		// unsigned int netPointIdx = 0;

		// for (unsigned int j = 0; j < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; j++) {
		// 	netPoints[j] = nxtNodes[i].netpoints[j];

		// 	gDists[j] = curPoint.distance2D({netPoints[j].x, 0.0f, netPoints[j].y});
		// 	hDists[j] = searchData.tgtPoint.distance2D({netPoints[j].x, 0.0f, netPoints[j].y});

		// 	gCosts[j] =
		// 		curSearchNode->GetPathCost(NODE_PATH_COST_G) +
		// 		curNodeSanitizedCost * gDists[j];
		// 	hCosts[j] = hDists[j] * hCostMult * float(!isTarget);

		// 	if (isTarget) {
		// 		gCosts[j] += nxtNode->GetMoveCost() * hDists[j];
		// 	}

		// 	if ((gCosts[j] + hCosts[j]) < (gCosts[netPointIdx] + hCosts[netPointIdx])) {
		// 		netPointIdx = j;
		// 	}
		// }
		// LOG("%s: [%d] nxtNode=%d gd=%f, hd=%f, gc=%f, hc=%f, fc=%f", __func__, searchDir, nxtNodesId
		// 		, gDists[netPointIdx], hDists[netPointIdx], gCosts[netPointIdx], hCosts[netPointIdx]
		// 		, gCosts[netPointIdx] + hCosts[netPointIdx]);
		// #endif

		// if (!isCurrent) {
		// 	UpdateNode(nextSearchNode, curSearchNode, netPointIdx);

		// 	// (*openNodes).push(nxtNode);
		// 	(*openNodes).emplace(nextSearchNode->GetIndex(), nextSearchNode->GetHeapPriority());

		// 	#ifdef QTPFS_TRACE_PATH_SEARCHES
		// 	searchIter.AddPushedNodeIdx(nxtNode->zmin() * mapDims.mapx + nxtNode->xmin());
		// 	#endif

		// 	continue;
		// }
		// if (gCosts[netPointIdx] >= nextSearchNode->GetPathCost(NODE_PATH_COST_G))
		// 	continue;

		if (gCost >= nextSearchNode->GetPathCost(NODE_PATH_COST_G))
			continue;

		// LOG("%s: [%d] nxtNode=%d updating gc from %f -> %f", __func__, searchDir, nxtNodesId
		// 		, nextSearchNode->GetPathCost(NODE_PATH_COST_G), gCosts[netPointIdx]);

		// 	if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */ /*&& searchDir == SearchThreadData::SEARCH_BACKWARD*/){

		// LOG("%s: [%d] adding node (%d) gcost %f < %f [old p:%f]", __func__, searchDir
		// 		, nextSearchNode->GetIndex()
		// 		, gCosts[netPointIdx]
		// 		, nextSearchNode->GetPathCost(NODE_PATH_COST_G)
		// 		, nextSearchNode->GetHeapPriority()
		// 		);

		// 	}

		// UpdateNode(nextSearchNode, curSearchNode, netPointIdx);
		LocalUpdateNode(nextSearchNode, curSearchNode, gCost, hCost, netPoint);
		(*searchData.openNodes).emplace(nextSearchNode->GetIndex(), nextSearchNode->GetHeapPriority());
	}
}

void QTPFS::PathSearch::Finalize(IPath* path) {
	ZoneScoped;

	// LOG("%s: [%p : %d] Finalize search.", __func__
	// 		, &nodeLayer[path->GetPathType()]
	// 		, path->GetPathType()
	// 		);
	path->SetRepathTriggerIndex(0);
	path->SetIsRawPath(rawPathCheck);
	if (!rawPathCheck) {
		TracePath(path);

		#ifdef QTPFS_SMOOTH_PATHS
		SmoothPath(path);
		#endif
	} else {
		auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

		path->AllocPoints(2);
		path->SetSourcePoint({fwd.srcPoint.x, 0.f, fwd.srcPoint.z});
		path->SetTargetPoint({fwd.tgtPoint.x, 0.f, fwd.tgtPoint.z});
		//path->SetGoalPosition(path->GetTargetPoint());
	}
	path->SetNextPointIndex(0);
	path->SetFirstNodeIdOfCleanPath(0);

	if (!path->IsBoundingBoxOverriden())
		path->SetBoundingBox();

	// Bad path is a special path where we did a full path to the nearest good node instead of
	// doing an exhaustive search to find a route to node we know cannot be found. This means
	// we need to adjust the search results to reflect that - i.e. it shouldn't look like a fully
	// successful path request.
	havePartPath |= (badGoal & haveFullPath);
	haveFullPath = (badGoal == true) ? false : haveFullPath;

	path->SetHasFullPath(haveFullPath);
	path->SetHasPartialPath(havePartPath);
}

void QTPFS::PathSearch::GetRectangleCollisionVolume(const QTPFS::SearchNode& snode, CollisionVolume& v, float3& rm) const {
	RECOIL_DETAILED_TRACY_ZONE;
	float3 vScales;
	
	// We cannot guarantee that the searchNode has been loaded with the quad dimensions.
	const INode* node = nodeLayer->GetPoolNode(snode.GetIndex());

	// rectangle dimensions (WS)
	vScales.x = ((node->xmax() - node->xmin()) * SQUARE_SIZE);
	vScales.z = ((node->zmax() - node->zmin()) * SQUARE_SIZE);
	vScales.y = 1.0f;

	// rectangle mid-point (WS)
	rm.x = ((node->xmax() + node->xmin()) * SQUARE_SIZE) >> 1;
	rm.z = ((node->zmax() + node->zmin()) * SQUARE_SIZE) >> 1;
	rm.y = 0.0f;

	#define CV CollisionVolume
	v.InitShape(vScales, ZeroVector, CV::COLVOL_TYPE_BOX, CV::COLVOL_HITTEST_CONT, CV::COLVOL_AXIS_Y);
	#undef CV
}

float3 QTPFS::PathSearch::FindNearestPointOnNodeToGoal(const QTPFS::SearchNode& node, const float3& goalPos) const {
	RECOIL_DETAILED_TRACY_ZONE;
	CollisionVolume rv;
	CollisionQuery cq;
	float3 rm;

	const float2& lastPoint2 = node.GetNeighborEdgeTransitionPoint();
	if (lastPoint2 == float2(goalPos.x, goalPos.z)) {
		return goalPos;
	}

	const float3 lastPoint({lastPoint2.x, 0.f, lastPoint2.y});
	GetRectangleCollisionVolume(node, rv, rm);
	bool collide = CCollisionHandler::IntersectBox(&rv, goalPos - rm, lastPoint - rm, &cq);
	if (collide) {
		// If the goalPos is inside the node, then the goalPos should be used.
		return (cq.InsideHit()) ? goalPos : cq.GetHitPos() + rm;
	}

	// No collision means the nearest point was really the nearest point. We can't do better.
	return  lastPoint;
}

// #pragma GCC push_options
// #pragma GCC optimize ("O0")

template<typename L, typename T>
bool isPresent(const L& list, const T& node) {
	return std::find_if(list.begin(), list.end(), [&node](auto& next){ return next.nodeId == node.nodeId; }) != list.end();
}

template<typename L>
bool isPresent(const L& list, const QTPFS::SearchNode& node) {
	return std::find_if(list.begin(), list.end(), [&node](auto& next){ return next.nodeId == node.GetIndex(); }) != list.end();
}

void QTPFS::PathSearch::TracePath(IPath* path) {
	RECOIL_DETAILED_TRACY_ZONE;
	constexpr uint32_t ONLY_NODE_ID_MASK = 0x80000000;
	struct TracePoint{
		float3 point;
		uint32_t nodeId;
		uint32_t nodeNumber;
		int xmin;
		int zmin;
		int xmax;
		int zmax;
		bool isBad = false;
		float dist = 0.f;
		uint32_t index = 0;
	};
	std::deque<TracePoint> points;
	int nodesWithoutPoints = 0;

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

	float3 boundaryMins(std::numeric_limits<float>::infinity(), 0.f, std::numeric_limits<float>::infinity());
	float3 boundaryMaxs(-1.f, 0.f, -1.f);

	if (fwd.srcSearchNode->GetIndex() != bwd.srcSearchNode->GetIndex())
	{
		// Only a bad path will be associated with the reverse path
		if (!haveFullPath)
		{
			const SearchNode* tmpNode = bwd.tgtSearchNode;
			const SearchNode* prvNode = nullptr;

			float3 prvPoint = bwd.tgtPoint;

			while (tmpNode != nullptr) {
				// reverse path didn't connect so record each point as bad for early drop out
				// for other partial searches.

				const float2& tmpPoint2 = tmpNode->GetNeighborEdgeTransitionPoint();
				const float3  tmpPoint  = {tmpPoint2.x, 0.0f, tmpPoint2.y};

				assert(prvPoint.x >= 0.f);
				assert(prvPoint.z >= 0.f);
				assert(prvPoint.x / SQUARE_SIZE < mapDims.mapx);
				assert(prvPoint.z / SQUARE_SIZE < mapDims.mapy);

				assert(!math::isinf(prvPoint.x) && !math::isinf(prvPoint.z));
				assert(!math::isnan(prvPoint.x) && !math::isnan(prvPoint.z));

				#ifndef NDEBUG
				if (prvNode != nullptr) {
					assert(tmpNode->GetIndex() != prvNode->GetIndex());

					INode* nn0 = nodeLayer->GetPoolNode(prvNode->GetIndex());
					INode* nn1 = nodeLayer->GetPoolNode(tmpNode->GetIndex());

					assert(nn1->GetNeighborRelation(nn0) != 0);
					assert(nn0->GetNeighborRelation(nn1) != 0);
				}

				assert( !isPresent(points, *tmpNode) );

				#endif

				auto& newPoint = points.emplace_back(prvPoint
						, tmpNode->GetIndex() | ONLY_NODE_ID_MASK
						, tmpNode->nodeNumber
						, tmpNode->xmin, tmpNode->zmin
						, tmpNode->xmax, tmpNode->zmax
						, true);
				nodesWithoutPoints++;
				boundaryMins.x = std::min(boundaryMins.x, float(tmpNode->xmin*SQUARE_SIZE));
				boundaryMins.z = std::min(boundaryMins.z, float(tmpNode->zmin*SQUARE_SIZE));
				boundaryMaxs.x = std::max(boundaryMaxs.x, float(tmpNode->xmax*SQUARE_SIZE));
				boundaryMaxs.z = std::max(boundaryMaxs.z, float(tmpNode->zmax*SQUARE_SIZE));

				#ifndef NDEBUG
				if (prvNode != nullptr) {
					assert(tmpNode->GetIndex() != prvNode->GetIndex());

					INode* nn0 = nodeLayer->GetPoolNode(prvNode->GetIndex());
					INode* nn1 = nodeLayer->GetPoolNode(tmpNode->GetIndex());

					assert(nn1->GetNeighborRelation(nn0) != 0);
					assert(nn0->GetNeighborRelation(nn1) != 0);

					AssertPointIsOnNodeEdge(prvPoint, tmpNode);
					AssertPointIsOnNodeEdge(prvPoint, prvNode);
				}
				prvNode = tmpNode;
				#endif

				// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
				// 	LOG("%s: revBadNodeRecord %d [%x] g=%d (%d,%d)-(%d,%d) [%f,%f]", __func__
				// 		, tmpNode->index, tmpNode->nodeNumber
				// 		, int(!newPoint.isBad)
				// 		, tmpNode->xmin, tmpNode->zmin, tmpNode->xmax, tmpNode->zmax
				// 		, tmpNode->selectedNetpoint.x, tmpNode->selectedNetpoint.y
				// 		);
				// }

				assert(tmpNode->xmax != 0 && tmpNode->zmax != 0);

				prvPoint = tmpPoint;
				tmpNode = tmpNode->GetPrevNode();
			}
		}
		else if (!useFwdPathOnly) {
			const SearchNode* tmpNode = bwd.tgtSearchNode;
			const SearchNode* nextNode = nullptr;

			float3 prvPoint = bwd.tgtPoint;

			// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
			// 	LOG("%s: bwd.tgtPoint is (%f, %f, %f)", __func__, bwd.tgtPoint.x, bwd.tgtPoint.y, bwd.tgtPoint.z);
			// }

			while (tmpNode != nullptr) {
				nextNode = tmpNode->GetPrevNode();

				const float2& tmpPoint2 = tmpNode->GetNeighborEdgeTransitionPoint();
				const float3  tmpPoint  = {tmpPoint2.x, 0.0f, tmpPoint2.y};

				assert(prvPoint.x >= 0.f);
				assert(prvPoint.z >= 0.f);
				assert(prvPoint.x / SQUARE_SIZE < mapDims.mapx);
				assert(prvPoint.z / SQUARE_SIZE < mapDims.mapy);

				assert(!math::isinf(prvPoint.x) && !math::isinf(prvPoint.z));
				assert(!math::isnan(prvPoint.x) && !math::isnan(prvPoint.z));
				// NOTE:
				//   waypoints should NEVER have identical coordinates
				//   one exception: tgtPoint can legitimately coincide
				//   with first transition-point, which we must ignore
				assert(tmpNode != nextNode);
				#ifndef NDEBUG
				if (nextNode != nullptr) {
					assert(tmpNode->GetIndex() != nextNode->GetIndex());

					INode* nn0 = nodeLayer->GetPoolNode(nextNode->GetIndex());
					INode* nn1 = nodeLayer->GetPoolNode(tmpNode->GetIndex());

					assert(nn1->GetNeighborRelation(nn0) != 0);
					assert(nn0->GetNeighborRelation(nn1) != 0);

					if (tmpNode == bwd.srcSearchNode) {
						AssertPointIsOnNodeEdge(tmpPoint, tmpNode);
						AssertPointIsOnNodeEdge(tmpPoint, nextNode);
					}
				}
				assert(prvPoint != ZeroVector);
				assert(tmpPoint != prvPoint || tmpNode == bwd.srcSearchNode || tmpNode == bwd.tgtSearchNode);

				assert( !isPresent(points, *tmpNode) );

				#endif

				points.emplace_back(prvPoint
						, tmpNode->GetIndex() //| (ONLY_NODE_ID_MASK * int(tmpPoint == prvPoint))
						, tmpNode->nodeNumber
						, tmpNode->xmin, tmpNode->zmin
						, tmpNode->xmax, tmpNode->zmax);
				//nodesWithoutPoints += int(tmpPoint == prvPoint);
				boundaryMins.x = std::min(boundaryMins.x, float(tmpNode->xmin*SQUARE_SIZE));
				boundaryMins.z = std::min(boundaryMins.z, float(tmpNode->zmin*SQUARE_SIZE));
				boundaryMaxs.x = std::max(boundaryMaxs.x, float(tmpNode->xmax*SQUARE_SIZE));
				boundaryMaxs.z = std::max(boundaryMaxs.z, float(tmpNode->zmax*SQUARE_SIZE));
				// LOG("%s: [%d] nxtNode=%d point (%f, %f, %f) [onlyNode=%d]", __func__, SearchThreadData::SEARCH_BACKWARD
				// 		, tmpNode->GetIndex(), prvPoint.x, prvPoint.y, prvPoint.z, int(tmpPoint == prvPoint));

				#ifndef QTPFS_SMOOTH_PATHS
				// make sure the back-pointers can never become dangling
				// (if smoothing IS enabled, we delay this until we reach
				// SmoothPath() because we still need them there)
				tmpNode->SetPrevNode(nullptr);
				#endif

				// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
				// 	LOG("%s: revGoodRecord %d [%x] g=%d (%d,%d)-(%d,%d) [%f,%f]", __func__
				// 		, tmpNode->index, tmpNode->nodeNumber
				// 		, int(!tmpNode->isNodeBad())
				// 		, tmpNode->xmin, tmpNode->zmin, tmpNode->xmax, tmpNode->zmax
				// 		, tmpNode->selectedNetpoint.x, tmpNode->selectedNetpoint.y
				// 		);
				// }

				prvPoint = tmpPoint;
				tmpNode = nextNode;
			}
		}

		const SearchNode* tmpNode = fwd.tgtSearchNode;
		
		#ifndef NDEBUG
		if (points.size() > 0) {
			const auto tmpPoint = bwd.tgtPoint;
			TracePoint* nextNode = &points[0];
			assert(tmpNode->GetIndex() != nextNode->nodeId);

			if (haveFullPath) {
				INode* nn0 = nodeLayer->GetPoolNode(nextNode->nodeId & 0xfffff);
				INode* nn1 = nodeLayer->GetPoolNode(tmpNode->GetIndex());

				assert(nn1->GetNeighborRelation(nn0) != 0);
				assert(nn0->GetNeighborRelation(nn1) != 0);

				AssertPointIsOnNodeEdge(tmpPoint, tmpNode);
				AssertPointIsOnNodeEdge(tmpPoint, nextNode);
			}
		}
		#endif

		const SearchNode* nextNode = (tmpNode != nullptr) ? tmpNode->GetPrevNode() : nullptr;

		float3 prvPoint = fwd.tgtPoint;

		while ((nextNode != nullptr) && (tmpNode != fwd.srcSearchNode)) {
			const float2& tmpPoint2 = tmpNode->GetNeighborEdgeTransitionPoint();
			const float3  tmpPoint  = {tmpPoint2.x, 0.0f, tmpPoint2.y};

			// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
			// 	LOG("%s: goodRecord %d [%x] g=%d (%d,%d)-(%d,%d) [%f,%f]", __func__
			// 		, tmpNode->index, tmpNode->nodeNumber
			// 		, int(!tmpNode->isNodeBad())
			// 		, tmpNode->xmin, tmpNode->zmin, tmpNode->xmax, tmpNode->zmax
			// 		, tmpNode->selectedNetpoint.x, tmpNode->selectedNetpoint.y
			// 		);
			// }

			assert(tmpPoint.x >= 0.f);
			assert(tmpPoint.z >= 0.f);
			assert(tmpPoint.x / SQUARE_SIZE < mapDims.mapx);
			assert(tmpPoint.z / SQUARE_SIZE < mapDims.mapy);

			assert(!math::isinf(tmpPoint.x) && !math::isinf(tmpPoint.z));
			assert(!math::isnan(tmpPoint.x) && !math::isnan(tmpPoint.z));
			// NOTE:
			//   waypoints should NEVER have identical coordinates
			//   one exception: tgtPoint can legitimately coincide
			//   with first transition-point, which we must ignore
			assert(tmpNode != nextNode);
			#ifndef NDEBUG
			if (nextNode != nullptr) {
				assert(tmpNode->GetIndex() != nextNode->GetIndex());

				INode* nn0 = nodeLayer->GetPoolNode(nextNode->GetIndex());
				INode* nn1 = nodeLayer->GetPoolNode(tmpNode->GetIndex());

				assert(nn1->GetNeighborRelation(nn0) != 0);
				assert(nn0->GetNeighborRelation(nn1) != 0);

				if (!useFwdPathOnly || tmpNode != fwd.tgtSearchNode) {
					AssertPointIsOnNodeEdge(tmpPoint, tmpNode);
					AssertPointIsOnNodeEdge(tmpPoint, nextNode);
				}
			}

			assert(tmpPoint != ZeroVector);
			assert(tmpPoint != prvPoint || tmpNode == fwd.tgtSearchNode || tmpNode == fwd.srcSearchNode);
			
			assert( !isPresent(points, *tmpNode) );

			#endif


			points.emplace_front(tmpPoint
					, tmpNode->GetIndex() //| (ONLY_NODE_ID_MASK * int(tmpPoint == prvPoint))
					, tmpNode->nodeNumber
					, tmpNode->xmin, tmpNode->zmin
					, tmpNode->xmax, tmpNode->zmax);
			//nodesWithoutPoints += int(tmpPoint == prvPoint);
			boundaryMins.x = std::min(boundaryMins.x, float(tmpNode->xmin*SQUARE_SIZE));
			boundaryMins.z = std::min(boundaryMins.z, float(tmpNode->zmin*SQUARE_SIZE));
			boundaryMaxs.x = std::max(boundaryMaxs.x, float(tmpNode->xmax*SQUARE_SIZE));
			boundaryMaxs.z = std::max(boundaryMaxs.z, float(tmpNode->zmax*SQUARE_SIZE));
			// LOG("%s: [%d] nxtNode=%d point (%f, %f, %f) [onlyNode=%d]", __func__, SearchThreadData::SEARCH_FORWARD
			// 		, tmpNode->GetIndex(), tmpPoint.x, tmpPoint.y, tmpPoint.z, int(tmpPoint == prvPoint));


			#ifndef QTPFS_SMOOTH_PATHS
			// make sure the back-pointers can never become dangling
			// (if smoothing IS enabled, we delay this until we reach
			// SmoothPath() because we still need them there)
			tmpNode->SetPrevNode(nullptr);
			#endif

			prvPoint = tmpPoint;
			tmpNode = nextNode;
			nextNode = tmpNode->GetPrevNode();
		}

		// ensure the starting quad is shared with other path searches.
		if (tmpNode != nullptr) {
			assert( !isPresent(points, *tmpNode) );

			points.emplace_front(float3()
					, tmpNode->GetIndex() | ONLY_NODE_ID_MASK
					, tmpNode->nodeNumber
					, tmpNode->xmin, tmpNode->zmin
					, tmpNode->xmax, tmpNode->zmax);
			// LOG("%s: [%d] tgtNode=%d point ", __func__, SearchThreadData::SEARCH_FORWARD
			// 		, tmpNode->GetIndex());
			nodesWithoutPoints++;
			boundaryMins.x = std::min(boundaryMins.x, float(tmpNode->xmin*SQUARE_SIZE));
			boundaryMins.z = std::min(boundaryMins.z, float(tmpNode->zmin*SQUARE_SIZE));
			boundaryMaxs.x = std::max(boundaryMaxs.x, float(tmpNode->xmax*SQUARE_SIZE));
			boundaryMaxs.z = std::max(boundaryMaxs.z, float(tmpNode->zmax*SQUARE_SIZE));

			#ifndef NDEBUG
			if (nextNode != nullptr) {
				assert(tmpNode->GetIndex() != nextNode->GetIndex());

				INode* nn0 = nodeLayer->GetPoolNode(nextNode->GetIndex());
				INode* nn1 = nodeLayer->GetPoolNode(tmpNode->GetIndex());

				assert(nn1->GetNeighborRelation(nn0) != 0);
				assert(nn0->GetNeighborRelation(nn1) != 0);
			}
			#endif

			// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
			// 	LOG("%s: last goodRecord %d [%x] g=%d (%d,%d)-(%d,%d) [%f,%f]", __func__
			// 		, tmpNode->index, tmpNode->nodeNumber
			// 		, int(!tmpNode->isNodeBad())
			// 		, tmpNode->xmin, tmpNode->zmin, tmpNode->xmax, tmpNode->zmax
			// 		, tmpNode->selectedNetpoint.x, tmpNode->selectedNetpoint.y
			// 		);
			// }
		}
	}

	// if source equals target, we need only two points
	if (!points.empty()) {
		path->AllocPoints(points.size() + (-nodesWithoutPoints) + 2);
		path->AllocNodes(points.size());

		path->SetBoundingBox(boundaryMins, boundaryMaxs);
	} else {
		path->AllocPoints(2);
		path->AllocNodes(0);
		assert(path->NumPoints() == 2);
		assert(path->GetNodeList().size() == 0);
	}

	if (doPathRepair) {
		// Set this back the final destination so the path will setup correctly for the full path.
		fwd.tgtPoint = goalPos;
	}

	// set waypoints with indices [1, N - 2] (if any)
	int pointIndex = 1;
	int nodeIndex = 0;
	float pathDist = 0.f;
	float3 lastPoint(fwd.srcPoint);

	for (TracePoint& curPoint : points) {
		int nodePointIndex = -1;
		float3& point   = curPoint.point;
		uint32_t nodeId = curPoint.nodeId;

		if ( (nodeId & ONLY_NODE_ID_MASK) == 0 ){
			assert(point != ZeroVector);
			assert(point.y == 0.f);
			pathDist += point.distance2D(lastPoint);
			curPoint.dist = pathDist;
			curPoint.index = pointIndex;

			nodePointIndex = pointIndex;
			path->SetPoint(pointIndex++, point);

			lastPoint = point;
		// { bool printMoveInfo = (selectedUnitsHandler.selectedUnits.size() == 1)
		// 					&& (selectedUnitsHandler.selectedUnits.find(path->GetOwner()->id) != selectedUnitsHandler.selectedUnits.end());
		// 	if (printMoveInfo) {
		// 		LOG("%s: setting point %d (%f, %f, %f)", __func__, nodePointIndex, point.x, point.y, point.z);
		// 	}}
		}

		assert( !isPresent(path->GetNodeList(), curPoint) );

		path->SetNode(nodeIndex, nodeId & ~ONLY_NODE_ID_MASK, curPoint.nodeNumber, float2(point.x, point.z), nodePointIndex, curPoint.isBad);
		path->SetNodeBoundary(nodeIndex, curPoint.xmin, curPoint.zmin, curPoint.xmax, curPoint.zmax);

		#ifndef NDEBUG
		if (nodeIndex > 1) {
			auto& currNode = path->GetNode(nodeIndex);
			auto& prevNode = path->GetNode(nodeIndex - 1);

			assert(prevNode.nodeId != (nodeId & ~ONLY_NODE_ID_MASK));

			bool bothBadNodes = curPoint.isBad && prevNode.IsNodeBad();
			bool bothGoodNodes = !curPoint.isBad && !prevNode.IsNodeBad();

			if (bothBadNodes || bothGoodNodes) {
				INode* nn0 = nodeLayer->GetPoolNode(prevNode.nodeId);
				INode* nn1 = nodeLayer->GetPoolNode((nodeId & ~ONLY_NODE_ID_MASK));

				assert(nn1->GetNeighborRelation(nn0) != 0);
				assert(nn0->GetNeighborRelation(nn1) != 0);

				float3 tmpPoint(currNode.netPoint.x, 0.f, currNode.netPoint.y);
				AssertPointIsOnNodeEdge(tmpPoint, &currNode);
				AssertPointIsOnNodeEdge(tmpPoint, &prevNode);
			}
		}
		#endif

		// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
		// 	auto* curNode = &path->GetNode(nodeIndex);
		// 	LOG("%s: traceNode %d [%x] g=%d (%d,%d)-(%d,%d) [%f,%f] [b? %d]", __func__
		// 		, curNode->nodeId, curNode->nodeNumber
		// 		, int(!curNode->badNode)
		// 		, curNode->xmin, curNode->zmin, curNode->xmax, curNode->zmax
		// 		, curNode->netPoint.x, curNode->netPoint.y
		// 		, int(curPoint.isBad)
		// 		);
		// }

		nodeIndex++;
		// LOG("%s: tgtNode=%d point (%f, %f, %f)", __func__
		// 		, nodeId, point.x, point.y, point.z);
	}

	//assert(path->NumPoints() - path->GetNodeList().size() == 1 + (-nodesWithoutPoints));
	
	uint32_t repathIndex = 0;
	if (!haveFullPath) {
		const float minRepathLength = modInfo.qtRefreshPathMinDist;
		bool pathIsBigEnoughForRepath = (pathDist >= minRepathLength);

		// This may result in a short path still not finding an index, but that's fine:
		// it isn't supposed to be perfect. It is more a helper.
		if (pathIsBigEnoughForRepath) {
			float halfWay = pathDist * 0.5f;
			float maxDist = pathDist - (minRepathLength/4);
			float minDist = (minRepathLength/4);

			for (auto it = points.rbegin(); it != points.rend(); ++it) {
				TracePoint& curPoint = *it;

				if ( (curPoint.nodeId & ONLY_NODE_ID_MASK) != 0 )
					continue;
				if (curPoint.dist > maxDist) // too close to end
					continue;
				if (curPoint.dist < minDist) // too close to beginning
					break;
				if (repathIndex != 0 && curPoint.dist < halfWay) // try to get point close to the middle.
					break;
				// prevent a repath being triggered immediately when the first two points are taken immediately.
				if (curPoint.index <= 2)
					break;

				repathIndex = curPoint.index;
			}
		}

		// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
		// 	LOG("%s: repathIndex=%d pathIsBigEnoughForRepath=%d min=%f pathDist=%f path=%d last=%d", __func__
		// 			, repathIndex, int(pathIsBigEnoughForRepath), minRepathLength, pathDist
		// 			, path->GetID(), int(points.size()-1));
		// }
	}

	// set the first (0) and last (N - 1) waypoint
	path->SetSourcePoint(fwd.srcPoint);
	path->SetTargetPoint(fwd.tgtPoint);
	path->SetGoalPosition(goalPos);
	path->SetRepathTriggerIndex(repathIndex);

		// 	if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */){
		// 	LOG("%s: repathIndex=%d path=%d last=%d", __func__
		// 			, path->GetRepathTriggerIndex()
		// 			, path->GetID(), int(points.size()-1));
		// 	LOG("%s: fwd.srcPoint(%f,%f,%f) fwd.tgtPoint(%f,%f,%f) goalPos(%f,%f,%f)", __func__
		// 			, fwd.srcPoint.x, fwd.srcPoint.y, fwd.srcPoint.z
		// 			, fwd.tgtPoint.x, fwd.tgtPoint.y, fwd.tgtPoint.z
		// 			, goalPos.x, goalPos.y, goalPos.z
		// 			);
		// }

	assert(fwd.srcPoint != ZeroVector);
	assert(fwd.tgtPoint != ZeroVector);
}

// #pragma GCC pop_options

void QTPFS::PathSearch::SmoothPath(IPath* path) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (path->NumPoints() == 2)
		return;

	for (unsigned int k = 0; k < QTPFS_MAX_SMOOTHING_ITERATIONS; k++) {
		if (!SmoothPathIter(path)) {
			// all waypoints stopped moving
			break;
		}
	}
}

bool QTPFS::PathSearch::SmoothPathIter(IPath* path) {
	RECOIL_DETAILED_TRACY_ZONE;
	// smooth in reverse order (target to source)
	//
	// should terminate when waypoints stop moving,
	// or after a small fixed number of iterations
	unsigned int ni = path->NumPoints() - 1;
	unsigned int nm = 0;

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	auto& nodePath = path->GetNodeList();
	auto getNextNodeIndex = [&nodePath](int i){
		while (--i > 0) {
			const IPath::PathNodeData* node = &nodePath[i];
			if (node->pathPointIndex > -1)
				break;
		}
		return i;
	};

	int nodeIdx = ni - 1; //getNextNodeIndex(nodePath.size());
	assert(nodeIdx > -1);

	IPath::PathNodeData* n1 = &nodePath[nodeIdx];
	IPath::PathNodeData* n0 = n1;
	INode* nn0 = nodeLayer->GetPoolNode(n1->nodeId);
	INode* nn1 = nn0;

	// Three points are needed to smooth a path entry.
	constexpr int firstNode = 2;

	for (; ni >= firstNode; --ni) {
		nodeIdx = ni - 1; //getNextNodeIndex(nodeIdx);
		// if (nodeIdx < 0)
		// 	break;

		n0 = n1;
		n1 = &nodePath[nodeIdx - 1];

		// if ((path->GetID() == 7340095 || path->GetID() == 10485810)) {
		// 	LOG("%s: ni=%d, nodeIdx=%d (%f,%f)->(%f,%f) [%d,%d][%d,%d]->[%d,%d][%d,%d]", __func__
		// 		, ni, nodeIdx
		// 		, n0->netPoint.x, n0->netPoint.y
		// 		, n1->netPoint.x, n1->netPoint.y
		// 		, n0->xmin, n0->zmin, n0->xmax, n0->zmax
		// 		, n1->xmin, n1->zmin, n1->xmax, n1->zmax);
		// }

		assert(n1->nodeId < nodeLayer->GetMaxNodesAlloced());

		nn0 = nn1;
		nn1 = nodeLayer->GetPoolNode(n1->nodeId);

		assert(nn1->GetNeighborRelation(nn0) != 0);
		assert(nn0->GetNeighborRelation(nn1) != 0);
		assert(ni < path->NumPoints());

		const unsigned int ngbRel = nn0->GetNeighborRelation(nn1);

		const float3 p0 = path->GetPoint(ni    );
		const float3 p1 = path->GetPoint(ni - 1);
		const float3 p2 = path->GetPoint(ni - 2);

		float3 pi;
		if (SmoothPathPoints(nn0, nn1, p0, p1, p2, pi)) {
			nm++;
			if (pi == p0 || pi == p2) {

				// TODO: switching to a diagonal needs the net selected point to be changed as well
				// otherwise there is no valid connection between the nodes.

				// if (path->GetID() == 34603576) {
				// 	LOG("%s: remove (%d) p0 (%f,%f) p1 (%f,%f) p2 (%f,%f) pi (%f,%f)", __func__
				// 		, ni - 1
				// 		, p0.x, p0.z, p1.x, p1.z, p2.x, p2.z, pi.x, pi.z);
				// }

				// the point is effectively removed.
				path->RemovePoint(ni - 1);

				int curNodeIdx = 0;
				if (pi == p0) {
					path->RemoveNode(nodeIdx);
					// if (path->GetID() == 34603576) {
					// 	LOG("%s: remove (%d) n0 (%f,%f)", __func__
					// 		, nodeIdx
					// 		, n0->netPoint.x, n0->netPoint.y);
					// }
					curNodeIdx = (ni < path->NumPoints()) ? nodeIdx : nodeIdx - 1;
				} else {
					// The starting point overlaps the middle point. Remove first quad.
					path->RemoveNode(nodeIdx - 1);

					// if (path->GetID() == 34603576) {
					// 	LOG("%s: remove (%d) n1 (%f,%f)", __func__
					// 		, nodeIdx - 1
					// 		, n1->netPoint.x, n1->netPoint.y);
					// }

					// move point forward so that the previous two nodes can be compared.
					curNodeIdx = --nodeIdx;
				}

				if (curNodeIdx > 0) {
					n0 = &nodePath[curNodeIdx];
					n1 = &nodePath[curNodeIdx - 1];

					auto* nodeCur = (n0->getWidth() <= n1->getWidth()) ? n0 : n1;
					auto* nodeRev = (n0->getWidth() <= n1->getWidth()) ? n1 : n0;

					int neighbourStatus = nodeCur->GetNeighborRelationStatus(*nodeRev);
					int width = nodeCur->getWidth();

					assert(neighbourStatus != 0);

					if (nodeCur->IsNeighbourCorner(neighbourStatus)) {
						float xpos = (nodeCur->xmin + width*nodeCur->GetLongitudeNeighbourSide(neighbourStatus)) * SQUARE_SIZE;
						float zpos = (nodeCur->zmin + width*nodeCur->GetLatitudeNeighbourSide(neighbourStatus)) * SQUARE_SIZE;

						n0->netPoint = float2(xpos, zpos);
						pi = float3(xpos, 0.f, zpos);
					} else {
						if (nodeCur->IsNeighbourLatitude(neighbourStatus)) {
							float xpos = (nodeCur->xmin + width*0.5f) * SQUARE_SIZE;
							float zpos = (nodeCur->zmin + width*nodeCur->GetLatitudeNeighbourSide(neighbourStatus)) * SQUARE_SIZE;

							n0->netPoint = float2(xpos, zpos);

							pi.x = std::clamp(pi.x, float(nodeCur->xmin * SQUARE_SIZE), float(nodeCur->xmax * SQUARE_SIZE));
							pi.z = zpos;
						} else {
							float xpos = (nodeCur->xmin + width*nodeCur->GetLongitudeNeighbourSide(neighbourStatus)) * SQUARE_SIZE;
							float zpos = (nodeCur->zmin + width*0.5f) * SQUARE_SIZE;

							n0->netPoint = float2(xpos, zpos);

							pi.x = xpos;
							pi.z = std::clamp(pi.z, float(nodeCur->zmin * SQUARE_SIZE), float(nodeCur->zmax * SQUARE_SIZE));
						}
					}

					AssertPointIsOnNodeEdge(pi, n0);
					AssertPointIsOnNodeEdge(pi, n1);

					float3 tmpPoint(n0->netPoint.x, 0.f, n0->netPoint.y);
					AssertPointIsOnNodeEdge(tmpPoint, n0);
					AssertPointIsOnNodeEdge(tmpPoint, n1);
				}

				#ifndef NDEBUG
				if (curNodeIdx > 0) {
					n0 = &nodePath[curNodeIdx];
					n1 = &nodePath[curNodeIdx - 1];
					INode* nn0 = nodeLayer->GetPoolNode(n0->nodeId);
					INode* nn1 = nodeLayer->GetPoolNode(n1->nodeId);

					// if (path->GetID() == 34603576) {
					// 	LOG("%s: new ni=%d, nodeIdx=%d (%f,%f)->(%f,%f) [%d,%d][%d,%d]->[%d,%d][%d,%d]", __func__
					// 		, ni - 1, curNodeIdx
					// 		, n0->netPoint.x, n0->netPoint.y
					// 		, n1->netPoint.x, n1->netPoint.y
					// 		, n0->xmin, n0->zmin, n0->xmax, n0->zmax
					// 		, n1->xmin, n1->zmin, n1->xmax, n1->zmax);
					// }

					assert(nn1->GetNeighborRelation(nn0) != 0);
					assert(nn0->GetNeighborRelation(nn1) != 0);

					AssertPointIsOnNodeEdge(pi, n0);
					AssertPointIsOnNodeEdge(pi, n1);
				}
				#endif

			} else {
				path->SetPoint(ni - 1, pi);
			}
		}
	}

	return (nm != 0);
}

// #pragma GCC pop_options


// Only smooths the beginning of the path.
void QTPFS::PathSearch::SmoothSharedPath(IPath* path) {
	RECOIL_DETAILED_TRACY_ZONE;
	// No smoothing can be done on 2 points.
	if (path->NumPoints() <= 2)
		return;

	auto& nodePath = path->GetNodeList();

	assert(nodePath.size() > 1);

	auto getNextNodeIndex = [&nodePath](int i){
		auto last = nodePath.size() - 1;
		while (++i < last) {
			const IPath::PathNodeData* node = &nodePath[i];
			if (node->pathPointIndex > -1)
				break;
		}
		return i;
	};

	// First three points are indices: 0, 1, 2
	int pi = 2;
	int nodeIndex1 = 0;
	int nodeIndex0 = getNextNodeIndex(nodeIndex1);

	assert(nodeIndex0 > -1);
	assert(nodeIndex0 < nodePath.size());

	const IPath::PathNodeData* n0 = &nodePath[nodeIndex0];
	const IPath::PathNodeData* n1 = &nodePath[nodeIndex1];

	assert(n0->pathPointIndex > -1);

	assert(n0->nodeId < nodeLayer->GetMaxNodesAlloced());
	assert(n1->nodeId < nodeLayer->GetMaxNodesAlloced());

	INode* nn0 = nodeLayer->GetPoolNode(n0->nodeId);
	INode* nn1 = nodeLayer->GetPoolNode(n1->nodeId);

	assert(nn1->GetNeighborRelation(nn0) != 0);
	assert(nn0->GetNeighborRelation(nn1) != 0);

	const float3 p0 = path->GetPoint(pi    );
	const float3 p1 = path->GetPoint(pi - 1);
	const float3 p2 = path->GetPoint(pi - 2);

	float3 newPoint;
	int nm = SmoothPathPoints(nn0, nn1, p0, p1, p2, newPoint);
	if (nm > 0) {
		// update node;
		path->SetPoint(pi - 1, newPoint);
	}
}

int QTPFS::PathSearch::SmoothPathPoints(const INode* nn0, const INode* nn1, const float3& p0, const float3& p1, const float3& p2, float3& result) const {
	RECOIL_DETAILED_TRACY_ZONE;
	float3 pi = ZeroVector;

	const unsigned int ngbRel = nn0->GetNeighborRelation(nn1);

	// check if we can reduce the angle between segments
	// p0-p1 and p1-p2 (ideally to zero degrees, making
	// p0-p2 a straight line) without causing either of
	// the segments to cross into other nodes
	//
	// p1 always lies on the node to the right and/or to
	// the bottom of the shared edge between p0 and p2,
	// and we move it along the edge-dimension (x or z)
	// between [xmin, xmax] or [zmin, zmax]
	const float3 p1p0 = (p1 - p0).SafeNormalize();
	const float3 p2p1 = (p2 - p1).SafeNormalize();
	const float3 p2p0 = (p2 - p0).SafeNormalize();
	const float   dot = p1p0.dot(p2p1);

	// if segments are already nearly parallel, skip
	if (dot >= 0.995f)
		return 0;

	// figure out if p1 is on a horizontal or a vertical edge
	// (if both of these are true, it is in fact in a corner)
	const bool hEdge = (((ngbRel & REL_NGB_EDGE_T) != 0) || ((ngbRel & REL_NGB_EDGE_B) != 0));
	const bool vEdge = (((ngbRel & REL_NGB_EDGE_L) != 0) || ((ngbRel & REL_NGB_EDGE_R) != 0));

	assert(hEdge || vEdge);

	#ifndef QTPFS_CORNER_CONNECTED_NODES

	// Don't allow the points to completely touch the corners.
	constexpr float borderAdjustInElmos = 1;
	#else
	constexpr float borderAdjustInElmos = 0;
	#endif

	// establish the x- and z-range within which p1 can be moved
	const float xmin = (std::max(nn1->xmin(), nn0->xmin()) * SQUARE_SIZE) + borderAdjustInElmos;
	const float zmin = (std::max(nn1->zmin(), nn0->zmin()) * SQUARE_SIZE) + borderAdjustInElmos;
	const float xmax = (std::min(nn1->xmax(), nn0->xmax()) * SQUARE_SIZE) - borderAdjustInElmos;
	const float zmax = (std::min(nn1->zmax(), nn0->zmax()) * SQUARE_SIZE) - borderAdjustInElmos;

	{
		// calculate intersection point between ray (p2 - p0) and edge
		// if pi lies between bounds, use that and move to next triplet
		//
		// cases:
		//     A) p0-p1-p2 (p2p0.xz >= 0 -- p0 in n0, p2 in n1)
		//     B) p2-p1-p0 (p2p0.xz <= 0 -- p2 in n1, p0 in n0)
		//
		// x- and z-distances to edge between n0 and n1
		const float dfx = (p2p0.x > 0.0f)?
			((nn0->xmax() * SQUARE_SIZE) - p0.x): // A(x)
			((nn0->xmin() * SQUARE_SIZE) - p0.x); // B(x)
		const float dfz = (p2p0.z > 0.0f)?
			((nn0->zmax() * SQUARE_SIZE) - p0.z): // A(z)
			((nn0->zmin() * SQUARE_SIZE) - p0.z); // B(z)

		const float dx = (math::fabs(p2p0.x) > 0.001f)? p2p0.x: 0.001f;
		const float dz = (math::fabs(p2p0.z) > 0.001f)? p2p0.z: 0.001f;
		const float tx = dfx / dx;
		const float tz = dfz / dz;

		bool ok = true;

		if (hEdge) {
			pi.x = p0.x + p2p0.x * tz;
			pi.z = p1.z;
		}
		if (vEdge) {
			pi.x = p1.x;
			pi.z = p0.z + p2p0.z * tx;
		}

		ok = ok && (pi.x >= xmin && pi.x <= xmax);
		ok = ok && (pi.z >= zmin && pi.z <= zmax);

		if (ok) {
			assert(!math::isinf(pi.x) && !math::isinf(pi.z));
			assert(!math::isnan(pi.x) && !math::isnan(pi.z));

			result = pi;
			return ((pi - p1).SqLength2D() > Square(0.05f));
		}
	}

	if (hEdge != vEdge) {
		// get the edge end-points
		float3 e0 = p1;
		float3 e1 = p1;

		if (hEdge) {
			e0.x = xmin;
			e1.x = xmax;
		}
		if (vEdge) {
			e0.z = zmin;
			e1.z = zmax;
		}

		// figure out what the angle between p0-p1 and p1-p2
		// would be after substituting the edge-ends for p1
		// (we want dot-products as close to 1 as possible)
		//
		// p0-e0-p2
		const float3 e0p0 = (e0 - p0).SafeNormalize();
		const float3 p2e0 = (p2 - e0).SafeNormalize();
		const float  dot0 = e0p0.dot(p2e0);
		// p0-e1-p2
		const float3 e1p0 = (e1 - p0).SafeNormalize();
		const float3 p2e1 = (p2 - e1).SafeNormalize();
		const float  dot1 = e1p0.dot(p2e1);

		// if neither end-point is an improvement, skip
		if (dot >= std::max(dot0, dot1))
			return 0;

		if (dot0 > std::max(dot1, dot)) { pi = e0; }
		if (dot1 >= std::max(dot0, dot)) { pi = e1; }

		assert(!math::isinf(pi.x) && !math::isinf(pi.z));
		assert(!math::isnan(pi.x) && !math::isnan(pi.z));
		result = pi;
		return ((pi - p1).SqLength2D() > Square(0.05f));
	}
	return 0;
}


bool QTPFS::PathSearch::SharedFinalize(const IPath* srcPath, IPath* dstPath) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(dstPath->GetID() != 0);
	assert(dstPath->GetID() != srcPath->GetID());
	// assert(dstPath->NumPoints() == 2);

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	// if ((searchID == 7340095 || searchID == 10485810) /*&& pathOwner != nullptr && pathOwner->id == 30809 */) {
	// 	LOG("%s: %d %d", __func__
	// 		, srcPath->GetID(), dstPath->GetID()
	// 		);
	// }

	// copy <srcPath> to <dstPath>
	dstPath->CopyPoints(*srcPath);
	dstPath->CopyNodes(*srcPath);
	dstPath->SetSourcePoint(fwd.srcPoint);
	if (srcPath->IsFullPath())
		dstPath->SetTargetPoint(fwd.tgtPoint);
	else
		dstPath->SetTargetPoint(srcPath->GetTargetPoint());

	if (srcPath->IsBoundingBoxOverriden()) {
		const float3& mins = srcPath->GetBoundingBoxMins();
		const float3& maxs = srcPath->GetBoundingBoxMaxs();
		dstPath->SetBoundingBox(mins, maxs);
	} else {
		dstPath->SetBoundingBox();
	}
	dstPath->SetFirstNodeIdOfCleanPath(0);
	dstPath->SetHasFullPath(srcPath->IsFullPath());
	dstPath->SetHasPartialPath(srcPath->IsPartialPath());
	dstPath->SetSearchTime(srcPath->GetSearchTime());
	dstPath->SetRepathTriggerIndex(srcPath->GetRepathTriggerIndex());
	dstPath->SetGoalPosition(goalPos);
	dstPath->SetIsRawPath(srcPath->IsRawPath());

	haveFullPath = srcPath->IsFullPath();
	havePartPath = srcPath->IsPartialPath();

	SmoothSharedPath(dstPath);

	return true;
}

// TODO: use node.h version?
unsigned int GetChildId(uint32_t nodeNumber, uint32_t i, uint32_t depth) {
	uint32_t shift = (QTPFS::QTNode::MAX_DEPTH - (depth + 1)) * QTPFS_NODE_NUMBER_SHIFT_STEP;
	return nodeNumber + ((i + 1) << shift);
}

const QTPFS::PathHashType QTPFS::PathSearch::GenerateHash(const INode* srcNode, const INode* tgtNode) const {
	RECOIL_DETAILED_TRACY_ZONE;
	uint32_t nodeSize = srcNode->xsize();

	if (rawPathCheck)
		return BAD_HASH;
	if (!allowPartialSearch)
		return BAD_HASH;
	if (nodeSize < QTPFS_SHARE_PATH_MIN_SIZE)
		return BAD_HASH;

	MoveDef* md = moveDefHandler.GetMoveDefByPathType(nodeLayer->GetNodelayer());
	int shift = GetNextBitShift(md->xsize);

	// is the node too small to have multiple units within it?
	if (nodeSize < (1<<shift)) 
		return BAD_HASH;

	// is the unit too big to be able to share paths?
	if ((1<<shift) > QTPFS_SHARE_PATH_MAX_SIZE) 
		return BAD_HASH;

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	uint32_t srcNodeNumber = GenerateVirtualNodeNumber(*nodeLayer, srcNode, QTPFS_SHARE_PATH_MAX_SIZE, fwd.srcPoint.x / SQUARE_SIZE, fwd.srcPoint.z / SQUARE_SIZE);

	return GenerateHash2(srcNodeNumber, tgtNode->GetNodeNumber());
}

const QTPFS::PathHashType QTPFS::PathSearch::GenerateVirtualHash(const INode* srcNode, const INode* tgtNode) const {
	RECOIL_DETAILED_TRACY_ZONE;
	uint32_t srcNodeSize = srcNode->xsize();
	uint32_t tgtNodeSize = tgtNode->xsize();

	if (rawPathCheck)
		return BAD_HASH;
	// if (srcNodeSize >= QTPFS_SHARE_PATH_MAX_SIZE && tgtNodeSize >= QTPFS_SHARE_PATH_MAX_SIZE)
	// 	return BAD_HASH;
	
	MoveDef* md = moveDefHandler.GetMoveDefByPathType(nodeLayer->GetNodelayer());
	int shift = GetNextBitShift(md->xsize);

	// is the unit too big to be able to share paths?
	if ((1<<shift) > QTPFS_PARTIAL_SHARE_PATH_MAX_SIZE) 
		return BAD_HASH;

	int srcX = srcNode->xmid();
	int srcZ = srcNode->zmid();
	INode* srcRootNode = nodeLayer->GetRootNode(srcX, srcZ);

	int tgtX = tgtNode->xmid();
	int tgtZ = tgtNode->zmid();
	INode* tgtRootNode = nodeLayer->GetRootNode(tgtX, tgtZ);

	std::uint32_t vSrcNodeId = GenerateVirtualNodeNumber(*nodeLayer, srcRootNode, QTPFS_PARTIAL_SHARE_PATH_MAX_SIZE, srcX, srcZ);
	std::uint32_t vTgtNodeId = GenerateVirtualNodeNumber(*nodeLayer, tgtRootNode, QTPFS_PARTIAL_SHARE_PATH_MAX_SIZE, tgtX, tgtZ);

	// Within same cell is too close?
	// if (vSrcNodeId == vTgtNodeId)
	// 	return BAD_HASH;

	return GenerateHash2(vSrcNodeId, vTgtNodeId);
}

const QTPFS::PathHashType QTPFS::PathSearch::GenerateHash2(uint32_t src, uint32_t dest) const {
	RECOIL_DETAILED_TRACY_ZONE;
	std::uint64_t k = nodeLayer->GetNodelayer();
	PathHashType result(std::uint64_t(src) + ((std::uint64_t(dest) << 32)), k);

	return result;
}

const std::uint32_t QTPFS::PathSearch::GenerateVirtualNodeNumber(const QTPFS::NodeLayer& nodeLayer, const INode* startNode, int nodeMaxSize, int x, int z, uint32_t* retDepth) {
	RECOIL_DETAILED_TRACY_ZONE;
	uint32_t nodeSize = startNode->xsize();
	uint32_t srcNodeNumber = startNode->GetNodeNumber();
	uint32_t xoff = startNode->xmin();
	uint32_t zoff = startNode->zmin();

	int depth = 0;
	while (nodeSize > nodeMaxSize) {
		// build the rest of the virtual node number
		bool isRight = x >= xoff + (nodeSize >> 1);
		bool isDown = z >= zoff + (nodeSize >> 1);
		int offset = 1*(isRight) + 2*(isDown);

		// TODO: sanity check if it isn't possible to go down this many levels?
		srcNodeNumber = GetChildId(srcNodeNumber, offset, depth);
		nodeSize >>= 1;

		// if (nodeLayer.GetNodelayer() == 2)
		// 	LOG("Generated %x (depth %d) (width %d)", srcNodeNumber, depth, nodeSize);

		xoff += nodeSize*isRight;
		zoff += nodeSize*isDown;
		++depth;
	}
	if (retDepth != nullptr)
		*retDepth = depth;
	return srcNodeNumber;
}
