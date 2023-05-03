/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include <cassert>
#include <limits>

#include "PathSearch.h"
#include "Path.h"
#include "PathCache.h"
#include "NodeLayer.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Log/ILog.h"

#include "Components/PathSearch.h"
#include "Systems/PathSearchSystem.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"

#include "Map/ReadMap.h"

#ifdef QTPFS_TRACE_PATH_SEARCHES
#include "Sim/Misc/GlobalSynced.h"
#endif

#include "System/float3.h"


void QTPFS::PathSearch::Initialize(
	NodeLayer* layer,
	PathCache* cache,
	const float3& sourcePoint,
	const float3& targetPoint,
	const SRectangle& searchArea
) {
	srcPoint = sourcePoint; srcPoint.ClampInBounds();
	tgtPoint = targetPoint; tgtPoint.ClampInBounds();

	nodeLayer = layer;
	pathCache = cache;

	searchRect = searchArea;
	searchExec = nullptr;

	INode* srcNode = nodeLayer->GetNode(srcPoint.x / SQUARE_SIZE, srcPoint.z / SQUARE_SIZE);
	INode* tgtNode = nodeLayer->GetNode(tgtPoint.x / SQUARE_SIZE, tgtPoint.z / SQUARE_SIZE);

	assert(srcPoint.x / SQUARE_SIZE >= 0);
	assert(srcPoint.z / SQUARE_SIZE >= 0);
	assert(srcPoint.x / SQUARE_SIZE < mapDims.mapx);
	assert(srcPoint.z / SQUARE_SIZE < mapDims.mapy);

	pathSearchHash = GenerateHash(srcNode, tgtNode);

	// curNode = nullptr;
	// nxtNode = nullptr;
	// minNode = srcNode;

	// LOG("%s: index %d gives node with index %d", __func__
	// 	, (srcPoint.z / SQUARE_SIZE) * nodeLayer->xsize + (srcPoint.x / SQUARE_SIZE)
	// 	, srcNode->GetIndex()
	// 	);

	// LOG("%s: sparse(%d) dense(%d) [%d]->[%d]", __func__
	// 		, (int)searchThreadData->allSearchedNodes.sparseIndex.size()
	// 		, (int)searchThreadData->allSearchedNodes.denseData.size()
	// 		, srcNode->GetIndex()
	// 		, searchThreadData->allSearchedNodes.sparseIndex[srcNode->GetIndex()]
	// 		);
}

void QTPFS::PathSearch::InitializeThread(SearchThreadData* threadData) {
	ZoneScoped;
	searchThreadData = threadData;
	// searchThreadData->Init(NodeLayer::POOL_TOTAL_SIZE, nodeLayer->GetNumLeafNodes());

	// TODO: do this a little later - after shared map has been checked
	searchThreadData->Init(nodeLayer->GetMaxNodesAlloced(), nodeLayer->GetNumLeafNodes());
	openNodes = &searchThreadData->openNodes;

	INode* srcNode = nodeLayer->GetNode(srcPoint.x / SQUARE_SIZE, srcPoint.z / SQUARE_SIZE);
	INode* tgtNode = nodeLayer->GetNode(tgtPoint.x / SQUARE_SIZE, tgtPoint.z / SQUARE_SIZE);

	srcSearchNode = &searchThreadData->allSearchedNodes.InsertINode(srcNode);
	tgtSearchNode = &searchThreadData->allSearchedNodes.InsertINodeIfNotPresent(tgtNode);
	curSearchNode = nullptr;
	nextSearchNode = nullptr;
	minSearchNode = srcSearchNode;
}

bool QTPFS::PathSearch::Execute(
	unsigned int searchStateOffset,
	unsigned int searchMagicNumber
) {
	// searchState = searchStateOffset; // starts at NODE_STATE_OFFSET
	searchMagic = searchMagicNumber; // starts at numTerrainChanges

	haveFullPath = (srcSearchNode == tgtSearchNode);
	havePartPath = false;

	// early-out
	if (haveFullPath)
		return true;

	#ifdef QTPFS_TRACE_PATH_SEARCHES
	searchExec = new PathSearchTrace::Execution(gs->frameNum);
	#endif

	auto& comp = systemGlobals.GetSystemComponent<PathMaxSpeedModSystemComponent>();

	// be as optimistic as possible: assume the remainder of our path will
	// cover only flat terrain with maximum speed-modifier between nxtPoint
	// and tgtPoint
	// this is admissable so long as the map is not LOCALLY changed in such
	// a way as to increase the maximum speedmod beyond the current layer's
	// cached maximum value
	switch (searchType) {
		// This guarantees the best path, but overestimates distance costs considerabily.
		// case PATH_SEARCH_ASTAR:    { hCostMult = 1.0f / nodeLayer->GetMaxRelSpeedMod(); } break;
		case PATH_SEARCH_ASTAR:
			hCostMult = 1.0f / ( comp.maxRelSpeedMod[nodeLayer->GetNodelayer()] );
			break;
		case PATH_SEARCH_DIJKSTRA:
			hCostMult = 0.0f;
			break;
	}


	// if (nodeLayer->GetNodelayer() == 2) {
	// 	LOG("%s: maxRelSpeedMod = %f, hCostMult = %f", __func__
	// 			, comp.maxRelSpeedMod[nodeLayer->GetNodelayer()], hCostMult);
	// }

	// allow the search to start from an impassable node (because single
	// nodes can represent many terrain squares, some of which can still
	// be passable and allow a unit to move within a node)
	// NOTE: we need to make sure such paths do not have infinite cost!
	// TODO: DO NOT MODIFY SHARED DATA IN MT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// if (srcNode->GetMoveCost() == QTPFS_POSITIVE_INFINITY)
	// 	srcNode->SetMoveCost(0.0f);

	// TODO: may need to shift to a node counter

	ResetState(srcSearchNode);
	UpdateNode(srcSearchNode, nullptr, 0);

	// LOG("%s: [%p] Beginning search from %d to %d", __func__
	// 		, nodeLayer
	// 		, srcSearchNode->GetIndex()
	// 		, tgtSearchNode->GetIndex()
	// 		);

	while (!(*openNodes).empty()) {
		IterateNodes();

		#ifdef QTPFS_TRACE_PATH_SEARCHES
		searchExec->AddIteration(searchIter);
		searchIter.Clear();
		#endif

		haveFullPath = (curSearchNode == tgtSearchNode);
		if (haveFullPath)
			searchThreadData->ResetQueue();
			//(*openNodes).reset();
	}

	havePartPath = (minSearchNode != srcSearchNode);

	// if (srcNode->GetMoveCost() == 0.0f)
	// 	srcNode->SetMoveCost(QTPFS_POSITIVE_INFINITY);

	#ifdef QTPFS_SUPPORT_PARTIAL_SEARCHES
	// adjust the target-point if we only got a partial result
	// NOTE:
	//   should adjust GMT::goalPos accordingly, otherwise
	//   units will end up spinning in-place over the last
	//   waypoint (since "atGoal" can never become true)
	if (!haveFullPath && havePartPath) {
		tgtSearchNode = minSearchNode;
		auto* minNode = nodeLayer->GetPoolNode(minSearchNode->GetIndex());
		tgtPoint.x = minNode->xmid() * SQUARE_SIZE;
		tgtPoint.z = minNode->zmid() * SQUARE_SIZE;
	}
	#endif

	return (haveFullPath || havePartPath);
}



void QTPFS::PathSearch::ResetState(SearchNode* node) {
	// will be copied into srcNode by UpdateNode()
	netPoints[0] = {srcPoint.x, srcPoint.z};

	gDists[0] = 0.0f;
	hDists[0] = srcPoint.distance(tgtPoint);
	gCosts[0] = 0.0f;
	hCosts[0] = hDists[0] * hCostMult;

	for (unsigned int i = 1; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
		netPoints[i] = {0.0f, 0.0f};

		gDists[i] = 0.0f;
		hDists[i] = 0.0f;
		gCosts[i] = 0.0f;
		hCosts[i] = 0.0f;
	}

	//(*openNodes).reset();
	//(*openNodes).push(node);
	searchThreadData->ResetQueue();
	(*openNodes).emplace(node->GetIndex(), 0.f);
}

void QTPFS::PathSearch::UpdateNode(SearchNode* nextNode, SearchNode* prevNode, unsigned int netPointIdx) {
	// NOTE:
	//   the heuristic must never over-estimate the distance,
	//   but this is *impossible* to achieve on a non-regular
	//   grid on which any node only has an average move-cost
	//   associated with it --> paths will be "nearly optimal"
	nextNode->SetPrevNode(prevNode);
	nextNode->SetPathCosts(gCosts[netPointIdx], hCosts[netPointIdx]);
	// nextNode->SetSearchState(searchState | NODE_STATE_OPEN);
	nextNode->SetNeighborEdgeTransitionPoint(netPoints[netPointIdx]);
}

void QTPFS::PathSearch::IterateNodes() {
	SearchQueueNode curOpenNode = (*openNodes).top();
	assert(searchThreadData->allSearchedNodes.isSet(curOpenNode.nodeIndex));
	curSearchNode = &searchThreadData->allSearchedNodes[curOpenNode.nodeIndex];
	// curSearchNode->SetSearchState(searchState | NODE_STATE_CLOSED);
	// curNode->SetSearchState(searchState | NODE_STATE_CLOSED);
	#ifdef QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES
	// in the non-conservative case, this is done from
	// NodeLayer::ExecNodeNeighborCacheUpdates instead
	curNode->SetMagicNumber(searchMagic);
	#endif

	(*openNodes).pop();

	#ifdef QTPFS_TRACE_PATH_SEARCHES
	searchIter.SetPoppedNodeIdx(curNode->zmin() * mapDims.mapx + curNode->xmin());
	#endif

	// LOG("%s: continuing search from %d to %d", __func__
	// 		, curSearchNode->GetIndex()
	// 		, tgtSearchNode->GetIndex()
	// 		);

	if (curSearchNode == tgtSearchNode)
		return;

	auto* curNode = nodeLayer->GetPoolNode(curOpenNode.nodeIndex);
	if (curNode->AllSquaresImpassable()) // TODO: special case for first node
		return;
	// Check if this node has already been processed already
	if (curSearchNode->GetHeapPriority() < curOpenNode.heapPriority)
		return;

	if (curNode->xmid() < searchRect.x1) return;
	if (curNode->zmid() < searchRect.z1) return;
	if (curNode->xmid() > searchRect.x2) return;
	if (curNode->zmid() > searchRect.z2) return;

	#ifdef QTPFS_SUPPORT_PARTIAL_SEARCHES
	// remember the node with lowest h-cost in case the search fails to reach tgtNode
	if (curSearchNode->GetPathCost(NODE_PATH_COST_H) < minSearchNode->GetPathCost(NODE_PATH_COST_H))
		minSearchNode = curSearchNode;
	#endif

	assert(curSearchNode->GetIndex() == curOpenNode.nodeIndex);

	// IterateNodeNeighbors(curNode->GetNeighbors());
	IterateNodeNeighbors(curNode);
}

// void QTPFS::PathSearch::IterateNodeNeighbors(const std::vector<INode*>& nxtNodes) {
void QTPFS::PathSearch::IterateNodeNeighbors(const INode* curNode) {
	// if curNode equals srcNode, this is just the original srcPoint
	// auto *curNode = nodeLayer->GetPoolNode(curSearchNode->GetIndex());
	const float2& curPoint2 = curSearchNode->GetNeighborEdgeTransitionPoint();
	const float3  curPoint  = {curPoint2.x, 0.0f, curPoint2.y};

	const std::vector<int>& nxtNodes = curNode->GetNeighbors();
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
		// auto* nxtNode = nxtNodes[i];
		auto* nxtNode = nodeLayer->GetPoolNode(nxtNodes[i]);
		
		// LOG("%s: target node search from %d to %d", __func__
		// 		, curNode->GetIndex()
		// 		, nxtNode->GetIndex()
		// 		);

		assert(curNode->GetNeighborRelation(nxtNode) != 0);
		assert(nxtNode->GetNeighborRelation(curNode) != 0);

		if (nxtNode->AllSquaresImpassable())
			continue;

		nextSearchNode = &searchThreadData->allSearchedNodes.InsertINodeIfNotPresent(nxtNode);

		// const bool isCurrent = (nextSearchNode->GetSearchState() >= searchState);
		// const bool isClosed = ((nextSearchNode->GetSearchState() & 1) == NODE_STATE_CLOSED);
		const bool isTarget = (nextSearchNode == tgtSearchNode);

		unsigned int netPointIdx = 0;

		#if (QTPFS_MAX_NETPOINTS_PER_NODE_EDGE == 1)
		/*if (!IntersectEdge(curNode, nxtNode, tgtPoint - curPoint))*/ {
			// if only one transition-point is allowed per edge,
			// this will always be the edge's center --> no need
			// to be fancy (note that this is not always the best
			// option, it causes local and global sub-optimalities
			// which SmoothPath can only partially address)
			netPoints[0] = curNode->GetNeighborEdgeTransitionPoint(1 + i);

			// cannot use squared-distances because that will bias paths
			// towards smaller nodes (eg. 1^2 + 1^2 + 1^2 + 1^2 != 4^2)
			gDists[0] = curPoint.distance({netPoints[0].x, 0.0f, netPoints[0].y});
			hDists[0] = tgtPoint.distance({netPoints[0].x, 0.0f, netPoints[0].y});
			gCosts[0] =
				curNode->GetPathCost(NODE_PATH_COST_G) +
				curNode->GetMoveCost() * gDists[0] +
				nxtNode->GetMoveCost() * hDists[0] * int(isTarget);
			hCosts[0] = hDists[0] * hCostMult * int(!isTarget);
		}
		#else
		// examine a number of possible transition-points
		// along the edge between curNode and nxtNode and
		// pick the one that minimizes g+h
		// this fixes a few cases that path-smoothing can
		// not handle; more points means a greater degree
		// of non-cardinality (but gets expensive quickly)
		for (unsigned int j = 0; j < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; j++) {
			netPoints[j] = curNode->GetNeighborEdgeTransitionPoint(1 + i * QTPFS_MAX_NETPOINTS_PER_NODE_EDGE + j);

			gDists[j] = curPoint.distance({netPoints[j].x, 0.0f, netPoints[j].y});
			hDists[j] = tgtPoint.distance({netPoints[j].x, 0.0f, netPoints[j].y});
			gCosts[j] =
				curSearchNode->GetPathCost(NODE_PATH_COST_G) +
				curNode->GetMoveCost() * gDists[j] +
				nxtNode->GetMoveCost() * hDists[j] * int(isTarget); // why is this here??????????????????????
			hCosts[j] = hDists[j] * hCostMult * int(!isTarget);

			if ((gCosts[j] + hCosts[j]) < (gCosts[netPointIdx] + hCosts[netPointIdx])) {
				netPointIdx = j;
			}
		}
		#endif

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

		// LOG("%s: adding node (%d) gcost %f < %f [old p:%f]", __func__
		// 		, nextSearchNode->GetIndex()
		// 		, gCosts[netPointIdx]
		// 		, nextSearchNode->GetPathCost(NODE_PATH_COST_G)
		// 		, nextSearchNode->GetHeapPriority()
		// 		);

		UpdateNode(nextSearchNode, curSearchNode, netPointIdx);
		(*openNodes).emplace(nextSearchNode->GetIndex(), nextSearchNode->GetHeapPriority());

		// restore ordering in case nxtNode was already open
		// (changing the f-cost of an OPEN node messes up the
		// queue's internal consistency; a pushed node remains
		// OPEN until it gets popped)
		// (*openNodes).resort(nxtNode);
	}
}

void QTPFS::PathSearch::Finalize(IPath* path) {

	// LOG("%s: [%p : %d] Finialize search.", __func__
	// 		, &nodeLayer[path->GetPathType()]
	// 		, path->GetPathType()
	// 		);

	TracePath(path);

	#ifdef QTPFS_SMOOTH_PATHS
	SmoothPath(path);
	#endif

	path->SetBoundingBox();

	// path remains in live-cache until DeletePath is called
	//pathCache->AddLivePath(path);
}

void QTPFS::PathSearch::TracePath(IPath* path) {
	std::deque<float3> points;
//	std::deque<float3>::const_iterator pointsIt;

	if (srcSearchNode != tgtSearchNode) {

		const SearchNode* tmpNode = tgtSearchNode;
		const SearchNode* prvNode = tmpNode->GetPrevNode();

		float3 prvPoint = tgtPoint;

		while ((prvNode != nullptr) && (tmpNode != srcSearchNode)) {
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
			assert(tmpPoint != prvPoint || tmpNode == tgtSearchNode);

			if (tmpPoint != prvPoint)
				points.push_front(tmpPoint);

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
	}

	// if source equals target, we need only two points
	if (!points.empty()) {
		path->AllocPoints(points.size() + 2);
	} else {
		assert(path->NumPoints() == 2);
	}

	// set waypoints with indices [1, N - 2] (if any)
	while (!points.empty()) {
		path->SetPoint((path->NumPoints() - points.size()) - 1, points.front());
		// LOG("%s: %" PRIx64  " [t:%d] added point %d (%f,%f,%f) ", __func__
		// 		, path->GetHash()
		// 		, path->GetPathType()
		// 		, (int)points.size()
		// 		, points.front().x
		// 		, points.front().y
		// 		, points.front().z
		// 		);
		points.pop_front();
	}

	// set the first (0) and last (N - 1) waypoint
	path->SetSourcePoint(srcPoint);
	path->SetTargetPoint(tgtPoint);
}

void QTPFS::PathSearch::SmoothPath(IPath* path) const {
	if (path->NumPoints() == 2)
		return;

	assert(srcSearchNode->GetPrevNode() == NULL);

	for (unsigned int k = 0; k < QTPFS_MAX_SMOOTHING_ITERATIONS; k++) {
		if (!SmoothPathIter(path)) {
			// all waypoints stopped moving
			break;
		}
	}

	SearchNode* n0 = tgtSearchNode;
	SearchNode* n1 = tgtSearchNode;

	while (n1 != srcSearchNode) {
		n0 = n1;
		n1 = n0->GetPrevNode();

		// reset back-pointers
		n0->SetPrevNode(NULL);
	}
}

bool QTPFS::PathSearch::SmoothPathIter(IPath* path) const {
	// smooth in reverse order (target to source)
	//
	// should terminate when waypoints stop moving,
	// or after a small fixed number of iterations
	unsigned int ni = path->NumPoints();
	unsigned int nm = 0;

	// SearchNode* srcSearchNode = &searchThreadData->allSearchedNodes[srcNode->GetIndex()];
	// SearchNode* tgtSearchNode = &searchThreadData->allSearchedNodes[tgtNode->GetIndex()];

	// const int srcIndex = srcNode->GetIndex();
	// const int tgtIndex = tgtNode->GetIndex();

	SearchNode* n0 = tgtSearchNode;
	SearchNode* n1 = tgtSearchNode;

	while (n1 != srcSearchNode) {
		n0 = n1;
		n1 = n0->GetPrevNode();
		ni -= 1;

		INode* nn0 = nodeLayer->GetPoolNode(n0->GetIndex());
		INode* nn1 = nodeLayer->GetPoolNode(n1->GetIndex());

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

	// const float3& p0 = srcPath->GetTargetPoint();
	// const float3& p1 = dstPath->GetTargetPoint();

	// if (p0.SqDistance(p1) < (SQUARE_SIZE * SQUARE_SIZE)) {
		// copy <srcPath> to <dstPath>
		dstPath->CopyPoints(*srcPath);
		dstPath->SetSourcePoint(srcPoint);
		dstPath->SetTargetPoint(tgtPoint);
		dstPath->SetBoundingBox();

		haveFullPath = true;

		// pathCache->AddLivePath(dstPath);
		return true;
	// }

	// return false;
}

const std::uint64_t QTPFS::PathSearch::GenerateHash(const INode* srcNode, const INode* tgtNode) const {

	std::uint64_t N = mapDims.mapx * mapDims.mapy;
	std::uint32_t k = nodeLayer->GetNodelayer();

	// auto* srcNode = nodeLayer->GetPoolNode(srcSearchNode->GetIndex());
	// auto* tgtNode = nodeLayer->GetPoolNode(tgtSearchNode->GetIndex()); 
	return (srcNode->GetNodeNumber() + (tgtNode->GetNodeNumber() * N) + (k * N * N));
}

