/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "MapRendering.h"

#include "Map/MapInfo.h"
#include "System/EventHandler.h"

#include <tracy/Tracy.hpp>

/**
 * @brief mapRenderingInst
 *
 * Global instance of CMapRendering
 */
CMapRendering mapRenderingInst;

void CMapRendering::Init() {
	//ZoneScoped;
	assert(mapInfo != nullptr);
	assert(IsGlobalInstance());

	splatTexScales = mapInfo->splats.texScales;
	splatTexMults = mapInfo->splats.texMults;
	splatDetailNormalDiffuseAlpha = mapInfo->smf.splatDetailNormalDiffuseAlpha;
	voidWater = mapInfo->map.voidWater;
	voidGround = mapInfo->map.voidGround;
}

bool CMapRendering::IsGlobalInstance() const {
	//ZoneScoped;
	return (this == &mapRenderingInst);
}
