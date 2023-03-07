/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_SYSTEMS_PATH_H__
#define QTPFS_SYSTEMS_PATH_H__

#include <vector>

#include "System/float3.h"
#include "System/Ecs/Components/BaseComponents.h"

namespace QTPFS {

struct Path {
	std::vector<float3> points;
	int nextPointIndex;
};

// corners of the bounding-box containing all our points
struct PathBoundingBox {
	float3 boundingBoxMins;
	float3 boundingBoxMaxs;
};

VOID_COMPONENT(PathIsTemp);
VOID_COMPONENT(PathIsDirty);

ALIAS_COMPONENT(PathOwnerId, int);
ALIAS_COMPONENT(PathHash, std::uint64_t);

}

#endif