#pragma once

#include "ParticleGenerator.h"

struct MuzzleFlameParticleData {
	float3 pos;
	float age;

	float3 randDir;
	float size;

	int32_t aIndex;
	int32_t drawOrder;
	float unused1;
	float unused2;

	AtlasedTexture texCoord1;
	AtlasedTexture texCoord2;

	int32_t GetMaxNumQuads() const {
		return
			1 * (texCoord1 != AtlasedTexture::DefaultAtlasTexture) +
			1 * (texCoord2 != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord1 = AtlasedTexture::DefaultAtlasTexture;
		texCoord2 = AtlasedTexture::DefaultAtlasTexture;
	}
};
static_assert(sizeof(MuzzleFlameParticleData) % 16 == 0);

class MuzzleFlameParticleGenerator : public ParticleGenerator<MuzzleFlameParticleData, MuzzleFlameParticleGenerator> {
	friend class ParticleGenerator<MuzzleFlameParticleData, MuzzleFlameParticleGenerator>;
public:
	MuzzleFlameParticleGenerator() {}
	~MuzzleFlameParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
