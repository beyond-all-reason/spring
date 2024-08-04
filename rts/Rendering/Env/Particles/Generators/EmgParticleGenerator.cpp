#include "EmgParticleGenerator.h"

#include "Rendering/GL/myGL.h"

#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/SpringMath.h"

EmgParticleGenerator::EmgParticleGenerator()
{

}

EmgParticleGenerator::~EmgParticleGenerator()
{

}

bool EmgParticleGenerator::GenerateGPU()
{
	auto token = shader->EnableScoped();

	UpdateCommonUniforms();

	glDispatchCompute((particles.size() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);

	shader->Disable();

	return true;
}

bool EmgParticleGenerator::GenerateCPU()
{
	return true;
}
