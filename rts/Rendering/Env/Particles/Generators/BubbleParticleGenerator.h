#pragma once

#include "ParticleGenerator.h"

struct BubbleParticleData {
	float3 pos;
	float alpha;

	float size;
	float sizeExpansion;
	int32_t drawOrder;
	float unused;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const { return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};
static_assert(sizeof(BubbleParticleData) % 16 == 0);

class BubbleParticleGenerator : public ParticleGenerator<BubbleParticleData, BubbleParticleData> {
public:
	BubbleParticleGenerator() {}
	~BubbleParticleGenerator() override {}
protected:
	bool GenerateCPUImpl() override { return false; }
};
