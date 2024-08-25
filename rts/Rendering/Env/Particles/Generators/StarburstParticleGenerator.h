#pragma once

#include "ParticleGenerator.h"
#include <array>

struct StarburstParticleData {
	CR_DECLARE_STRUCT(StarburstParticleData)

	float3 partPos;
	int32_t missileAge;

	float3 partSpeed;
	uint32_t curTracerPart;

	int32_t drawOrder;
	std::array<int32_t, 3> unused;

	std::array<float4, 3> tracerPosSpeed;
	std::array<float4, 3> tracerDirNumMods;
	std::array<float, 3 * 20> allAgeMods;

	AtlasedTexture texCoord1;
	AtlasedTexture texCoord3;

	int32_t GetMaxNumQuads() const {
		return
			3 * 20 * (texCoord1 != AtlasedTexture::DefaultAtlasTexture) +
			1 * (texCoord3 != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord1 = AtlasedTexture::DefaultAtlasTexture;
		texCoord3 = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(StarburstParticleData) % 16 == 0);

class StarburstParticleGenerator : public ParticleGenerator<StarburstParticleData, StarburstParticleGenerator> {
	friend class ParticleGenerator<StarburstParticleData, StarburstParticleGenerator>;
public:
	StarburstParticleGenerator() {}
	~StarburstParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};