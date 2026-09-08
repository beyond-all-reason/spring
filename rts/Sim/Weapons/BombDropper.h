/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Weapon.h"

class CBombDropper: public CWeapon
{
	CR_DECLARE_DERIVED(CBombDropper)
public:
	CBombDropper(CUnit* owner = nullptr, const WeaponDef* def = nullptr, bool useTorps = false);

	float GetPredictedImpactTime(const float3& p) const override final;

private:
	bool CanFire(bool ignoreAngleGood, bool ignoreTargetType, bool ignoreRequestedDir) const override final;

	bool TestTarget(const float3& pos, const SWeaponTarget& trg) const override final;
	bool TestRange(const float3& tgtPos, const SWeaponTarget& trg) const override final;
	// TODO: requires sampling parabola from aimFromPos down to dropPos
	bool HaveFreeLineOfFire(const float3& srcPos, const float3& tgtPos, const SWeaponTarget& trg, TargetCheckResult* result = nullptr, int avoidFlagsOverride = -1) const override final { return true; }
	void FireImpl(const bool scriptCall) override final;

private:
	/// if we should drop torpedoes
	bool dropTorpedoes;
	/// range of bombs (torpedoes) after they hit ground/water
	float torpMoveRange;
	float tracking;
};
