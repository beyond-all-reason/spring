#pragma once

#include <array>
#include <algorithm>

#include "ParticleGenerator.h"
#include "System/SpringMath.h"

struct FireballParticleData {
	CR_DECLARE_STRUCT(FireballParticleData)

	std::array<float4, 12> sparkPosSize;

	float3 dgunPos;
	float dgunSize;

	float3 animParams1;
	int32_t numSparks;

	float3 animParams2;
	int32_t drawOrder;

	float3 speed;
	float checkCol;

	AtlasedTexture texCoord1;
	AtlasedTexture texCoord2;

	int32_t GetMaxNumQuads() const {
		const auto numFire = std::min(10, numSparks);
		return
			numSparks * (texCoord1 != AtlasedTexture::DefaultAtlasTexture) +
			numFire   * (texCoord2 != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord1 = AtlasedTexture::DefaultAtlasTexture;
		texCoord2 = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(FireballParticleData) % 16 == 0);

class FireballParticleGenerator : public ParticleGenerator<FireballParticleData, FireballParticleGenerator> {
	friend class ParticleGenerator<FireballParticleData, FireballParticleGenerator>;
public:
	FireballParticleGenerator() {}
	~FireballParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};