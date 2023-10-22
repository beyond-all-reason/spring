/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include <cassert>
#include <limits>

#include "PathSearch.h"
#include "Path.h"
#include "PathCache.h"
#include "NodeLayer.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/ModInfo.h"
#include "System/Log/ILog.h"
#include "Game/SelectedUnitsHandler.h"
#include "Sim/Objects/SolidObject.h"

#include "Components/PathSpeedModInfo.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"

#include "Map/ReadMap.h"

#ifdef QTPFS_TRACE_PATH_SEARCHES
#include "Sim/Misc/GlobalSynced.h"
#endif

#include "System/float3.h"

// The bit shift needed for the power of two number that is slightly bigger than the given number.
int GetNextBitShift(int n)
{
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

void QTPFS::PathSearch::Initialize(
	NodeLayer* layer,
	const float3& sourcePoint,
	const float3& targetPoint,
	const CSolidObject* owner
) {
	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	fwd.srcPoint = sourcePoint; fwd.srcPoint.ClampInBounds();
	fwd.tgtPoint = targetPoint; fwd.tgtPoint.ClampInBounds();

	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];
	bwd.srcPoint = fwd.tgtPoint;
	bwd.tgtPoint = fwd.srcPoint;

	pathOwner = owner;
	nodeLayer = layer;
	searchExec = nullptr;

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
}

void QTPFS::PathSearch::InitializeThread(SearchThreadData* threadData) {
	ZoneScoped;
	searchThreadData = threadData;

	badGoal = false;

	searchThreadData->Init(nodeLayer->GetMaxNodesAlloced(), nodeLayer->GetNumLeafNodes());

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	INode* srcNode = nodeLayer->GetNode(fwd.srcPoint.x / SQUARE_SIZE, fwd.srcPoint.z / SQUARE_SIZE);
	INode* tgtNode = nodeLayer->GetNode(fwd.tgtPoint.x / SQUARE_SIZE, fwd.tgtPoint.z / SQUARE_SIZE);

	if (tgtNode->AllSquaresImpassable()) {
		// find nearest acceptable node because this will otherwise trigger a full walk of every pathable node.
		INode* altTgtNode = nodeLayer->GetNearestNodeInArea
			( SRectangle
					( std::max(int(tgtNode->xmin()) - 16, 0)
					, std::max(int(tgtNode->zmin()) - 16, 0)
					, std::min(int(tgtNode->xmax()) + 16, mapDims.mapx)
					, std::min(int(tgtNode->zmax()) + 16, mapDims.mapy)
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

	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

	// note src and tgt are swapped when searching backwards.
	bwd.srcSearchNode = &searchThreadData->allSearchedNodes[SearchThreadData::SEARCH_BACKWARD].InsertINode(tgtNode->GetIndex());
	bwd.tgtSearchNode = &searchThreadData->allSearchedNodes[SearchThreadData::SEARCH_BACKWARD].InsertINodeIfNotPresent(srcNode->GetIndex());

	assert(fwd.srcSearchNode != nullptr);
	assert(bwd.srcSearchNode != nullptr);

	for (int i=0; i<SearchThreadData::SEARCH_DIRECTIONS; ++i) {
		auto& data = directionalSearchData[i];
		data.openNodes = &searchThreadData->openNodes[i];
		data.minSearchNode = data.srcSearchNode;

		while (!data.openNodes->empty())
			data.openNodes->pop();
	}
	curSearchNode = nullptr;
	nextSearchNode = nullptr;
	// minSearchNode = fwd.srcSearchNode;
}

void QTPFS::PathSearch::LoadPartialPath(IPath* path) {
	ZoneScoped;
	auto& nodes = path->GetNodeList();

	auto addNode = [this](uint32_t dir, uint32_t nodeId, uint32_t prevNodeId, const float2& netPoint, uint32_t stepIndex){
		auto& searchNodes = searchThreadData->allSearchedNodes[dir];
		SearchNode& searchNode = searchNodes.InsertINodeIfNotPresent(nodeId);
		SearchNode* prevSearchNode = (prevNodeId != -1) ? &searchNodes.InsertINodeIfNotPresent(prevNodeId) : nullptr;
		searchNode.SetPrevNode(prevSearchNode);
		searchNode.SetNeighborEdgeTransitionPoint(netPoint);
		searchNode.SetStepIndex(stepIndex);
	};

	uint32_t badNodeCount = 0;
	{
		uint32_t stepIndex = 1;
		uint32_t prevNodeId = -1;
		uint32_t badPrevNodeId = -1;
		std::for_each(nodes.begin(), nodes.end(), [&addNode, &prevNodeId, &badPrevNodeId, &stepIndex, &badNodeCount](const IPath::PathNodeData& node){
			if (node.netPoint.x != -1.f) {
				addNode(SearchThreadData::SEARCH_FORWARD, node.nodeId, prevNodeId, node.netPoint, stepIndex++);
				prevNodeId = node.nodeId;
			} else {
				// mark node being on an incomplete route and won't link back to forward src.
				// used by reverse partial search to drop out early. Set stepindex to non-zero so
				// it can't be confused with a genuine search step by the forward searcher.
				addNode(SearchThreadData::SEARCH_FORWARD, node.nodeId, badPrevNodeId, node.netPoint, 999);
				badPrevNodeId = node.nodeId;
				badNodeCount++;
			}
		});
	}
	{
		uint32_t stepIndex = nodes.size() - badNodeCount;
		float2 prevNetPoint;
		uint32_t prevNodeId = -1;
		std::for_each(nodes.rbegin(), nodes.rend(), [&addNode, &prevNodeId, &prevNetPoint, &stepIndex](const IPath::PathNodeData& node){
			if (node.netPoint.x != -1.f) {
				addNode(SearchThreadData::SEARCH_BACKWARD, node.nodeId, prevNodeId, prevNetPoint, stepIndex--);
				prevNodeId = node.nodeId;
				prevNetPoint = node.netPoint;
			}
			// we don't need to set incomplete route step indicies becasue the reverse path
			// explicitly doesn't capture the step index if it hits an early drop out.
		});
	}

	searchEarlyDrop = false;
}

bool QTPFS::PathSearch::Execute(unsigned int searchStateOffset) {
	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	haveFullPath = (fwd.srcSearchNode == fwd.tgtSearchNode);
	havePartPath = false;

	// early-out
	if (haveFullPath) 
		return true;

	if (rawPathCheck)
		return ExecuteRawSearch();

	return ExecutePathSearch();
}

void QTPFS::PathSearch::InitStartingSearchNodes() {
	fwdPathConnected = false;
	bwdPathConnected = false;
	fwdAreaSearchLimit = std::numeric_limits<int>::max();
	searchThreadData->ResetQueue();

	for (int i = 0; i < QTPFS::SEARCH_DIRS; ++i) {
		auto& data = directionalSearchData[i];
		ResetState(data.srcSearchNode, data);
		UpdateNode(data.srcSearchNode, nullptr, 0);
	}
}

void QTPFS::PathSearch::UpdateHcostMult() {
	auto& comp = systemGlobals.GetSystemComponent<PathSpeedModInfoSystemComponent>();

	// be as optimistic as possible: assume the remainder of our path will
	// cover only flat terrain with maximum speed-modifier between nxtPoint
	// and tgtPoint
	switch (searchType) {
		// This, by default, guarantees the best path, but underestimates distance costs considerabily.
		// Searching more quads than neccessary is an impact to performance. So a slight increase to the
		// hcost can be applied to help. Care needs to be taken because if it goes too far, then
		// performance can drop further instead as the search shifts to best-first-search.
		case PATH_SEARCH_ASTAR:
			{
				const auto& speedModInfo = comp.relSpeedModinfos[nodeLayer->GetNodelayer()];
				const float maxSpeedMod = speedModInfo.max;
				const float meanSpeedMod = speedModInfo.mean;
				const float chosenSpeedMod = maxSpeedMod - (maxSpeedMod - meanSpeedMod) * modInfo.pfHcostMult;
				hCostMult = 1.0f / chosenSpeedMod;
			}
			break;
		case PATH_SEARCH_DIJKSTRA:
			hCostMult = 0.0f;
			break;
	}
}

void QTPFS::PathSearch::RemoveOutdatedOpenNodesFromQueue() {
	// Remove any out-of-date node entries in the queue.
	for (int i = 0; i < QTPFS::SEARCH_DIRS; ++i) {
		DirectionalSearchData& searchData = directionalSearchData[i];

		while (!(*searchData.openNodes).empty()) {
			SearchQueueNode curOpenNode = (*searchData.openNodes).top();
			assert(searchThreadData->allSearchedNodes[i].isSet(curOpenNode.nodeIndex));
			curSearchNode = &searchThreadData->allSearchedNodes[i][curOpenNode.nodeIndex];
			
			// Check if this node entity is valid
			if (curOpenNode.heapPriority <= curSearchNode->GetHeapPriority())
				break;

			// remove the entry
			(*searchData.openNodes).pop();
		}
	}
}

bool QTPFS::PathSearch::IsNodeActive(const SearchNode& curSearchNode) const {
	// prevNode catches all cases except the starting node.
	// Path H-cost will avoid reporting unwalked target node as a false positive, but catch starting node.
	return (curSearchNode.GetPrevNode() != nullptr)
		|| (curSearchNode.GetPathCost(NODE_PATH_COST_H) != std::numeric_limits<float>::infinity());
}

void QTPFS::PathSearch::SetForwardSearchLimit() {
	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

	/* These values have been chosen by testing and anlysis. They give a reasonable starting point
	 * balancing performance gains against the chance that a poor path will result. I suspect these
	 * can be improved, but this will need further stress testing.
	 * 
	 * TODO: make into mod rules so that games can calibrate them.
	 */
	// Maximum area of the map to search.
	constexpr float maxRelativeMapAreaToSearch = (1.f/16.f);

	// How wide to spread as distance increases.
	constexpr float areaToSearchScale = (1.f/8.f);

	// Nodes soemtimes get revisited, so increase the resultant area to compensate.
	// We don't keep track of whether a node has been visited before because that would incur an
	// otherwise unneccessary cache write-back for every node visited.
	constexpr float scaleForNodeRevisits = 1.1f;

	float maxMapLength = std::max(mapDims.mapx, mapDims.mapy);
	float maxSearchArea = mapDims.mapx*mapDims.mapy*maxRelativeMapAreaToSearch;
	float dist = 0.f;

	if (hCostMult != 0.f) {
		dist = bwd.minSearchNode->GetPathCost(NODE_PATH_COST_H) / (hCostMult * SQUARE_SIZE);
	}
	else {
		dist = fwd.tgtPoint.distance2D(fwd.srcPoint)/SQUARE_SIZE;
	}
	fwdAreaSearchLimit = std::clamp(dist*dist*areaToSearchScale, 100.f, maxSearchArea) * scaleForNodeRevisits;
}

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

	auto nodeIsTemp = [](const SearchNode& curSearchNode) {
		return (curSearchNode.GetPathCost(NODE_PATH_COST_H) == std::numeric_limits<float>::infinity());
	};

	auto nodeIsEarlyDrop = [](const SearchNode& curSearchNode) {
		return (curSearchNode.GetNeighborEdgeTransitionPoint().x == -1.f);
	};

	auto reverseTrace = [this](int dir) {
		auto& fwd = directionalSearchData[dir];
		auto& fwdSearchNodes = searchThreadData->allSearchedNodes[dir];
		auto& revSearchNodes = searchThreadData->allSearchedNodes[1 - dir];

		if (!revSearchNodes.isSet(fwd.tgtSearchNode->GetIndex()))
			return;

		SearchNode* revCurNode = &revSearchNodes[fwd.tgtSearchNode->GetIndex()];
		while (revCurNode->GetPrevNode() != nullptr) {
			revCurNode = revCurNode->GetPrevNode();
		}
		fwd.tgtSearchNode = &fwdSearchNodes[revCurNode->GetIndex()];
		fwd.minSearchNode = fwd.tgtSearchNode;
	};

	assert(fwd.srcSearchNode != nullptr);

	while (continueSearching) {
		RemoveOutdatedOpenNodesFromQueue();

		if (!(*fwd.openNodes).empty()) {
			// fwdNodesSearched++;
			IterateNodes(SearchThreadData::SEARCH_FORWARD);

			// Search area limits are only used when the reverse search has determined that the
			// goal is not reachable.
			if (fwd.areaSearched >= fwdAreaSearchLimit)
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

					// If step Index is 0, then a full path was found. This can happen for partial
					// searches, if the partial path isn't actually close enough.
					if (curSearchNode->GetStepIndex() == 0) { isFullSearch = true; }
					fwd.tgtSearchNode = curSearchNode;
					searchThreadData->ResetQueue(SearchThreadData::SEARCH_FORWARD);
				}

				haveFullPath = (isFullSearch) ? fwdPathConnected : bwdPathConnected & fwdPathConnected;
				if (haveFullPath){
					assert(!searchEarlyDrop);
					fwd.tgtSearchNode = curSearchNode->GetPrevNode();
					bwd.tgtSearchNode = &bwdNode;

					const float2& searchTransitionPoint = curSearchNode->GetNeighborEdgeTransitionPoint();
					bwd.tgtPoint = float3(searchTransitionPoint.x, 0.f, searchTransitionPoint.y);
					searchThreadData->ResetQueue(SearchThreadData::SEARCH_BACKWARD);
				}
			}
		}

		if (!(*bwd.openNodes).empty()) {
			IterateNodes(SearchThreadData::SEARCH_BACKWARD);

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

					// If step Index is 0, then a full path was found. This can happen for partial
					// searches, if the partial path isn't actually close enough.
					if (curSearchNode->GetStepIndex() == 0) { isFullSearch = true; }
				}
				if (bwdPathConnected || searchEarlyDrop) {
					bwd.tgtSearchNode = curSearchNode;
					searchThreadData->ResetQueue(SearchThreadData::SEARCH_BACKWARD);
				}

				haveFullPath = (isFullSearch) ? bwdPathConnected : bwdPathConnected & fwdPathConnected;
				if (haveFullPath) {
					assert(!searchEarlyDrop);
					bwd.tgtSearchNode = curSearchNode->GetPrevNode();
					fwd.tgtSearchNode = &fwdNode;

					const float2& searchTransitionPoint = curSearchNode->GetNeighborEdgeTransitionPoint();
					bwd.tgtPoint = float3(searchTransitionPoint.x, 0.f, searchTransitionPoint.y);
					searchThreadData->ResetQueue(SearchThreadData::SEARCH_FORWARD);
				}
			}
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

	if (searchEarlyDrop) {
		// move forward only needed for incomplete partial searches.
		reverseTrace(SearchThreadData::SEARCH_FORWARD);
		reverseTrace(SearchThreadData::SEARCH_BACKWARD);
	} else if (doPartialSearch) {
		// Sanity check the path is properly connected. Partial searches can fail this check.
		// If a partial search found a full path instead of connecting to the partial path, then
		// isFullSearch is set to true and a check isn't needed.
		if (haveFullPath) {
			if (!isFullSearch) {
				if (fwdStepIndex > bwdStepIndex){
					rejectPartialSearch = true;
					// LOG("%s: rejecting partial path.", __func__);
					return false;
				}
			}
		} else {
			// if the partial path could not connect the reverse path, then we need to reject.
			if (fwdPathConnected && !bwdPathConnected) {
				rejectPartialSearch = true;
				// LOG("%s: rejecting partial path.", __func__);
				return false;
			}
		}
	}

	havePartPath = (fwd.minSearchNode != fwd.srcSearchNode);

	#ifdef QTPFS_SUPPORT_PARTIAL_SEARCHES
	// adjust the target-point if we only got a partial result
	// NOTE:
	//   movement code has handles special cases where the last waypoint isn't reachable so there
	//   is no need to change the target point anymore.
	if (!haveFullPath && havePartPath) {
		fwd.tgtSearchNode = fwd.minSearchNode;
		auto* minNode = nodeLayer->GetPoolNode(fwd.minSearchNode->GetIndex());

		// used to trace a bad path to give partial searches a chance of an early out
		// in reverse searches.
		bwd.tgtSearchNode = bwd.minSearchNode;
	}
	#endif

	return (haveFullPath || havePartPath);
}


bool QTPFS::PathSearch::ExecuteRawSearch() {
	ZoneScoped;
	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	haveFullPath = moveDefHandler.GetMoveDefByPathType(nodeLayer->GetNodelayer())
			->DoRawSearch( pathOwner, fwd.srcPoint, fwd.tgtPoint, pathOwner->speed
						 , true, true, false, nullptr, nullptr, searchThreadData->threadId);

	return haveFullPath;
}


void QTPFS::PathSearch::ResetState(SearchNode* node, struct DirectionalSearchData& searchData) {
	// will be copied into srcNode by UpdateNode()
	netPoints[0] = {searchData.srcPoint.x, searchData.srcPoint.z};

	gDists[0] = 0.0f;
	hDists[0] = searchData.srcPoint.distance(searchData.tgtPoint);
	gCosts[0] = 0.0f;
	hCosts[0] = hDists[0] * hCostMult;

	for (unsigned int i = 1; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
		netPoints[i] = {0.0f, 0.0f};

		gDists[i] = 0.0f;
		hDists[i] = 0.0f;
		gCosts[i] = 0.0f;
		hCosts[i] = 0.0f;
	}

	// searchThreadData->ResetQueue();
	(*searchData.openNodes).emplace(node->GetIndex(), 0.f);
}

void QTPFS::PathSearch::UpdateNode(SearchNode* nextNode, SearchNode* prevNode, unsigned int netPointIdx) {
	// NOTE:
	//   the heuristic must never over-estimate the distance,
	//   but this is *impossible* to achieve on a non-regular
	//   grid on which any node only has an average move-cost
	//   associated with it --> paths will be "nearly optimal"
	nextNode->SetPrevNode(prevNode);
	nextNode->SetPathCosts(gCosts[netPointIdx], hCosts[netPointIdx]);
	nextNode->SetNeighborEdgeTransitionPoint(netPoints[netPointIdx]);
}

void QTPFS::PathSearch::IterateNodes(unsigned int searchDir) {
	ZoneScoped;
	DirectionalSearchData& searchData = directionalSearchData[searchDir];

	SearchQueueNode curOpenNode = (*searchData.openNodes).top();
	(*searchData.openNodes).pop();

	// LOG("%s: curNode=%d", __func__, curOpenNode.nodeIndex);

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

	// Check if we've linked up with the other search
	auto& otherNodes = searchThreadData->allSearchedNodes[1 - searchDir];
	if (otherNodes.isSet(curSearchNode->GetIndex())){
		// Check whether it has been processed yet
		if (IsNodeActive(otherNodes[curSearchNode->GetIndex()]))
			return;
	}

	#ifdef QTPFS_SUPPORT_PARTIAL_SEARCHES

	if (searchDir == SearchThreadData::SEARCH_FORWARD) {
		// remember the node with lowest h-cost in case the search fails to reach tgtNode
		if (curSearchNode->GetPathCost(NODE_PATH_COST_H) < searchData.minSearchNode->GetPathCost(NODE_PATH_COST_H))
			searchData.minSearchNode = curSearchNode;
	}

	#endif

	assert(curSearchNode->GetIndex() == curOpenNode.nodeIndex);

	auto* curNode = nodeLayer->GetPoolNode(curOpenNode.nodeIndex);
	searchData.areaSearched += curNode->area();

	IterateNodeNeighbors(curNode, searchDir);
}

void QTPFS::PathSearch::IterateNodeNeighbors(const INode* curNode, unsigned int searchDir) {
	DirectionalSearchData& searchData = directionalSearchData[searchDir];

	const float2& curPoint2 = curSearchNode->GetNeighborEdgeTransitionPoint();
	const float3  curPoint  = {curPoint2.x, 0.0f, curPoint2.y};

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

		const bool isTarget = (nextSearchNode == searchData.tgtSearchNode);
		QTPFS::INode *nxtNode = nullptr;
		if (isTarget) {
			nxtNode = nodeLayer->GetPoolNode(nxtNodesId);
			assert(curNode->GetNeighborRelation(nxtNode) != 0);
			assert(nxtNode->GetNeighborRelation(curNode) != 0);
		}

		unsigned int netPointIdx = 0;

		// #if (QTPFS_MAX_NETPOINTS_PER_NODE_EDGE == 1)
		// /*if (!IntersectEdge(curNode, nxtNode, tgtPoint - curPoint))*/ {
		// 	// if only one transition-point is allowed per edge,
		// 	// this will always be the edge's center --> no need
		// 	// to be fancy (note that this is not always the best
		// 	// option, it causes local and global sub-optimalities
		// 	// which SmoothPath can only partially address)
		// 	netPoints[0] = curNode->GetNeighborEdgeTransitionPoint(1 + i);

		// 	// cannot use squared-distances because that will bias paths
		// 	// towards smaller nodes (eg. 1^2 + 1^2 + 1^2 + 1^2 != 4^2)
		// 	gDists[0] = curPoint.distance({netPoints[0].x, 0.0f, netPoints[0].y});
		// 	hDists[0] = tgtPoint.distance({netPoints[0].x, 0.0f, netPoints[0].y});
		// 	gCosts[0] =
		// 		curNode->GetPathCost(NODE_PATH_COST_G) +
		// 		curNode->GetMoveCost() * gDists[0] +
		// 		nxtNode->GetMoveCost() * hDists[0] * int(isTarget);
		// 	hCosts[0] = hDists[0] * hCostMult * int(!isTarget);
		// }
		// #else
		// examine a number of possible transition-points
		// along the edge between curNode and nxtNode and
		// pick the one that minimizes g+h
		// this fixes a few cases that path-smoothing can
		// not handle; more points means a greater degree
		// of non-cardinality (but gets expensive quickly)
		for (unsigned int j = 0; j < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; j++) {
			netPoints[j] = nxtNodes[i].netpoints[j];

			gDists[j] = curPoint.distance({netPoints[j].x, 0.0f, netPoints[j].y});
			hDists[j] = searchData.tgtPoint.distance({netPoints[j].x, 0.0f, netPoints[j].y});

			// Allow units to escape if starting in a closed node - a cost of inifinity would prevent them escaping.
			const float curNodeSanitizedCost = curNode->AllSquaresImpassable() ? QTPFS_CLOSED_NODE_COST : curNode->GetMoveCost();
			gCosts[j] =
				curSearchNode->GetPathCost(NODE_PATH_COST_G) +
				curNodeSanitizedCost * gDists[j];
			hCosts[j] = hDists[j] * hCostMult * int(!isTarget);

			if (isTarget) {
				gCosts[j] += nxtNode->GetMoveCost() * hDists[j];
			}

			if ((gCosts[j] + hCosts[j]) < (gCosts[netPointIdx] + hCosts[netPointIdx])) {
				netPointIdx = j;
			}
		}
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
		if (gCosts[netPointIdx] >= nextSearchNode->GetPathCost(NODE_PATH_COST_G))
			continue;

		// LOG("%s: [%d] nxtNode=%d updating gc from %f -> %f", __func__, searchDir, nxtNodesId
		// 		, nextSearchNode->GetPathCost(NODE_PATH_COST_G), gCosts[netPointIdx]);

		// LOG("%s: adding node (%d) gcost %f < %f [old p:%f]", __func__
		// 		, nextSearchNode->GetIndex()
		// 		, gCosts[netPointIdx]
		// 		, nextSearchNode->GetPathCost(NODE_PATH_COST_G)
		// 		, nextSearchNode->GetHeapPriority()
		// 		);

		UpdateNode(nextSearchNode, curSearchNode, netPointIdx);
		(*searchData.openNodes).emplace(nextSearchNode->GetIndex(), nextSearchNode->GetHeapPriority());
	}
}

void QTPFS::PathSearch::Finalize(IPath* path) {
	ZoneScoped;

	// LOG("%s: [%p : %d] Finialize search.", __func__
	// 		, &nodeLayer[path->GetPathType()]
	// 		, path->GetPathType()
	// 		);
	if (!rawPathCheck) {
		TracePath(path);

		#ifdef QTPFS_SMOOTH_PATHS
		SmoothPath(path);
		#endif
	}

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

void QTPFS::PathSearch::TracePath(IPath* path) {
	constexpr uint32_t ONLY_NODE_ID_MASK = 0x80000000;
	struct TracePoint{
		float3 point;
		uint32_t nodeId;
	};
	std::deque<TracePoint> points;
	int nodesWithoutPoints = 0;

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

	{
		// Only a bad path will be associated with the reverse path
		if (!haveFullPath)
		{
			const SearchNode* tmpNode = bwd.tgtSearchNode;
			while (tmpNode != nullptr) {
				// reverse path didn't connect so record each point as bad for early drop out
				// for other partial searches.
				points.emplace_back(float3(-1.f, 0.f, -1.f), tmpNode->GetIndex() | ONLY_NODE_ID_MASK);
				nodesWithoutPoints++;

				tmpNode = tmpNode->GetPrevNode();
			}
		}
		else {
			const SearchNode* tmpNode = bwd.tgtSearchNode;
			const SearchNode* prvNode = nullptr;

			float3 prvPoint = bwd.tgtPoint;

			while (tmpNode != nullptr) {
				prvNode = tmpNode->GetPrevNode();

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
				assert(tmpNode != prvNode);
				assert(prvPoint != float3());

				// This check is not helpful. In bigger maps this triggers for points on small (1x1) quads
				// assert(tmpPoint != prvPoint || tmpNode == fwd.tgtSearchNode);

				points.emplace_back(prvPoint, tmpNode->GetIndex() | (ONLY_NODE_ID_MASK * int(tmpPoint == prvPoint)));
				nodesWithoutPoints += int(tmpPoint == prvPoint);
				// LOG("%s: [%d] nxtNode=%d point (%f, %f, %f) [onlyNode=%d]", __func__, SearchThreadData::SEARCH_BACKWARD
				// 		, tmpNode->GetIndex(), prvPoint.x, prvPoint.y, prvPoint.z, int(tmpPoint == prvPoint));

				#ifndef QTPFS_SMOOTH_PATHS
				// make sure the back-pointers can never become dangling
				// (if smoothing IS enabled, we delay this until we reach
				// SmoothPath() because we still need them there)
				tmpNode->SetPrevNode(nullptr);
				#endif

				prvPoint = tmpPoint;
				tmpNode = prvNode;
			}
		}

		const SearchNode* tmpNode = fwd.tgtSearchNode;
		const SearchNode* prvNode = (tmpNode != nullptr) ? tmpNode->GetPrevNode() : nullptr;

		float3 prvPoint = fwd.tgtPoint;

		while ((prvNode != nullptr) && (tmpNode != fwd.srcSearchNode)) {
			const float2& tmpPoint2 = tmpNode->GetNeighborEdgeTransitionPoint();
			const float3  tmpPoint  = {tmpPoint2.x, 0.0f, tmpPoint2.y};

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
			assert(tmpNode != prvNode);
			assert(tmpPoint != float3());

			// assert(tmpPoint != prvPoint || tmpNode == fwd.tgtSearchNode || doPartialSearch);

			points.emplace_front(tmpPoint, tmpNode->GetIndex() | (ONLY_NODE_ID_MASK * int(tmpPoint == prvPoint)));
			nodesWithoutPoints += int(tmpPoint == prvPoint);
			// LOG("%s: [%d] nxtNode=%d point (%f, %f, %f) [onlyNode=%d]", __func__, SearchThreadData::SEARCH_FORWARD
			// 		, tmpNode->GetIndex(), tmpPoint.x, tmpPoint.y, tmpPoint.z, int(tmpPoint == prvPoint));


			#ifndef QTPFS_SMOOTH_PATHS
			// make sure the back-pointers can never become dangling
			// (if smoothing IS enabled, we delay this until we reach
			// SmoothPath() because we still need them there)
			tmpNode->SetPrevNode(nullptr);
			#endif

			prvPoint = tmpPoint;
			tmpNode = prvNode;
			prvNode = tmpNode->GetPrevNode();
		}

		// ensure the starting quad is shared with other path searches.
		if (tmpNode != nullptr) {
			points.emplace_front(float3(), tmpNode->GetIndex() | ONLY_NODE_ID_MASK);
			// LOG("%s: [%d] tgtNode=%d point ", __func__, SearchThreadData::SEARCH_FORWARD
			// 		, tmpNode->GetIndex());
			nodesWithoutPoints++;
		}
	}

	// if source equals target, we need only two points
	if (!points.empty()) {
		path->AllocPoints(points.size() + 2 + (-nodesWithoutPoints));
		path->AllocNodes(points.size());
	} else {
		assert(path->NumPoints() == 2);
	}

	// set waypoints with indices [1, N - 2] (if any)
	int pointIndex = 1;
	int nodeIndex = 0;
	while (!points.empty()) {
		int nodePointIndex = -1;
		float3& point   = points.front().point;
		uint32_t nodeId = points.front().nodeId;

		if ( (nodeId & ONLY_NODE_ID_MASK) == 0 ){
			assert(point != float3());
			nodePointIndex = pointIndex;
			path->SetPoint(pointIndex++, point);
			// LOG("%s: setting point (%f, %f, %f)", __func__, point.x, point.y, point.z);
		}
		path->SetNode(nodeIndex++, nodeId & ~ONLY_NODE_ID_MASK, float2(point.x, point.z), nodePointIndex);
		// LOG("%s: tgtNode=%d point (%f, %f, %f)", __func__
		// 		, nodeId, point.x, point.y, point.z);
		points.pop_front();
	}

	// set the first (0) and last (N - 1) waypoint
	path->SetSourcePoint(fwd.srcPoint);
	path->SetTargetPoint(fwd.tgtPoint);

	assert(fwd.srcPoint != float3());
	assert(fwd.tgtPoint != float3());
}

void QTPFS::PathSearch::SmoothPath(IPath* path) {
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
	// smooth in reverse order (target to source)
	//
	// should terminate when waypoints stop moving,
	// or after a small fixed number of iterations
	unsigned int ni = path->NumPoints();
	unsigned int nm = 0;

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	auto& nodePath = path->GetNodeList();
	auto getNextNodeIndex = [&nodePath](int i){
		while (--i >= 0) {
			const IPath::PathNodeData* node = &nodePath[i];
			if (node->pathPointIndex > -1)
				break;
		}
		return i;
	};

	int nodeIdx = getNextNodeIndex(nodePath.size());
	assert(nodeIdx > -1);

	const IPath::PathNodeData* n1 = &nodePath[nodeIdx];
	INode* nn0 = nodeLayer->GetPoolNode(n1->nodeId);
	INode* nn1 = nn0;

	for (; ni > 0;) {
		nodeIdx = getNextNodeIndex(nodeIdx);
		if (nodeIdx < 0)
			break;

		n1 = &nodePath[nodeIdx];

		nn0 = nn1;
		nn1 = nodeLayer->GetPoolNode(n1->nodeId);

		ni -= 1;

		assert(nn1->GetNeighborRelation(nn0) != 0);
		assert(nn0->GetNeighborRelation(nn1) != 0);
		assert(ni < path->NumPoints());

		const unsigned int ngbRel = nn0->GetNeighborRelation(nn1);

		const float3 p0 = path->GetPoint(ni    );
		const float3 p1 = path->GetPoint(ni - 1);
		const float3 p2 = path->GetPoint(ni - 2);

		float3 pi = ZeroVector;

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
			continue;

		// figure out if p1 is on a horizontal or a vertical edge
		// (if both of these are true, it is in fact in a corner)
		const bool hEdge = (((ngbRel & REL_NGB_EDGE_T) != 0) || ((ngbRel & REL_NGB_EDGE_B) != 0));
		const bool vEdge = (((ngbRel & REL_NGB_EDGE_L) != 0) || ((ngbRel & REL_NGB_EDGE_R) != 0));

		assert(hEdge || vEdge);

		// establish the x- and z-range within which p1 can be moved
		const unsigned int xmin = std::max(nn1->xmin(), nn0->xmin());
		const unsigned int zmin = std::max(nn1->zmin(), nn0->zmin());
		const unsigned int xmax = std::min(nn1->xmax(), nn0->xmax());
		const unsigned int zmax = std::min(nn1->zmax(), nn0->zmax());

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

			ok = ok && (pi.x >= (xmin * SQUARE_SIZE) && pi.x <= (xmax * SQUARE_SIZE));
			ok = ok && (pi.z >= (zmin * SQUARE_SIZE) && pi.z <= (zmax * SQUARE_SIZE));

			if (ok) {
				nm += ((pi - p1).SqLength2D() > Square(0.05f));

				assert(!math::isinf(pi.x) && !math::isinf(pi.z));
				assert(!math::isnan(pi.x) && !math::isnan(pi.z));
				// LOG("%s: %" PRIx64  " [t:%d] added point %d (%f,%f,%f) ", __func__
				// 		, path->GetHash()
				// 		, path->GetPathType()
				// 		, ni - 1
				// 		, pi.x
				// 		, pi.y
				// 		, pi.z
				// 		);
				path->SetPoint(ni - 1, pi);
				continue;
			}
		}

		if (hEdge != vEdge) {
			// get the edge end-points
			float3 e0 = p1;
			float3 e1 = p1;

			if (hEdge) {
				e0.x = xmin * SQUARE_SIZE;
				e1.x = xmax * SQUARE_SIZE;
			}
			if (vEdge) {
				e0.z = zmin * SQUARE_SIZE;
				e1.z = zmax * SQUARE_SIZE;
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
				continue;

			if (dot0 > std::max(dot1, dot)) { pi = e0; }
			if (dot1 >= std::max(dot0, dot)) { pi = e1; }

			nm += ((pi - p1).SqLength2D() > Square(0.05f));

			assert(!math::isinf(pi.x) && !math::isinf(pi.z));
			assert(!math::isnan(pi.x) && !math::isnan(pi.z));
			path->SetPoint(ni - 1, pi);
		}
	}

	return (nm != 0);
}



bool QTPFS::PathSearch::SharedFinalize(const IPath* srcPath, IPath* dstPath) {
	assert(dstPath->GetID() != 0);
	assert(dstPath->GetID() != srcPath->GetID());
	assert(dstPath->NumPoints() == 2);

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

	// copy <srcPath> to <dstPath>
	dstPath->CopyPoints(*srcPath);
	dstPath->CopyNodes(*srcPath);
	dstPath->SetSourcePoint(fwd.srcPoint);
	dstPath->SetTargetPoint(fwd.tgtPoint);
	dstPath->SetBoundingBox();
	dstPath->SetHasFullPath(srcPath->IsFullPath());
	dstPath->SetHasPartialPath(srcPath->IsPartialPath());
	dstPath->SetSearchTime(srcPath->GetSearchTime());

	haveFullPath = srcPath->IsFullPath();
	havePartPath = srcPath->IsPartialPath();

	return true;
}

unsigned int GetChildId(uint32_t nodeNumber, uint32_t i, uint32_t rootMask) {
	uint32_t rootId = rootMask & nodeNumber;
	uint32_t nodeId = ((~rootMask) & nodeNumber);
	return rootId | ((nodeId << 2) + (i + 1));
}

const std::uint64_t QTPFS::PathSearch::GenerateHash(const INode* srcNode, const INode* tgtNode) const {
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
	uint32_t srcNodeNumber = GenerateVirtualNodeNumber(srcNode, QTPFS_SHARE_PATH_MAX_SIZE, fwd.srcPoint.x / SQUARE_SIZE, fwd.srcPoint.z / SQUARE_SIZE);

	return GenerateHash2(srcNodeNumber, tgtNode->GetNodeNumber());
}

const std::uint64_t QTPFS::PathSearch::GenerateVirtualHash(const INode* srcNode, const INode* tgtNode) const {
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

	std::uint32_t vSrcNodeId = GenerateVirtualNodeNumber(srcRootNode, QTPFS_PARTIAL_SHARE_PATH_MAX_SIZE, srcX, srcZ);
	std::uint32_t vTgtNodeId = GenerateVirtualNodeNumber(tgtRootNode, QTPFS_PARTIAL_SHARE_PATH_MAX_SIZE, tgtX, tgtZ);

	// Within same cell is too close?
	// if (vSrcNodeId == vTgtNodeId)
	// 	return BAD_HASH;

	return GenerateHash2(vSrcNodeId, vTgtNodeId);
}

const std::uint64_t QTPFS::PathSearch::GenerateHash2(uint32_t src, uint32_t dest) const {
	std::uint64_t N = mapDims.mapx * mapDims.mapy;
	std::uint32_t k = nodeLayer->GetNodelayer();

	return (src + (dest * N) + (k * N * N));
}

const std::uint32_t QTPFS::PathSearch::GenerateVirtualNodeNumber(const INode* startNode, int nodeMaxSize, int x, int z) const {
	uint32_t nodeSize = startNode->xsize();
	uint32_t srcNodeNumber = startNode->GetNodeNumber();
	uint32_t xoff = startNode->xmin();
	uint32_t zoff = startNode->zmin();

	while (nodeSize > nodeMaxSize) {
		// build the rest of the virtual node number
		bool isRight = x >= xoff + (nodeSize >> 1);
		bool isDown = z >= zoff + (nodeSize >> 1);
		int offset = 1*(isRight) + 2*(isDown);

		// TODO: sanity check if it isn't possible to go down this many levels?
		srcNodeNumber = GetChildId(srcNodeNumber, offset, nodeLayer->GetRootMask());

		nodeSize >>= 1;
		xoff += nodeSize*isRight;
		zoff += nodeSize*isDown;
	}
	return srcNodeNumber;
}
