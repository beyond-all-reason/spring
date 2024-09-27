#pragma once

#include "ParticleGenerator.h"

struct DirtParticleData {
	CR_DECLARE_STRUCT(DirtParticleData)
	float3 pos;
	float alpha;

	float3 speed;
	SColor color;

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
static_assert(sizeof(DirtParticleData) % 16 == 0);

class DirtParticleGenerator : public ParticleGenerator<DirtParticleData, DirtParticleGenerator> {
	friend class ParticleGenerator<DirtParticleData, DirtParticleGenerator>;
public:
	DirtParticleGenerator() {}
	~DirtParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
