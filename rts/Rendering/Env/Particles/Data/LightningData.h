#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

// needs Update()
struct alignas(16) LightningData {
	float3 startPos;
	float radius;
	float3 targetPos;
	float thickness;
	float4 texCoord;
	SColor col;
};

static_assert(sizeof(FireBallSparkData) % 16 == 0);
static_assert(sizeof(FireBallDgunData) % 16 == 0);