/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
// #undef NDEBUG

#include <cassert>

#include "PathCache.h"
#include "PathDefines.h"

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Log/ILog.h"
#include "System/Rectangle.h"

#include "Registry.h"
#include "Components/Path.h"
#include "Components/PathSpeedModInfo.h"

#include <tracy/Tracy.hpp>

static void GetRectangleCollisionVolume(const SRectangle& r, CollisionVolume& v, float3& rm) {
	//ZoneScoped;
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

bool QTPFS::PathCache::MarkDeadPaths(const SRectangle& r, int pathType) {
	//ZoneScoped;
	auto pathView = registry.view<IPath>(/*entt::exclude<PathIsDirty>*/);
	if (pathView.empty())
		return false;

	// NOTE: not static, we run in multiple threads
	CollisionVolume rv;
	float3 rm;

	GetRectangleCollisionVolume(r, rv, rm);

	// LOG("%s: pathType %d has %d entires at start", __func__, pathType, (int)dirtyPaths[pathType].size());

	// "mark" any live path crossing the area of a terrain
	// deformation, for which some or all of its waypoints
	// might now be invalid and need to be recomputed

	// go in reverse so that entries can be removed without impacting the loop.
	for (auto entity : pathView) {
		// if (deadPaths.contains(it->first)) continue; // ... so we don't need this

		// LOG("%s: %x is Dirty=%d", __func__, (int)entity, (int)registry.all_of<PathIsDirty>(entity));

		if (registry.any_of<PathIsDirty, PathSearchRef>(entity)) continue;

		IPath* path = &pathView.get<IPath>(entity);

		if (path->IsSynced() == false) continue;
		if (path->GetPathType() != pathType) { continue; }
		// auto-repath should already be triggered.
		if (path->GetRepathTriggerIndex() != 0 && path->GetNextPointIndex() > path->GetRepathTriggerIndex()) {
			continue;
		}

		// LOG("%s: %x is processing", __func__, (int)entity);

		const float3& pathMins = path->GetBoundingBoxMins();
		const float3& pathMaxs = path->GetBoundingBoxMaxs();

		// if rectangle does not overlap bounding-box, skip this path
		if ((r.x2 * SQUARE_SIZE) < pathMins.x) { continue; }
		if ((r.z2 * SQUARE_SIZE) < pathMins.z) { continue; }
		if ((r.x1 * SQUARE_SIZE) > pathMaxs.x) { continue; }
		if ((r.z1 * SQUARE_SIZE) > pathMaxs.z) { continue; }

		// First check the boundary boxes
		bool searchQuads = path->IsBoundingBoxOverriden();
		bool intersectsQuads = false;
		if (searchQuads) {
			auto& pathNodeList = path->GetNodeList();
			for (auto& node : pathNodeList) {
				if (r.x2 < node.xmin) { continue; }
				if (r.z2 < node.zmin) { continue; }
				if (r.x1 > node.xmax) { continue; }
				if (r.z1 > node.zmax) { continue; }

				intersectsQuads = true;
				break;
			}
		}

		bool searchPaths = (!searchQuads || intersectsQuads);
		bool intersectsPath = false;
		int autoRefreshOnNode = 0;
		if (searchPaths) {
			// figure out if <path> has at least one edge crossing <r>
			// we only care about the segments we have not yet visited
			const unsigned int minIdx = std::max(path->GetNextPointIndex(), 2U) - 2;
			const unsigned int maxIdx = (path->GetRepathTriggerIndex() > 0)
							? path->GetRepathTriggerIndex()
							: std::max(path->NumPoints(), 1u) - 1;

			for (unsigned int i = minIdx; i < maxIdx; i++) {
				const float3& p0 = path->GetPoint(i    );
				const float3& p1 = path->GetPoint(i + 1);

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
				if (havePointInRect || edgeCrossesRect) {
					// assert(tempPaths.find(path->GetID()) == tempPaths.end());
					// assert(std::find(dirtyPaths[pathType].begin(), dirtyPaths[pathType].end(), entity) == dirtyPaths[pathType].end());
					intersectsPath = true;

					bool triggerImmediateRepath = i <= (minIdx + 1);
					if (!triggerImmediateRepath)
						autoRefreshOnNode = i + 1; // trigger happens when waypoints are requested, which always grabs one ahead.

					// LOG("%s: %x is Dirtied (pathType %d)", __func__, (int)entity, pathType);
					break;
				}
			}
		}

		if (intersectsQuads || intersectsPath) {
			DirtyPathDetail dirtyPathDetail;
			dirtyPathDetail.pathEntity = entity;
			dirtyPathDetail.clearSharing = true;
			dirtyPathDetail.clearPath = intersectsPath && (autoRefreshOnNode == 0);
			dirtyPathDetail.autoRepathTrigger = autoRefreshOnNode;

			dirtyPaths[pathType].emplace_back(dirtyPathDetail);
		}
	}

	// LOG("%s: pathType %d has %d entires at end", __func__, pathType, (int)dirtyPaths[pathType].size());

	return true;
}


