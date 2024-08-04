#pragma once

#include <tuple>
#include <memory>

#include "EmgParticleGenerator.h"
#include "ExplosiveParticleGenerator.h"

class ParticleGeneratorHandler {
public:
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
};