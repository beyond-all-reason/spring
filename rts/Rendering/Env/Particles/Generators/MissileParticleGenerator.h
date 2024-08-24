#pragma once

#include "ParticleGenerator.h"

struct MissileData {
	float3 pos;
	float fsize;

	float3 speed;
	int32_t drawOrder;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const {
		return
			1 * (texCoord != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(MissileData) % 16 == 0);

class MissileParticleGenerator : public ParticleGenerator<MissileData, MissileParticleGenerator> {
	friend class ParticleGenerator<MissileData, MissileParticleGenerator>;
public:
	MissileParticleGenerator() {}
	~MissileParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};