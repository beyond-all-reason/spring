#pragma once

#include <tuple>
#include <memory>

#include "Rendering/GL/VBO.h"

#include "EmgParticleGenerator.h"
#include "ExplosiveParticleGenerator.h"

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
		std::unique_ptr<EmgParticleGenerator>,
		std::unique_ptr<ExplosiveParticleGenerator>
	> generators;

	int32_t numQuads;

	VBO vertVBO;
	VBO indcVBO;

	VBO atomicCntVBO;					//for the GPU based updates
};