/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_SYSTEMS_PATH_SPEED_MOD_INFO_H__
#define QTPFS_SYSTEMS_PATH_SPEED_MOD_INFO_H__

#include <deque>
#include <vector>

#include "Sim/MoveTypes/MoveDefHandler.h"
#include "System/float3.h"
#include "System/Rectangle.h"

namespace QTPFS {

class INode;

struct NodeLayerSpeedInfoSweep {
	static constexpr std::size_t page_size = MoveDefHandler::MAX_MOVE_DEFS;

	int updateMaxNodes = 0;
	float updateCurMaxSpeed = 0.f;
	float updateCurSumSpeed = 0.f;
	float updateNumLeafNodes = 0.f;
	int layerNum = -1;
	bool updateInProgress = false;
};

constexpr int NEXT_FRAME_NEVER = std::numeric_limits<decltype(NEXT_FRAME_NEVER)>::max();

struct PathSpeedModInfo {
	float mean;
	float max;
};

struct PathSpeedModInfoSystemComponent {
	static constexpr std::size_t page_size = 1;

	std::array<PathSpeedModInfo, MoveDefHandler::MAX_MOVE_DEFS> relSpeedModinfos;
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

}

#endif