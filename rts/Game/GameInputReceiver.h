/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAME_INPUT_RECEIVER
#define GAME_INPUT_RECEIVER

#include "Game/UI/InputReceiver.h"


class CGameInputReceiver : public CInputReceiver
{
	public:
		CGameInputReceiver();
		~CGameInputReceiver();

		void MouseRelease(int x, int y, int button);
};

#endif /* GAME_INPUT_RECEIVER */

