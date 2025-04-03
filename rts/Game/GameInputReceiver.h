/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAME_INPUT_RECEIVER
#define GAME_INPUT_RECEIVER

#include "Game/UI/InputReceiver.h"
#include "Game/UI/KeySet.h"
#include "Game/Action.h"

class CGameInputReceiver : public CInputReceiver
{
public:
	CGameInputReceiver();
	~CGameInputReceiver();

	bool KeyPressed(int keyCode, int scanCode, bool isRepeat) override;
	bool KeyReleased(int keyCode, int scanCode) override;

	bool MousePress(int x, int y, int button) override;
	void MouseRelease(int x, int y, int button) override;

	ActionList lastActionList;
private:
	bool TryOnPressActions(bool isRepeat);

	CTimedKeyChain curKeyCodeChain;
	CTimedKeyChain curScanCodeChain;
};

#endif /* GAME_INPUT_RECEIVER */

