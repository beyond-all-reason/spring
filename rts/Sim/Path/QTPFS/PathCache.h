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

		struct DirtyPathDetail {
			entt::entity pathEntity;
			int autoRepathTrigger;
			bool clearSharing;
			bool clearPath;
		};

		bool MarkDeadPaths(const SRectangle& r, int pathType);

		void Init(int pathTypes) {
			dirtyPaths.clear();
			dirtyPaths.resize(pathTypes);
		}

		void SetLayerPathCount(int pathType, int paths) {
			dirtyPaths[pathType].reserve(paths);
		}

		std::vector< std::vector<DirtyPathDetail> > dirtyPaths;
	};
}

#endif

