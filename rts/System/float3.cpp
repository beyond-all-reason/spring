/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/float3.h"

#include <algorithm>

#include "System/creg/creg_cond.h"
#include "System/SpringMath.h"

CR_BIND(float3, )
CR_REG_METADATA(float3, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z)))

//! gets initialized later when the map is loaded
float float3::maxxpos = -1.0f;
float float3::maxzpos = -1.0f;

float3 float3::PickNonParallel() const
{
	// https://math.stackexchange.com/questions/3122010/how-to-deterministically-pick-a-vector-that-is-guaranteed-to-be-non-parallel-to
	auto [mi, Mi] = std::minmax_element(std::begin(xyz), std::end(xyz), [](const auto& a, const auto& b) { return math::fabs(a) < math::fabs(b); });
	float3 npVec{ 0.0f };
	npVec.xyz[std::distance(std::begin(xyz), mi)] = *Mi;

	// don't normalize as it most likely will go as argument to cross,
	// and the cross result will need to be normalized anyway
	return npVec;
}

bool float3::IsInBounds() const
{
	assert(maxxpos > 0.0f); // check if initialized

	return ((x >= 0.0f && x <= maxxpos) && (z >= 0.0f && z <= maxzpos));
}


void float3::ClampInBounds()
{
	assert(maxxpos > 0.0f); // check if initialized

	x = std::clamp(x, 0.0f, maxxpos);
	z = std::clamp(z, 0.0f, maxzpos);
}


bool float3::IsInMap() const
{
	assert(maxxpos > 0.0f); // check if initialized

	return ((x >= 0.0f && x <= maxxpos + 1) && (z >= 0.0f && z <= maxzpos + 1));
}


void float3::ClampInMap()
{
	assert(maxxpos > 0.0f); // check if initialized

	x = std::clamp(x, 0.0f, maxxpos + 1);
	z = std::clamp(z, 0.0f, maxzpos + 1);
}


float3 float3::min(const float3 v1, const float3 v2)
{
	return {std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z)};
}

float3 float3::max(const float3 v1, const float3 v2)
{
	return {std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z)};
}

float3 float3::fabs(const float3 v)
{
	return {std::fabs(v.x), std::fabs(v.y), std::fabs(v.z)};
}

float3 float3::sign(const float3 v)
{
	return {Sign(v.x), Sign(v.y), Sign(v.z)};
}

bool float3::equals(const float3& f, const float3& eps) const
{
	return (epscmp(x, f.x, eps.x) && epscmp(y, f.y, eps.y) && epscmp(z, f.z, eps.z));
}

float3 float3::snapToAxis() const {
	// https://gamedev.stackexchange.com/questions/83601/from-3d-rotation-snap-to-nearest-90-directions/183342#183342
	float nx = std::abs(x);
	float ny = std::abs(y);
	float nz = std::abs(z);
	if (nx > ny && nx > nz) {
		return float3(Sign(x), 0, 0);
	} else if (ny > nx && ny > nz) {
		return float3(0, Sign(y), 0);
	} else {
		return float3(0, 0, Sign(z));
	}
}