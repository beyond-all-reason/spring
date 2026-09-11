/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>

#include "Sim/Misc/TargetCheckResult.h"

class float3;
class CUnit;
class CFeature;
class CWeapon;
class CPlasmaRepulser;
class CSolidObject;
struct CollisionQuery;

namespace Collision {
	enum {
		NOENEMIES    = 1 << 0,
		NOFRIENDLIES = 1 << 1,
		NOFEATURES   = 1 << 2,
		NONEUTRALS   = 1 << 3,
		NOFIREBASES  = 1 << 4, // ignored by raytraces; not added to avoidFlags
		NONONTARGETS = 1 << 5, // ignored by raytraces; not added to avoidFlags
		NOGROUND     = 1 << 6,
		NOCLOAKED    = 1 << 7,
		// Query-only subdivisions of friendlies. Never stored in weapon flags.
		NOMOBILEFRIENDLIES = 1 << 8,
		NOSTATICFRIENDLIES = 1 << 9,
		NOUNITS      = NOENEMIES | NOFRIENDLIES | NONEUTRALS
	};
	bool SkipFriendly(const CUnit* unit, int traceFlags);
}

namespace TraceRay {
	struct SShieldDist {
		CPlasmaRepulser* rep;
		float dist;
	};

	float TraceRay(
		const float3& pos,
		const float3& dir,
		float traceLength,
		int traceFlags,
		const CUnit* owner,
		CUnit*& hitUnit,
		CFeature*& hitFeature,
		CollisionQuery* hitColQuery = nullptr,
		int queryFlags = 0
	);
	float TraceRay(
		const float3& pos,
		const float3& dir,
		float traceLength,
		int traceFlags,
		int allyTeam,
		const CUnit* owner,
		CUnit*& hitUnit,
		CFeature*& hitFeature,
		CollisionQuery* hitColQuery,
		int queryFlags = 0
	);

	void TraceRayShields(
		const CWeapon* emitter,
		const float3& start,
		const float3& dir,
		float length,
		std::vector<SShieldDist>& hitShields
	);

	float GuiTraceRay(
		const float3& start,
		const float3& dir,
		const float length,
		const CUnit* exclude,
		const CUnit*& hitUnit,
		const CFeature*& hitFeature,
		bool useRadar,
		bool groundOnly = false,
		bool ignoreWater = true
	);

	/**
	 * @return Clear if unobstructed, otherwise the first blocker category
	 * within the firing cone of \<owner\>.
	 */
	TargetCheckResult TestCone(
		const float3& from,
		const float3& dir,
		float length,
		float spread,
		int allyteam,
		int traceFlags,
		CUnit* owner);

	/**
	 * @return Clear if unobstructed, otherwise the first blocker category
	 * within the firing trajectory of \<owner\>.
	 */
	TargetCheckResult TestTrajectoryCone(
		const float3& from,
		const float3& dir,
		float length,
		float linear,
		float quadratic,
		float spread,
		int allyteam,
		int traceFlags,
		CUnit* owner);
}
