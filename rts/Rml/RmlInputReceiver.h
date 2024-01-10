/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RMLINPUT_H
#define RMLINPUT_H

#include "Game/UI/InputReceiver.h"
#include "Rml/Backends/RmlUi_Backend.h"

class CRmlInputReceiver : public CInputReceiver
{
public:
	CRmlInputReceiver() : CInputReceiver(FRONT), rml_active(false){};
	~CRmlInputReceiver() = default;

	/**
	IsAbove is used for determining which cursor icon should be displayed. This is tracked in ProcessMouse functions when RmlUi handles
	the event	and that state is true if rml is capturing the mouse.
	*/
	bool IsAbove(int x, int y) { return rml_active; };
	void setActive(bool active) { rml_active = active; };

	bool KeyPressed(int keyCode, int scanCode, bool isRepeat) { return RmlGui::ProcessKeyPressed(keyCode, scanCode, isRepeat); }
	void MouseMove(int x, int y, int dx, int dy, int button) { RmlGui::ProcessMouseMove(x, y, dx, dy, button); }
	bool MousePress(int x, int y, int button) { return RmlGui::ProcessMousePress(x, y, button); }
	void MouseRelease(int x, int y, int button) { RmlGui::ProcessMouseRelease(x, y, button); }

private:
	bool rml_active;
};
#endif
