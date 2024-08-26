#pragma once

#include "ParticleGenerator.h"

// needs Update()
struct LightningParticleData {
	CR_DECLARE_STRUCT(LightningParticleData)

	float3 startPos;
	float thickness;

	float3 targetPos;
	float unused;

	std::array<float, 24> displacements;

	AtlasedTexture texCoord;

	SColor color;
	int32_t drawOrder;
	std::array<float, 2> unused2;

	int32_t GetMaxNumQuads() const {
		return
			(displacements.size() - 2 - 2) * (texCoord != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(LightningParticleData) % 16 == 0);

class LightningParticleGenerator : public ParticleGenerator<LightningParticleData, LightningParticleGenerator> {
	friend class ParticleGenerator<LightningParticleData, LightningParticleGenerator>;
public:
	LightningParticleGenerator() {}
	~LightningParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};