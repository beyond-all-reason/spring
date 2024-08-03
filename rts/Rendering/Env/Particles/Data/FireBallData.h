#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

// needs Update()
struct alignas(16) FireBallSparkData {
	float3 pos;
	float size;
	float3 speed;
	SColor col;
	float4 texCoord;
};

// needs Update()
struct alignas(16) FireBallDgunData {
	float3 pos;
	float posUpdate;
	float3 speed;
	float size;
	float4 texCoord;
	struct {
		uint32_t numFire;
		uint32_t maxCol;
		uint32_t unused1;
		uint32_t unused2;
	};
};

static_assert(sizeof(FireBallSparkData) % 16 == 0);
static_assert(sizeof(FireBallDgunData) % 16 == 0);