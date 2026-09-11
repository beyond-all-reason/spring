/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Weapon.h"

class CMeleeWeapon: public CWeapon
{
	CR_DECLARE_DERIVED(CMeleeWeapon)
public:
	CMeleeWeapon(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}

private:
	TargetCheckResult HaveFreeLineOfFire(const float3& srcPos, const float3& tgtPos, const SWeaponTarget& trg, int avoidFlagsOverride = -1) const override final { return TargetCheckResult::Clear; }
	void FireImpl(const bool scriptCall) override final;
};
