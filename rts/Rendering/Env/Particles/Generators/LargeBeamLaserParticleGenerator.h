#pragma once

#include "ParticleGenerator.h"

struct alignas(16) LargeBeamLaserData {
	float3 startPos;
	int32_t drawOrder;

	float3 targetPos;
	// gap

	float thickness;
	float coreThickness;
	float flareSize;
	float tileLength;

	float scrollSpeed;
	float pulseSpeed;
	SColor coreColStart;
	SColor edgeColStart;

	AtlasedTexture texCoord1;
	AtlasedTexture texCoord2;
	AtlasedTexture texCoord3;
	AtlasedTexture texCoord4;

	int32_t GetMaxNumQuads() const {
		const float beamLength = (targetPos - startPos).Length();

		int32_t numQuads1 = 2 + std::max(0, 2 + static_cast<int32_t>(std::truncf(beamLength / tileLength)));

		return
			numQuads1 * (texCoord1 != AtlasedTexture::DefaultAtlasTexture) +
			2         * (texCoord2 != AtlasedTexture::DefaultAtlasTexture) +
			4         * (texCoord3 != AtlasedTexture::DefaultAtlasTexture) +
			2         * (texCoord4 != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord1 = AtlasedTexture::DefaultAtlasTexture;
		texCoord2 = AtlasedTexture::DefaultAtlasTexture;
		texCoord3 = AtlasedTexture::DefaultAtlasTexture;
		texCoord4 = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(LargeBeamLaserData) % 16 == 0);

class LargeBeamLaserParticleGenerator : public ParticleGenerator<LargeBeamLaserData, LargeBeamLaserParticleGenerator> {
public:
	LargeBeamLaserParticleGenerator() {}
	~LargeBeamLaserParticleGenerator() override {}
protected:
	bool GenerateCPUImpl() override { return false; }
};