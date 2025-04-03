/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GameInputReceiver.h"

#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/ScanCodes.h"
#include "Lua/LuaInputReceiver.h"
#include "Lua/LuaMenu.h"
#include "Lua/LuaUI.h"
#include <Rml/Backends/RmlUi_Backend.h>


CGameInputReceiver::CGameInputReceiver()
: CInputReceiver(MANUAL)
{
}


CGameInputReceiver::~CGameInputReceiver() = default;

bool CGameInputReceiver::KeyPressed(int keyCode, int scanCode, bool isRepeat)
{
	if (!game->IsGameOver() && !isRepeat)
		playerHandler.Player(gu->myPlayerNum)->currentStats.keyPresses++;

	const CKeySet kc(keyCode, CKeySet::KSKeyCode);
	const CKeySet ks(scanCode, CKeySet::KSScanCode);

	curKeyCodeChain.push_back(kc, spring_gettime(), isRepeat);
	curScanCodeChain.push_back(ks, spring_gettime(), isRepeat);

	lastActionList = keyBindings.GetActionList(curKeyCodeChain, curScanCodeChain);

	if (RmlGui::ProcessKeyPressed(keyCode, scanCode, isRepeat))
		return false;

	if (gameTextInput.ConsumePressedKey(keyCode, scanCode, lastActionList))
		return false;

	if (luaInputReceiver->KeyPressed(keyCode, scanCode, isRepeat))
		return false;


	// try the input receivers
	for (CInputReceiver* recv: CInputReceiver::GetReceivers()) {
		if (recv != nullptr && recv->KeyPressed(keyCode, scanCode, isRepeat))
			return false;
	}

	return TryOnPressActions(isRepeat);
}

bool CGameInputReceiver::KeyReleased(int keyCode, int scanCode)
{
	if (RmlGui::ProcessKeyReleased(keyCode, scanCode))
		return false;

	if (gameTextInput.ConsumeReleasedKey(keyCode, scanCode))
		return false;

	// update actionlist for lua consumer
	lastActionList = keyBindings.GetActionList(keyCode, scanCode);

	if (luaInputReceiver->KeyReleased(keyCode, scanCode))
		return false;

	// try the input receivers
	for (CInputReceiver* recv: CInputReceiver::GetReceivers()) {
		if (recv != nullptr && recv->KeyReleased(keyCode, scanCode)) {
			return false;
		}
	}

	for (const Action& action: lastActionList) {
		if (game->ActionReleased(action))
			return false;
	}

	return false;
}


bool CGameInputReceiver::MousePress(int x, int y, int button)
{
	int keyCode = CKeyCodes::GetMouseButtonSymbol(button);
	int scanCode = CScanCodes::GetMouseButtonSymbol(button);

	const CKeySet kc(keyCode, CKeySet::KSKeyCode);
	const CKeySet ks(scanCode, CKeySet::KSScanCode);
	bool isRepeat = false;

	const auto now = spring_gettime();
	curKeyCodeChain.push_back(kc, now, isRepeat);
	curScanCodeChain.push_back(ks, now, isRepeat);

	lastActionList = keyBindings.GetActionList(curKeyCodeChain, curScanCodeChain);

	return TryOnPressActions(isRepeat);
}

void CGameInputReceiver::MouseRelease(int x, int y, int button)
{
	int keyCode = CKeyCodes::GetMouseButtonSymbol(button);
	int scanCode = CScanCodes::GetMouseButtonSymbol(button);

	// update actionlist for lua consumer
	lastActionList = keyBindings.GetActionList(keyCode, scanCode);

	for (const Action& action: lastActionList) {
		if (game->ActionReleased(action))
			return;
	}

	return;
}

bool CGameInputReceiver::TryOnPressActions(bool isRepeat)
{
	// try our list of actions
	for (const Action& action: lastActionList) {
		if (game->ActionPressed(action, isRepeat)) {
			return true;
		}
	}

	// maybe a widget is interested?
	// allowing all listeners to process for backwards compatibility.
	bool handled = false;

	if (luaUI != nullptr) {
		for (const Action& action: lastActionList) {
			handled |= luaUI->GotChatMsg(action.rawline, false);
		}
	}

	if (luaMenu != nullptr) {
		for (const Action& action: lastActionList) {
			handled |= luaMenu->GotChatMsg(action.rawline, false);
		}
	}

	return handled;
}
