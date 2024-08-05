/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EMG_PROJECTILE_H
#define _EMG_PROJECTILE_H

#include "WeaponProjectile.h"
#include "Rendering/Env/Particles/Generators/EmgParticleGenerator.h"

class CEmgProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CEmgProjectile)
public:
	// creg only
	CEmgProjectile() { }

	CEmgProjectile(const ProjectileParams& params);
	~CEmgProjectile() override;

	void Update() override;
	void Draw() override;

	int GetProjectilesCount() const override;

	int ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed) override;

private:
	float intensity;
	float3 color;
	size_t pgOffset;
};

#endif // _EMG_PROJECTILE_H
