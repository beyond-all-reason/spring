/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExplosionListener.h"

#include "System/ContainerUtil.h"
#include "System/Misc/TracyDefs.h"

std::vector<IExplosionListener*> CExplosionCreator::explosionListeners;

IExplosionListener::~IExplosionListener()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CExplosionCreator::RemoveExplosionListener(this);
}

void CExplosionCreator::AddExplosionListener(IExplosionListener* listener)
{
	RECOIL_DETAILED_TRACY_ZONE;
	spring::VectorInsertUnique(explosionListeners, listener, true);
}

void CExplosionCreator::RemoveExplosionListener(IExplosionListener* listener)
{
	RECOIL_DETAILED_TRACY_ZONE;
	spring::VectorErase(explosionListeners, listener);
}

void CExplosionCreator::FireExplosionEvent(const CExplosionParams& event)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (auto& expList: explosionListeners) {
		expList->ExplosionOccurred(event);
	}
}
