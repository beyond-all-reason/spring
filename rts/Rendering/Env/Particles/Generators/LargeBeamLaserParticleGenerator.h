#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

//no regular Update() needed
struct alignas(16) LargeBeamLaserData {
	float3 startPos;
	float unused1;
	float3 targetPos;
	float tileLength;
	struct {
		SColor coreColStart;
		SColor edgeColStart;
		float scrollSpeed;
		float pulseSpeed;
	};
	float4 texCoord1;
	float4 texCoord2;
	struct {
		float thickness;
		float coreThickness;
		float flareSize;
		float decay;
	}
};

static_assert(sizeof(LargeBeamLaserData) % 16 == 0);