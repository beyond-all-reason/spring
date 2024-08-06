#pragma once

#include "ParticleGenerator.h"

struct alignas(16) FlameParticleData {
	float3 pos;
	float radius;

	float3 animParams;
	int32_t drawOrder;

	float3 rotParams;
	float curTime;

	SColor color0;
	SColor color1;
	float colEdge0;
	float colEdge1;

	AtlasedTexture texCoord;

	int32_t GetNumQuads() const { return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(FlameParticleData) % 16 == 0);


class FlameParticleGenerator : public ParticleGenerator<FlameParticleData, FlameParticleGenerator> {
public:
	FlameParticleGenerator() {}
	~FlameParticleGenerator() override {}
protected:
	bool GenerateCPUImpl() override { return false; }
};