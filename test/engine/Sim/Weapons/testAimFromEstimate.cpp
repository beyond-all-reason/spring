/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include <cmath>

#include "Sim/Weapons/AimFromEstimate.h"
#include "System/float3.h"

namespace {
	// reference model built with explicit rotations: a turret that yaws around a
	// vertical axis through <pivot>, carrying a barrel that pitches around the
	// turret's local right axis at <turretOffset>
	struct ReferenceTurret {
		float3 pivot;
		float3 turretOffset; // (lateral, 0, forward) in turret space
		float3 barrelOffset; // (0, up, forward) in barrel space

		static float3 Yaw(const float3& v, float yaw) {
			const float c = std::cos(yaw);
			const float s = std::sin(yaw);
			// local frame: x = right, z = front; yaw 0 aims along +z, positive yaw turns towards +x
			return float3(v.x * c + v.z * s, v.y, -v.x * s + v.z * c);
		}
		static float3 Pitch(const float3& v, float pitch) {
			const float c = std::cos(pitch);
			const float s = std::sin(pitch);
			// rotate in the (front, up) plane, positive pitch lifts the front
			return float3(v.x, v.y * c + v.z * s, -v.y * s + v.z * c);
		}

		float3 AimDir(float yaw, float pitch) const {
			return Yaw(Pitch(FwdVector, pitch), yaw);
		}
		float3 Muzzle(float yaw, float pitch) const {
			return pivot + Yaw(turretOffset + Pitch(barrelOffset, pitch), yaw);
		}
		AimFromEstimate Estimate() const {
			AimFromEstimate e;
			e.pivot = pivot;
			e.lateral = turretOffset.x;
			e.forward = turretOffset.z;
			e.barrelForward = barrelOffset.z;
			e.barrelUp = barrelOffset.y;
			return e;
		}
	};

	bool Near(const float3& a, const float3& b, float eps = 1e-3f) {
		return (a - b).SqLength() <= (eps * eps);
	}
}

TEST_CASE("AimFromEstimate matches an explicitly rotated two-axis turret")
{
	const ReferenceTurret turrets[] = {
		{ float3(0.0f, 20.0f, 0.0f), float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 25.0f) },   // plain barrel on the yaw axis
		{ float3(-1.0f, 21.8f, -8.5f), float3(-6.3f, 0.0f, 0.0f), float3(0.0f, -4.0f, 19.4f) },   // Stumpy-like: barrel beside the yaw axis
		{ float3(0.4f, 23.4f, 0.1f), float3(-0.4f, 0.0f, 1.3f), float3(0.0f, -1.0f, 26.4f) },  // Mart-like: long pitching barrel
		{ float3(0.0f, 66.7f, 0.0f), float3(0.0f, 0.0f, 0.0f), float3(0.0f, -2.3f, 26.4f) },  // LLT-like: tall pivot
	};

	for (const auto& t : turrets) {
		const AimFromEstimate e = t.Estimate();

		for (int yi = 0; yi < 16; yi++) {
			for (int pi = -3; pi <= 3; pi++) {
				const float yaw = yi * (2.0f * 3.14159265f / 16.0f);
				const float pitch = pi * 0.25f; // -0.75 .. 0.75 rad

				const float3 dir = t.AimDir(yaw, pitch);
				const float3 want = t.Muzzle(yaw, pitch);

				CHECK(Near(e.Eval(dir), want));
				// direction length must not matter
				CHECK(Near(e.Eval(dir * 137.0f), want));
			}
		}
	}
}

TEST_CASE("AimFromEstimate degenerate inputs")
{
	AimFromEstimate e;
	e.pivot = float3(1.0f, 2.0f, 3.0f);
	e.lateral = 4.0f;
	e.forward = 5.0f;
	e.barrelForward = 6.0f;
	e.barrelUp = 7.0f;

	SECTION("zero direction returns the pivot") {
		CHECK(Near(e.Eval(ZeroVector), e.pivot));
	}

	SECTION("straight up falls back to aiming forward, barrel pitched 90 degrees") {
		// H = front, R = right, k = 0, s = 1
		const float3 want = e.pivot + float3(e.lateral, e.barrelForward, e.forward - e.barrelUp);
		CHECK(Near(e.Eval(UpVector), want));
	}

	SECTION("only the pivot set reproduces a fixed point for every direction") {
		AimFromEstimate fixedPoint;
		fixedPoint.pivot = float3(-3.0f, 40.0f, 8.0f);
		CHECK(Near(fixedPoint.Eval(FwdVector), fixedPoint.pivot));
		CHECK(Near(fixedPoint.Eval(float3(1.0f, -0.5f, -1.0f)), fixedPoint.pivot));
	}

	SECTION("lateral offset points along +x when aiming forward") {
		AimFromEstimate lat;
		lat.lateral = 3.0f;
		CHECK(Near(lat.Eval(FwdVector), float3(3.0f, 0.0f, 0.0f)));
		// and along -z when aiming towards +x (turned by 90 degrees)
		CHECK(Near(lat.Eval(float3(1.0f, 0.0f, 0.0f)), float3(0.0f, 0.0f, -3.0f)));
	}
}
