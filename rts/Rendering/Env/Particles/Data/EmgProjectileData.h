#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

//no regular Update() needed
struct alignas(16) EmgProjectileData {
	float3 pos;
	float radius;
	float3 speed;
	float gravity;
	float4 texCoord;
	struct {
		SColor color;
		float intensity;
		float strFrame;
		float endFrame;
	};
};

static_assert(sizeof(EmgProjectileData) % 16 == 0);