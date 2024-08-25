#pragma once

#include "ParticleGenerator.h"

// no need for regular Update()
struct TorpedoParticleData {
	CR_DECLARE_STRUCT(TorpedoParticleData)
	float3 pos;
	int32_t drawOrder;

	float3 speed; // non-normalized dir
	float unused;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const {
		return
			8 * (texCoord != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(TorpedoParticleData) % 16 == 0);

class TorpedoParticleGenerator : public ParticleGenerator<TorpedoParticleData, TorpedoParticleGenerator> {
	friend class ParticleGenerator<TorpedoParticleData, TorpedoParticleGenerator>;
public:
	TorpedoParticleGenerator() {}
	~TorpedoParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};