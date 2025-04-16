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


void DumpHistory(int dumpId, bool serverRequest)
{
	
#ifdef SYNC_HISTORY
	auto history = CSyncChecker::GetHistory();

	unsigned nextIndex = history.first;
	unsigned* data = history.second;

	std::fstream file;

	if (!gs->cheatEnabled && !serverRequest)
		return;

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

		file.write((char *)&data[nextIndex], sizeof(unsigned)*(MAX_SYNC_HISTORY-nextIndex));
		if (index > 0)
			file.write((char *)data, sizeof(unsigned)*nextIndex);

		LOG("[%s] finished dump-file \"%s\"", __func__, name.c_str());
	}

	if (file.bad() || !file.is_open())
		return;

	file.close();
#endif // SYNC_HISTORY
}

