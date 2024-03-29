/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_INPUT_RECEIVER
#define LUA_INPUT_RECEIVER

#include <string>

#include "Game/UI/InputReceiver.h"


class CLuaInputReceiver : public CInputReceiver
{
	public:
		CLuaInputReceiver();
		~CLuaInputReceiver();

		static CLuaInputReceiver* GetInstance();

		bool KeyPressed(int keyCode, int scanCode, bool isRepeat);
		bool KeyReleased(int keyCode, int scanCode);

		bool MousePress(int x, int y, int button);
		void MouseMove(int x, int y, int dx, int dy, int button);
		void MouseRelease(int x, int y, int button);
		bool IsAbove(int x, int y);
		std::string GetTooltip(int x,int y);

		void Draw();
};

#define luaInputReceiver CLuaInputReceiver::GetInstance()

#endif /* LUA_INPUT_RECEIVER */

