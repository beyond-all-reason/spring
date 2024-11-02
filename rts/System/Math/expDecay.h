/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/float3.h"

inline float expDecay(float a, float b, float decay, float dt)
{
	return b+(a-b)*math::exp(-decay*dt);
}

inline void expDecay(float3& a, float3 b, float decay, float dt)
{
	a.x = expDecay(a.x, b.x, decay, dt);
	a.y = expDecay(a.y, b.y, decay, dt);
	a.z = expDecay(a.z, b.z, decay, dt);
}
