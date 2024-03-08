#pragma once

#include <string>

#include "System/type2.h"
#include "System/float3.h"
#include "Rendering/Textures/TextureAtlas.h"

class CUnit;
struct GroundDecal {
public:
	enum class Type : uint8_t {
		DECAL_NONE      = 0,
		DECAL_PLATE     = 1,
		DECAL_EXPLOSION = 2,
		DECAL_TRACK     = 3,
		DECAL_LUA       = 4
	};
public:
	bool IsValid() const { return info.type > Type::DECAL_NONE || alpha > 0.0f; }
	void MarkInvalid() { info.type = Type::DECAL_NONE; }
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

	// not sent to the shader
	struct TypeID {
		Type type : 8;
		uint32_t id : 24;
	} info;
public:
	static uint32_t GetNextId() {
		nextId = (nextId % GroundDecal::ID_WRAPAROUND) + 1; return nextId;
	}
	static inline uint32_t nextId = 0; // 0 in fact is reserved and never used
	static constexpr uint32_t ID_WRAPAROUND = 1 << 20;
};