/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Weapon.h"

class CMissileLauncher: public CWeapon
{
	CR_DECLARE_DERIVED(CMissileLauncher)
public:
	CMissileLauncher(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}

	void UpdateWantedDir() override final;

private:
	const float3& GetAimFromPos(bool useMuzzle = false) const override { return weaponMuzzlePos; }

	TargetCheckResult HaveFreeLineOfFire(const float3& srcPos, const float3& tgtPos, const SWeaponTarget& trg, int avoidFlagsOverride = -1) const override final;
	void FireImpl(const bool scriptCall) override final;
};
