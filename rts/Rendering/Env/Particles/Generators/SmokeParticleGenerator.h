#pragma once

#include "ParticleGenerator.h"

struct SmokeParticleData {
	CR_DECLARE_STRUCT(SmokeParticleData)
	float3 pos;
	float size;

	float startSize;
	float sizeExpansion;
	float ageRate;
	float unused;

	float3 speed;
	int32_t createFrame;

	float3 animParams;
	SColor color;

	float3 rotParams;
	int32_t drawOrder;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const { return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};
static_assert(sizeof(SmokeParticleData) % 16 == 0);

class SmokeParticleGenerator : public ParticleGenerator<SmokeParticleData, SmokeParticleGenerator> {
	friend class ParticleGenerator<SmokeParticleData, SmokeParticleGenerator>;
public:
	SmokeParticleGenerator() {}
	~SmokeParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
