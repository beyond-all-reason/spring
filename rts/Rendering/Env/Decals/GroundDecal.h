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

	float4 forcedNormal;

	// not sent to the shader
	uint32_t id;
public:
	static uint32_t GetNextId() { nextId = (nextId % GroundDecal::ID_WRAPAROUND) + 1; return nextId; }
	static inline uint32_t nextId = 0; // 0 in fact is reserved and never used
	static constexpr uint32_t ID_WRAPAROUND = 1 << 20;
};