#pragma once

#include "ParticleGenerator.h"

// no need for regular Update()
struct TorpedoData {
	float3 partPos;
	int32_t drawOrder;

	float3 partSpeed; // non-normalized dir
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

static_assert(sizeof(TorpedoData) % 16 == 0);

class TorpedoParticleGenerator : public ParticleGenerator<TorpedoData, TorpedoParticleGenerator> {
	friend class ParticleGenerator<TorpedoData, TorpedoParticleGenerator>;
public:
	TorpedoParticleGenerator() {}
	~TorpedoParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};