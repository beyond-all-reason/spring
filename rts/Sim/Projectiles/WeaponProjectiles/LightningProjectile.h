/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "WeaponProjectile.h"

class CWeapon;

class CLightningProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CLightningProjectile)
public:
	// creg only
	CLightningProjectile() { }

	CLightningProjectile(const ProjectileParams& params);
	~CLightningProjectile() override;

	void Update() override;
	void Draw() override;
	void DrawOnMinimap() const override;

	int GetProjectilesCount() const override;

private:
	std::array<std::array<float, 12>, 2> displacements;
	size_t pgOffset;
};