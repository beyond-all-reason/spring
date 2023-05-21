/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHCACHE_HDR
#define QTPFS_PATHCACHE_HDR

#include <vector>

#include "PathEnums.h"
#include "Path.h"
#include "System/UnorderedMap.hpp"
#include "System/Threading/SpringThreading.h"

#include "Registry.h"

#ifdef GetTempPath
#undef GetTempPath
#undef GetTempPathA
#endif

struct SRectangle;

namespace QTPFS {
	struct PathCache {
		PathCache() {
			numCacheHits.resize(PATH_TYPE_DEAD + 1, 0);
			numCacheMisses.resize(PATH_TYPE_DEAD + 1, 0);
		}

		typedef spring::unordered_map<unsigned int, IPath*> PathMap;
		typedef spring::unordered_map<unsigned int, IPath*>::iterator PathMapIt;

		bool MarkDeadPaths(const SRectangle& r, int pathType);
		// void KillDeadPaths();

		// const IPath* GetTempPath(unsigned int pathID) const { return (GetConstPath(pathID, PATH_TYPE_TEMP)); }
		// const IPath* GetLivePath(unsigned int pathID) const { return (GetConstPath(pathID, PATH_TYPE_LIVE)); }
		// const IPath* GetDeadPath(unsigned int pathID) const { return (GetConstPath(pathID, PATH_TYPE_DEAD)); }
		//       IPath* GetTempPath(unsigned int pathID)       { return (GetPath(pathID, PATH_TYPE_TEMP)); }
		//       IPath* GetLivePath(unsigned int pathID)       { return (GetPath(pathID, PATH_TYPE_LIVE)); }
		//       IPath* GetDeadPath(unsigned int pathID)       { return (GetPath(pathID, PATH_TYPE_DEAD)); }

		void AddTempPath(IPath* path);
		//void AddLivePath(IPath* path);

		void Init(int pathTypes) {
			dirtyPaths.clear();
			dirtyPaths.resize(pathTypes);
		}

		void SetLayerPathCount(int pathType, int paths) {
			dirtyPaths[pathType].reserve(paths);
		}

		void DelPath(unsigned int pathID);

		// must be in deadPaths if calling this
		bool ReleaseLivePath(unsigned int pathID);

		// const PathMap& GetTempPaths() const { return tempPaths; }
		// const PathMap& GetLivePaths() const { return livePaths; }
		// const PathMap& GetDeadPaths() const { return deadPaths; }

		// std::vector<IPath> allPaths;
		// std::vector<int> spareIds;

		// struct DirtyPath {
		// 	DirtyPath(int newPathId, int newPathType)
		// 		: pathId(newPathId)
		// 		, pathType(newPathType)
		// 		{}

		// 	int pathId;
		// 	int pathType;
		// };

		std::vector< std::vector<entt::entity> > dirtyPaths;

	private:
		const IPath* GetConstPath(unsigned int pathID, unsigned int pathType) const;
		      IPath* GetPath(unsigned int pathID, unsigned int pathType);

		// PathMap tempPaths;
		// PathMap livePaths;
		// PathMap deadPaths;

		std::vector<unsigned int> numCacheHits;
		std::vector<unsigned int> numCacheMisses;
	};
}

#endif

