#pragma once

#include <string>
#include <array>

#include "System/type2.h"
#include "System/float3.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/GL/VertexArrayTypes.h"

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
	bool IsValid() const { return info.type > static_cast<uint8_t>(Type::DECAL_NONE); }
	void MarkInvalid() { info.type = static_cast<uint8_t>(Type::DECAL_NONE); }
public:
	float refHeight;
	float minHeight;
	float maxHeight;
	float forceHeightMode;

	float2 posTL;
	float2 posTR;
	float2 posBR;
	float2 posBL;

	AtlasedTexture texMainOffsets;
	AtlasedTexture texNormOffsets;

	float alpha;
	float alphaFalloff;
	float glow;
	float glowFalloff;

	float rot;
	float height;
	float dotElimExp;
	float cmAlphaMult;

	float createFrameMin;
	float createFrameMax;
	float uvWrapDistance;
	float uvTraveledDistance;

	float3 forcedNormal;
	float visMult;

	struct TypeID {
		uint32_t type : 8;
		uint32_t id   : 24;
	} info;
	SColor tintColor;
	std::array<SColor, 2> glowColorMap;
public:
	static uint32_t GetNextId() {
		nextId = (nextId % GroundDecal::ID_WRAPAROUND) + 1; return nextId;
	}
	static inline uint32_t nextId = 0; // 0 in fact is reserved and never used
	static constexpr uint32_t ID_WRAPAROUND = 1 << 20;

	static const std::array<AttributeDef, 10> attributeDefs;
};
static_assert(sizeof(GroundDecal::info) == 4u);