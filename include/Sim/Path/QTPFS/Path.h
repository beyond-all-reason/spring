/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATH_H_
#define QTPFS_PATH_H_

#include <algorithm>
#include <vector>

#include "PathDefines.h"
#include "Sim/MoveTypes/MoveDefHandler.h"

#include "System/float3.h"

#include "Map/ReadMap.h"

#include "System/TimeProfiler.h"

#include "System/Log/ILog.h"

class CSolidObject;

namespace QTPFS {
	struct IPath {
		struct PathNodeData {
			uint32_t nodeId = -1U;
			uint32_t nodeNumber = -1U;
			float2 netPoint;
			int pathPointIndex = -1;
			int xmin = 0;
			int zmin = 0;
			int xmax = 0;
			int zmax = 0;
			bool badNode = false;

			bool IsNodeBad() const { return badNode; }
			void SetNodeBad(bool state) { badNode = state; }

			int getWidth() const { return xmax - xmin; }

			static constexpr int NEIGHBOUR_LEFT = 0x0;
			static constexpr int NEIGHBOUR_RIGHT = 0x1;
			static constexpr int NEIGHBOUR_TOP = 0x0;
			static constexpr int NEIGHBOUR_BOTTOM = 0x2;
			static constexpr int NEIGHBOUR_NO_TOUCH = 0x0;
			static constexpr int NEIGHBOUR_LONGITUDE_TOUCH = 0x4;
			static constexpr int NEIGHBOUR_LATITUDE_TOUCH = 0x8;

			static constexpr int NEIGHBOUR_TOUCH = (NEIGHBOUR_LONGITUDE_TOUCH|NEIGHBOUR_LATITUDE_TOUCH);

			static int GetLongitudeNeighbourSide(int neighbourRelationStatus) {
				return (neighbourRelationStatus & NEIGHBOUR_RIGHT);
			}

			static int GetLatitudeNeighbourSide(int neighbourRelationStatus) {
				return (neighbourRelationStatus & NEIGHBOUR_BOTTOM) >> 1;
			}

			static int IsNeighbourCorner(int neighbourRelationStatus) {
				return (neighbourRelationStatus & NEIGHBOUR_TOUCH) == NEIGHBOUR_TOUCH;
			}

			static int IsNeighbourLongitude(int neighbourRelationStatus) {
				return (neighbourRelationStatus & NEIGHBOUR_TOUCH) == NEIGHBOUR_LONGITUDE_TOUCH;
			}

			static int IsNeighbourLatitude(int neighbourRelationStatus) {
				return (neighbourRelationStatus & NEIGHBOUR_TOUCH) == NEIGHBOUR_LATITUDE_TOUCH;
			}

			int GetNeighborRelationStatus(const PathNodeData& other) const {
				bool top = (zmin == other.zmax);
				bool bottom = (zmax == other.zmin);
				bool left = (xmin == other.xmax);
				bool right = (xmax == other.xmin);

				return (NEIGHBOUR_BOTTOM * bottom) + (NEIGHBOUR_RIGHT * right)
						+ (NEIGHBOUR_LONGITUDE_TOUCH * (left|right))
						+ (NEIGHBOUR_LATITUDE_TOUCH  * (top|bottom));
			}
		};

		IPath() {}
		IPath(const IPath& other) { *this = other; }
		IPath& operator = (const IPath& other) {
			if (this == &other) return *this;

			pathID = other.pathID;
			pathType = other.pathType;

			nextPointIndex = other.nextPointIndex;
			numPathUpdates = other.numPathUpdates;
			firstNodeIdOfCleanPath = other.firstNodeIdOfCleanPath;

			hash   = other.hash;
			virtualHash = other.virtualHash;
			radius = other.radius;
			synced = other.synced;
			haveFullPath = other.haveFullPath;
			havePartialPath = other.havePartialPath;
			boundingBoxOverride = other.boundingBoxOverride;
			isRawPath = other.isRawPath;
			points = other.points;
			nodes = other.nodes;

			boundingBoxMins = other.boundingBoxMins;
			boundingBoxMaxs = other.boundingBoxMaxs;

			repathAtPointIndex = other.repathAtPointIndex;
			goalPosition = other.goalPosition;

			owner = other.owner;
			searchTime = other.searchTime;
			return *this;
		}
		IPath(IPath&& other) { *this = std::move(other); }
		IPath& operator = (IPath&& other) {
			if (this == &other) return *this;

			pathID = other.pathID;
			pathType = other.pathType;

			nextPointIndex = other.nextPointIndex;
			numPathUpdates = other.numPathUpdates;
			firstNodeIdOfCleanPath = other.firstNodeIdOfCleanPath;

			hash   = other.hash;
			virtualHash = other.virtualHash;
			radius = other.radius;
			synced = other.synced;
			haveFullPath = other.haveFullPath;
			havePartialPath = other.havePartialPath;
			boundingBoxOverride = other.boundingBoxOverride;
			isRawPath = other.isRawPath;
			points = std::move(other.points);
			nodes = std::move(other.nodes);

			boundingBoxMins = other.boundingBoxMins;
			boundingBoxMaxs = other.boundingBoxMaxs;

			repathAtPointIndex = other.repathAtPointIndex;
			goalPosition = other.goalPosition;

			owner = other.owner;
			searchTime = other.searchTime;

			return *this;
		}
		~IPath() { points.clear(); nodes.clear(); }

		void SetID(unsigned int pathID) { this->pathID = pathID; }
		unsigned int GetID() const { return pathID; }

		void SetNextPointIndex(unsigned int nextPointIndex) { this->nextPointIndex = nextPointIndex; }
		void SetNumPathUpdates(unsigned int numPathUpdates) { this->numPathUpdates = numPathUpdates; }
		unsigned int GetNextPointIndex() const { return nextPointIndex; }
		unsigned int GetNumPathUpdates() const { return numPathUpdates; }

		void SetHash(PathHashType hash) { this->hash = hash; }
		void SetVirtualHash(PathHashType virtualHash) { this->virtualHash = virtualHash; }
		void SetRadius(float radius) { this->radius = radius; }
		void SetSynced(bool synced) { this->synced = synced; }
		void SetHasFullPath(bool fullPath) { this->haveFullPath = fullPath; }
		void SetHasPartialPath(bool partialPath) { this->havePartialPath = partialPath; }

		float GetRadius() const { return radius; }
		PathHashType GetHash() const { return hash; }
		PathHashType GetVirtualHash() const { return virtualHash; }
		bool IsSynced() const { return synced; }
		bool IsFullPath() const { return haveFullPath; }
		bool IsPartialPath() const { return havePartialPath; }

		void SetBoundingBox() {
			boundingBoxMins.x = 1e6f; boundingBoxMaxs.x = -1e6f;
			boundingBoxMins.z = 1e6f; boundingBoxMaxs.z = -1e6f;

			const unsigned int begin = (nextPointIndex >= 2U) ? nextPointIndex - 2U : 0U;
			// const unsigned int end = (repathAtPointIndex > 0U) ? repathAtPointIndex + 1U : points.size();
			const unsigned int end = points.size();
			
			for (unsigned int n = begin; n < end; n++) {
				boundingBoxMins.x = std::min(boundingBoxMins.x, points[n].x);
				boundingBoxMins.z = std::min(boundingBoxMins.z, points[n].z);
				boundingBoxMaxs.x = std::max(boundingBoxMaxs.x, points[n].x);
				boundingBoxMaxs.z = std::max(boundingBoxMaxs.z, points[n].z);
			}

			boundingBoxOverride = false;

			checkPointInBounds(boundingBoxMins);
			checkPointInBounds(boundingBoxMaxs);
		}

		void SetBoundingBox(float3 mins, float3 maxs) {
			boundingBoxMins = mins;
			boundingBoxMaxs = maxs;

			boundingBoxOverride = true;

			checkPointInBounds(boundingBoxMins);
			checkPointInBounds(boundingBoxMaxs);
		}

		bool IsBoundingBoxOverriden() const { return boundingBoxOverride; }

		// // This version is only safe if decltype(points)::value_type == float4
		// void SetBoundingBoxSse() {
		// 	float minResults[4] = { -1e6f, -1e6f, -1e6f, -1e6f };
		// 	float maxResults[4] = { 1e6f, 1e6f, 1e6f, 1e6f };
		// 	const int endIdx = points.size();

		// 	// Main loop for finding maximum height values
		// 	__m128 bestMin = _mm_loadu_ps(minResults);
		// 	__m128 bestMax = _mm_loadu_ps(maxResults);
		// 	for (int i = 0; i < endIdx; ++i) {
		// 		__m128 next = _mm_loadu_ps((float*)&points[i]);
		// 		bestMin = _mm_min_ps(bestMin, next);
		// 		bestMax = _mm_max_ps(bestMax, next);
		// 	}
		// 	_mm_storeu_ps(minResults, bestMin);
		// 	_mm_storeu_ps(maxResults, bestMax);

		// 	boundingBoxMins.x = minResults[0]; boundingBoxMaxs.x = maxResults[0];
		// 	boundingBoxMins.y = minResults[2]; boundingBoxMaxs.y = maxResults[2];

		// 	checkPointInBounds(boundingBoxMins);
		// 	checkPointInBounds(boundingBoxMaxs);
		// }

		const float3& GetBoundingBoxMins() const { return boundingBoxMins; }
		const float3& GetBoundingBoxMaxs() const { return boundingBoxMaxs; }

		void SetPoint(unsigned int i, const float3& p) {
			checkPointInBounds(p);
			points[std::min(i, NumPoints() - 1)] = p;
		}
		const float3& GetPoint(unsigned int i) const { return points[std::min(i, NumPoints() - 1)]; }

		void RemovePoint(unsigned int index) {
			unsigned int start = std::min(index, NumPoints() - 1), end = NumPoints() - 1;
			for (unsigned int i = start; i < end; ++i) { points[i] = points[i+1]; }
			points.pop_back();
			if (index < repathAtPointIndex) repathAtPointIndex--;
		}

		void SetNode(unsigned int i, uint32_t nodeId, uint32_t nodeNumber, float2&& netpoint, int pointIdx, bool isBad) {
			nodes[i].netPoint = netpoint;
			nodes[i].nodeNumber = nodeNumber;
			nodes[i].nodeId = nodeId;
			nodes[i].pathPointIndex = pointIdx;
			nodes[i].SetNodeBad(isBad);
		}

		const PathNodeData& GetNode(unsigned int i) const {
			return nodes[i];
		};

		void RemoveNode(size_t index) {
			if (index >= nodes.size()) { return; }
			unsigned int start = index, end = nodes.size() - 1;
			for (unsigned int i = start; i < end; ++i) { nodes[i] = nodes[i+1]; }
			nodes.pop_back();
		}

		void SetNodeBoundary(unsigned int i, int xmin, int zmin, int xmax, int zmax) {
			nodes[i].xmin = xmin;
			nodes[i].zmin = zmin;
			nodes[i].xmax = xmax;
			nodes[i].zmax = zmax;
		}
		// There are always (points - 1) valid path nodes.
		uint32_t GetGoodNodeCount() const { return points.size() - 1; };

		void SetSourcePoint(const float3& p) { /* checkPointInBounds(p); */ assert(points.size() >= 2); points[                0] = p; }
		void SetTargetPoint(const float3& p) { /* checkPointInBounds(p); */ assert(points.size() >= 2); points[points.size() - 1] = p; }
		const float3& GetSourcePoint() const { return points[                0]; }
		const float3& GetTargetPoint() const { return points[points.size() - 1]; }

		void checkPointInBounds(const float3& p) {
			assert(p.x >= 0.f);
			assert(p.z >= 0.f);
			assert(p.x / SQUARE_SIZE <= mapDims.mapx);
			assert(p.z / SQUARE_SIZE <= mapDims.mapy);
		}

		void SetOwner(const CSolidObject* o) { owner = o; }
		const CSolidObject* GetOwner() const { return owner; }

		unsigned int NumPoints() const { return (points.size()); }
		void AllocPoints(unsigned int n) {
			points.clear();
			points.resize(n);
		}
		void CopyPoints(const IPath& p) {
			AllocPoints(p.NumPoints());

			for (unsigned int n = 0; n < p.NumPoints(); n++) {
				points[n] = p.GetPoint(n);
			}
		}
		void AllocNodes(unsigned int n) {
			nodes.clear();
			nodes.resize(n);
		}
		void CopyNodes(const IPath& p) {
			AllocNodes(p.nodes.size());

			for (unsigned int n = 0; n < p.nodes.size(); n++) {
				nodes[n] = p.GetNode(n);
			}
		}

		void SetPathType(int newPathType) { assert(pathType < moveDefHandler.GetNumMoveDefs()); pathType = newPathType; }
		int GetPathType() const { return pathType; }

		std::vector<PathNodeData>& GetNodeList() { return nodes; };

		void SetSearchTime(spring_time time) { searchTime = time; }

		spring_time GetSearchTime() const { return searchTime; }

		// Incomplete paths need to be rebuilt from time to time as the owner makes progress.
		unsigned int GetRepathTriggerIndex() const { return repathAtPointIndex; }
		void SetRepathTriggerIndex(unsigned int index) { repathAtPointIndex = index; }

		void ClearGetRepathTriggerIndex() { repathAtPointIndex = 0; }

		float3 GetGoalPosition() const { return goalPosition; }
		void SetGoalPosition(float3 point) { goalPosition = point; }

		unsigned int GetFirstNodeIdOfCleanPath() const { return firstNodeIdOfCleanPath; }
		void SetFirstNodeIdOfCleanPath(int nodeId) { firstNodeIdOfCleanPath = nodeId; }

		bool IsRawPath() const { return isRawPath; }
		void SetIsRawPath(bool enable) { isRawPath = enable; }

	private:
		unsigned int pathID = 0;
		int pathType = 0;

		unsigned int nextPointIndex = 0; // index of the next waypoint to be visited
		unsigned int repathAtPointIndex = 0; // minimum index of the waypoint to trigger a repath.
		unsigned int numPathUpdates = 0; // number of times this path was invalidated
		unsigned int firstNodeIdOfCleanPath = 0;

		// Identifies the layer, target quad and source quad for a search query so that similar
		// searches can be combined.
		PathHashType hash = BAD_HASH;

		// Similar to hash, but the target quad and source quad numbers may not relate to actual
		// leaf nodes in the quad tree. They represent the quad that would be there if the leaf node
		// was exactly the size of QTPFS_PARTIAL_SHARE_PATH_MAX_SIZE. This allows searches that
		// start and/or end in different, but close, quads. This is used to handle partially-
		// shared path searches.
		PathHashType virtualHash = BAD_HASH;
		float radius = 0.f;

		// Whether this AFFECTS synced state (like heatmaps and whatnot).
		// It NEVER DEPENDS on unsynced state so even an "unsynced" call
		// is safe to make from synced.
		//
		// Additionally, synced calls are batched and deferred.
		//
		// In practice, this means calls from actual unit movement usually
		// set this to synced, and Lua calls (which need to know results
		// immediately but don't yet involve any unit movement) unsynced.
		bool synced = true;

		bool haveFullPath = true;
		bool havePartialPath = false;
		bool boundingBoxOverride = false;
		bool isRawPath = false;

		std::vector<float3> points;
		std::vector<PathNodeData> nodes;

		// corners of the bounding-box containing all our points
		float3 boundingBoxMins;
		float3 boundingBoxMaxs;

		// Used for incomplete paths to allow repathing attempts to the real goal.
		float3 goalPosition;

		// object that requested this path (NULL if none)
		const CSolidObject* owner = nullptr;

		spring_time searchTime;
	};
}

#endif

