/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExplosionListener.h"
#include "System/ContainerUtil.h"

#include <tracy/Tracy.hpp>

std::vector<IExplosionListener*> CExplosionCreator::explosionListeners;


IExplosionListener::~IExplosionListener()
{
	//ZoneScoped;
	CExplosionCreator::RemoveExplosionListener(this);
}

void CExplosionCreator::AddExplosionListener(IExplosionListener* listener)
{
	//ZoneScoped;
	spring::VectorInsertUnique(explosionListeners, listener, true);
}

void CExplosionCreator::RemoveExplosionListener(IExplosionListener* listener)
{
	//ZoneScoped;
	spring::VectorErase(explosionListeners, listener);
}

void CExplosionCreator::FireExplosionEvent(const CExplosionParams& event)
{
	//ZoneScoped;
	for (auto& expList: explosionListeners) {
		expList->ExplosionOccurred(event);
	}
}

