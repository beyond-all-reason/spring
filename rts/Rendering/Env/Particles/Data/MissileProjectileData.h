#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

//needs Update()
struct alignas(16) MissileProjectileData {
	float3 pos;
	float fsize;
	float3 speed;
	float unused;
	float4 texCoord;
};

static_assert(sizeof(MissileProjectileData) % 16 == 0);