#pragma once

#include "ParticleGenerator.h"

struct ExplosiveParticleData {
	CR_DECLARE_STRUCT(ExplosiveParticleData)
	float3 pos;
	float size;

	float3 speed;
	int32_t createFrame;

	float3 dir;
	int32_t drawOrder;

	SColor color0;
	SColor color1;
	uint32_t numStages;
	uint32_t noGap;

	float3 animParams;
	float alphaDecay;

	float3 rotParams;
	float sizeDecay;

	float separation;
	float colEdge0;
	float colEdge1;
	float curTime;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const { return numStages * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};
static_assert(sizeof(ExplosiveParticleData) % 16 == 0);

class ExplosiveParticleGenerator : public ParticleGenerator<ExplosiveParticleData, ExplosiveParticleGenerator> {
	friend class ParticleGenerator<ExplosiveParticleData, ExplosiveParticleGenerator>;
public:
	ExplosiveParticleGenerator() {}
	~ExplosiveParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
