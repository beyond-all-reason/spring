/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GameInputReceiver.h"
#include "Game.h"


CGameInputReceiver::CGameInputReceiver()
: CInputReceiver(MANUAL)
{
}


CGameInputReceiver::~CGameInputReceiver() = default;


void CGameInputReceiver::MouseRelease(int x, int y, int button)
{
	activeController->MouseRelease(x, y, button);
}

