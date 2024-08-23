#pragma once

#include <vector>
#include <atomic>
#include <type_traits>

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
#include "Game/GlobalUnsynced.h"
#include "lua/LuaParser.h"
#include "Sim/Misc/GlobalSynced.h"

// to not repeat in derived classes
#include "Rendering/Textures/TextureAtlas.h"
#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

struct ParticleGeneratorDefs {
	// bindings 0 and 1 are occupied by MATRIX_SSBO_BINDING_IDX and MATUNI_SSBO_BINDING_IDX
	// for particles generator
	static constexpr int32_t DATA_SSBO_BINDING_IDX = 2;
	static constexpr int32_t VERT_SSBO_BINDING_IDX = 3;
	static constexpr int32_t SIZE_SSBO_BINDING_IDX = 5;

	static constexpr int32_t SIZE_SSBO_NUM_ELEMENTS = 256;

	// spreaded by 32 to reduce false sharing
	static constexpr int32_t SIZE_SSBO_NUM_ELEM = 00; // number of elements (filled out later)
	static constexpr int32_t SIZE_SSBO_QUAD_IDX = 32; // number of produced quads
	static constexpr int32_t SIZE_SSBO_OOBC_IDX = 64; // out of bounds write attempts
	static constexpr int32_t SIZE_SSBO_CULL_IDX = 96; // number of culled quads

	static constexpr int32_t WORKGROUP_SIZE = 512;
};

struct ParticleGeneratorGlobal {
	inline static std::atomic_uint32_t atomicCntVal = {0};	//for the CPU based updates
};

template<typename ParticleDataType, typename ParticleGenType>
class ParticleGenerator {
public:
	using MyType = ParticleGenerator<ParticleDataType, ParticleGenType>;

	// some protection from silly typos
	static_assert(std::is_member_function_pointer_v<decltype(&ParticleDataType::Invalidate)>);
	static_assert(std::is_member_function_pointer_v<decltype(&ParticleDataType::GetMaxNumQuads)>);
	static_assert(!std::is_same_v<ParticleDataType, ParticleGenType>);

	class UpdateToken {
	public:
		UpdateToken(const UpdateToken& other) {
			ref = other.ref;
			pos = other.pos;

			ref->numQuads -= ref->particles[pos].GetMaxNumQuads();
		}
		UpdateToken(UpdateToken&&) = delete;
		UpdateToken& operator=(UpdateToken&&) = delete;
		UpdateToken& operator=(const UpdateToken&) = delete;

		UpdateToken(MyType* ref_, size_t pos_)
			: ref{ ref_ }
			, pos{ pos_ }
		{
			ref->numQuads -= ref->particles[pos].GetMaxNumQuads();
		}
		~UpdateToken()
		{
			ref->numQuads += ref->particles[pos].GetMaxNumQuads();
		}
		operator size_t() const { return pos; }
	private:
		MyType* ref;
		size_t pos;
	};

	friend class MyType::UpdateToken;

	ParticleGenerator();
	virtual ~ParticleGenerator();

	void Generate(int32_t totalNumQuads);

	size_t Add(const ParticleDataType& data = {});
	void Update(size_t pos, const ParticleDataType& data);
	void Del(size_t pos);

	const ParticleDataType& Get(size_t pos) const;
	[[nodiscard]] std::pair<UpdateToken, ParticleDataType*> Get(size_t pos);

	int32_t GetMaxNumQuads() const { return numQuads; }
protected:
	static Shader::IProgramObject* GetShader();

	void UpdateBufferData(); //on GPU
	void UpdateCommonUniforms(Shader::IProgramObject* shader, int32_t totalNumQuads) const;
	void RunComputeShader() const;

	virtual bool GenerateCPUImpl() = 0;

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
	bool GenerateGPU(Shader::IProgramObject* shader, int32_t totalNumQuads);
};

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::UpdateBufferData()
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (!particlesUpdateList.NeedUpdate())
		return;

	using DeductedParticleDataType = typename decltype(particles)::value_type;
	static constexpr auto DataSize = sizeof(DeductedParticleDataType);
	static_assert(std::is_same_v<DeductedParticleDataType, ParticleDataType>);

	dataVBO.Bind();
	if (dataVBO.ReallocToFit(DataSize * particles.size())) {
		dataVBO.SetBufferSubData(particles);
	} else {
		for (auto itPair = particlesUpdateList.GetNext(); itPair.has_value(); itPair = particlesUpdateList.GetNext(itPair)) {
			auto [offt, size] = particlesUpdateList.GetOffsetAndSize(itPair.value());
			GLintptr byteOffs = offt * DataSize;
			GLintptr byteSize = size * DataSize;
			dataVBO.SetBufferSubData(byteOffs, byteSize, particles.data() + offt/* in elements */);
		}
	}
	dataVBO.Unbind();
	particlesUpdateList.ResetNeedUpdateAll();
}

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::UpdateCommonUniforms(Shader::IProgramObject* shader, int32_t totalNumQuads) const
{
	RECOIL_DETAILED_TRACY_ZONE;

	const float3& camPos = camera->GetPos();

	assert(shader->IsBound());

	shader->SetUniform("arraySizes",
		static_cast<int32_t>(particles.size()),
		totalNumQuads
	);

	shader->SetUniform("frameInfo", static_cast<float>(gs->frameNum), globalRendering->timeOffset, gu->modGameTime);
	shader->SetUniformMatrix4x4("camDirPos", false, CMatrix44f{ camera->GetPos(), camera->GetRight(), camera->GetUp(), camera->GetForward() }.m);
	shader->SetUniform4v("frustumPlanes[0]", &camera->GetFrustumPlane(0).x);
	shader->SetUniform4v("frustumPlanes[1]", &camera->GetFrustumPlane(1).x);
	shader->SetUniform4v("frustumPlanes[2]", &camera->GetFrustumPlane(2).x);
	shader->SetUniform4v("frustumPlanes[3]", &camera->GetFrustumPlane(3).x);
	shader->SetUniform4v("frustumPlanes[4]", &camera->GetFrustumPlane(4).x);
	shader->SetUniform4v("frustumPlanes[5]", &camera->GetFrustumPlane(5).x);
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
inline bool ParticleGenerator<ParticleDataType, ParticleGenType>::GenerateGPU(Shader::IProgramObject* shader, int32_t totalNumQuads)
{
	RECOIL_DETAILED_TRACY_ZONE;
	UpdateBufferData();

	dataVBO.BindBufferRange(ParticleGeneratorDefs::DATA_SSBO_BINDING_IDX);
	{
		auto token = shader->EnableScoped();
		UpdateCommonUniforms(shader, totalNumQuads);
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
	      std::string numQuads  = root.GetString("NumQuads" , ""); numQuads.erase(std::remove(numQuads.begin(), numQuads.end(), '\n'), numQuads.cend());
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
	shader->SetFlag("FRUSTUM_CULLING", true);

	shader->SetFlag("DATA_SSBO_BINDING_IDX", ParticleGeneratorDefs::DATA_SSBO_BINDING_IDX);
	shader->SetFlag("VERT_SSBO_BINDING_IDX", ParticleGeneratorDefs::VERT_SSBO_BINDING_IDX);
	//shader->SetFlag("IDCS_SSBO_BINDING_IDX", ParticleGeneratorDefs::IDCS_SSBO_BINDING_IDX);
	shader->SetFlag("SIZE_SSBO_BINDING_IDX", ParticleGeneratorDefs::SIZE_SSBO_BINDING_IDX);
	shader->SetFlag("SIZE_SSBO_QUAD_IDX", ParticleGeneratorDefs::SIZE_SSBO_QUAD_IDX);
	shader->SetFlag("SIZE_SSBO_OOBC_IDX", ParticleGeneratorDefs::SIZE_SSBO_OOBC_IDX);
	shader->SetFlag("SIZE_SSBO_CULL_IDX", ParticleGeneratorDefs::SIZE_SSBO_CULL_IDX);

	shader->Link();

	shader->Enable();

	shader->SetUniform("arraySizes", 0, 0);
	shader->SetUniform("frameInfo", 0.0f, 0.0f, 0.0f);
	shader->SetUniform("camPos", 0.0f, 0.0f, 0.0f);
	shader->SetUniformMatrix4x4("camView", false, CMatrix44f::Zero().m);
	static constexpr float4 ZERO4 = float4{ 0.0f };
	shader->SetUniform4v("frustumPlanes[0]", &ZERO4.x);
	shader->SetUniform4v("frustumPlanes[1]", &ZERO4.x);
	shader->SetUniform4v("frustumPlanes[2]", &ZERO4.x);
	shader->SetUniform4v("frustumPlanes[3]", &ZERO4.x);
	shader->SetUniform4v("frustumPlanes[4]", &ZERO4.x);
	shader->SetUniform4v("frustumPlanes[5]", &ZERO4.x);


	shader->Disable();

	shader->Validate();

	shader->SetReloadComplete();

	return shader;
}


template<typename ParticleDataType, typename ParticleGenType>
inline ParticleGenerator<ParticleDataType, ParticleGenType>::ParticleGenerator()
	: numQuads{ 0 }
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
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::Generate(int32_t totalNumQuads)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (particles.empty())
		return;

	auto* shader = GetShader();

	if (shader && shader->IsValid() && !GenerateGPU(shader, totalNumQuads)) {
		if (!GenerateCPU()) {
			LOG_L(L_ERROR, "Failed to run particle generator of type %s", GenTypeName);
		}
	}
}

template<typename ParticleDataType, typename ParticleGenType>
inline size_t ParticleGenerator<ParticleDataType, ParticleGenType>::Add(const ParticleDataType& data)
{
	RECOIL_DETAILED_TRACY_ZONE;

	numQuads += data.GetMaxNumQuads();

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
	numQuads -= data.GetMaxNumQuads();
	data.Invalidate(); // the shader should know what particles to skip
	particlesUpdateList.SetUpdate(pos);

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