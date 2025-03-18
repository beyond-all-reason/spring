/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "SunLighting.h"

#include "Map/MapInfo.h"
#include "System/EventHandler.h"
#include "System/StringHash.h"

#include "System/Misc/TracyDefs.h"

/**
 * @brief sunLightingInst
 *
 * Global instance of CSunLighting
 */
CSunLighting sunLightingInst;

CSunLighting::CSunLighting() {
	RECOIL_DETAILED_TRACY_ZONE;
	colors[0] = &groundAmbientColor;
	colors[1] = &groundDiffuseColor;
	colors[2] = &groundSpecularColor;

	colors[3] = &modelAmbientColor;
	colors[4] = &modelDiffuseColor;
	colors[5] = &modelSpecularColor;

	specularExponent    = 0.0f;
	groundShadowDensity = 0.0f;
	modelShadowDensity  = 0.0f;
}

// need an explicit copy-ctor because of colors[]
CSunLighting::CSunLighting(const CSunLighting& sl) {
	RECOIL_DETAILED_TRACY_ZONE;
	colors[0] = &groundAmbientColor;
	colors[1] = &groundDiffuseColor;
	colors[2] = &groundSpecularColor;

	colors[3] = &modelAmbientColor;
	colors[4] = &modelDiffuseColor;
	colors[5] = &modelSpecularColor;

	Copy(sl);
}

CSunLighting& CSunLighting::operator = (const CSunLighting& sl) {
	RECOIL_DETAILED_TRACY_ZONE;
	Copy(sl);
	return (*this);
}


void CSunLighting::Init() {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(mapInfo != nullptr);
	assert(IsGlobalInstance());

	const CMapInfo::light_t& light = mapInfo->light;

	groundAmbientColor   = light.groundAmbientColor;
	groundDiffuseColor   = light.groundDiffuseColor;
	groundSpecularColor  = light.groundSpecularColor;

	modelAmbientColor    = light.modelAmbientColor;
	modelDiffuseColor    = light.modelDiffuseColor;
	modelSpecularColor   = light.modelSpecularColor;

	specularExponent     = light.specularExponent;
	groundShadowDensity  = light.groundShadowDensity;
	modelShadowDensity   = light.modelShadowDensity;

	updated = true;
}

void CSunLighting::Copy(const CSunLighting& sl) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(   colors[0] == &   groundAmbientColor);
	assert(sl.colors[0] == &sl.groundAmbientColor);

	if (sl == (*this))
		return;

	for (size_t n = 0; n < sizeof(colors) / sizeof(colors[0]); n++)
		*colors[n] = *sl.colors[n];

	specularExponent    = sl.specularExponent;
	groundShadowDensity = sl.groundShadowDensity;
	modelShadowDensity  = sl.modelShadowDensity;

	if (!IsGlobalInstance())
		return;

	// send event only if at least one value was changed for the global instance
	eventHandler.SunChanged();
}


bool CSunLighting::SetValue(const char* key, const float4 value) {
	RECOIL_DETAILED_TRACY_ZONE;
	switch (hashString(key)) {
		case hashString("specularExponent"): {
			specularExponent = value.x; return true;
		} break;
		case hashString("groundShadowDensity"): {
			groundShadowDensity = value.x; return true;
		} break;
		case hashString("modelShadowDensity"): {
			modelShadowDensity = value.x; return true;
		} break;

		case hashString("groundAmbientColor"): {
			*(colors[0]) = value; return true;
		} break;
		case hashString("groundDiffuseColor"): {
			*(colors[1]) = value; return true;
		} break;
		case hashString("groundSpecularColor"): {
			*(colors[2]) = value; return true;
		} break;

		case hashString( "unitAmbientColor"):
		case hashString("modelAmbientColor"): {
			*(colors[3]) = value; return true;
		} break;
		case hashString( "unitDiffuseColor"):
		case hashString("modelDiffuseColor"): {
			*(colors[4]) = value; return true;
		} break;
		case hashString( "unitSpecularColor"):
		case hashString("modelSpecularColor"): {
			*(colors[5]) = value; return true;
		} break;

		default: {
		} break;
	}

	return false;
}


bool CSunLighting::operator == (const CSunLighting& sl) const {
	RECOIL_DETAILED_TRACY_ZONE;
	for (unsigned int n = 0; n < sizeof(colors) / sizeof(colors[0]); n++) {
		if (colors[n] != sl.colors[n])
			return false;
	}

	if (groundShadowDensity != sl.groundShadowDensity)
		return false;
	if (modelShadowDensity != sl.modelShadowDensity)
		return false;

	return (specularExponent == sl.specularExponent);
}

bool CSunLighting::IsGlobalInstance() const {
	RECOIL_DETAILED_TRACY_ZONE;
	return (this == &sunLightingInst);
}

