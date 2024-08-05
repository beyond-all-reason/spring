#pragma once

#include "ParticleGenerator.h"

//no regular Update() needed
struct alignas(16) EmgParticleData {
	float3 pos;
	float radius;

	float3 animParams;
	SColor color;

	float3 rotParams;
	int32_t drawOrder;

	AtlasedTexture texCoord;

	int32_t GetNumQuads() const { return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
};
static_assert(sizeof(EmgParticleData) % 16 == 0);

class EmgParticleGenerator : public ParticleGenerator<EmgParticleData, EmgParticleGenerator> {
public:
	EmgParticleGenerator() {}
	~EmgParticleGenerator() override {}
protected:
	bool GenerateCPUImpl() override { return false; }
};
