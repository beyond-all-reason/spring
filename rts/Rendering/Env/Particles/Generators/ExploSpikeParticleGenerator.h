#pragma once

#include "ParticleGenerator.h"

struct ExploSpikeParticleData {
	float3 pos;
	float alpha;

	float3 speed;
	float alphaDecay;

	float3 dir;
	SColor color;	

	float length;
	float lengthGrowth;
	float width;
	int32_t drawOrder;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const { return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};
static_assert(sizeof(ExploSpikeParticleData) % 16 == 0);

class ExploSpikeParticleGenerator : public ParticleGenerator<ExploSpikeParticleData, ExploSpikeParticleGenerator> {
	friend class ParticleGenerator<ExploSpikeParticleData, ExploSpikeParticleGenerator>;
public:
	ExploSpikeParticleGenerator() {}
	~ExploSpikeParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
