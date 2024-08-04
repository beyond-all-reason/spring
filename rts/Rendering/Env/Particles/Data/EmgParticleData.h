#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/Color.h"

//no regular Update() needed
struct alignas(16) EmgParticleData {
	float3 pos;
	float radius;
	float3 speed;
	float gravity;
	AtlasedTexture texCoord;

	SColor color;
	float intensity;
	float creatFrame;
	float destrFrame;
};

static_assert(sizeof(EmgParticleData) % 16 == 0);