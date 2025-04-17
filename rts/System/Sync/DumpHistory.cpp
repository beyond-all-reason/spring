/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <fstream>

#include "DumpHistory.h"
#include "SyncChecker.h"

#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Net/GameServer.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"


void DumpHistory(int dumpId, int frameNum, bool serverRequest)
{
	
#ifdef SYNC_HISTORY
	if (frameNum < gs->frameNum - MAX_SYNC_HISTORY_FRAMES) {
		LOG("[%s] request for history beyond history limit (%d)", __func__, gs->frameNum);
		return;
	}
	auto history = CSyncChecker::GetFrameHistory(frameNum);

	unsigned startIndex = std::get<0>(history);
	unsigned endIndex = std::get<1>(history);
	unsigned* data = std::get<2>(history);

	unsigned firstRangeSize;
	unsigned secondRangeSize = 0;
	if (endIndex > startIndex) {
		firstRangeSize = endIndex - startIndex;
	} else {
		firstRangeSize = MAX_SYNC_HISTORY - startIndex;
		secondRangeSize = endIndex + 1;
	}


	std::fstream file;

	if (!gs->cheatEnabled && !serverRequest)
		return;

	unsigned rewindFrames = gs->frameNum - frameNum - 1;

	LOG("[%s] dumping history (for %d)", __func__, gs->frameNum);

	std::string name = (gameServer != nullptr)? "Server": "Client";
	name += "GameHistory-";
	name += IntToString(dumpId);
	name += "-[";
	name += IntToString(gs->frameNum);
	name += "].bin";

	file.open(name.c_str(), std::ios::out|std::ios::binary);

	if (file.is_open()) {
		unsigned version = 0;
		LOG("[%s] starting dump-file \"%s\"", __func__, name.c_str());

		file.write((char *)&version, sizeof(unsigned));
		file.write((char *)game->gameID, sizeof(unsigned char)*16);
		// gu->myPlayerNum

		file.write((char *)&data[startIndex], sizeof(unsigned)*firstRangeSize);
		if (secondRangeSize > 0)
			file.write((char *)data, sizeof(unsigned)*secondRangeSize);

		LOG("[%s] finished dump-file \"%s\"", __func__, name.c_str());
	}

	if (file.bad() || !file.is_open())
		return;

	file.close();
#endif // SYNC_HISTORY
}

