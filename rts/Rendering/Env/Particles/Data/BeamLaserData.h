#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

//no regular Update() needed
struct alignas(16) BeamLaserData {
	float3 startPos;
	float unused0;
	float3 targetPos;
	float unused1;
	struct {
		SColor coreColStart;
		SColor coreColEnd;
		SColor edgeColStart;
		SColor edgeColEnd;
	};
	float4 texCoord1;
	float4 texCoord2;
	float4 texCoord3;
	float4 thicknessFlareDecay;
};

static_assert(sizeof(BeamLaserData) % 16 == 0);