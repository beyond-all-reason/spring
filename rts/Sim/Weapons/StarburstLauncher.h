/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Weapon.h"

class CStarburstLauncher: public CWeapon
{
	CR_DECLARE_DERIVED(CStarburstLauncher)
public:
	CStarburstLauncher(CUnit* owner = nullptr, const WeaponDef* def = nullptr);

	float GetRange2D(float boost, float ydiff) const override final;

private:
	const float3& GetAimFromPos(bool useMuzzle = false) const override { return weaponMuzzlePos; }

	TargetCheckResult HaveFreeLineOfFire(const float3& srcPos, const float3& tgtPos, const SWeaponTarget& trg, int avoidFlagsOverride = -1) const override final;
	void FireImpl(const bool scriptCall) override final;

private:
	float tracking;
	float uptime;
};
