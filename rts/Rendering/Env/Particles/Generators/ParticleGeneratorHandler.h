#pragma once

#include <tuple>
#include <memory>

#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"

// Weapon Projectiles
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
// CEG Classes
#include "BitmapMuzzleFlameParticleGenerator.h"
#include "BubbleParticleGenerator.h"

namespace Shader {
	struct IProgramObject;
}

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
	void RenderAll();

	template<typename PGT>
	auto& GetGenerator() {
		return *(std::get<std::unique_ptr<PGT>>(generators).get());
	}

	static ParticleGeneratorHandler& GetInstance() {
		static ParticleGeneratorHandler instance;
		return instance;
	}
private:
	void ReallocateBuffersPre();
	void ReallocateBuffersPost();
private:
	std::tuple<
		// Weapon Projectiles
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
		std::unique_ptr<TorpedoParticleGenerator>,
		// CEG classes
		std::unique_ptr<BitmapMuzzleFlameParticleGenerator>,
		std::unique_ptr<BubbleParticleGenerator>
	> generators;

	int32_t numQuads;
	int32_t sortElemsPerThread;
	int32_t sortHistNumWorkGroups;

	VBO vertVBO;
	VBO indcVBO;
	VBO cntrVBO;	//for the GPU based updates
	VBO indrVBO;
	VBO keysInVBO;
	VBO valsInVBO;
	VBO keysOutVBO;
	VBO valsOutVBO;
	VBO histVBO;

	VAO vao;

	Shader::IProgramObject* indirParamsShader;
	Shader::IProgramObject* keyValShader;
	Shader::IProgramObject* radixHistShader;
	Shader::IProgramObject* radixSortShader;
	Shader::IProgramObject* indcsProdShader;

	static constexpr int32_t RADIX_BIN_BIT_SIZE = 8;
	static constexpr int32_t HIST_BIN_SIZE = (1 << RADIX_BIN_BIT_SIZE);

	static constexpr bool PROCESS_TRIANGLES = true;
	static constexpr bool USE_PROJECTED_DISTANCE = true;
};