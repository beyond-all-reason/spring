/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Sim/Projectiles/Projectile.h"

class CMuzzleFlame : public CProjectile
{
	CR_DECLARE_DERIVED(CMuzzleFlame)

public:
	CMuzzleFlame() { }
	CMuzzleFlame(const float3& pos, const float3& speed, const float3& dir, float size);

	void Draw() override;
	void Update() override;

	int GetProjectilesCount() const override;

private:
	float size;
	int age;
	int numFlame;
	int numSmoke;

	std::vector<float3> randSmokeDir;
};