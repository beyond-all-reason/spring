#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

//no regular Update() needed
struct alignas(16) ExplosiveProjectileData {
	float3 pos;
	float radius;
	struct {
		SColor color0;
		SColor color1;
		uint32_t numStages;
		uint32_t noGap;
	}
	struct {
		float alphaDecay;
		float sizeDecay;
		float separation;
		float alphaDecay;
	};
	float4 texCoord;
}

static_assert(sizeof(ExplosiveProjectileData) % 16 == 0);