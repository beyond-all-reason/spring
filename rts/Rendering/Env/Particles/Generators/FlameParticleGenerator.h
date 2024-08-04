#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

//update every Update();
struct alignas(16) FlameParticleData {
	float3 pos;
	float radius;
	float3 speed;
	float radiusGrowth;
	float3 spread;
	float unused2;
	struct {
		SColor color0;
		SColor color1;
		uint32_t unused2;
		uint32_t unused3;
	};
	float4 texCoord;
	float3 rotParams;
	float curTime;
	float3 animParams;
	float unused4;
};

static_assert(sizeof(FlameParticleData) % 16 == 0);