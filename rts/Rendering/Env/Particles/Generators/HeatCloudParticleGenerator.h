#pragma once

#include "ParticleGenerator.h"

struct HeatCloudParticleData {
	float3 pos;
	float maxHeat;

	float3 speed;
	float heat;

	float3 animParams;
	float size;

	float3 rotParams;
	float sizeGrowth;	
	
	float sizeMod;
	int32_t drawOrder;
	int32_t createFrame;
	float unused;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const { return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};
static_assert(sizeof(HeatCloudParticleData) % 16 == 0);

class HeatCloudParticleGenerator : public ParticleGenerator<HeatCloudParticleData, HeatCloudParticleGenerator> {
	friend class ParticleGenerator<HeatCloudParticleData, HeatCloudParticleGenerator>;
public:
	HeatCloudParticleGenerator() {}
	~HeatCloudParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
