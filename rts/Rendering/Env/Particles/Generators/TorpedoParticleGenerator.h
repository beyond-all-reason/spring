#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

// no need for regular Update()
struct alignas(16) TorpedoData {
	float3 pos;
	float unused;
	float4 texCoord;
};

static_assert(sizeof(TorpedoData) % 16 == 0);