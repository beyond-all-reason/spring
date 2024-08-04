#pragma once

#include "ParticleGenerator.h"
#include "../Data/EmgParticleData.h"


class EmgParticleGenerator : public ParticleGenerator<EmgParticleData> {
public:
	EmgParticleGenerator();
	~EmgParticleGenerator();
protected:
	bool GenerateGPU() override;
	bool GenerateCPU() override;
private:
	static constexpr int32_t WORKGROUP_SIZE = 512;
};
