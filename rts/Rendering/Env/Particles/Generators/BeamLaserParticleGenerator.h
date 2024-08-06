#pragma once

#include "ParticleGenerator.h"

//no regular Update() needed
struct alignas(16) BeamLaserData {
	float3 startPos;
	SColor coreColStart;

	float3 targetPos;
	SColor coreColEnd;

	float3 animParams1;
	SColor edgeColStart;

	float3 animParams2;
	SColor edgeColEnd;

	float3 animParams3;
	int32_t drawOrder;

	AtlasedTexture texCoord1;
	AtlasedTexture texCoord2;
	AtlasedTexture texCoord3;

	float thickness;
	float coreThickness;
	float flareSize;
	float midTexX2;

	int32_t GetNumQuads() const {
		return
			2 * (texCoord1 != AtlasedTexture::DefaultAtlasTexture) +
			4 * (texCoord2 != AtlasedTexture::DefaultAtlasTexture) +
			2 * (texCoord3 != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord1 = AtlasedTexture::DefaultAtlasTexture;
		texCoord2 = AtlasedTexture::DefaultAtlasTexture;
		texCoord3 = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(BeamLaserData) % 16 == 0);

class BeamLaserParticleGenerator : public ParticleGenerator<BeamLaserData, BeamLaserParticleGenerator> {
public:
	BeamLaserParticleGenerator() {}
	~BeamLaserParticleGenerator() override {}
protected:
	bool GenerateCPUImpl() override { return false; }
};
