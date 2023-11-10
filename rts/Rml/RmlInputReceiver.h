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

	bool IsAbove(int x, int y) { return rml_active; };
	void setActive(bool active) { rml_active = active; };

private:
	bool rml_active;
};
#endif
