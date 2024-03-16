/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "InputReceiver.h"
#include "Lua/LuaInputReceiver.h"
#include "Rendering/GL/myGL.h"
#include "System/Rectangle.h"

#include <tracy/Tracy.hpp>


float CInputReceiver::guiAlpha = 0.8f;

CInputReceiver* CInputReceiver::activeReceiver = nullptr;

CInputReceiver::CInputReceiver(Where w)
{
	//ZoneScoped;
	switch (w) {
		case FRONT: { GetReceivers().push_front(this); } break;
		case BACK: { GetReceivers().push_back(this); } break;
		default: {} break;
	}
}

CInputReceiver::~CInputReceiver()
{
	//ZoneScoped;
	if (activeReceiver == this)
		activeReceiver = nullptr;

	for (CInputReceiver*& r: GetReceivers()) {
		if (r == this) {
			// we may be deleted while there are still iterators active
			// inputReceivers.erase(ri);
			r = nullptr;
			break;
		}
	}
}

void CInputReceiver::CollectGarbage()
{
	//ZoneScoped;
	// remove dead receivers
	std::deque<CInputReceiver*>& prvInputReceivers = GetReceivers();
	std::deque<CInputReceiver*> nxtInputReceivers;

	for (CInputReceiver* r: prvInputReceivers) {
		if (r == nullptr)
			continue;

		nxtInputReceivers.push_back(r);
	}

	prvInputReceivers.swap(nxtInputReceivers);
}

void CInputReceiver::DrawReceivers()
{
	//ZoneScoped;
	std::deque<CInputReceiver*>& receivers = GetReceivers();

	// draw back to front
	for (auto it = receivers.rbegin(); it != receivers.rend(); ++it) {
		CInputReceiver* r = *it;

		if (r == nullptr)
			continue;

		r->Draw();
	}
}

CInputReceiver* CInputReceiver::GetReceiverAt(int x, int y)
{
	//ZoneScoped;
	// always ask Lua first
	if (luaInputReceiver != nullptr && luaInputReceiver->IsAbove(x, y))
		return luaInputReceiver;

	for (CInputReceiver* recv: GetReceivers()) {
		if (recv == nullptr)
			continue;
		if (!recv->IsAbove(x, y))
			continue;

		return recv;
	}

	return nullptr;
}

bool CInputReceiver::InBox(float x, float y, const TRectangle<float>& box) const
{
	//ZoneScoped;
	return ((x > box.x1) && (x < box.x2)  &&  (y > box.y1) && (y < box.y2));
}

