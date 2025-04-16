/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <fstream>
#include <vector>
#include <list>

#include "fmt/format.h"
#include "fmt/printf.h"

#include "DumpHistory.h"
#include "SyncChecker.h"

#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/GameVersion.h"
#include "Net/GameServer.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Scripts/CobEngine.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Map/ReadMap.h"
#include "System/StringUtil.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/Log/ILog.h"
#include "System/SpringHash.h"

void DumpHistory(int dumpId, bool serverRequest)
{
	
#ifdef SYNC_HISTORY
	auto history = CSyncChecker::GetHistory();
	auto index = history.first;
	auto data = history.second;

	static std::fstream file;
	static int gMinFrameNum = -1;
	static int gMaxFrameNum = -1;
	static int gFramePeriod =  1;

	const int oldMinFrameNum = gMinFrameNum;
	const int oldMaxFrameNum = gMaxFrameNum;

	if (!gs->cheatEnabled && !serverRequest)
		return;

	LOG("[%s] dumping history (for %d)", __func__, gs->frameNum);

	if (file.is_open()) {
		file.flush();
		file.close();
	}

	std::string name = (gameServer != nullptr)? "Server": "Client";
	name += "GameHistory-";
	name += IntToString(dumpId);
	name += "-[";
	name += IntToString(gs->frameNum);
	name += "].txt";

	file.open(name.c_str(), std::ios::out);

	if (file.is_open()) {
		unsigned version = 0;
		LOG("[%s] starting dump-file \"%s\"", __func__, name.c_str());
		file.write((char *)&version, sizeof(unsigned));
		file.write((char *)game->gameID, sizeof(unsigned char)*16);
		file.write((char *)data, sizeof(unsigned)*MAX_SYNC_HISTORY);
		LOG("[%s] finished dump-file \"%s\"", __func__, name.c_str());
	}

	if (file.bad() || !file.is_open())
		return;

	file.close();
#endif // SYNC_HISTORY
}

