/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "WeaponProjectile.h"

class CBeamLaserProjectile: public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CBeamLaserProjectile)
public:
	// creg only
	CBeamLaserProjectile() { }

	CBeamLaserProjectile(const ProjectileParams& params);
	~CBeamLaserProjectile() override;

	void Update() override;
	void Draw() override;
	void DrawOnMinimap() const override;

	int GetProjectilesCount() const override;

private:
	uint8_t ccsColor[4];
	uint8_t cceColor[4];
	uint8_t ecsColor[4];
	uint8_t eceColor[4];

	size_t pgOffset;
};