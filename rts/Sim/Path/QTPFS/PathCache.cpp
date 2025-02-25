/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
// #undef NDEBUG

#include <bit>
#include <cassert>

#include "PathCache.h"

#include "Node.h"
#include "NodeLayer.h"
#include "PathDefines.h"
#include "PathSearch.h"

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Log/ILog.h"
#include "System/Rectangle.h"

#include "Registry.h"
#include "Components/Path.h"
#include "Components/PathSpeedModInfo.h"

#include "System/Misc/TracyDefs.h"

// #pragma GCC push_options
// #pragma GCC optimize ("O0")

static void GetRectangleCollisionVolume(const SRectangle& r, CollisionVolume& v, float3& rm) {
	RECOIL_DETAILED_TRACY_ZONE;
	float3 vScales;

	// rectangle dimensions (WS)
	vScales.x = ((r.x2 - r.x1) * SQUARE_SIZE);
	vScales.z = ((r.z2 - r.z1) * SQUARE_SIZE);
	vScales.y = 1.0f;

	// rectangle mid-point (WS)
	rm.x = ((r.x1 + r.x2) * SQUARE_SIZE) >> 1;
	rm.z = ((r.z1 + r.z2) * SQUARE_SIZE) >> 1;
	rm.y = 0.0f;

	#define CV CollisionVolume
	v.InitShape(vScales, ZeroVector, CV::COLVOL_TYPE_BOX, CV::COLVOL_HITTEST_CONT, CV::COLVOL_AXIS_Y);
	#undef CV
}

bool QTPFS::PathCache::MarkDeadPaths(const SRectangle& r, const NodeLayer& nodeLayer) {
	RECOIL_DETAILED_TRACY_ZONE;
	auto pathView = registry.view<IPath>(/*entt::exclude<PathIsDirty>*/);
	if (pathView.empty())
		return false;

	// Used only for raw paths.
	CollisionVolume rv;
	float3 rm;
	GetRectangleCollisionVolume(r, rv, rm);

	// get root node - get layer as input, not the pathType
	int pathType = nodeLayer.GetNodelayer();

	// create virtual node
	const QTPFS::INode* rootNode = nodeLayer.GetRootNode(r.x1, r.z1);

	// TODO refactor search for node generation?
	uint32_t damageDepth = 0;
	const int damageWidth = r.GetWidth();
	const uint32_t damagedNodeNumber = PathSearch::GenerateVirtualNodeNumber(nodeLayer, rootNode, damageWidth, r.x1, r.z1, &damageDepth);
	const uint32_t damageDepthShift = (QTPFS::QTNode::MAX_DEPTH - damageDepth) * QTPFS_NODE_NUMBER_SHIFT_STEP;
	const uint32_t damageDepthMask = (-1U) << (damageDepthShift);

	// if (nodeLayer.GetNodelayer() == 16)
	// 	LOG("%s: area damaged [%d,%d]-[%d,%d]: %x", __func__
	// 		, r.x1, r.z1, r.x2, r.z2, damagedNodeNumber);

	// "mark" any live path crossing the area of a terrain
	// deformation, for which some or all of its waypoints
	// might now be invalid and need to be recomputed

	bool pathWasChecked = false;
	auto testNodeDamage = [/*&testPathIntersect,*/ &nodeLayer, damagedNodeNumber, damageDepthMask, damageWidth, &pathWasChecked, &r]
			( const QTPFS::IPath::PathNodeData& node
			, const IPath* path
			, int i
			) {
		// if (path->GetID() == 357564596) {
		// 	LOG("%s: %d does [%x] overlap [%x] %d ?", __func__, i
		// 		, damagedNodeNumber
		// 		, node.nodeNumber
		// 		, node.nodeId);

		// 	LOG("%s: %d [%d,%d][%d,%d] overlaps %d [%d,%d][%d,%d]", __func__, i
		// 		, r.x1, r.z1, r.x2, r.z2
		// 		, node.nodeId
		// 		, node.xmin, node.zmin, node.xmax, node.zmax);
		// }

		pathWasChecked = false;
		int nodeWidth = node.xmax - node.xmin;
		if (nodeWidth <= damageWidth) {
			bool nodesOverlap = ( damagedNodeNumber == (node.nodeNumber & damageDepthMask) );
			// if (path->GetID() == 290455867)
				// LOG("Damage mask=%x nodesOverlap=%d", damageDepthMask, int(nodesOverlap));
			return nodesOverlap;
		} else {
			// the rect is sized to either 16x16 or the size of the quad it is in, so if the current node is bigger
			// than the damage rect, then it can't possibly be overlapping - (damaged nodes are never revisited by a
			// path.)
			return false;
		}
	};

	// go in reverse so that entries can be removed without impacting the loop.
	for (auto entity : pathView) {
		// if (deadPaths.contains(it->first)) continue; // ... so we don't need this

		// LOG("%s: %x is Dirty=%d", __func__, (int)entity, (int)registry.all_of<PathIsDirty>(entity));

		// if (registry.any_of<PathIsDirty, PathSearchRef>(entity)) { continue; }

		// path hasn't been built yet.
		if (registry.any_of<PathIsTemp>(entity)) { continue; }

		IPath* path = &pathView.get<IPath>(entity);

		if (path->IsSynced() == false) { continue; }
		if (path->GetPathType() != pathType) { continue; }

		// LOG("%s: %x is processing", __func__, (int)entity);

		const float3& pathMins = path->GetBoundingBoxMins();
		const float3& pathMaxs = path->GetBoundingBoxMaxs();

		// if rectangle does not overlap bounding-box, skip this path
		if ((r.x2 * SQUARE_SIZE) < pathMins.x) { continue; }
		if ((r.z2 * SQUARE_SIZE) < pathMins.z) { continue; }
		if ((r.x1 * SQUARE_SIZE) > pathMaxs.x) { continue; }
		if ((r.z1 * SQUARE_SIZE) > pathMaxs.z) { continue; }

		auto& pathNodeList = path->GetNodeList();
		bool intersectsQuads = false;
		bool intersectsPath = false;
		int autoRefreshOnNode = 0;
		const unsigned int minIdx = std::max(path->GetNextPointIndex(), 2U) - 2;
		unsigned int pathGoodFromNodeId = path->GetFirstNodeIdOfCleanPath();

		if (path->IsRawPath() || pathNodeList.size() == 0) {
			assert(path->NumPoints() == 2);
			const float3& p0 = path->GetPoint(0);
			const float3& p1 = path->GetPoint(1);

			const bool p0InRect =
				((p0.x >= (r.x1 * SQUARE_SIZE) && p0.x < (r.x2 * SQUARE_SIZE)) &&
				(p0.z >= (r.z1 * SQUARE_SIZE) && p0.z < (r.z2 * SQUARE_SIZE)));
			const bool p1InRect =
				((p1.x >= (r.x1 * SQUARE_SIZE) && p1.x < (r.x2 * SQUARE_SIZE)) &&
				(p1.z >= (r.z1 * SQUARE_SIZE) && p1.z < (r.z2 * SQUARE_SIZE)));
			const bool havePointInRect = (p0InRect || p1InRect);

			// NOTE:
			//     box-volume tests in its own space, but points are
			//     in world-space so we must inv-transform them first
			//     (p0 --> p0 - rm, p1 --> p1 - rm)
			const bool
				xRangeInRect = (p0.x >= (r.x1 * SQUARE_SIZE) && p1.x <  (r.x2 * SQUARE_SIZE)),
				xRangeExRect = (p0.x <  (r.x1 * SQUARE_SIZE) && p1.x >= (r.x2 * SQUARE_SIZE)),
				zRangeInRect = (p0.z >= (r.z1 * SQUARE_SIZE) && p1.z <  (r.z2 * SQUARE_SIZE)),
				zRangeExRect = (p0.z <  (r.z1 * SQUARE_SIZE) && p1.z >= (r.z2 * SQUARE_SIZE));
			const bool edgeCrossesRect =
				(xRangeExRect && zRangeInRect) ||
				(xRangeInRect && zRangeExRect) ||
				CCollisionHandler::IntersectBox(&rv, p0 - rm, p1 - rm, NULL);

			// LOG("%s: %x havePointInRect=%d edgeCrossesRect=%d", __func__, (int)entity
			// 		, (int)havePointInRect, (int)edgeCrossesRect);

			// remember the ID of each path affected by the deformation
			intersectsPath = (havePointInRect || edgeCrossesRect);
		}
		else
		{
			// First check the boundary boxes
			const bool pathWasClean = path->IsBoundingBoxOverriden();

			// if (path->GetID() == 357564596) {
			// 	LOG("%s: %d nodeSize=%d start=%d", __func__, pathType, int(pathNodeList.size()), pathGoodFromNodeId);
			// 	LOG("%s: pathNodeList=%d, NumPoints=%d", __func__, int(pathNodeList.size()), int(path->NumPoints()));
			// }

			const uint32_t pathNodeStart = (pathWasClean) ? pathNodeList.size() : path->GetGoodNodeCount();
		
			// reverse search to determine where any path repair will be needed up to.
			for (int i = pathNodeStart; i > pathGoodFromNodeId; --i) {
				const QTPFS::IPath::PathNodeData& node = pathNodeList[i-1];

				// If the path is shared, then we need to check bad nodes so that path sharing can be stopped if required.
				// Otherwise, bad nodes can be ignored.
				if (node.IsNodeBad()) {
					assert(pathWasClean);
					if (/*pathWasClean &*/ !intersectsQuads) {
						if (testNodeDamage(node, path, i-1))
							intersectsQuads = true;
					}
					continue;
				}

				if (!testNodeDamage(node, path, i-1)) { continue; }

				// int pIndex = (i - 1) + int(path->NumPoints() - pathNodeList.size());
				// if (pIndex > 0) {
				// 	const float3& p0 = path->GetPoint(pIndex);
				// 	const float3& p1 = path->GetPoint(pIndex - 1);

				// 	LOG("%s: node [%d,%d][%d,%d] p0 = (%f,%f)[%d,%d] p1 = (%f,%f)[%d,%d]", __func__
				// 		, node.xmin, node.zmin, node.xmax, node.zmax
				// 		, p0.x, p0.z, int(p0.x / SQUARE_SIZE), int(p0.z / SQUARE_SIZE)
				// 		, p1.x, p1.z, int(p1.x / SQUARE_SIZE), int(p1.z / SQUARE_SIZE)
				// 		);
				// }

				// LOG("%s: node %d is hit", __func__, i-1);

				pathGoodFromNodeId = i;
				intersectsQuads = true;
				break;
			}
			// if (path->GetID() == 290455867) 
			// 	LOG("%s: now pathGoodFromNodeId=%d", __func__, pathGoodFromNodeId);

			// If a reverse search finds no collisions through the whole path, then a forward search isn't needed.
			if (pathGoodFromNodeId > 0) {
				// figure out if <path> has at least one edge crossing <r>
				// we only care about the segments we have not yet visited

				// Forward search. A hit here tells us, when the path needs to be regenerated.
				// The last two path points reside in the last node.
				const unsigned int triggerIndex = path->GetRepathTriggerIndex();
				const unsigned int maxIdx = (triggerIndex > 0) ? triggerIndex : pathNodeList.size();
				const unsigned int rPathBadAtNodeId = pathGoodFromNodeId - 1;

				// if (path->GetID() == 357564596)
				// 	LOG("%s: minIdx %d, maxIdx %d", __func__, minIdx, maxIdx);

				for (unsigned int i = minIdx; i < maxIdx; i++) {
					const QTPFS::IPath::PathNodeData& node = pathNodeList[i];

					// Bad nodes only occur at the end, if found, then stop. They do not affect the path the unit is
					// following.
					if (node.IsNodeBad()) { break; }

					intersectsPath = (i >= rPathBadAtNodeId) || testNodeDamage(node, path, i);

					// remember the ID of each path affected by the deformation
					if (intersectsPath) {
						bool triggerImmediateRepath = ( i <= (minIdx + 1) );
						if (!triggerImmediateRepath)
							autoRefreshOnNode = i + 1; // trigger happens when waypoints are requested, which always grabs one ahead.

						// LOG("%s: %x is Dirtied (pathType %d)", __func__, (int)entity, pathType);
						break;
					}
				}
			}
		}

		if (intersectsQuads || intersectsPath) {
			bool remainingPathIsDirty = (pathGoodFromNodeId > minIdx) || path->IsRawPath();

			DirtyPathDetail dirtyPathDetail;
			dirtyPathDetail.pathEntity = entity;
			dirtyPathDetail.clearSharing = true;
			dirtyPathDetail.clearPath = intersectsPath && (autoRefreshOnNode == 0) && remainingPathIsDirty;
			dirtyPathDetail.autoRepathTrigger = autoRefreshOnNode;

			// No point noting that the path is clean before the point the owner has reached. The boundary check cuts
			// out the path before the owner's position.
			dirtyPathDetail.nodesAreCleanFromNodeId = std::max(pathGoodFromNodeId, minIdx);

			// if (path->GetID() == 357564596)
			// 	LOG("%s: trig=%d, clearPath=%d, clean=%d", __func__
			// 			, dirtyPathDetail.autoRepathTrigger
			// 			, int(dirtyPathDetail.clearPath)
			// 			, dirtyPathDetail.nodesAreCleanFromNodeId);

			dirtyPaths[pathType].emplace_back(dirtyPathDetail);
		}
	}

	// LOG("%s: pathType %d has %d entries at end", __func__, pathType, (int)dirtyPaths[pathType].size());

	return true;
}

// #pragma GCC pop_options
