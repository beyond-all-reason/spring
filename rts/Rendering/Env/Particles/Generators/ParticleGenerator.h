#pragma once

#include <vector>
#include <atomic>

#include <fmt/format.h>
#include <fmt/printf.h>

#include "Rendering/GlobalRendering.h"
#include "Rendering/Common/UpdateList.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/GL/VBO.h"
#include "System/Log/ILog.h"
#include "System/TypeToStr.h"
#include "System/UnorderedSet.hpp"
#include "System/FileSystem/FileHandler.h"
#include "Game/Camera.h"
#include "lua/LuaParser.h"
#include "Sim/Misc/GlobalSynced.h"

// to not repeat in derived classes
#include "Rendering/Textures/TextureAtlas.h"
#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

struct ParticleGeneratorDefs {
	// bindings 0 and 1 are busy by MATRIX_SSBO_BINDING_IDX and MATUNI_SSBO_BINDING_IDX
	static constexpr int32_t DATA_SSBO_BINDING_IDX = 2;
	static constexpr int32_t VERT_SSBO_BINDING_IDX = 3;
	static constexpr int32_t IDCS_SSBO_BINDING_IDX = 4;
	static constexpr int32_t ATOM_SSBO_BINDING_IDX = 5;

	static constexpr int32_t ATOM_SSBO_NUM_ELEMENTS = 1024;
	static constexpr int32_t ATOM_SSBO_QUAD_IDX = 0;
	static constexpr int32_t ATOM_SSBO_STAT_IDX = 512; // leave some space to avoid false sharing

	static constexpr int32_t WORKGROUP_SIZE = 512;
};

struct ParticleGeneratorGlobal {
	inline static std::atomic_uint32_t atomicCntVal = {0};	//for the CPU based updates
};

template<typename ParticleDataType, typename ParticleGenType>
class ParticleGenerator {
public:
	using MyType = ParticleGenerator<ParticleDataType, ParticleGenType>;

	class UpdateToken {
	public:
		UpdateToken(const UpdateToken& other) {
			ref = other.ref;
			pos = other.pos;

			ref->numQuads -= ref->particles[pos].GetNumQuads();
		}
		UpdateToken(UpdateToken&&) = delete;
		UpdateToken& operator=(UpdateToken&&) = delete;
		UpdateToken& operator=(const UpdateToken&) = delete;

		UpdateToken(MyType* ref_, size_t pos_)
			: ref{ ref_ }
			, pos{ pos_ }
		{
			ref->numQuads -= ref->particles[pos].GetNumQuads();
		}
		~UpdateToken()
		{
			ref->numQuads += ref->particles[pos].GetNumQuads();
		}
	private:
		MyType* ref;
		size_t pos;
	};

	friend class MyType::UpdateToken;

	ParticleGenerator();
	virtual ~ParticleGenerator();

	void Generate();

	size_t Add(const ParticleDataType& data);
	void Update(size_t pos, const ParticleDataType& data);
	void Del(size_t pos);

	const ParticleDataType& Get(size_t pos) const;
	[[nodiscard]] std::pair<UpdateToken, ParticleDataType*> Get(size_t pos);

	int32_t GetNumQuads() const { return numQuads; }
protected:
	static Shader::IProgramObject* GetShader();

	void UpdateBufferData(); //on GPU
	void UpdateCommonUniforms(Shader::IProgramObject* shader) const;
	void RunComputeShader() const;

	virtual bool GenerateCPUImpl() = 0;

	float currFrame;
	int32_t numQuads;

	spring::unordered_set<size_t> freeList;
	std::vector<ParticleDataType> particles;
	UpdateList particlesUpdateList;

	VBO dataVBO;

	static constexpr const char* DataTypeName = spring::TypeToCStr<ParticleDataType>();
	static constexpr const char* GenTypeName  = spring::TypeToCStr<ParticleGenType >();
	static constexpr const char* ProgramClass = "[ParticleGenerator]";
private:
	bool GenerateCPU();
	bool GenerateGPU(Shader::IProgramObject* shader);
};

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::UpdateBufferData()
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (!particlesUpdateList.NeedUpdate())
		return;

	static constexpr auto DataSize = sizeof(decltype(particles)::value_type);
	static_assert(std::is_same_v<decltype(particles)::value_type, ParticleDataType>);

	dataVBO.Bind();
	if (dataVBO.ReallocToFit(DataSize * particles.size())) {
		dataVBO.SetBufferSubData(particles);
	} else {
		for (auto itPair = particlesUpdateList.GetNext(); itPair.has_value(); itPair = particlesUpdateList.GetNext(itPair)) {
			auto offSize = particlesUpdateList.GetOffsetAndSize(itPair.value());
			GLintptr byteOffs = offSize.first  * DataSize;
			GLintptr byteSize = offSize.second * DataSize;
			dataVBO.SetBufferSubData(byteOffs, byteSize, particles.data() + offSize.first/* in elements */);
		}
	}
	dataVBO.Unbind();
	particlesUpdateList.ResetNeedUpdateAll();
}

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::UpdateCommonUniforms(Shader::IProgramObject* shader) const
{
	RECOIL_DETAILED_TRACY_ZONE;

	const float3& camPos = camera->GetPos();

	assert(shader->IsBound());

	shader->SetUniform("arraySizes",
		static_cast<int32_t>(particles.size()),
		numQuads
	);

	shader->SetUniform("currFrame", currFrame);
	shader->SetUniform("camPos", camPos.x, camPos.y, camPos.z);
	shader->SetUniformMatrix4x4("camView", false, camera->GetViewMatrix().m);
}

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::RunComputeShader() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glDispatchCompute((particles.size() + ParticleGeneratorDefs::WORKGROUP_SIZE - 1) / ParticleGeneratorDefs::WORKGROUP_SIZE, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template<typename ParticleDataType, typename ParticleGenType>
inline bool ParticleGenerator<ParticleDataType, ParticleGenType>::GenerateCPU()
{
	RECOIL_DETAILED_TRACY_ZONE;
	return GenerateCPUImpl();
}

template<typename ParticleDataType, typename ParticleGenType>
inline bool ParticleGenerator<ParticleDataType, ParticleGenType>::GenerateGPU(Shader::IProgramObject* shader)
{
	RECOIL_DETAILED_TRACY_ZONE;
	UpdateBufferData();

	dataVBO.BindBufferRange(ParticleGeneratorDefs::DATA_SSBO_BINDING_IDX);
	{
		auto token = shader->EnableScoped();
		UpdateCommonUniforms(shader);
		RunComputeShader();
	}
	dataVBO.UnbindBufferRange(ParticleGeneratorDefs::DATA_SSBO_BINDING_IDX);

	return true;
}

template<typename ParticleDataType, typename ParticleGenType>
inline Shader::IProgramObject* ParticleGenerator<ParticleDataType, ParticleGenType>::GetShader()
{
	auto* shader = shaderHandler->GetProgramObject(ProgramClass, GenTypeName);

	if (shader) {
		if (!shader->IsReloadRequested())
			return shader;
		else {
			shaderHandler->ReleaseProgramObject(ProgramClass, GenTypeName);
			shader = nullptr;
		}
	}

	static constexpr const char* shaderTemplateFilePath = "shaders/GLSL/ParticleGeneratorTemplate.glsl";

	CFileHandler shaderTemplateFile(shaderTemplateFilePath);
	std::string shaderSrc;
	if (shaderTemplateFile.FileExists()) {
		shaderSrc.resize(shaderTemplateFile.FileSize());
		shaderTemplateFile.Read(&shaderSrc[0], shaderTemplateFile.FileSize());
	}
	else {
		LOG_L(L_ERROR, "[%s] file not found \"%s\"", __FUNCTION__, shaderTemplateFilePath);
		return nullptr;
	}

	const std::string particleLuaFilePath = fmt::format("shaders/GLSL/Particles/{}.lua", GenTypeName);
	LuaParser parser(particleLuaFilePath, SPRING_VFS_BASE, SPRING_VFS_BASE, { false }, { false });
	parser.SetupLua(false, false, true);
	if (!parser.Execute()) {
		LOG_L(L_ERROR, "Failed to parse lua particles shader \"%s\": %s", particleLuaFilePath.c_str(), parser.GetErrorLog().c_str());
		return nullptr;
	}
	const LuaTable root = parser.GetRoot();

	const std::string inputData = root.GetString("InputData", "");
	const std::string inputDefs = root.GetString("InputDefs", "");
	const std::string earlyExit = root.GetString("EarlyExit", "");
	const std::string numQuads  = root.GetString("NumQuads" , "");
	const std::string mainCode  = root.GetString("MainCode" , "");

	shaderSrc = fmt::sprintf(shaderSrc,
		inputData,
		inputDefs,
		earlyExit,
		numQuads,
		mainCode
	);

	if (!shader)
		shader = shaderHandler->CreateProgramObject(ProgramClass, GenTypeName);

	shader->AttachShaderObject(shaderHandler->CreateShaderObject(shaderSrc, "", GL_COMPUTE_SHADER));

	shader->SetFlag("WORKGROUP_SIZE", ParticleGeneratorDefs::WORKGROUP_SIZE);
	shader->SetFlag("FRUSTUM_CULLING", false);

	shader->SetFlag("DATA_SSBO_BINDING_IDX", ParticleGeneratorDefs::DATA_SSBO_BINDING_IDX);
	shader->SetFlag("VERT_SSBO_BINDING_IDX", ParticleGeneratorDefs::VERT_SSBO_BINDING_IDX);
	shader->SetFlag("IDCS_SSBO_BINDING_IDX", ParticleGeneratorDefs::IDCS_SSBO_BINDING_IDX);
	shader->SetFlag("ATOM_SSBO_BINDING_IDX", ParticleGeneratorDefs::ATOM_SSBO_BINDING_IDX);
	shader->SetFlag("ATOM_SSBO_QUAD_IDX", ParticleGeneratorDefs::ATOM_SSBO_QUAD_IDX);
	shader->SetFlag("ATOM_SSBO_STAT_IDX", ParticleGeneratorDefs::ATOM_SSBO_STAT_IDX);

	shader->Link();

	shader->Enable();

	shader->SetUniform("currFrame", 0);
	shader->SetUniform("camPos", 0, 0, 0);
	shader->SetUniformMatrix4x4("camView", false, CMatrix44f::Zero().m);

	shader->Disable();

	shader->Validate();

	shader->SetReloadComplete();

	return shader;
}


template<typename ParticleDataType, typename ParticleGenType>
inline ParticleGenerator<ParticleDataType, ParticleGenType>::ParticleGenerator()
	:currFrame{ 0 }
	,numQuads{ 0 }
{
	static_cast<void>(GetShader()); //warm up shader creation
	dataVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
}

template<typename ParticleDataType, typename ParticleGenType>
inline ParticleGenerator<ParticleDataType, ParticleGenType>::~ParticleGenerator()
{
	shaderHandler->ReleaseProgramObject(ProgramClass, GenTypeName);

	dataVBO.Release();
}

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::Generate()
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (particles.empty())
		return;

	currFrame = gs->frameNum + globalRendering->timeOffset;
	auto* shader = GetShader();

	if (shader && shader->IsValid() && !GenerateGPU(shader)) {
		if (!GenerateCPU()) {
			LOG_L(L_ERROR, "Failed to run particle generator of type %s", GenTypeName);
		}
	}
}

template<typename ParticleDataType, typename ParticleGenType>
inline size_t ParticleGenerator<ParticleDataType, ParticleGenType>::Add(const ParticleDataType& data)
{
	RECOIL_DETAILED_TRACY_ZONE;

	numQuads += data.GetNumQuads();

	if (!freeList.empty()) {
		auto it = freeList.begin();
		const size_t pos = *it; freeList.erase(it);
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
	RECOIL_DETAILED_TRACY_ZONE;
	assert(pos < particles.size());

	numQuads += data.GetNumQuads() - particles[pos].GetNumQuads();
	particles[pos] = data;
	particlesUpdateList.SetUpdate(pos);
}

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::Del(size_t pos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(pos < particles.size());

	auto& data = particles[pos];
	numQuads -= data.GetNumQuads();
	data.Invalidate(); // the shader should know what particles to skip

	freeList.emplace(pos);

	// pop as many invalid particles as we can
	for (size_t i = particles.size() - 1; i > 0; --i) {
		auto it = freeList.find(i);
		if (it == freeList.end())
			break;

		particles.pop_back();
		particlesUpdateList.PopBack();
		freeList.erase(it);
	}
}

template<typename ParticleDataType, typename ParticleGenType>
inline std::pair<typename ParticleGenerator<ParticleDataType, ParticleGenType>::UpdateToken, ParticleDataType*> ParticleGenerator<ParticleDataType, ParticleGenType>::Get(size_t pos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(pos < particles.size());

	particlesUpdateList.SetUpdate(pos);
	return std::pair(UpdateToken(this, pos), &particles[pos]);
}

template<typename ParticleDataType, typename ParticleGenType>
inline const ParticleDataType& ParticleGenerator<ParticleDataType, ParticleGenType>::Get(size_t pos) const
{
	assert(pos < particles.size());
	return particles[pos];
}