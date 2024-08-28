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
#include "DirtParticleGenerator.h"
#include "ExploSpikeParticleGenerator.h"
#include "GeoSquareParticleGenerator.h"
#include "HeatCloudParticleGenerator.h"
#include "MuzzleFlameParticleGenerator.h"
#include "NanoParticleGenerator.h"
#include "SimpleParticleGenerator.h"

namespace Shader {
	struct IProgramObject;
}

class ParticleGeneratorHandler {
public:
	struct ParticleGeneratorStats {
		uint32_t totalQuads;
		uint32_t totalElems;
		uint32_t culledQuads;
		uint32_t oobQuads;
	};
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

	const auto& GetStats() const { return pgs; }

	template<typename PGT>
	auto& GetGenerator() {
		return std::get<PGT>(*generators);
	}
	template<typename PGT>
	const auto& GetGenerator() const {
		return std::get<PGT>(*generators);
	}

	static ParticleGeneratorHandler& GetInstance() {
		static ParticleGeneratorHandler instance;
		return instance;
	}

	bool EnableSorting(bool b) { return (SORT_PARTICLES =               b); }
	bool ToggleSorting(      ) { return (SORT_PARTICLES = !SORT_PARTICLES); }
	bool EnableTriangles(bool b) { return (PROCESS_TRIANGLES =                  b); }
	bool ToggleTriangles(      ) { return (PROCESS_TRIANGLES = !PROCESS_TRIANGLES); }
	bool EnableProjDistance(bool b) { return (USE_PROJECTED_DISTANCE =                       b); }
	bool ToggleProjDistance(      ) { return (USE_PROJECTED_DISTANCE = !USE_PROJECTED_DISTANCE); }
private:
	void ReallocateBuffersPre();
	void ReallocateBuffersPost();
	void AsyncUpdateStatistics();
private:
	using GeneratorsTuple = std::tuple<
		// Weapon Projectiles
		BeamLaserParticleGenerator,
		EmgParticleGenerator,
		ExplosiveParticleGenerator,
		FireballParticleGenerator,
		FlameParticleGenerator,
		LargeBeamLaserParticleGenerator,
		LaserParticleGenerator,
		LightningParticleGenerator,
		MissileParticleGenerator,
		StarburstParticleGenerator,
		TorpedoParticleGenerator,
		// CEG classes
		BitmapMuzzleFlameParticleGenerator,
		BubbleParticleGenerator,
		DirtParticleGenerator,
		ExploSpikeParticleGenerator,
		GeoSquareParticleGenerator,
		HeatCloudParticleGenerator,
		MuzzleFlameParticleGenerator,
		NanoParticleGenerator,
		// skipped shields and repulsor
		SimpleParticleGenerator
	>;

	std::unique_ptr<GeneratorsTuple> generators;

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

	VBO statVBO;

	VAO vao;

	GLsync statSync{};

	ParticleGeneratorStats pgs;

	Shader::IProgramObject* indirParamsShader;
	Shader::IProgramObject* keyValShader;
	Shader::IProgramObject* radixHistShader;
	Shader::IProgramObject* radixSortShader;
	Shader::IProgramObject* indcsProdShader;

	static constexpr int32_t RADIX_BIN_BIT_SIZE = 8;
	static constexpr int32_t HIST_BIN_SIZE = (1 << RADIX_BIN_BIT_SIZE);

	bool SORT_PARTICLES = true;
	bool PROCESS_TRIANGLES = false;
	bool USE_PROJECTED_DISTANCE = false;
};