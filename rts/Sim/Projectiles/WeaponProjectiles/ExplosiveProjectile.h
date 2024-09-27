/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "WeaponProjectile.h"

class CExplosiveProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CExplosiveProjectile)
public:
	// creg only
	CExplosiveProjectile() { }

	CExplosiveProjectile(const ProjectileParams& params);
	~CExplosiveProjectile() override;

	void Update() override;
	void Draw() override;

	int GetProjectilesCount() const override;

	int ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed) override;
private:
	float invttl;
	float curTime;
	size_t pgOffset;
};