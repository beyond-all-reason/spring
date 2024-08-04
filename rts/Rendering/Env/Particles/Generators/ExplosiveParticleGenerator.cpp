#include "ExplosiveParticleGenerator.h"

#include "Rendering/GL/myGL.h"

#include "Rendering/Shaders/ShaderHandler.h"
#include "Game/Camera.h"

ExplosiveParticleGenerator::ExplosiveParticleGenerator()
{

}

ExplosiveParticleGenerator::~ExplosiveParticleGenerator()
{

}

bool ExplosiveParticleGenerator::GenerateGPU()
{
	auto token = shader->EnableScoped();

	UpdateCommonUniforms();

	glDispatchCompute((particles.size() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);

	return true;
}

bool ExplosiveParticleGenerator::GenerateCPU()
{
	return true;
}
