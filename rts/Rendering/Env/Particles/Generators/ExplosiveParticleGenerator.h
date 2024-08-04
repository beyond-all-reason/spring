#pragma once

#include "ParticleGenerator.h"
#include "../Data/ExplosiveParticleData.h"


class ExplosiveParticleGenerator : public ParticleGenerator<ExplosiveParticleData> {
public:
	ExplosiveParticleGenerator();
	~ExplosiveParticleGenerator();
protected:
	bool GenerateGPU() override;
	bool GenerateCPU() override;
private:
	static constexpr int32_t WORKGROUP_SIZE = 512;
};
