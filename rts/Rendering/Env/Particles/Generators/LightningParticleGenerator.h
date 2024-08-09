#pragma once

#include "ParticleGenerator.h"

// needs Update()
struct alignas(16) LightningData {
	float3 startPos;
	float thickness;

	float3 targetPos;
	float unused;
	// gap

	std::array<float, 24> displacements;

	AtlasedTexture texCoord;

	SColor col;
	int32_t drawOrder;
	//2 gaps

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
public:
	LightningParticleGenerator() {}
	~LightningParticleGenerator() override {}
protected:
	bool GenerateCPUImpl() override { return false; }
};