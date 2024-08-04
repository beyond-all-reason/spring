#pragma once

#include <vector>

#include "Rendering/Common/UpdateList.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GL/VBO.h"
#include "System/Log/ILog.h"
#include "System/TypeToStr.h"

template<typename ParticleDataType>
class ParticleGenerator {
public:
	//using DataType = ParticleDataType;
	ParticleGenerator() = default;
	virtual ~ParticleGenerator() {}

	void Generate();

	size_t Add(const ParticleDataType& data);
	void Update(size_t pos, const ParticleDataType& data);
	ParticleDataType& Update(size_t pos);
	void Del(size_t pos);
	const ParticleDataType& Get(size_t pos) const;
protected:
	float prevFrame;
	float currFrame;

	size_t numQuads = 0;

	virtual bool GenerateGPU() = 0;
	virtual bool GenerateCPU() = 0;

	std::vector<size_t> freeList;
	std::vector<ParticleDataType> particles;
	UpdateList particlesUpdateList;

	Shader::IProgramObject* shader;
	VBO vertVBO;
	VBO indcVBO;
};

template<typename ParticleDataType>
inline void ParticleGenerator<ParticleDataType>::Generate()
{
	if (particles.empty())
		return;

	if (shader && shader->IsValid() && !GenerateGPU()) {
		if (!GenerateCPU()) {
			LOG_L(L_ERROR, "Failed to run particle generator of type %s", spring::TypeToCStr<ParticleDataType>());
		}
	}
}

template<typename ParticleDataType>
inline size_t ParticleGenerator<ParticleDataType>::Add(const ParticleDataType& data)
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

template<typename ParticleDataType>
inline void ParticleGenerator<ParticleDataType>::Update(size_t pos, const ParticleDataType& data)
{
	assert(pos < particles.size());
	particles[pos] = data;
	particlesUpdateList.SetUpdate(pos);
}

template<typename ParticleDataType>
inline ParticleDataType& ParticleGenerator<ParticleDataType>::Update(size_t pos)
{
	assert(pos < particles.size());
	particlesUpdateList.SetUpdate(pos);
	return particles[pos];
}

template<typename ParticleDataType>
inline void ParticleGenerator<ParticleDataType>::Del(size_t pos)
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

template<typename ParticleDataType>
inline const ParticleDataType& ParticleGenerator<ParticleDataType>::Get(size_t pos) const
{
	assert(pos < particles.size());
	return particles[pos];
}