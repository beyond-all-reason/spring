/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RMLINPUT_H
#define RMLINPUT_H

#include "Game/UI/InputReceiver.h"
#include "Backends/RmlUi_Backend.h"

class CRmlInputReceiver : public CInputReceiver
{
public:
	CRmlInputReceiver() : CInputReceiver(FRONT), rml_active(false){};
	~CRmlInputReceiver() = default;

	/**
		IsAbove is used for determining which part of the game is interacting with the mouse.
		<br/><br/>
		Unlike the other implementations of this function, tracking of this
		state is handled enirely by the RmlUI ProcessMouse functions.
		<br/><br/>
		This results in the x and y parameters here being completely ignored

		@param x ignored, the "IsAbove" state is handled elsewhere
		@param y ignored, this "IsAbove" state is handled elsewhere
		@return If the mouse is interacting with any RmlUI elements
	*/
	bool IsAbove(int x = 0, int y = 0) { return rml_active; };
	void setActive(bool active) { rml_active = active; };

	bool KeyPressed(int keyCode, int scanCode, bool isRepeat) { return RmlGui::ProcessKeyPressed(keyCode, scanCode, isRepeat); };
	void MouseMove(int x, int y, int dx, int dy, int button) { RmlGui::ProcessMouseMove(x, y, dx, dy, button); };
	bool MousePress(int x, int y, int button) { return RmlGui::ProcessMousePress(x, y, button); };
	void MouseRelease(int x, int y, int button) { RmlGui::ProcessMouseRelease(x, y, button); };

private:
	bool rml_active;
};
#endif
