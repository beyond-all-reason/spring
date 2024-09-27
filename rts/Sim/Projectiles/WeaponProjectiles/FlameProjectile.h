/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "WeaponProjectile.h"

class CFlameProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CFlameProjectile)
public:
	// creg only
	CFlameProjectile() { }

	CFlameProjectile(const ProjectileParams& params);
	~CFlameProjectile() override;

	void Update() override;
	void Draw() override;
	void Collision() override;

	int GetProjectilesCount() const override;

	int ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed) override;

private:
	float curTime;
	/// precentage of lifetime when the projectile is active and can collide
	float physLife;
	float invttl;

	float3 spread;
	size_t pgOffset;
};