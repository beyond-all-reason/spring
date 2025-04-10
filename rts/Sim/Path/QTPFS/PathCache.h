/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHCACHE_HDR
#define QTPFS_PATHCACHE_HDR

#include "NodeLayer.h"
#include "Path.h"
#include "PathEnums.h"
#include "Registry.h"

#include "System/Threading/SpringThreading.h"
#include "System/UnorderedMap.hpp"

#include <vector>

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
		int nodesAreCleanFromNodeId;
		bool clearSharing;
		bool clearPath;
	};

	bool MarkDeadPaths(const SRectangle& r, const NodeLayer& nodeLayer);

	void Init(int pathTypes)
	{
		dirtyPaths.clear();
		dirtyPaths.resize(pathTypes);
	}

	void SetLayerPathCount(int pathType, int paths) { dirtyPaths[pathType].reserve(paths); }

	std::vector<std::vector<DirtyPathDetail>> dirtyPaths;
};
} // namespace QTPFS

#endif
