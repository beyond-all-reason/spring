#pragma once

#include "ParticleGenerator.h"

//no regular Update() needed
struct BitmapMuzzleFlameData {
	float3 pos;
	float invttl;

	float3 dir;
	int32_t createFrame;

	float3 rotParams;
	int32_t drawOrder;

	float3 animParams;
	float sizeGrowth;

	float size;
	float length;
	float frontOffset;
	float unused;

	SColor col0;
	SColor col1;
	float edge0;
	float edge1;

	AtlasedTexture sideTexture;
	AtlasedTexture frontTexture;

	int32_t GetMaxNumQuads() const {
		return
			2 * (sideTexture != AtlasedTexture::DefaultAtlasTexture) +
			1 * (frontTexture != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		sideTexture = AtlasedTexture::DefaultAtlasTexture;
		frontTexture = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(BitmapMuzzleFlameData) % 16 == 0);

class BitmapMuzzleFlameParticleGenerator : public ParticleGenerator<BitmapMuzzleFlameData, BitmapMuzzleFlameParticleGenerator> {
	friend class ParticleGenerator<BitmapMuzzleFlameData, BitmapMuzzleFlameParticleGenerator>;
public:
	BitmapMuzzleFlameParticleGenerator() {}
	~BitmapMuzzleFlameParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
