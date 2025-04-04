/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HAPFS_PATHSEARCH_HDR
#define HAPFS_PATHSEARCH_HDR

#include "System/float3.h"

class CSolidObject;
class MoveDef;

namespace HAPFS {
struct PathSearch {
	PathSearch(CSolidObject* caller,
	    const MoveDef* moveDef,
	    float3 startPos,
	    float3 goalPos,
	    float goalRadius,
	    unsigned int pathId)
	    : caller(caller)
	    , moveDef(moveDef)
	    , startPos(startPos)
	    , goalPos(goalPos)
	    , goalRadius(goalRadius)
	    , pathId(pathId)
	{
	}

	CSolidObject* caller;
	const MoveDef* moveDef;
	float3 startPos;
	float3 goalPos;
	float goalRadius;
	unsigned int pathId;
};

enum class ExtendPathResType {
	EXTEND_MAX_RES,
	EXTEND_MED_RES
};

struct PathExtension {
	PathExtension() {}

	PathExtension(ExtendPathResType pathResToExtend)
	    : pathResToExtend(pathResToExtend)
	{
	}

	ExtendPathResType pathResToExtend = ExtendPathResType::EXTEND_MED_RES;
};
} // namespace HAPFS

#endif
