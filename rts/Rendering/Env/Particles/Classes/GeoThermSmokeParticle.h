/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SmokeParticle.h"

class CFeature;

class CGeoThermSmokeParticle : public CSmokeParticle
{
	CR_DECLARE(CGeoThermSmokeParticle)
public:
	CGeoThermSmokeParticle() { }
	CGeoThermSmokeParticle(
		const float3& pos,
		const float3& spd,
		int ttl,
		const CFeature* geo
	);

	void Update();
	void UpdateDir();

	void DrawOnMinimap() {}

	static void GeoThermDestroyed(const CFeature* geo);

private:
	const CFeature* geo;
};