#include "EmgParticleGenerator.h"

#include "Rendering/GL/myGL.h"

#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/SpringMath.h"
#include "Game/Camera.h"

EmgParticleGenerator::EmgParticleGenerator()
{
	shader = shaderHandler->CreateProgramObject("[EmgParticleGenerator]", "Generator");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/EmgParticleGenerator.glsl", "", GL_COMPUTE_SHADER));

	shader->SetFlag("WORKGROUP_SIZE", WORKGROUP_SIZE);

	shader->Link();

	const float3& camPos = camera->GetPos();
	const float frame = gs->frameNum + globalRendering->timeOffset;

	shader->Enable();

	shader->SetUniform("frameData", prevFrame, currFrame);
	shader->SetUniform("camPos", camPos.x, camPos.y, camPos.z);
	shader->SetUniformMatrix3x3("camView", false, camera->GetViewMatrix().m);

	shader->Disable();

	shader->Validate();
}

EmgParticleGenerator::~EmgParticleGenerator()
{
	shaderHandler->ReleaseProgramObjects("[EmgParticleGenerator]");
}

bool EmgParticleGenerator::GenerateGPU()
{
	const float3& camPos = camera->GetPos();
	prevFrame = std::exchange(currFrame, gs->frameNum + globalRendering->timeOffset);

	shader->Enable();	

	shader->SetUniform("arraySizes", static_cast<int32_t>(particles.size()), 0, 0);
	shader->SetUniform("frameData", prevFrame, currFrame);
	shader->SetUniform("camPos", camPos.x, camPos.y, camPos.z);
	shader->SetUniformMatrix3x3("camView", false, camera->GetViewMatrix().m);

	glDispatchCompute((particles.size() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);

	shader->Disable();

	return true;
}

bool EmgParticleGenerator::GenerateCPU()
{
	return true;
}
