#pragma once

#include "ParticleGenerator.h"

struct LargeBeamLaserParticleData {
	CR_DECLARE_STRUCT(LargeBeamLaserParticleData)

	float3 startPos;
	int32_t drawOrder;

	float3 targetPos;
	float unused;

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

		// underestimates the number of quads. FIX
		int32_t numQuads1 = 4 + 2 * std::max(0, static_cast<int32_t>(std::truncf(beamLength / tileLength)));

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

static_assert(sizeof(LargeBeamLaserParticleData) % 16 == 0);

class LargeBeamLaserParticleGenerator : public ParticleGenerator<LargeBeamLaserParticleData, LargeBeamLaserParticleGenerator> {
	friend class ParticleGenerator<LargeBeamLaserParticleData, LargeBeamLaserParticleGenerator>;
public:
	LargeBeamLaserParticleGenerator() {}
	~LargeBeamLaserParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};