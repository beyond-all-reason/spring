/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include <cassert>
#include <limits>

#include "lib/streflop/streflop_cond.h"

#include "Node.h"
#include "NodeLayer.h"
#include "PathDefines.h"
#include "PathManager.h"
#include "PathThreads.h"

#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/YardmapStatusEffectsMap.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"

#include "System/Misc/TracyDefs.h"

unsigned int QTPFS::QTNode::MIN_SIZE_X;
unsigned int QTPFS::QTNode::MIN_SIZE_Z;
unsigned int QTPFS::QTNode::MAX_DEPTH;



// void QTPFS::INode::SetPathCost(unsigned int type, float cost) {
// 	#ifndef QTPFS_ENABLE_MICRO_OPTIMIZATION_HACKS
// 	switch (type) {
// 		case NODE_PATH_COST_F: { fCost = cost; return; } break;
// 		case NODE_PATH_COST_G: { gCost = cost; return; } break;
// 		case NODE_PATH_COST_H: { hCost = cost; return; } break;
// 	}

// 	assert(false);
// 	#else
// 	assert(&gCost == &fCost + 1);
// 	assert(&hCost == &gCost + 1);
// 	assert(type <= NODE_PATH_COST_H);

// 	*(&fCost + type) = cost;
// 	#endif
// }

float QTPFS::SearchNode::GetPathCost(unsigned int type) const {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(&gCost == &fCost + 1);
	assert(&hCost == &gCost + 1);
	assert(type <= NODE_PATH_COST_H);

	return *(&fCost + type);
}

// float QTPFS::INode::GetPathCost(unsigned int type) const {
// 	#ifndef QTPFS_ENABLE_MICRO_OPTIMIZATION_HACKS
// 	switch (type) {
// 		case NODE_PATH_COST_F: { return fCost; } break;
// 		case NODE_PATH_COST_G: { return gCost; } break;
// 		case NODE_PATH_COST_H: { return hCost; } break;
// 	}

// 	assert(false);
// 	return 0.0f;
// 	#else
// 	assert(&gCost == &fCost + 1);
// 	assert(&hCost == &gCost + 1);
// 	assert(type <= NODE_PATH_COST_H);

// 	return *(&fCost + type);
// 	#endif
// }

float QTPFS::INode::GetDistance(const INode* n, unsigned int type) const {
	RECOIL_DETAILED_TRACY_ZONE;
	const float dx = float(xmid() * SQUARE_SIZE) - float(n->xmid() * SQUARE_SIZE);
	const float dz = float(zmid() * SQUARE_SIZE) - float(n->zmid() * SQUARE_SIZE);

	switch (type) {
		case NODE_DIST_EUCLIDEAN: { return (math::sqrt((dx * dx) + (dz * dz))); } break;
		case NODE_DIST_MANHATTAN: { return (math::fabs(dx) + math::fabs(dz)); } break;
	}

	return -1.0f;
}

unsigned int QTPFS::INode::GetNeighborRelation(const INode* ngb) const {
	RECOIL_DETAILED_TRACY_ZONE;
	unsigned int rel = 0;

	rel |= ((xmin() == ngb->xmax()) * REL_NGB_EDGE_L);
	rel |= ((xmax() == ngb->xmin()) * REL_NGB_EDGE_R);
	rel |= ((zmin() == ngb->zmax()) * REL_NGB_EDGE_T);
	rel |= ((zmax() == ngb->zmin()) * REL_NGB_EDGE_B);

	return rel;
}

unsigned int QTPFS::INode::GetRectangleRelation(const SRectangle& r) const {
	RECOIL_DETAILED_TRACY_ZONE;
	// NOTE: we consider "interior" to be the set of all
	// legal indices, and conversely "exterior" the set
	// of all illegal indices (min-edges are inclusive,
	// max-edges are exclusive)
	//
	if ((r.x1 >= xmin() && r.x2 <  xmax()) && (r.z1 >= zmin() && r.z2 <  zmax())) { return REL_RECT_INTERIOR_NODE; }
	if ((r.x1 >= xmax() || r.x2 <  xmin()) || (r.z1 >= zmax() || r.z2 <  zmin())) { return REL_RECT_EXTERIOR_NODE; }
	if ((r.x1 <  xmin() && r.x2 >= xmax()) && (r.z1 <  zmin() && r.z2 >= zmax())) { return REL_NODE_INTERIOR_RECT; }

	return REL_NODE_OVERLAPS_RECT;
}

float2 QTPFS::INode::GetNeighborEdgeTransitionPoint(const INode* ngb, const float3& pos, float alpha) const {
	RECOIL_DETAILED_TRACY_ZONE;
	float2 p;

	const unsigned int
		minx = std::max(xmin(), ngb->xmin()),
		maxx = std::min(xmax(), ngb->xmax());
	const unsigned int
		minz = std::max(zmin(), ngb->zmin()),
		maxz = std::min(zmax(), ngb->zmax());

	// NOTE:
	//     do not use integer arithmetic for the mid-points,
	//     the path-backtrace expects all waypoints to have
	//     unique world-space coordinates (ortho-projection
	//     mode is broken in that regard) and this would not
	//     hold for a path through multiple neighboring nodes
	//     with xsize and/or zsize equal to 1 heightmap square
	#ifndef QTPFS_ORTHOPROJECTED_EDGE_TRANSITIONS
	const float
		midx = minx * (1.0f - alpha) + maxx * (0.0f + alpha),
		midz = minz * (1.0f - alpha) + maxz * (0.0f + alpha);
	#endif

	switch (GetNeighborRelation(ngb)) {
		// corners
		case REL_NGB_EDGE_T | REL_NGB_EDGE_L: { p.x = xmin() * SQUARE_SIZE; p.y = zmin() * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_T | REL_NGB_EDGE_R: { p.x = xmax() * SQUARE_SIZE; p.y = zmin() * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_B | REL_NGB_EDGE_R: { p.x = xmax() * SQUARE_SIZE; p.y = zmax() * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_B | REL_NGB_EDGE_L: { p.x = xmin() * SQUARE_SIZE; p.y = zmax() * SQUARE_SIZE; } break;

		#ifdef QTPFS_ORTHOPROJECTED_EDGE_TRANSITIONS
		#define CAST static_cast<unsigned int>

		// edges
		// clamp <pos> (assumed to be inside <this>) to
		// the shared-edge bounds and ortho-project it
		case REL_NGB_EDGE_T: { p.x = std::clamp(CAST(pos.x / SQUARE_SIZE), minx, maxx) * SQUARE_SIZE; p.y = minz * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_B: { p.x = std::clamp(CAST(pos.x / SQUARE_SIZE), minx, maxx) * SQUARE_SIZE; p.y = maxz * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_R: { p.y = std::clamp(CAST(pos.z / SQUARE_SIZE), minz, maxz) * SQUARE_SIZE; p.x = maxx * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_L: { p.y = std::clamp(CAST(pos.z / SQUARE_SIZE), minz, maxz) * SQUARE_SIZE; p.x = minx * SQUARE_SIZE; } break;

		// <ngb> had better be an actual neighbor
		case 0: { assert(false); } break;

		#undef CAST
		#else

		// edges
		case REL_NGB_EDGE_T:                  { p.x = midx   * SQUARE_SIZE; p.y = zmin() * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_R:                  { p.x = xmax() * SQUARE_SIZE; p.y = midz   * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_B:                  { p.x = midx   * SQUARE_SIZE; p.y = zmax() * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_L:                  { p.x = xmin() * SQUARE_SIZE; p.y = midz   * SQUARE_SIZE; } break;

		// <ngb> had better be an actual neighbor
		case 0: { assert(false); } break;

		#endif
	}

	return p;
}

// clip an OVERLAPPING rectangle against our boundaries
//
// NOTE:
//     the rectangle is only ASSUMED to not lie completely
//     inside <this> (in which case this function acts as
//     no-op), we do not explicitly test
//
//     both REL_RECT_EXTERIOR_NODE and REL_NODE_OVERLAPS_RECT
//     relations can produce zero- or negative-area rectangles
//     when clipping --> need to ensure to not leave move-cost
//     at its default value (0.0, which no node can logically
//     have)
//
SRectangle QTPFS::INode::ClipRectangle(const SRectangle& r) const {
	RECOIL_DETAILED_TRACY_ZONE;
	SRectangle cr = r;
	cr.x1 = std::max(int(xmin()), r.x1);
	cr.z1 = std::max(int(zmin()), r.z1);
	cr.x2 = std::min(int(xmax()), r.x2);
	cr.z2 = std::min(int(zmax()), r.z2);
	return cr;
}


void QTPFS::QTNode::InitStatic() {
	RECOIL_DETAILED_TRACY_ZONE;
	MIN_SIZE_X = std::max(1u, mapInfo->pfs.qtpfs_constants.minNodeSizeX);
	MIN_SIZE_Z = std::max(1u, mapInfo->pfs.qtpfs_constants.minNodeSizeZ);
	MAX_DEPTH  = std::max(1u, mapInfo->pfs.qtpfs_constants.maxNodeDepth);
}

void QTPFS::QTNode::Init(
	const QTNode* parent,
	unsigned int nn,
	unsigned int x1, unsigned int z1,
	unsigned int x2, unsigned int z2,
	unsigned int idx
) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(MIN_SIZE_X > 0);
	assert(MIN_SIZE_Z > 0);

	nodeNumber = nn;

	// for leafs, all children remain NULL
	childBaseIndex = -1u;

	_xmin = x1;
	_xmax = x2;
	_zmin = z1;
	_zmax = z2;

	assert(x2 < (1 << 16));
	assert(z2 < (1 << 16));
	assert(xsize() != 0);
	assert(zsize() != 0);

	moveCostAvg = -1.0f;
	uint32_t depth = (parent != nullptr) ? (parent->GetDepth() + 1) : 0;
	index = (idx & NODE_INDEX_MASK) + ((depth << DEPTH_BIT_OFFSET) & DEPTH_MASK);

	neighbours.clear();
}


std::uint64_t QTPFS::QTNode::GetCheckSum(const NodeLayer& nl) const {
	RECOIL_DETAILED_TRACY_ZONE;
	std::uint64_t sum = 0;

	{
		const unsigned char* minByte = reinterpret_cast<const unsigned char*>(&nodeNumber);
		const unsigned char* maxByte = reinterpret_cast<const unsigned char*>(&childBaseIndex) + sizeof(childBaseIndex);

		assert(minByte < maxByte);

		// INode bytes (unpadded)
		for (const unsigned char* byte = minByte; byte != maxByte; byte++) {
			sum ^= ((((byte + 1) - minByte) << 8) * (*byte));
		}
	}
	{
		const unsigned char* minByte = reinterpret_cast<const unsigned char*>(&nodeNumber);
		const unsigned char* maxByte = reinterpret_cast<const unsigned char*>(&moveCostAvg) + sizeof(moveCostAvg);
		// const unsigned char* minByte = reinterpret_cast<const unsigned char*>(&_xminxmax);
		// const unsigned char* maxByte = reinterpret_cast<const unsigned char*>(&prevMagicNum) + sizeof(prevMagicNum);

		assert(minByte < maxByte);

		// QTNode bytes (unpadded)
		for (const unsigned char* byte = minByte; byte != maxByte; byte++) {
			sum ^= ((((byte + 1) - minByte) << 8) * (*byte));
		}
	}

	if (!IsLeaf()) {
		for (unsigned int n = 0; n < QTNODE_CHILD_COUNT; n++) {
			sum ^= (((nodeNumber << 8) + 1) * nl.GetPoolNode(childBaseIndex + n)->GetCheckSum(nl));
		}
	}

	return sum;
}


bool QTPFS::QTNode::CanSplit(unsigned int depth, bool forced) const {
	RECOIL_DETAILED_TRACY_ZONE;
	// NOTE: caller must additionally check IsLeaf() before calling Split()
	if (forced)
		return ((xsize() >> 1) > 0 && (zsize() >> 1) > 0);

	if (depth >= MAX_DEPTH)
		return false;

	#ifdef QTPFS_CONSERVATIVE_NODE_SPLITS
	if (xsize() <= MIN_SIZE_X)
		return false;
	if (zsize() <= MIN_SIZE_Z)
		return false;
	#else
	// aggressive splitting, important with respect to yardmaps
	// (one yardmap square represents four heightmap squares; a
	// node represents MIN_SIZE_X by MIN_SIZE_Z of such squares)
	if (((xsize() >> 1) ==          0) || ((zsize() >> 1) ==          0)) return false;
	if (( xsize()       <= MIN_SIZE_X) && ( zsize()       <= MIN_SIZE_Z)) return false;
	#endif

	return true;
}

// #pragma GCC push_options
// #pragma GCC optimize ("O0")

bool QTPFS::QTNode::Split(NodeLayer& nl, unsigned int depth, bool forced) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!CanSplit(depth, forced))
		return false;

	// can only split leaf-nodes (ie. nodes with no children)
	assert(IsLeaf());

	unsigned int childIndices[QTNODE_CHILD_COUNT];

	// silently refuse to split further if pool is exhausted
	if ((childIndices[NODE_IDX_TL] = nl.AllocPoolNode(this, GetChildID(NODE_IDX_TL, nl.GetRootMask()),  xmin(), zmin(),  xmid(), zmid())) == -1u) return false;
	if ((childIndices[NODE_IDX_TR] = nl.AllocPoolNode(this, GetChildID(NODE_IDX_TR, nl.GetRootMask()),  xmid(), zmin(),  xmax(), zmid())) == -1u) return false;
	if ((childIndices[NODE_IDX_BL] = nl.AllocPoolNode(this, GetChildID(NODE_IDX_BL, nl.GetRootMask()),  xmin(), zmid(),  xmid(), zmax())) == -1u) return false;
	if ((childIndices[NODE_IDX_BR] = nl.AllocPoolNode(this, GetChildID(NODE_IDX_BR, nl.GetRootMask()),  xmid(), zmid(),  xmax(), zmax())) == -1u) return false;

	assert(childIndices[1] == (childIndices[0] + 1));
	assert(childIndices[2] == (childIndices[1] + 1));
	assert(childIndices[3] == (childIndices[2] + 1));

	childBaseIndex = childIndices[0];

	neighbours.clear();
	// netpoints.clear();

	if (AllSquaresImpassable()) {
		nl.DecreaseClosedNodeCounter();
	} else {
		nl.DecreaseOpenNodeCounter();
	}

	nl.SetNumLeafNodes(nl.GetNumLeafNodes() + (4 - 1));
	assert(!IsLeaf());
	return true;
}

// #pragma GCC pop_options

bool QTPFS::QTNode::Merge(NodeLayer& nl) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (IsLeaf()) {
		if (AllSquaresImpassable()) {
			nl.DecreaseClosedNodeCounter();
		} else {
			nl.DecreaseOpenNodeCounter();
		}

		return false;
	}

	neighbours.clear();
	// netpoints.clear();

	// get rid of our children completely
	for (unsigned int i = 0; i < QTNODE_CHILD_COUNT; i++) {
		nl.GetPoolNode(childBaseIndex + i)->Merge(nl);
	}

	// NOTE: return indices in reverse order (BL, BR, TR, TL) of allocation by Split
	nl.FreePoolNode(childBaseIndex + 3);
	nl.FreePoolNode(childBaseIndex + 2);
	nl.FreePoolNode(childBaseIndex + 1);
	nl.FreePoolNode(childBaseIndex + 0);

	childBaseIndex = -1u;

	nl.SetNumLeafNodes(nl.GetNumLeafNodes() - (4 - 1));
	assert(IsLeaf());
	return true;
}


#ifdef QTPFS_SLOW_ACCURATE_TESSELATION
	// re-tesselate a tree from the deepest node <n> that contains
	// rectangle <r> (<n> will be found from any higher node passed
	// in)
	//
	// this code can be VERY slow in the worst-case (eg. when <r>
	// overlaps all four children of the ROOT node), but minimizes
	// the overall number of nodes in the tree at any given time
	void QTPFS::QTNode::PreTesselate(NodeLayer& nl, const SRectangle& r, SRectangle& ur, unsigned int depth) {
		bool cont = false;

		if (!IsLeaf()) {
			for (unsigned int i = 0; i < QTNODE_CHILD_COUNT; i++) {
				QTNode* cn = nl.GetPoolNode(childBaseIndex + i);

				if ((cont |= (cn->GetRectangleRelation(r) == REL_RECT_INTERIOR_NODE))) {
					// only need to descend down one branch
					cn->PreTesselate(nl, r, ur, depth + 1);
					break;
				}
			}
		}

		if (!cont) {
			ur.x1 = std::min(ur.x1, int(xmin()));
			ur.z1 = std::min(ur.z1, int(zmin()));
			ur.x2 = std::max(ur.x2, int(xmax()));
			ur.z2 = std::max(ur.z2, int(zmax()));

			Merge(nl);
			Tesselate(nl, r, depth);
		}
	}

#else

	void QTPFS::QTNode::PreTesselate(NodeLayer& nl, const SRectangle& r, SRectangle& ur, unsigned int depth, const UpdateThreadData* threadData) {
		const unsigned int rel = GetRectangleRelation(r);

		// LOG("%s: [%d:%d]", __func__, nl.GetNodelayer(), depth);

		// use <r> if it is fully inside <this>, otherwise clip against our edges
		const SRectangle& cr = (rel != REL_RECT_INTERIOR_NODE)? ClipRectangle(r): r;

		if ((cr.x2 - cr.x1) <= 0 || (cr.z2 - cr.z1) <= 0)
			return;

		// continue recursion while our CHILDREN are still larger than the clipped rectangle
		//
		// NOTE: this is a trade-off between minimizing the number of leaf-nodes (achieved by
		// re-tesselating in its entirety the deepest node that fully contains <r>) and cost
		// of re-tesselating (which grows as the node-count decreases, kept under control by
		// breaking <r> up into pieces while descending further)
		//
		const bool leaf = IsLeaf();
		const bool cont = (rel == REL_RECT_INTERIOR_NODE) ||
			(((xsize() >> 1) > (cr.x2 - cr.x1)) &&
			 ((zsize() >> 1) > (cr.z2 - cr.z1)));

		// LOG("%s: [%d:%d] leaf=%d !cont=%d", __func__, nl.GetNodelayer(), depth, (int)leaf, (int)!cont);

		// TODO: re-merge back up the root node if necessary
		if (leaf || !cont) {
			Merge(nl);
			Tesselate(nl, r, depth, threadData);
			return;
		}

		// LOG("%s: [%d] (%d,%d:%d,%d) update (%d,%d:%d,%d)", __func__
		// 	, nl.GetNodelayer(), xmin(), zmin(), xmax(), zmax()
		// 	, r.x1, r.z1, r.x2, r.z2
		// 	);

		for (unsigned int i = 0; i < QTNODE_CHILD_COUNT; i++) {
			nl.GetPoolNode(childBaseIndex + i)->PreTesselate(nl, r, ur, depth + 1, threadData);
		}
	}

#endif


// #pragma GCC push_options
// #pragma GCC optimize ("O0")

void QTPFS::QTNode::Tesselate(NodeLayer& nl, const SRectangle& r, unsigned int depth, const UpdateThreadData* threadData) {
	unsigned int numNewBinSquares = 0; // nr. of squares in <r> that changed bin after deformation
	unsigned int numDifBinSquares = 0; // nr. of different bin-types across all squares within <r>
	unsigned int numClosedSquares = 0;

	// if true, we are at the bottom of the recursion
	bool wantSplit = false;
	bool needSplit = false;

	// LOG("%s: [%d] (%d,%d:%d,%d) update (%d,%d:%d,%d)", __func__
	// 	, nl.GetNodelayer(), xmin(), zmin(), xmax(), zmax()
	// 	, r.x1, r.z1, r.x2, r.z2
	// 	);

	// if we just entered Tesselate from PreTesselate, <this> was
	// merged and we need to update squares across the entire node
	//
	// if we entered from a higher-level Tesselate, <this> is newly
	// allocated and we STILL need to update across the entire node
	//
	// this means the rectangle is actually irrelevant: only use it
	// has is that numNewBinSquares can be calculated for area under
	// rectangle rather than full node
	//
	// we want to *keep* splitting so long as not ALL squares
	// within <r> share the SAME bin, OR we keep finding one
	// that SWITCHED bins after the terrain change (we already
	// know this is true for the entire rectangle or we would
	// not have reached PreTesselate)
	//
	// NOTE: during tree construction, numNewBinSquares is ALWAYS
	// non-0 for the entire map-rectangle (see NodeLayer::Update)
	//
	// NOTE: if <r> fully overlaps <this>, then splitting is *not*
	// technically required whenever numRefBinSquares is zero, ie.
	// when ALL squares in <r> changed bins in unison
	//
	UpdateMoveCost(threadData, nl, r, numNewBinSquares, numDifBinSquares, numClosedSquares, wantSplit, needSplit);
	if (!needSplit && !AllSquaresImpassable()) {
		UpdateExitOnly(nl, needSplit);
	}

	if ((wantSplit && Split(nl, depth, false)) || (needSplit && Split(nl, depth, true))) {
		for (unsigned int i = 0; i < QTNODE_CHILD_COUNT; i++) {
			QTNode* cn = nl.GetPoolNode(childBaseIndex + i);
			cn->Tesselate(nl, r, depth + 1, threadData);
			assert(cn->GetMoveCost() != -1.0f);
		}
	}
}

// #pragma GCC pop_options


bool QTPFS::QTNode::UpdateMoveCost(
	const UpdateThreadData* threadData,
	NodeLayer& nl,
	const SRectangle& r,
	unsigned int& numNewBinSquares,
	unsigned int& numDifBinSquares,
	unsigned int& numClosedSquares,
	bool& wantSplit,
	bool& needSplit
) {
	RECOIL_DETAILED_TRACY_ZONE;
	const std::vector<SpeedBinType>& curSpeedBins = nl.GetCurSpeedBins();
	const std::vector<SpeedModType>& curSpeedMods = nl.GetCurSpeedMods();

	assert(int(xmin()) >= r.x1);
	assert(int(xmax()) <= r.x2);
	assert(int(zmin()) >= r.z1);
	assert(int(zmax()) <= r.z2);

	const int rw = r.GetWidth();
	const SpeedBinType refSpeedBin = curSpeedBins[(zmin() - r.z1) * rw + (xmin() - r.x1)];

	// <this> can either just have been merged or added as
	// new child of split parent; in the former case we can
	// restrict ourselves to <r> and update the sum in part
	// (as well as checking homogeneousness just for squares
	// in <r> with a single reference point outside it)
	assert(moveCostAvg == -1.0f || moveCostAvg > 0.0f);

	float speedModSum = 0.0f;

	for (unsigned int hmz = zmin(); hmz < zmax(); hmz++) {
		for (unsigned int hmx = xmin(); hmx < xmax(); hmx++) {
			const unsigned int sqrIdx = (hmz - r.z1) * rw + (hmx - r.x1);

			assert(sqrIdx >= 0);
			assert(sqrIdx < curSpeedBins.size());
			assert(sqrIdx < curSpeedMods.size());

			const SpeedBinType curSpeedBin = curSpeedBins[sqrIdx];

			numDifBinSquares += int(curSpeedBin != refSpeedBin);
			numClosedSquares += int(curSpeedMods[sqrIdx] <= 0);

			speedModSum += (curSpeedMods[sqrIdx] / float(NodeLayer::MaxSpeedModTypeValue()));
		}
	}

	// (re-)calculate the average cost of this node
	assert(speedModSum >= 0.0f);

	float speedModAvg = speedModSum / area();
	moveCostAvg = (speedModAvg <= 0.001f) ? QTPFS_POSITIVE_INFINITY : (nl.UseShortestPath() ? 1.f : (1.f /  speedModAvg));

	// no node can have ZERO traversal cost
	assert(moveCostAvg > 0.0f);

	wantSplit |= (numDifBinSquares > 0);
	needSplit |= (numClosedSquares > 0 && numClosedSquares < area());

	// if we are not going to tesselate this node further
	// and there is at least one impassable square inside
	// it, make sure the pathfinder will not pick us
	//
	// HACK:
	//   set the cost for *!PARTIALLY!* closed nodes to a
	//   non-infinite value since these are often created
	//   along factory exit lanes (most on non-square maps
	//   or when MIN_SIZE_X > 1 or MIN_SIZE_Z > 1), but do
	//   ensure their cost is still high enough so they get
	//   expanded only when absolutely necessary
	//
	//   units with footprint dimensions equal to the size
	//   of a lane would otherwise be unable to find a path
	//   out of their factories
	//
	//   this is crucial for handling the squares underneath
	//   static obstacles (eg. factories) if MIN_SIZE_* != 1
	if (numClosedSquares > 0) {
		if (numClosedSquares < area()) {
			moveCostAvg = QTPFS_CLOSED_NODE_COST * (numClosedSquares / float(xsize() * xsize()));
		} else {
			moveCostAvg = QTPFS_POSITIVE_INFINITY;
		}
	}

	if (AllSquaresImpassable()) {
		nl.IncreaseClosedNodeCounter();
	} else {
		nl.IncreaseOpenNodeCounter();
	}

	// For performance reasons, the maximum node size should match the damage size because mutliple damaged regions
	// that doesn't result in a subdivision will cause the entire node to be re-evaluated over several frames. The
	// larger the node, the larger the performance impact. This often occurs for impassable terrain or hard terrain
	// such as metal.
	needSplit |= (xsize() > QTPFS_MAP_DAMAGE_SIZE);

	wantSplit &= (xsize() > 16); // try not to split below 16 if possible.
	wantSplit &= !(nl.UseShortestPath());

	return (wantSplit || needSplit);
}


bool QTPFS::QTNode::UpdateExitOnly(NodeLayer& nl, bool& needSplit) {
	bool exitOnlyStatePresent[2] = {false, false};

	auto checkRangeForSplit = [this, &nl, &exitOnlyStatePresent]() {
		MoveDef *md = moveDefHandler.GetMoveDefByPathType(nl.GetNodelayer());

		for (int z = zmin(); z < zmax(); ++z) {
			for (int x = xmin(); x < xmax(); ++x) {
				bool isExitOnlyZone = md->IsInExitOnly(x, z);
				exitOnlyStatePresent[isExitOnlyZone] = true;

				// if the other state is also true, then multiple exitOnly states are present and a split is
				// needed.
				if (exitOnlyStatePresent[!isExitOnlyZone] == true)
					return true;
			}
		}
		return false;
	};
	needSplit = checkRangeForSplit();

	if (!needSplit) {
		bool isExitOnlyZone = exitOnlyStatePresent[true];
		index |= uint32_t(isExitOnlyZone)<<EXIT_ONLY_BIT_OFFSET;
	}

	return needSplit;
}

// get the maximum number of neighbors this node
// can have, based on its position / size and the
// assumption that all neighbors are 1x1
//
// NOTE: this intentionally does not count corners
unsigned int QTPFS::QTNode::GetMaxNumNeighbors() const {
	unsigned int n = 0;

	if (xmin() > (               0)) { n += zsize(); } // count EDGE_L ngbs
	if (xmax() < (mapDims.mapx - 1)) { n += zsize(); } // count EDGE_R ngbs
	if (zmin() > (               0)) { n += xsize(); } // count EDGE_T ngbs
	if (zmax() < (mapDims.mapy - 1)) { n += xsize(); } // count EDGE_B ngbs

	return n;
}


// THIS FUNCTION IS NOT USED
// Loading the cache seems to be slower than regenerating the data live at the moment.
void QTPFS::QTNode::Serialize(std::fstream& fStream, NodeLayer& nodeLayer, unsigned int* streamSize, unsigned int depth, bool readMode) {
	RECOIL_DETAILED_TRACY_ZONE;
	// overwritten when de-serializing
	unsigned int numChildren = QTNODE_CHILD_COUNT * (1 - int(IsLeaf()));

	(*streamSize) += (3 * sizeof(unsigned int));
	(*streamSize) += (3 * sizeof(float));

	if (readMode) {
		fStream.read(reinterpret_cast<char*>(&nodeNumber), sizeof(unsigned int));
		fStream.read(reinterpret_cast<char*>(&numChildren), sizeof(unsigned int));
		fStream.read(reinterpret_cast<char*>(&childBaseIndex), sizeof(unsigned int));

		// fStream.read(reinterpret_cast<char*>(&speedModAvg), sizeof(float));
		// fStream.read(reinterpret_cast<char*>(&speedModSum), sizeof(float));
		fStream.read(reinterpret_cast<char*>(&moveCostAvg), sizeof(float));

		if (numChildren > 0) {
			// re-create child nodes
			assert(IsLeaf());
			Split(nodeLayer, depth, true);
		} else {
			// node was a leaf in an earlier life, register it
			// nodeLayer.RegisterNode(this);
		}
	} else {
		fStream.write(reinterpret_cast<const char*>(&nodeNumber), sizeof(unsigned int));
		fStream.write(reinterpret_cast<const char*>(&numChildren), sizeof(unsigned int));
		fStream.write(reinterpret_cast<const char*>(&childBaseIndex), sizeof(unsigned int));

		// fStream.write(reinterpret_cast<const char*>(&speedModAvg), sizeof(float));
		// fStream.write(reinterpret_cast<const char*>(&speedModSum), sizeof(float));
		fStream.write(reinterpret_cast<const char*>(&moveCostAvg), sizeof(float));
	}

	for (unsigned int i = 0; i < numChildren; i++) {
		nodeLayer.GetPoolNode(childBaseIndex + i)->Serialize(fStream, nodeLayer, streamSize, depth + 1, readMode);
	}
}


// this is *either* called from ::GetNeighbors when the conservative
// update-scheme is enabled, *or* from PM::ExecQueuedNodeLayerUpdates
// (never both)
bool QTPFS::QTNode::UpdateNeighborCache(NodeLayer& nodeLayer, UpdateThreadData& threadData) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(IsLeaf());

	unsigned int ngbRels = 0;
	unsigned int maxNgbs = GetMaxNumNeighbors();

	int newNeighbors = 0;

	// number of size-1 quad neighbours around the 4 edges + 4 corners. 
	constexpr size_t maxNumberOfNeighbours = QTPFS_MAX_NODE_SIZE*4 + 4;
	std::array<INode*, maxNumberOfNeighbours> neighborCache;

	auto allowExitLink = [this](INode* ngb) {
		return IsExitOnly() || !ngb->IsExitOnly();
	};

	// if (gs->frameNum > -1 && nodeLayer == 2)
	// 	LOG("%s: [%d] maxNgbs = %d", __func__, index, maxNgbs);

	// regenerate our neighbor cache
	if (maxNgbs > 0) {

		// Build Map Area
		SRectangle& r = threadData.areaRelinked;
		int rWidth = r.GetWidth();

		SRectangle relinkArea(xmin() - 1, zmin() - 1, xmax() + 1, zmax() + 1);
		relinkArea.ClampIn(r);

		SRectangle nodeArea(xmin(), zmin(), xmax(), zmax());
		nodeArea.ClampIn(threadData.areaRelinkedInner);

		if (RectIntersects(threadData.areaRelinkedInner)) {
			neighbours.clear();
		} else {
			for (int ni = neighbours.size(); ni-- > 0;) {
				auto curNode = nodeLayer.GetPoolNode(neighbours[ni].nodeId);
				if (curNode->NodeDeactivated()
					|| !curNode->IsLeaf()
					|| curNode->RectIntersects(threadData.areaRelinkedInner)
				) {
					neighbours[ni] = neighbours.back();
					neighbours.pop_back();
				}
			}
		}

		if (xmin() > relinkArea.x1) {
			INode* ngb = nullptr;
			const unsigned int hmx = xmin() - 1;

			// walk along EDGE_L (west) neighbors
			for (unsigned int hmz = nodeArea.z1; hmz < nodeArea.z2; ) {
				int relinkNodeindex = (hmz - r.z1) * rWidth + (hmx - r.x1);
				ngb = threadData.relinkNodeGrid[relinkNodeindex];
				
				// if (nodeLayer.GetNodelayer() == 2) {
				// 	LOG("Linking x %d -> [%d] (%d,%d) [%d,%d]", ngb->GetIndex(), relinkNodeindex, hmx, hmz, (hmx - r.x1), (hmz - r.z1));
				// }
				hmz = ngb->zmax();

				if (!ngb->AllSquaresImpassable() && allowExitLink(ngb))
					neighborCache[newNeighbors++] = ngb;

				assert(GetNeighborRelation(ngb) != 0);
				assert(ngb->GetNeighborRelation(this) != 0);
				assert(ngb->IsLeaf());
				assert(ngb->xmin() <= hmx);
				assert(ngb->xmax() >= hmx);
				assert(ngb->zmin() <= hmz);
				assert(ngb->zmax() >= hmz);

				// if (gs->frameNum > -1 && nodeLayer == 2)
				//  	LOG("%s: [%x] linked neighbour %x", __func__, nodeNumber, ngb->GetNodeNumber());
			}

			int insideEdge = !(nodeArea.z1 == nodeArea.z2 && xmin() == nodeArea.x1);
			ngbRels |= REL_NGB_EDGE_L * insideEdge;
		}
		if (xmax() < relinkArea.x2) {
			INode* ngb = nullptr;
			const unsigned int hmx = xmax() + 0;

			// walk along EDGE_R (east) neighbors
			for (unsigned int hmz = nodeArea.z1; hmz < nodeArea.z2; ) {
				int relinkNodeindex = (hmz - r.z1) * rWidth + (hmx - r.x1);
				ngb = threadData.relinkNodeGrid[relinkNodeindex];
				
				// if (nodeLayer.GetNodelayer() == 2) {
				// 	LOG("Linking x %d -> [%d] (%d,%d) [%d,%d] = %d", ngb->GetIndex(), relinkNodeindex, hmx, hmz, (hmx - r.x1), (hmz - r.z1), rWidth);
				// }
				hmz = ngb->zmax();

				if (!ngb->AllSquaresImpassable() && allowExitLink(ngb))
					neighborCache[newNeighbors++] = ngb;

				assert(GetNeighborRelation(ngb) != 0);
				assert(ngb->GetNeighborRelation(this) != 0);
				assert(ngb->IsLeaf());
				assert(ngb->xmin() <= hmx);
				assert(ngb->xmax() >= hmx);
				assert(ngb->zmin() <= hmz);
				assert(ngb->zmax() >= hmz);

				// if (gs->frameNum > -1 && nodeLayer == 2)
				//  	LOG("%s: [%x] linked neighbour %x", __func__, nodeNumber, ngb->GetNodeNumber());
			}

			int insideEdge = !(nodeArea.z1 == nodeArea.z2 && xmax() == nodeArea.x2);
			ngbRels |= REL_NGB_EDGE_R * insideEdge;
		}

		if (zmin() > relinkArea.z1) {
			INode* ngb = nullptr;
			const unsigned int hmz = zmin() - 1;

			// walk along EDGE_T (north) neighbors
			for (unsigned int hmx = nodeArea.x1; hmx < nodeArea.x2; ) {
				int relinkNodeindex = (hmz - r.z1) * rWidth + (hmx - r.x1);
				ngb = threadData.relinkNodeGrid[relinkNodeindex];

				// if (nodeLayer.GetNodelayer() == 2) {
				// 	LOG("Linking z %d -> [%d] (%d,%d) [%d,%d] = %d", ngb->GetIndex(), relinkNodeindex, hmx, hmz, (hmx - r.x1), (hmz - r.z1), rWidth);
				// }
				hmx = ngb->xmax();

				if (!ngb->AllSquaresImpassable() && allowExitLink(ngb))
					neighborCache[newNeighbors++] = ngb;

				assert(GetNeighborRelation(ngb) != 0);
				assert(ngb->GetNeighborRelation(this) != 0);
				assert(ngb->IsLeaf());
				assert(ngb->xmin() <= hmx);
				assert(ngb->xmax() >= hmx);
				assert(ngb->zmin() <= hmz);
				assert(ngb->zmax() >= hmz);

				// if (gs->frameNum > -1 && nodeLayer == 2)
				//  	LOG("%s: [%x] linked neighbour %x", __func__, nodeNumber, ngb->GetNodeNumber());
			}

			int insideEdge = !(nodeArea.x1 == nodeArea.x2 && zmin() == nodeArea.z1);
			ngbRels |= REL_NGB_EDGE_T * insideEdge;
		}
		if (zmax() < relinkArea.z2) {
			INode* ngb = nullptr;
			const unsigned int hmz = zmax() + 0;

			// walk along EDGE_B (south) neighbors
			for (unsigned int hmx = nodeArea.x1; hmx < nodeArea.x2; ) {
				int relinkNodeindex = (hmz - r.z1) * rWidth + (hmx - r.x1);
				ngb = threadData.relinkNodeGrid[relinkNodeindex];

				// if (nodeLayer.GetNodelayer() == 2) {
				// 	LOG("Linking %d -> [%d] (%d,%d) [%d,%d]", ngb->GetIndex(), relinkNodeindex, hmx, hmz, (hmx - r.x1), (hmz - r.z1));
				// }
				hmx = ngb->xmax();

				if (!ngb->AllSquaresImpassable() && allowExitLink(ngb))
					neighborCache[newNeighbors++] = ngb;

				assert(GetNeighborRelation(ngb) != 0);
				assert(ngb->GetNeighborRelation(this) != 0);
				assert(ngb->IsLeaf());
				assert(ngb->xmin() <= hmx);
				assert(ngb->xmax() >= hmx);
				assert(ngb->zmin() <= hmz);
				assert(ngb->zmax() >= hmz);

				// if (gs->frameNum > -1 && nodeLayer == 2)
				//  	LOG("%s: [%x] linked neighbour %x", __func__, nodeNumber, ngb->GetNodeNumber());
			}

			int insideEdge = !(nodeArea.x1 == nodeArea.x2 && zmax() == nodeArea.z2);
			ngbRels |= REL_NGB_EDGE_B * insideEdge;
		}

		#ifdef QTPFS_CORNER_CONNECTED_NODES
		// top- and bottom-left corners
		if ((ngbRels & REL_NGB_EDGE_L) != 0) {
			if ((ngbRels & REL_NGB_EDGE_T) != 0) {
				const unsigned int hmx = xmin() - r.x1;
				const unsigned int hmz = zmin() - r.z1;
				const INode* ngbL = threadData.relinkNodeGrid[(hmz + 0) * rWidth + (hmx - 1)];
				const INode* ngbT = threadData.relinkNodeGrid[(hmz - 1) * rWidth + (hmx + 0)];
					  INode* ngbC = threadData.relinkNodeGrid[(hmz - 1) * rWidth + (hmx - 1)];

				// VERT_TL ngb must be distinct from EDGE_L and EDGE_T ngbs
				if (ngbC != ngbL && ngbC != ngbT) {
					if (ngbL->AllSquaresAccessible() && ngbT->AllSquaresAccessible()) {
						if (!ngbC->AllSquaresImpassable() && allowExitLink(ngbC))
							neighborCache[newNeighbors++] = ngbC;

						assert(GetNeighborRelation(ngbC) != 0);
						assert(ngbC->GetNeighborRelation(this) != 0);
						assert(ngbC->IsLeaf());

						// if (gs->frameNum > -1 && nodeLayer == 2)
						// 	LOG("%s: [%x] linked neighbour %x", __func__, nodeNumber, ngb->GetNodeNumber());
					}
				}
			}
			if ((ngbRels & REL_NGB_EDGE_B) != 0) {
				const unsigned int hmx = xmin() - r.x1;
				const unsigned int hmz = zmax() - r.z1;
				const INode* ngbL = threadData.relinkNodeGrid[(hmz - 1) * rWidth + (hmx - 1)];
				const INode* ngbB = threadData.relinkNodeGrid[(hmz + 0) * rWidth + (hmx + 0)];
					  INode* ngbC = threadData.relinkNodeGrid[(hmz + 0) * rWidth + (hmx - 1)];

				// VERT_BL ngb must be distinct from EDGE_L and EDGE_B ngbs
				if (ngbC != ngbL && ngbC != ngbB) {
					if (ngbL->AllSquaresAccessible() && ngbB->AllSquaresAccessible()) {
						if (!ngbC->AllSquaresImpassable() && allowExitLink(ngbC))
							neighborCache[newNeighbors++] = ngbC;

						assert(GetNeighborRelation(ngbC) != 0);
						assert(ngbC->GetNeighborRelation(this) != 0);
						assert(ngbC->IsLeaf());

						// if (gs->frameNum > -1 && nodeLayer == 2)
						// 	LOG("%s: [%x] linked neighbour %x", __func__, nodeNumber, ngb->GetNodeNumber());
					}
				}
			}
		}

		// top- and bottom-right corners
		if ((ngbRels & REL_NGB_EDGE_R) != 0) {
			if ((ngbRels & REL_NGB_EDGE_T) != 0) {
				const unsigned int hmx = xmax() - r.x1;
				const unsigned int hmz = zmin() - r.z1;
				const INode* ngbR = threadData.relinkNodeGrid[(hmz + 0) * rWidth + (hmx + 0)];
				const INode* ngbT = threadData.relinkNodeGrid[(hmz - 1) * rWidth + (hmx - 1)];
					  INode* ngbC = threadData.relinkNodeGrid[(hmz - 1) * rWidth + (hmx + 0)];

				// VERT_TR ngb must be distinct from EDGE_R and EDGE_T ngbs
				if (ngbC != ngbR && ngbC != ngbT) {
					if (ngbR->AllSquaresAccessible() && ngbT->AllSquaresAccessible()) {
						// neighbours.push_back(ngbC);
						// neighbours.push_back(ngbC->GetIndex());
						if (!ngbC->AllSquaresImpassable() && allowExitLink(ngbC))
							neighborCache[newNeighbors++] = ngbC;

						assert(GetNeighborRelation(ngbC) != 0);
						assert(ngbC->GetNeighborRelation(this) != 0);
						assert(ngbC->IsLeaf());

						// if (gs->frameNum > -1 && nodeLayer == 2)
						// 	LOG("%s: [%x] linked neighbour %x", __func__, nodeNumber, ngb->GetNodeNumber());
					}
				}
			}
			if ((ngbRels & REL_NGB_EDGE_B) != 0) {
				const unsigned int hmx = xmax() - r.x1;
				const unsigned int hmz = zmax() - r.z1;
				const INode* ngbR = threadData.relinkNodeGrid[(hmz - 1) * rWidth + (hmx + 0)];
				const INode* ngbB = threadData.relinkNodeGrid[(hmz + 0) * rWidth + (hmx - 1)];
					  INode* ngbC = threadData.relinkNodeGrid[(hmz + 0) * rWidth + (hmx + 0)];

				// VERT_BR ngb must be distinct from EDGE_R and EDGE_B ngbs
				if (ngbC != ngbR && ngbC != ngbB) {
					if (ngbR->AllSquaresAccessible() && ngbB->AllSquaresAccessible()) {
						if (!ngbC->AllSquaresImpassable() && allowExitLink(ngbC))
							neighborCache[newNeighbors++] = ngbC;

						assert(GetNeighborRelation(ngbC) != 0);
						assert(ngbC->GetNeighborRelation(this) != 0);
						assert(ngbC->IsLeaf());

						// if (gs->frameNum > -1 && nodeLayer == 2)
						// 	LOG("%s: [%x] linked neighbour %x", __func__, nodeNumber, ngb->GetNodeNumber());
					}
				}
			}
		}
		#endif

		assert(newNeighbors < maxNumberOfNeighbours);

		maxNgbs = neighbours.size() + newNeighbors;
		neighbours.reserve(maxNgbs);

		for (int i = 0; i < newNeighbors; i++) {
			INode* ngb = neighborCache[i];
			NeighbourPoints newNeighbour;
			newNeighbour.nodeId = ngb->GetIndex();
			for (unsigned int i = 0; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
				newNeighbour.netpoints[i] = (INode::GetNeighborEdgeTransitionPoint(ngb, {}, QTPFS_NETPOINT_EDGE_SPACING_SCALE * (i + 1)));
			}
			neighbours.emplace_back(newNeighbour);
		}
	}

	return true;
}

