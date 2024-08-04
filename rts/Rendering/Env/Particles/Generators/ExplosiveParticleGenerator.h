#pragma once

#include "ParticleGenerator.h"

struct alignas(16) ExplosiveParticleData {
	float3 pos;
	float radius;

	SColor color0;
	SColor color1;
	uint32_t numStages;
	uint32_t noGap;

	float alphaDecay;
	float sizeDecay;
	float separation;

	AtlasedTexture texCoord;

	size_t GetNumQuads() const { return numStages; }
};
static_assert(sizeof(ExplosiveParticleData) % 16 == 0);

class ExplosiveParticleGenerator : public ParticleGenerator<ExplosiveParticleData, ExplosiveParticleGenerator> {
public:
	ExplosiveParticleGenerator();
	~ExplosiveParticleGenerator();
protected:
	bool GenerateGPU() override;
	bool GenerateCPU() override;
};
