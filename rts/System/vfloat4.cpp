/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/vfloat4.h"
#include "System/creg/creg_cond.h"
#include "System/SpringMath.h"

CR_BIND(vfloat4, )
CR_REG_METADATA(vfloat4, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z)))

//! gets initialized later when the map is loaded
float vfloat4::maxxpos = -1.0f;
float vfloat4::maxzpos = -1.0f;

bool vfloat4::IsInBounds() const
{
	assert(maxxpos > 0.0f); // check if initialized

	return ((x >= 0.0f && x <= maxxpos) && (z >= 0.0f && z <= maxzpos));
}

void vfloat4::ClampInBounds()
{
	assert(maxxpos > 0.0f); // check if initialized

	x = Clamp(x, 0.0f, maxxpos);
	z = Clamp(z, 0.0f, maxzpos);
}


bool vfloat4::IsInMap() const
{
	assert(maxxpos > 0.0f); // check if initialized

	return ((x >= 0.0f && x <= maxxpos + 1) && (z >= 0.0f && z <= maxzpos + 1));
}


void vfloat4::ClampInMap()
{
	assert(maxxpos > 0.0f); // check if initialized

	x = Clamp(x, 0.0f, maxxpos + 1);
	z = Clamp(z, 0.0f, maxzpos + 1);
}

#ifndef USESSEvfloat4

vfloat4 vfloat4::min(const vfloat4 v1, const vfloat4 v2)
{
	return {std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z)};
}

vfloat4 vfloat4::max(const vfloat4 v1, const vfloat4 v2)
{
	return {std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z)};
}

#else

vfloat4 vfloat4::min(const vfloat4 v1, const vfloat4 v2) {
	vfloat4 result;
	__m128 a = _mm_loadu_ps(v1.xyzw);
	__m128 b = _mm_loadu_ps(v2.xyzw);
	_mm_storeu_ps(result.xyzw, _mm_min_ps(a, b));

	return result;
}

vfloat4 vfloat4::max(const vfloat4 v1, const vfloat4 v2) {
	vfloat4 result;
	__m128 a = _mm_loadu_ps(v1.xyzw);
	__m128 b = _mm_loadu_ps(v2.xyzw);
	_mm_storeu_ps(result.xyzw, _mm_max_ps(a, b));

	return result;
}
#endif

vfloat4 vfloat4::fabs(const vfloat4 v)
{
	return {std::fabs(v.x), std::fabs(v.y), std::fabs(v.z)};
}

vfloat4 vfloat4::sign(const vfloat4 v)
{
	return {Sign(v.x), Sign(v.y), Sign(v.z)};
}

bool vfloat4::equals(const vfloat4& f, const vfloat4& eps) const
{
	return (epscmp(x, f.x, eps.x) && epscmp(y, f.y, eps.y) && epscmp(z, f.z, eps.z));
}

