/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_SYSTEMS_PATH_SEARCH_H__
#define QTPFS_SYSTEMS_PATH_SEARCH_H__

#include <deque>
#include <vector>

#include "System/float3.h"
#include "System/Rectangle.h"

namespace QTPFS {

class INode;

// std::queue<INode*> openNodes; << per thread.
struct PathSearchSystemComponent {
	static constexpr std::size_t page_size = 1;
	std::vector< std::deque<INode*> > openNodes;
};

struct _PathSearch {
	int pathType;
	SRectangle searchRect;
	float3 srcPoint;
	float3 tgtPoint;
};

}

#endif