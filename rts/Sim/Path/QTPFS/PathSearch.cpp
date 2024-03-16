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

#ifdef QTPFS_TRACE_PATH_SEARCHES
#include "Sim/Misc/GlobalSynced.h"
#endif

#include "System/float3.h"

#include <tracy/Tracy.hpp>

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
	//ZoneScoped;
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
	//ZoneScoped;
	dest.xmin = src.xmin();
	dest.zmin = src.zmin();
	dest.xmax = src.xmax();
	dest.zmax = src.zmax();
}

void QTPFS::PathSearch::Initialize(
	NodeLayer* layer,
	const float3& sourcePoint,
	const float3& targetPoint,
	const CSolidObject* owner
) {
	//ZoneScoped;
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

void QTPFS::PathSearch::InitializeThread(SearchThreadData* threadData) {
	ZoneScoped;
	searchThreadData = threadData;

	badGoal = false;

	// add 2 just in case the start and end nodes are closed. They can escape those nodes and check
	// all the open nodes. No more is required because nodes don't link themselves to closed nodes.
	searchThreadData->Init(nodeLayer->GetMaxNodesAlloced(), nodeLayer->GetNumOpenNodes() + 2);

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

	INode* srcNode = nodeLayer->GetNode(fwd.srcPoint.x / SQUARE_SIZE, fwd.srcPoint.z / SQUARE_SIZE);
	INode* tgtNode = nodeLayer->GetNode(fwd.tgtPoint.x / SQUARE_SIZE, fwd.tgtPoint.z / SQUARE_SIZE);

	if (tgtNode->AllSquaresImpassable()) {
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

	for (int i=0; i<SearchThreadData::SEARCH_DIRECTIONS; ++i) {
		auto& data = directionalSearchData[i];
		data.openNodes = &searchThreadData->openNodes[i];
		data.minSearchNode = data.srcSearchNode;

		while (!data.openNodes->empty())
			data.openNodes->pop();
	}
	curSearchNode = nullptr;
	nextSearchNode = nullptr;
}

// #pragma GCC push_options
// #pragma GCC optimize ("O0")

void QTPFS::PathSearch::LoadPartialPath(IPath* path) {
	ZoneScoped;
	auto& nodes = path->GetNodeList();

	assert(path->GetPathType() == pathType);

	searchEarlyDrop = false;

	auto addNode = [this](uint32_t dir, uint32_t nodeId, uint32_t prevNodeId, const float2& netPoint, uint32_t stepIndex){
		auto& searchNodes = searchThreadData->allSearchedNodes[dir];
		SearchNode& searchNode = searchNodes.InsertINodeIfNotPresent(nodeId);
		SearchNode* prevSearchNode = (prevNodeId != -1) ? &searchNodes.InsertINodeIfNotPresent(prevNodeId) : nullptr;
		searchNode.SetPrevNode(prevSearchNode);
		searchNode.SetNeighborEdgeTransitionPoint(netPoint);
		searchNode.SetStepIndex(stepIndex);

		auto* curNode = nodeLayer->GetPoolNode(nodeId);
		CopyNodeBoundaries(searchNode, *curNode);

		#ifndef NDEBUG
		if (prevNodeId != -1) {
			INode* nn0 = nodeLayer->GetPoolNode(prevNodeId);
			INode* nn1 = nodeLayer->GetPoolNode(nodeId);

			assert(nn1->GetNeighborRelation(nn0) != 0);
			assert(nn0->GetNeighborRelation(nn1) != 0);
		}
		#endif
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
	expectIncompletePartialSearch = (badNodeCount > 0);
}

// #pragma GCC pop_options

bool QTPFS::PathSearch::Execute(unsigned int searchStateOffset) {
	//ZoneScoped;
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
	//ZoneScoped;
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
		ResetState(data.srcSearchNode, data);
		UpdateNode(data.srcSearchNode, nullptr, 0);

		auto* curNode = nodeLayer->GetPoolNode(data.srcSearchNode->GetIndex());
		CopyNodeBoundaries(*data.srcSearchNode, *curNode);
	}
}

void QTPFS::PathSearch::UpdateHcostMult() {
	//ZoneScoped;
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
				hCostMult = 1.0f / speedModInfo.max;
			}
			break;
		case PATH_SEARCH_DIJKSTRA:
			hCostMult = 0.0f;
			break;
	}

	adjustedGoalDistance = goalDistance * hCostMult;
}

void QTPFS::PathSearch::RemoveOutdatedOpenNodesFromQueue(int searchDir) {
	//ZoneScoped;
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
	//ZoneScoped;
	// prevNode catches all cases except the starting node.
	// Path H-cost will avoid reporting unwalked target node as a false positive, but catch starting node.
	return (curSearchNode.GetPrevNode() != nullptr)
		|| (curSearchNode.GetPathCost(NODE_PATH_COST_H) != std::numeric_limits<float>::infinity());
}

static float CircularEaseOut(float t) {
	//ZoneScoped;
	// Only using 0-1 range, the sqrt is too intense early on.
	return /*math::sqrt(*/ 1 - Square(t-1); //);
}

void QTPFS::PathSearch::SetForwardSearchLimit() {
	//ZoneScoped;
	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];
	auto& bwd = directionalSearchData[SearchThreadData::SEARCH_BACKWARD];

	/* These values have been chosen by testing and anlysis. They give a reasonable starting point
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

	auto copyNodeBoundary = [this](SearchNode& dest) {
		if (!(dest.xmax > 0 || dest.zmax > 0)) {
			auto* curNode = nodeLayer->GetPoolNode(dest.GetIndex());
			CopyNodeBoundaries(dest, *curNode);
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

	while (continueSearching) {
		if (!(*fwd.openNodes).empty()) {
			fwdNodesSearched++;
			IterateNodes(SearchThreadData::SEARCH_FORWARD);

			// Search area limits are only used when the reverse search has determined that the
			// goal is not reachable. UPDATE: not anymore, they are now always in effect due to
			// several scenarios that impact performance.
			// 1. Maps with huge islands.
			// 2. Players wall off the map in PvE modes, create huge artifical islands.
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
					fwd.tgtSearchNode = curSearchNode->GetPrevNode();
					bwd.tgtSearchNode = &bwdNode;

					const float2& searchTransitionPoint = curSearchNode->GetNeighborEdgeTransitionPoint();
					bwd.tgtPoint = float3(searchTransitionPoint.x, 0.f, searchTransitionPoint.y);
					searchThreadData->ResetQueue(SearchThreadData::SEARCH_BACKWARD);
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

		// Final position needs to be the closest point on the last quad to the goal.
		fwd.tgtPoint = FindNearestPointOnNodeToGoal(*fwd.tgtSearchNode, goalPos);
	} else if (badGoal) {
		// reverse source point has already been setup as the closest point to the goalPos.
		fwd.tgtPoint = bwd.srcPoint;
	} else if (useFwdPathOnly) {
		fwd.tgtPoint = FindNearestPointOnNodeToGoal(*fwd.tgtSearchNode, goalPos);
	}
	#endif

	// LOG("%s: search %x result(%d||%d) nodes searched (%d, %d) free nodes (%i)", __func__, this->GetID()
	// 		, int(haveFullPath), int(havePartPath), int(fwdNodesSearched), int(bwdNodesSearched)
	// 		, nodeLayer->GetNumOpenNodes());

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
	//ZoneScoped;
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

	(*searchData.openNodes).emplace(node->GetIndex(), 0.f);
}

void QTPFS::PathSearch::UpdateNode(SearchNode* nextNode, SearchNode* prevNode, unsigned int netPointIdx) {
	//ZoneScoped;
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

	// remember the node with lowest h-cost in case the search fails to reach tgtNode
	if (curSearchNode->GetPathCost(NODE_PATH_COST_H) < searchData.minSearchNode->GetPathCost(NODE_PATH_COST_H))
		searchData.minSearchNode = curSearchNode;

	#endif

	if (curSearchNode->GetPathCost(NODE_PATH_COST_H) <= adjustedGoalDistance)
		return;

	assert(curSearchNode->GetIndex() == curOpenNode.nodeIndex);

	auto* curNode = nodeLayer->GetPoolNode(curOpenNode.nodeIndex);

	curSearchNode->xmin = curNode->xmin();
	curSearchNode->xmax = curNode->xmax();
	curSearchNode->zmin = curNode->zmin();
	curSearchNode->zmax = curNode->zmax();

	IterateNodeNeighbors(curNode, searchDir);
}

void QTPFS::PathSearch::IterateNodeNeighbors(const INode* curNode, unsigned int searchDir) {
	//ZoneScoped;
	DirectionalSearchData& searchData = directionalSearchData[searchDir];

	const float2& curPoint2 = curSearchNode->GetNeighborEdgeTransitionPoint();
	const float3  curPoint  = {curPoint2.x, 0.0f, curPoint2.y};

	// Allow units to escape if starting in a closed node - a cost of inifinity would prevent them escaping.
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

			gCosts[j] =
				curSearchNode->GetPathCost(NODE_PATH_COST_G) +
				curNodeSanitizedCost * gDists[j];
			hCosts[j] = hDists[j] * hCostMult * int(!isTarget);

			if (isTarget) {
				gCosts[j] += nxtNode->GetMoveCost() * hDists[j];
			}

			// if ((gCosts[j] + hCosts[j]) < (gCosts[netPointIdx] + hCosts[netPointIdx])) {
			// 	netPointIdx = j;
			// }
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
	} else {
		auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

		path->AllocPoints(2);
		path->SetSourcePoint({fwd.srcPoint.x, 0.f, fwd.srcPoint.z});
		path->SetTargetPoint({fwd.tgtPoint.x, 0.f, fwd.tgtPoint.z});
		path->SetGoalPosition(path->GetTargetPoint());
	}
	path->SetRepathTriggerIndex(0);
	path->SetNextPointIndex(0);

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
	//ZoneScoped;
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
	//ZoneScoped;
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

	// No collision means the nearest point was really the nearest point. We can't do better.
	return (collide) ? cq.GetHitPos() + rm : lastPoint;
}

void QTPFS::PathSearch::TracePath(IPath* path) {
	//ZoneScoped;
	constexpr uint32_t ONLY_NODE_ID_MASK = 0x80000000;
	struct TracePoint{
		float3 point;
		uint32_t nodeId;
		int xmin;
		int zmin;
		int xmax;
		int zmax;
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
			while (tmpNode != nullptr) {
				// reverse path didn't connect so record each point as bad for early drop out
				// for other partial searches.
				points.emplace_back(float3(-1.f, 0.f, -1.f)
						, tmpNode->GetIndex() | ONLY_NODE_ID_MASK
						, tmpNode->xmin, tmpNode->zmin
						, tmpNode->xmax, tmpNode->zmax);
				nodesWithoutPoints++;
				boundaryMins.x = std::min(boundaryMins.x, float(tmpNode->xmin*SQUARE_SIZE));
				boundaryMins.z = std::min(boundaryMins.z, float(tmpNode->zmin*SQUARE_SIZE));
				boundaryMaxs.x = std::max(boundaryMaxs.x, float(tmpNode->xmax*SQUARE_SIZE));
				boundaryMaxs.z = std::max(boundaryMaxs.z, float(tmpNode->zmax*SQUARE_SIZE));

				tmpNode = tmpNode->GetPrevNode();
			}
		}
		else if (!useFwdPathOnly) {
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
				#ifndef NDEBUG
				if (prvNode != nullptr) {
					assert(tmpNode->GetIndex() != prvNode->GetIndex());

					INode* nn0 = nodeLayer->GetPoolNode(prvNode->GetIndex());
					INode* nn1 = nodeLayer->GetPoolNode(tmpNode->GetIndex());

					assert(nn1->GetNeighborRelation(nn0) != 0);
					assert(nn0->GetNeighborRelation(nn1) != 0);
				}
				#endif
				assert(prvPoint != float3());

				// This check is not helpful. In bigger maps this triggers for points on small (1x1) quads
				// assert(tmpPoint != prvPoint || tmpNode == fwd.tgtSearchNode);

				points.emplace_back(prvPoint
						, tmpNode->GetIndex() | (ONLY_NODE_ID_MASK * int(tmpPoint == prvPoint))
						, tmpNode->xmin, tmpNode->zmin
						, tmpNode->xmax, tmpNode->zmax);
				nodesWithoutPoints += int(tmpPoint == prvPoint);
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
			#ifndef NDEBUG
			if (prvNode != nullptr) {
				assert(tmpNode->GetIndex() != prvNode->GetIndex());

				INode* nn0 = nodeLayer->GetPoolNode(prvNode->GetIndex());
				INode* nn1 = nodeLayer->GetPoolNode(tmpNode->GetIndex());

				assert(nn1->GetNeighborRelation(nn0) != 0);
				assert(nn0->GetNeighborRelation(nn1) != 0);
			}
			#endif
			assert(tmpPoint != ZeroVector);

			// assert(tmpPoint != prvPoint || tmpNode == fwd.tgtSearchNode || doPartialSearch);

			points.emplace_front(tmpPoint
					, tmpNode->GetIndex() | (ONLY_NODE_ID_MASK * int(tmpPoint == prvPoint))
					, tmpNode->xmin, tmpNode->zmin
					, tmpNode->xmax, tmpNode->zmax);
			nodesWithoutPoints += int(tmpPoint == prvPoint);
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
			tmpNode = prvNode;
			prvNode = tmpNode->GetPrevNode();
		}

		// ensure the starting quad is shared with other path searches.
		if (tmpNode != nullptr) {
			points.emplace_front(float3()
					, tmpNode->GetIndex() | ONLY_NODE_ID_MASK
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
			if (prvNode != nullptr) {
				assert(tmpNode->GetIndex() != prvNode->GetIndex());

				INode* nn0 = nodeLayer->GetPoolNode(prvNode->GetIndex());
				INode* nn1 = nodeLayer->GetPoolNode(tmpNode->GetIndex());

				assert(nn1->GetNeighborRelation(nn0) != 0);
				assert(nn0->GetNeighborRelation(nn1) != 0);
			}
			#endif
		}
	}

	// if source equals target, we need only two points
	if (!points.empty()) {
		path->AllocPoints(points.size() + (-nodesWithoutPoints) + 2);
		path->AllocNodes(points.size());

		path->SetBoundingBox(boundaryMins, boundaryMaxs);
	} else {
		path->AllocPoints(2);
		assert(path->NumPoints() == 2);
		assert(path->GetNodeList().size() == 0);
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
		#ifndef NDEBUG
		if (nodeIndex > 0) {
			assert(path->GetNode(nodeIndex - 1).nodeId != (nodeId & ~ONLY_NODE_ID_MASK));

			bool bothBadNodes = (point.x == (-1.f) && path->GetNode(nodeIndex - 1).netPoint.x == (-1.f));
			bool bothGoodNodes = (point.x != (-1.f) && path->GetNode(nodeIndex - 1).netPoint.x != (-1.f));

			if (bothBadNodes || bothGoodNodes) {
				INode* nn0 = nodeLayer->GetPoolNode(path->GetNode(nodeIndex - 1).nodeId);
				INode* nn1 = nodeLayer->GetPoolNode((nodeId & ~ONLY_NODE_ID_MASK));

				assert(nn1->GetNeighborRelation(nn0) != 0);
				assert(nn0->GetNeighborRelation(nn1) != 0);
			}
		}
		#endif
		path->SetNode(nodeIndex, nodeId & ~ONLY_NODE_ID_MASK, float2(point.x, point.z), nodePointIndex);
		path->SetNodeBoundary(nodeIndex, curPoint.xmin, curPoint.zmin, curPoint.xmax, curPoint.zmax);
		nodeIndex++;
		// LOG("%s: tgtNode=%d point (%f, %f, %f)", __func__
		// 		, nodeId, point.x, point.y, point.z);
	}
	
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
	}

	// set the first (0) and last (N - 1) waypoint
	path->SetSourcePoint(fwd.srcPoint);
	path->SetTargetPoint(fwd.tgtPoint);
	path->SetGoalPosition(goalPos);
	path->SetRepathTriggerIndex(repathIndex);

	assert(fwd.srcPoint != ZeroVector);
	assert(fwd.tgtPoint != ZeroVector);
}

void QTPFS::PathSearch::SmoothPath(IPath* path) {
	//ZoneScoped;
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
	//ZoneScoped;
	// smooth in reverse order (target to source)
	//
	// should terminate when waypoints stop moving,
	// or after a small fixed number of iterations
	unsigned int ni = path->NumPoints();
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

	int nodeIdx = getNextNodeIndex(nodePath.size());
	assert(nodeIdx > -1);

	IPath::PathNodeData* n1 = &nodePath[nodeIdx];
	IPath::PathNodeData* n0 = n1;
	INode* nn0 = nodeLayer->GetPoolNode(n1->nodeId);
	INode* nn1 = nn0;
	constexpr int firstNode = 2;

	// Three points are needed to smooth a path entry.
	for (; ni > firstNode;) {
		nodeIdx = getNextNodeIndex(nodeIdx);
		if (nodeIdx < 0)
			break;

		n0 = n1;
		n1 = &nodePath[nodeIdx];

		assert(n1->nodeId < nodeLayer->GetMaxNodesAlloced());

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

		float3 pi;
		if (SmoothPathPoints(nn0, nn1, p0, p1, p2, pi)) {
			nm++;
			// Don't allow the first point to remove nodes or points.
			if (ni > firstNode && (pi == p0 || pi == p2)) {
				// the point is effectively removed.
				path->RemovePoint(ni - 1);
				// empty point reference
				n0->pathPointIndex = -1;
			} else {
				path->SetPoint(ni - 1, pi);
			}
		}
	}

	return (nm != 0);
}

// Only smooths the beginning of the path.
void QTPFS::PathSearch::SmoothSharedPath(IPath* path) {
	//ZoneScoped;
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

	// First three points are indicies: 0, 1, 2
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
	//ZoneScoped;
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
	//ZoneScoped;
	assert(dstPath->GetID() != 0);
	assert(dstPath->GetID() != srcPath->GetID());
	// assert(dstPath->NumPoints() == 2);

	auto& fwd = directionalSearchData[SearchThreadData::SEARCH_FORWARD];

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
	dstPath->SetHasFullPath(srcPath->IsFullPath());
	dstPath->SetHasPartialPath(srcPath->IsPartialPath());
	dstPath->SetSearchTime(srcPath->GetSearchTime());
	dstPath->SetRepathTriggerIndex(srcPath->GetRepathTriggerIndex());
	dstPath->SetGoalPosition(goalPos);

	haveFullPath = srcPath->IsFullPath();
	havePartPath = srcPath->IsPartialPath();

	SmoothSharedPath(dstPath);

	return true;
}

unsigned int GetChildId(uint32_t nodeNumber, uint32_t i, uint32_t rootMask) {
	//ZoneScoped;
	uint32_t rootId = rootMask & nodeNumber;
	uint32_t nodeId = ((~rootMask) & nodeNumber);
	return rootId | ((nodeId << 2) + (i + 1));
}

const QTPFS::PathHashType QTPFS::PathSearch::GenerateHash(const INode* srcNode, const INode* tgtNode) const {
	//ZoneScoped;
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

const QTPFS::PathHashType QTPFS::PathSearch::GenerateVirtualHash(const INode* srcNode, const INode* tgtNode) const {
	//ZoneScoped;
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

const QTPFS::PathHashType QTPFS::PathSearch::GenerateHash2(uint32_t src, uint32_t dest) const {
	//ZoneScoped;
	std::uint64_t k = nodeLayer->GetNodelayer();
	PathHashType result(std::uint64_t(src) + ((std::uint64_t(dest) << 32)), k);

	return result;
}

const std::uint32_t QTPFS::PathSearch::GenerateVirtualNodeNumber(const INode* startNode, int nodeMaxSize, int x, int z) const {
	//ZoneScoped;
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
