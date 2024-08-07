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

	int32_t GetNumQuads() const {
		/*
		const float3 zdir = (targetPos - startPos).SafeANormalize();
		const float startTex = 1.0 - std::modf(frameInfo.z * scrollSpeed, 1.0f);
		float beamTileMinDst = tileLength * (1.0 - startTex);
		const float beamLength = (targetPos - startPos).dot(zdir);
		const float beamTileMaxDst = beamLength - tilelength;

		int32_t numQuads1 = (beamTileMinDst > beamLength) ?
			2 :
			4 + 2 * int(std::ceilf((beamTileMaxDst - beamTileMinDst) / tilelength));
		*/
		int32_t numQuads1 = 100;

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