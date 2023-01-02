/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNSYNCED_GAME_RELEASE_COMMANDS_H
#define UNSYNCED_GAME_RELEASE_COMMANDS_H

#include "IGameCommands.h"

class IUnsyncedActionExecutor;


class UnsyncedGameReleaseCommands : public IGameCommands<IUnsyncedActionExecutor>
{
public:
	static UnsyncedGameReleaseCommands*& GetInstance() {
		static UnsyncedGameReleaseCommands* singleton = nullptr;
		return singleton;
	}

	static void CreateInstance();
	static void DestroyInstance(bool reload);

	void AddDefaultActionExecutors() override;
};

#define unsyncedGameReleaseCommands UnsyncedGameReleaseCommands::GetInstance()

#endif // UNSYNCED_GAME_RELEASE_COMMANDS_H
