/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BEAM_LASER_PROJECTILE_H
#define BEAM_LASER_PROJECTILE_H

#include "WeaponProjectile.h"
#include "System/Color.h"

class CBeamLaserProjectile: public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CBeamLaserProjectile)
public:
	// creg only
	CBeamLaserProjectile() { }

	CBeamLaserProjectile(const ProjectileParams& params);

	void Update() override;
	void Draw() override;
	void DrawOnMinimap() override;

	int GetProjectilesCount() const override;

private:
	SColor coreColStart;
	SColor coreColEnd;
	SColor edgeColStart;
	SColor edgeColEnd;

	float thickness;
	float corethickness;
	float flaresize;
	float decay;
	float midtexx;
};

#endif // BEAM_LASER_PROJECTILE_H
