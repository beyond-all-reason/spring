/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/float3.h"

/**
 * @brief Predicts where a weapon's muzzle ends up once its script has aimed.
 *
 * Line-of-fire tests that run before the turret has turned (target selection,
 * SlowUpdate re-validation) have to guess the muzzle position. This models the
 * muzzle as a yaw pivot plus an offset that turns with yaw only plus an offset
 * that turns with yaw and pitch:
 *
 *   m = pivot + lateral*R + forward*H + barrelForward*(k*H + s*U) + barrelUp*(k*U - s*H)
 *
 * where H is the horizontal aim direction, R = (H.z, 0, -H.x) its perpendicular,
 * U is up, k = cos(pitch) and s = sin(pitch). All vectors are in unit-local space
 * (rightdir, updir, frontdir components), the frame CWeapon::CallAimingScript
 * derives heading and pitch in. Evaluation needs one sqrt and no trigonometry,
 * and the expression is linear in the five offsets, so they can be fitted from
 * observed (aim direction, muzzle position) pairs with plain least squares.
 *
 * Weapons that alternate between several muzzle pieces get one estimate for the
 * mean muzzle: which barrel fires next depends on where the script advances its
 * counter relative to the engine's QueryWeapon re-query (measured: Storm fires
 * the current piece, AK mostly the other one, Pawn alternates), so at aiming
 * time the barrel cannot be predicted from the current piece.
 */
struct AimFromEstimate {
	/// yaw axis position relative to the unit origin
	float3 pivot;
	/// offset perpendicular to the aim direction (positive along rightdir when aiming forward), turns with yaw only
	float lateral = 0.0f;
	/// offset along the horizontal aim direction, turns with yaw only
	float forward = 0.0f;
	/// offset along the barrel, turns with yaw and pitch
	float barrelForward = 0.0f;
	/// offset perpendicular to the barrel (upwards at zero pitch), turns with yaw and pitch
	float barrelUp = 0.0f;

	/// unit-local muzzle position for a unit-local aim direction (need not be normalised)
	float3 Eval(const float3& dir) const
	{
		const float hSq = dir.x * dir.x + dir.z * dir.z;
		const float len = math::sqrt(hSq + dir.y * dir.y);

		if (len <= 0.0f)
			return pivot;

		const float h = math::sqrt(hSq);
		const float3 H = (h > 1e-6f) ? float3(dir.x / h, 0.0f, dir.z / h) : FwdVector;
		const float3 R(H.z, 0.0f, -H.x);
		const float s = dir.y / len; // sin(pitch)
		const float k = h / len;     // cos(pitch)

		return pivot
			+ R * lateral
			+ H * (forward + barrelForward * k - barrelUp * s)
			+ UpVector * (barrelForward * s + barrelUp * k);
	}
};
