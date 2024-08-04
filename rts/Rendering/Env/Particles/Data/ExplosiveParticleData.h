#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/Color.h"

//no regular Update() needed
struct alignas(16) ExplosiveParticleData {
	float3 pos;
	float radius;

	SColor color0;
	SColor color1;
	uint32_t numStages;
	uint32_t noGap;

	float alphaDecay;
	float sizeDecay;
	float separation;
	float unused;

	AtlasedTexture texCoord;
};

static_assert(sizeof(ExplosiveParticleData) % 16 == 0);