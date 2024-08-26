#pragma once

#include "ParticleGenerator.h"

struct FlameParticleData {
	CR_DECLARE_STRUCT(FlameParticleData)

	float3 pos;
	float drawRadius;

	float3 speed;
	int32_t createFrame;

	float3 animParams;
	int32_t drawOrder;

	float3 rotParams;
	float curTime;

	SColor color0;
	SColor color1;
	float colEdge0;
	float colEdge1;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const { return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(FlameParticleData) % 16 == 0);

class FlameParticleGenerator : public ParticleGenerator<FlameParticleData, FlameParticleGenerator> {
	friend class ParticleGenerator<FlameParticleData, FlameParticleGenerator>;
public:
	FlameParticleGenerator() {}
	~FlameParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};