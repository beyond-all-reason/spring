/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <array>
#include <functional>
#include <tuple>

#include "UnsyncedGameReleaseCommands.h"

#include "UnsyncedActionExecutor.h"
#include "System/SafeUtil.h"

#include "InMapDraw.h"
#include "Game/UI/GameInfo.h"
#include "Game/UI/MouseHandler.h"
#include "Game/Camera.h"

namespace { // prevents linking problems in case of duplicate symbols

class DisableDrawInMapActionExecutor : public IUnsyncedActionExecutor {
public:
	DisableDrawInMapActionExecutor() : IUnsyncedActionExecutor("DrawInMap", "Enables drawing on the map") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		inMapDrawer->SetDrawMode(false);
		return true;
	}
};

class MouseStateActionExecutor : public IUnsyncedActionExecutor {
public:
	MouseStateActionExecutor() : IUnsyncedActionExecutor("MouseState", "Unlocks middle click lock scroll") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		mouse->ToggleMiddleClickScroll();
		return true;
	}
};

class GameInfoCloseActionExecutor : public IUnsyncedActionExecutor {
public:
	GameInfoCloseActionExecutor() : IUnsyncedActionExecutor("GameInfoClose", "Closes game info panel") {
	}

	bool Execute(const UnsyncedAction& action) const final {
		CGameInfo::Disable();
		return true;
	}
};

class MouseActionExecutor : public IUnsyncedActionExecutor {
public:
	MouseActionExecutor(int _button): IUnsyncedActionExecutor(
		"Mouse" + IntToString(_button),
		"Simulates a release of mouse-button " + IntToString(_button)
	) {
		button = _button;
	}

	bool Execute(const UnsyncedAction& action) const final {
		if (!action.IsRepeat())
			mouse->MouseRelease(mouse->lastx, mouse->lasty, button);

		return true;
	}

private:
	int button;
};

class CameraMoveActionExecutor : public IUnsyncedActionExecutor {
public:
	CameraMoveActionExecutor(
		int _moveStateIdx,
		const std::string& commandPostfix
	): IUnsyncedActionExecutor("Move" + commandPostfix, "Moves the camera " + commandPostfix + " a bit") {
		moveStateIdx = _moveStateIdx;
	}

	bool Execute(const UnsyncedAction& action) const final {
		camera->SetMovState(moveStateIdx, false);

		return false;
	}

private:
	int moveStateIdx;
};

}; // namespace


void UnsyncedGameReleaseCommands::AddDefaultActionExecutors()
{
	if (!actionExecutors.empty())
		return;

	AddActionExecutor(AllocActionExecutor<MouseActionExecutor>(1));
	AddActionExecutor(AllocActionExecutor<MouseActionExecutor>(2));
	AddActionExecutor(AllocActionExecutor<MouseActionExecutor>(3));
	// HACK   somehow weird things happen when MouseRelease is called for button 4 and 5.
	// Note that SYS_WMEVENT on windows also only sends MousePress events for these buttons.
	// AddActionExecutor(AllocActionExecutor<MouseActionExecutor>(4));
	// AddActionExecutor(AllocActionExecutor<MouseActionExecutor>(5));
	AddActionExecutor(AllocActionExecutor<MouseStateActionExecutor>());
	AddActionExecutor(AllocActionExecutor<GameInfoCloseActionExecutor>());
	AddActionExecutor(AllocActionExecutor<DisableDrawInMapActionExecutor>());
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_FWD, "Forward"));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_BCK, "Back"   ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_LFT, "Left"   ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_RGT, "Right"  ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_UP , "Up"     ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_DWN, "Down"   ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_FST, "Fast"   ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_SLW, "Slow"   ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_TLT, "Tilt"   ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_RST, "Reset"  ));
	AddActionExecutor(AllocActionExecutor<CameraMoveActionExecutor>(CCamera::MOVE_STATE_RTT, "Rotate" ));
}


static uint8_t ugcSingletonMem[sizeof(UnsyncedGameReleaseCommands)];

void UnsyncedGameReleaseCommands::CreateInstance() {
	UnsyncedGameReleaseCommands*& singleton = GetInstance();

	if (singleton != nullptr)
		return;

	singleton = new (ugcSingletonMem) UnsyncedGameReleaseCommands();
}

void UnsyncedGameReleaseCommands::DestroyInstance(bool reload) {
	UnsyncedGameReleaseCommands*& singleton = GetInstance();

	// executors should be inaccessible in between reloads
	if (reload)
		return;

	spring::SafeDestruct(singleton);
	std::memset(ugcSingletonMem, 0, sizeof(ugcSingletonMem));
}
