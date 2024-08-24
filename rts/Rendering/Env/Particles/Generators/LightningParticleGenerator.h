#pragma once

#include "ParticleGenerator.h"

// needs Update()
struct LightningData {
	float3 startPos;
	float thickness;

	float3 targetPos;
	float unused;

	std::array<float, 24> displacements;

	AtlasedTexture texCoord;

	SColor col;
	int32_t drawOrder;
	float unused2[2];

	int32_t GetMaxNumQuads() const {
		return
			(displacements.size() - 2 - 2) * (texCoord != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(LightningData) % 16 == 0);

class LightningParticleGenerator : public ParticleGenerator<LightningData, LightningParticleGenerator> {
	friend class ParticleGenerator<LightningData, LightningParticleGenerator>;
public:
	LightningParticleGenerator() {}
	~LightningParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};