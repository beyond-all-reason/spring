#pragma once

#include "ParticleGenerator.h"

//no regular Update() needed
struct alignas(16) EmgParticleData {
	float3 pos;
	float radius;
	float3 speed;
	float gravity;
	AtlasedTexture texCoord;

	SColor color;
	float intensity;
	float creatFrame;
	float destrFrame;
};
static_assert(sizeof(EmgParticleData) % 16 == 0);

class EmgParticleGenerator : public ParticleGenerator<EmgParticleData, EmgParticleGenerator> {
public:
	EmgParticleGenerator();
	~EmgParticleGenerator();
protected:
	bool GenerateGPU() override;
	bool GenerateCPU() override;
};
