#pragma once

#include <string>

#include "System/type2.h"
#include "System/float3.h"
#include "Rendering/Textures/TextureAtlas.h"

class CUnit;
struct GroundDecal {
public:
	//enum class DecalType { EXPLOSION, BUILDING, GHOST, LUA };
	bool IsValid() const { return (alpha > 0.0f); }
	void MarkInvalid() { alpha = 0.0f; }
public:
	float2 posTL;
	float2 posTR;
	float2 posBR;
	float2 posBL;

	AtlasedTexture texMainOffsets;
	AtlasedTexture texNormOffsets;

	float alpha; // > 1.0f - glow, < 1.0f - alpha
	float alphaFalloff;
	float rot;
	float height;

	float createFrameMin;
	float createFrameMax;
	float uvWrapDistance;
	float uvTraveledDistance;

	float3 forcedNormal;
	float visMult;
};