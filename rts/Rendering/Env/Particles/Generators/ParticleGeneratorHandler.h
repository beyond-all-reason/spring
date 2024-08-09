#pragma once

#include <tuple>
#include <memory>

#include "Rendering/GL/VBO.h"

#include "BeamLaserParticleGenerator.h"
#include "EmgParticleGenerator.h"
#include "ExplosiveParticleGenerator.h"
#include "FireballParticleGenerator.h"
#include "FlameParticleGenerator.h"
#include "LargeBeamLaserParticleGenerator.h"
#include "LaserParticleGenerator.h"
#include "LightningParticleGenerator.h"
#include "MissileParticleGenerator.h"
#include "StarburstParticleGenerator.h"
#include "TorpedoParticleGenerator.h"

class ParticleGeneratorHandler {
public:
	ParticleGeneratorHandler() = default;
	ParticleGeneratorHandler(ParticleGeneratorHandler&&) = delete;
	ParticleGeneratorHandler(const ParticleGeneratorHandler&) = delete;
	ParticleGeneratorHandler& operator=(ParticleGeneratorHandler&&) = delete;
	ParticleGeneratorHandler& operator=(const ParticleGeneratorHandler&) = delete;

	void Init();
	void Kill();
	void GenerateAll();

	template<typename PGT>
	auto& GetGenerator() {
		return *(std::get<std::unique_ptr<PGT>>(generators).get());
	}

	static ParticleGeneratorHandler& GetInstance() {
		static ParticleGeneratorHandler instance;
		return instance;
	}
private:
	std::tuple<
		std::unique_ptr<BeamLaserParticleGenerator>,
		std::unique_ptr<EmgParticleGenerator>,
		std::unique_ptr<ExplosiveParticleGenerator>,
		std::unique_ptr<FireballParticleGenerator>,
		std::unique_ptr<FlameParticleGenerator>,
		std::unique_ptr<LargeBeamLaserParticleGenerator>,
		std::unique_ptr<LaserParticleGenerator>,
		std::unique_ptr<LightningParticleGenerator>,
		std::unique_ptr<MissileParticleGenerator>,
		std::unique_ptr<StarburstParticleGenerator>,
		std::unique_ptr<TorpedoParticleGenerator>
	> generators;

	int32_t numQuads;

	VBO vertVBO;
	VBO indcVBO;

	VBO counterVBO;	//for the GPU based updates
};