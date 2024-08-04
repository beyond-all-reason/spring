#pragma once

#include <vector>

#include <fmt/format.h>

#include "Rendering/GlobalRendering.h"
#include "Rendering/Common/UpdateList.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/GL/VBO.h"
#include "System/Log/ILog.h"
#include "System/TypeToStr.h"
#include "Game/Camera.h"
#include "Sim/Misc/GlobalSynced.h"

// to not repeat in derived classes
#include "Rendering/Textures/TextureAtlas.h"
#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

template<typename ParticleDataType, typename ParticleGenType>
class ParticleGenerator {
public:
	//using DataType = ParticleDataType;
	ParticleGenerator();
	virtual ~ParticleGenerator();

	void Generate();

	size_t Add(const ParticleDataType& data);
	void Update(size_t pos, const ParticleDataType& data);
	ParticleDataType& Update(size_t pos);
	void Del(size_t pos);
	const ParticleDataType& Get(size_t pos) const;
protected:
	void UpdateCommonUniforms() const;

	size_t numQuads = 0;

	virtual bool GenerateGPU() = 0;
	virtual bool GenerateCPU() = 0;

	std::vector<size_t> freeList;
	std::vector<ParticleDataType> particles;
	UpdateList particlesUpdateList;

	Shader::IProgramObject* shader;
	VBO vertVBO;
	VBO indcVBO;

	static constexpr int32_t WORKGROUP_SIZE = 512;

	static constexpr const char* DataTypeName = spring::TypeToCStr<ParticleDataType>();
	static constexpr const char* GenTypeName  = spring::TypeToCStr<ParticleGenType >();
};

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::UpdateCommonUniforms() const
{
	const float3& camPos = camera->GetPos();
	const float frame = gs->frameNum + globalRendering->timeOffset;

	assert(shader->IsBound());

	shader->SetUniform("arraySizes", static_cast<int32_t>(particles.size()), 0, 0);
	shader->SetUniform("currFrame", frame);
	shader->SetUniform("camPos", camPos.x, camPos.y, camPos.z);
	shader->SetUniformMatrix3x3("camView", false, camera->GetViewMatrix().m);
}

template<typename ParticleDataType, typename ParticleGenType>
inline ParticleGenerator<ParticleDataType, ParticleGenType>::ParticleGenerator()
{
	shader = shaderHandler->CreateProgramObject(fmt::format("[{}]", GenTypeName), "Generator");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(fmt::format("GLSL/{}.glsl", GenTypeName), "", GL_COMPUTE_SHADER));

	shader->SetFlag("WORKGROUP_SIZE", WORKGROUP_SIZE);
	shader->SetFlag("FRUSTUM_CULLING", true);

	shader->Link();

	shader->Enable();

	shader->SetUniform("currFrame", 0);
	shader->SetUniform("camPos", 0, 0, 0);
	static constexpr std::array<float, 9> ZERO9 = {0.0f};
	shader->SetUniformMatrix3x3("camView", false, ZERO9.data());

	shader->Disable();

	shader->Validate();
}

template<typename ParticleDataType, typename ParticleGenType>
inline ParticleGenerator<ParticleDataType, ParticleGenType>::~ParticleGenerator()
{
	shaderHandler->ReleaseProgramObjects(fmt::format("[{}]", GenTypeName));
}

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::Generate()
{
	if (particles.empty())
		return;

	if (shader && shader->IsValid() && !GenerateGPU()) {
		if (!GenerateCPU()) {
			LOG_L(L_ERROR, "Failed to run particle generator of type %s", GenTypeName);
		}
	}
}

template<typename ParticleDataType, typename ParticleGenType>
inline size_t ParticleGenerator<ParticleDataType, ParticleGenType>::Add(const ParticleDataType& data)
{
	//numQuads += ParticleDataType::NUM_QUADS;

	if (!freeList.empty()) {
		const size_t pos = freeList.back(); freeList.pop_back();
		particles[pos] = data;
		particlesUpdateList.SetUpdate(pos);
		return pos;
	}

	particles.emplace_back(data);
	particlesUpdateList.EmplaceBackUpdate();
	return particles.size() - 1;
}

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::Update(size_t pos, const ParticleDataType& data)
{
	assert(pos < particles.size());
	particles[pos] = data;
	particlesUpdateList.SetUpdate(pos);
}

template<typename ParticleDataType, typename ParticleGenType>
inline ParticleDataType& ParticleGenerator<ParticleDataType, ParticleGenType>::Update(size_t pos)
{
	assert(pos < particles.size());
	particlesUpdateList.SetUpdate(pos);
	return particles[pos];
}

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::Del(size_t pos)
{
	assert(pos < particles.size());

	//numQuads -= ParticleDataType::NUM_QUADS;

	if (pos == particles.size() - 1) {
		particles.pop_back();
		particlesUpdateList.PopBack();
		return;
	}

	particles[pos].Invalidate();

	freeList.emplace_back(pos);
}

template<typename ParticleDataType, typename ParticleGenType>
inline const ParticleDataType& ParticleGenerator<ParticleDataType, ParticleGenType>::Get(size_t pos) const
{
	assert(pos < particles.size());
	return particles[pos];
}