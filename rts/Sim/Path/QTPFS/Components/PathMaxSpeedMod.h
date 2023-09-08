/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_SYSTEMS_PATH_MAX_SPEED_MOD_H__
#define QTPFS_SYSTEMS_PATH_MAX_SPEED_MOD_H__

#include <deque>
#include <vector>

#include "Sim/MoveTypes/MoveDefHandler.h"
#include "System/float3.h"
#include "System/Rectangle.h"

namespace QTPFS {

class INode;

struct NodeLayerMaxSpeedSweep {
	static constexpr std::size_t page_size = MoveDefHandler::MAX_MOVE_DEFS;
	int updateMaxNodes = 0;
	float updateCurMaxSpeed = 0.f;
	int layerNum = -1;
	bool updateInProgress = false;
};

constexpr int NEXT_FRAME_NEVER = std::numeric_limits<decltype(NEXT_FRAME_NEVER)>::max();

struct PathMaxSpeedModSystemComponent {
	static constexpr std::size_t page_size = 1;
	std::array<float, MoveDefHandler::MAX_MOVE_DEFS> maxRelSpeedMod;
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