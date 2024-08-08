#pragma once

#include "ParticleGenerator.h"
// needs Update()
struct alignas(16) LaserData {
	float3 drawPos;
	float curLength;

	float3 dir;
	float maxLength;

	float thickness;
	float coreThickness;
	SColor color1;
	SColor color2;

	float lodDistance;
	int32_t drawOrder;
	int32_t checkCol;
	float stayTime;

	float speedf;
	// 3 x gap

	AtlasedTexture texCoord1;
	AtlasedTexture texCoord2;

	int32_t GetMaxNumQuads() const {
		return
			2 * (texCoord1 != AtlasedTexture::DefaultAtlasTexture) +
			4 * (texCoord2 != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord1 = AtlasedTexture::DefaultAtlasTexture;
		texCoord2 = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(LaserData) % 16 == 0);

class LaserParticleGenerator : public ParticleGenerator<LaserData, LaserParticleGenerator> {
public:
	LaserParticleGenerator() {}
	~LaserParticleGenerator() override {}
protected:
	bool GenerateCPUImpl() override { return false; }
};