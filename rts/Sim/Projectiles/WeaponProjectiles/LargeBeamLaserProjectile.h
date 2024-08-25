/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LARGE_BEAM_LASER_PROJECTILE_H
#define LARGE_BEAM_LASER_PROJECTILE_H

#include "WeaponProjectile.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/Color.h"

class CLargeBeamLaserProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CLargeBeamLaserProjectile)
public:
	// creg only
	CLargeBeamLaserProjectile() { }

	CLargeBeamLaserProjectile(const ProjectileParams& params);
	~CLargeBeamLaserProjectile() override;

	void Update() override;
	void Draw() override;
	void DrawOnMinimap() const override;

	int GetProjectilesCount() const override;

private:
	uint8_t ccsColor[4];
	uint8_t ecsColor[4];

	float thickness;
	float corethickness;
	float flaresize;
	float tilelength;
	float scrollspeed;
	float pulseSpeed;
	float decay;
	size_t pgOffset;
};

#endif // LARGE_BEAM_LASER_PROJECTILE_H
