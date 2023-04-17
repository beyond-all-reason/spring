/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_SYSTEMS_PATH_SEARCH_H__
#define QTPFS_SYSTEMS_PATH_SEARCH_H__

#include <deque>
#include <vector>

#include "System/float3.h"
#include "System/Rectangle.h"

namespace QTPFS {

class INode;

struct PathSearchSystemComponent {
	static constexpr std::size_t page_size = 1;
	std::vector< std::deque<INode*> > openNodes;
};

struct NodeLayerMaxSpeedSweep {
	static constexpr std::size_t page_size = 256;
	int updateMaxNodes = 0;
	float updateCurMaxSpeed = 0.f;
	int layerNum = -1;

	bool requestUpdate = false;
	bool updateInProgress = false;
};

constexpr int NEXT_FRAME_NEVER = std::numeric_limits<decltype(NEXT_FRAME_NEVER)>::max();

// std::queue<INode*> openNodes; << per thread.
struct PathMaxSpeedModSystemComponent {
	static constexpr std::size_t page_size = 1;
	std::array<float, 256> maxRelSpeedMod; // TODO: get the non-magic number
	int refreshTimeInFrames = 0;
	int startRefreshOnFrame = 0;
	int refeshDelayInFrames = 0;

	enum STATES {
		STATE_INIT,
		STATE_READY,
		STATE_UPDATING,
	};

	int state = STATE_INIT;
};

struct _PathSearch {
	int pathType;
	SRectangle searchRect;
	float3 srcPoint;
	float3 tgtPoint;
};

}

#endif