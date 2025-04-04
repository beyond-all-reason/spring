/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "MapDamage.h"

#include "BasicMapDamage.h"
#include "MapInfo.h"
#include "NoMapDamage.h"

#include "Game/GameSetup.h"
#include "System/Misc/TracyDefs.h"

static CDummyMapDamage dummyMapDamage;
static CBasicMapDamage basicMapDamage;

// never null
IMapDamage* mapDamage = &dummyMapDamage;

IMapDamage* IMapDamage::InitMapDamage()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (mapInfo->map.notDeformable) {
		dummyMapDamage.Init();
		return &dummyMapDamage;
	}
	if (gameSetup->disableMapDamage) {
		dummyMapDamage.Init();
		return &dummyMapDamage;
	}

	basicMapDamage.Init();
	return &basicMapDamage;
}

void IMapDamage::FreeMapDamage(IMapDamage* p)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(p == mapDamage);
	mapDamage = &dummyMapDamage;
}
