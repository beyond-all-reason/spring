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
#include "System/StringUtil.h"
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
#include "System/creg/creg_cond.h"

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

	ParticleGenerator();
	~ParticleGenerator();

	void Generate(int32_t totalNumQuads);

	size_t Add(const ParticleDataType& data = {});
	void Update(size_t pos, const ParticleDataType& data);
	void Del(size_t pos);

	const ParticleDataType& Get(size_t pos) const;
	      ParticleDataType& Get(size_t pos);

	void SetMaxNumQuads();
	int32_t GetMaxNumQuads() const { return maxNumQuads; }
protected:
	static Shader::IProgramObject* GetShader();

	void UpdateBufferData(); //on GPU
	void UpdateCommonUniforms(Shader::IProgramObject* shader, int32_t totalNumQuads) const;
	void RunComputeShader() const;

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

	float currFrame = 0.0f;
	float prevFrame = 0.0f;
	int32_t maxNumQuads;
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

	assert(shader->IsBound());

	shader->SetUniform("arraySizes",
		static_cast<int32_t>(particles.size()),
		totalNumQuads
	);

	prevFrame = std::exchange(currFrame, static_cast<float>(gs->frameNum));

	shader->SetUniform("frameInfo", currFrame, globalRendering->timeOffset, gu->modGameTime, prevFrame);

	shader->SetUniform3v("camPos", &camera->GetPos().x);
	shader->SetUniform3v("camDir[0]", &camera->GetRight().x);
	shader->SetUniform3v("camDir[1]", &camera->GetUp().x);
	shader->SetUniform3v("camDir[2]", &camera->GetForward().x);
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
	return static_cast<ParticleGenType*>(this)->GenerateCPUImpl();
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

	std::ostringstream dataStructSS;
	std::ostringstream dataToFieldsSS;
	std::ostringstream earlyExitAutoSS;
	{
		std::vector<std::string> allAtlassedTextures;

		constexpr auto VEC4_BYTE_SIZE = 4 * sizeof(float);
		const auto* cregClass = ParticleDataType::StaticClass();
		assert(cregClass->size % VEC4_BYTE_SIZE == 0);
		const uint32_t numVec4 = cregClass->size / VEC4_BYTE_SIZE;
		std::map<uint32_t, std::tuple<std::string, std::string, uint32_t>> membersMap;
		for (const auto& member : cregClass->members) {
			membersMap[member.offset] = std::make_tuple(std::string{ member.name }, member.type->GetName(), member.type->GetSize());
		}

		dataStructSS << "struct InputData {\n";
		dataStructSS << "\tvec4 info[" << numVec4 << "];\n";
		dataStructSS << "};";

		static constexpr const char* TABS = "\t";
		for (const auto& [offt, fieldInfo] : membersMap) {
			const auto& name = std::get<0>(fieldInfo);
			auto typeName = std::get<1>(fieldInfo);
			auto size = std::get<2>(fieldInfo);

			size_t arraySize = 1;

			if (name.rfind("unused") != std::string::npos)
				continue;

			if (typeName.rfind("ignored") != std::string::npos)
				continue;

			if (auto ps = typeName.rfind("["); ps != std::string::npos) {
				auto pe = typeName.rfind("]");
				bool failed;
				arraySize = StringToInt<size_t>(typeName.substr(ps + 1, pe - ps - 1), &failed);
				assert(!failed);
				typeName = typeName.substr(0, ps);
				size /= arraySize; // will account for that later
			}

			std::string glslType;
			std::string reinterpretString;
			std::string reinterpretString2;

			switch (hashString(typeName))
			{
			case hashString("float4"): {
				glslType = "vec4";
			} break;
			case hashString("float3"): {
				glslType = "vec3";
			} break;
			case hashString("float2"): {
				glslType = "vec2";
			} break;
			case hashString("int"): {
				reinterpretString = "floatBitsToInt";
				glslType = "int";
			} break;
			case hashString("uint"): {
				reinterpretString = "floatBitsToUint";
				glslType = "uint";
			} break;
			case hashString("float"): {
				glslType = "float";
			} break;
			case hashString("SColor"): {
				glslType = "vec4";
				reinterpretString = "floatBitsToUint";
				reinterpretString2 = "GetPackedColor";
			} break;
			case hashString("AtlasedTexture"): {
				glslType = "vec4";
				allAtlassedTextures.emplace_back(name);
			} break;
			default:
				assert(false);
				break;
			}

			if (arraySize > 1) {
				dataToFieldsSS << TABS << glslType << " " << name << "[" << arraySize << "];\n";
			}

			assert(size <= VEC4_BYTE_SIZE);

			for (int ai = 0; ai < arraySize; ++ai) {
				uint32_t localOffset = (offt + ai * size);
				uint32_t infoIndex = (localOffset / VEC4_BYTE_SIZE);
				uint32_t vec4Index = (localOffset % VEC4_BYTE_SIZE) / sizeof(float);

				std::ostringstream infoTxtSS;
				if (!reinterpretString2.empty())
					infoTxtSS << reinterpretString2 << "(";
				if (!reinterpretString.empty())
					infoTxtSS << reinterpretString << "(";

				assert(vec4Index + (size / 4) <= 4);

				static constexpr const char* SWIZZLE = "xyzw";
				infoTxtSS << fmt::format("dataIn[gl_GlobalInvocationID.x].info[{}]", infoIndex);
				if (size != VEC4_BYTE_SIZE) {
					infoTxtSS << ".";
					for (int sr = 0; sr < (size / 4); ++sr) {
						infoTxtSS << SWIZZLE[vec4Index + sr];
					}
				}

				if (!reinterpretString.empty())
					infoTxtSS << ")";
				if (!reinterpretString2.empty())
					infoTxtSS << ")";

				if (arraySize == 1) {
					dataToFieldsSS << TABS << glslType << " " << name << " = " << infoTxtSS.str() << ";\n";
				}
				else {
					dataToFieldsSS << TABS << name << "[" << ai << "]" << " = " << infoTxtSS.str() << ";\n";
				}
			}
		}

		const std::string boolType = (allAtlassedTextures.size() == 1) ? "bool" : fmt::format("bvec{}", allAtlassedTextures.size());
		static constexpr const char* VT_FORMAT_COM = "({}.z - {}.x) * ({}.w - {}.y) > 0.0,\n";
		static constexpr const char* VT_FORMAT_END = "({}.z - {}.x) * ({}.w - {}.y) > 0.0\n";
		earlyExitAutoSS << TABS << boolType << " validTextures = " << boolType << "(\n";
		for (size_t ati = 0; ati < allAtlassedTextures.size(); ++ati) {
			const auto& at = allAtlassedTextures[ati];
			if (ati == allAtlassedTextures.size() - 1)
				earlyExitAutoSS << TABS << "\t" << fmt::format(VT_FORMAT_END, at, at, at, at);
			else
				earlyExitAutoSS << TABS << "\t" << fmt::format(VT_FORMAT_COM, at, at, at, at);
		}
		earlyExitAutoSS << TABS << ");\n\n";
		earlyExitAutoSS << TABS << "bool anyValidTextures = " << ((allAtlassedTextures.size() == 1) ?
			"validTextures;\n" :
			"any(validTextures);\n");

		earlyExitAutoSS << TABS << "if (!anyValidTextures)\n";
		earlyExitAutoSS << TABS << "\treturn;\n";
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

	const std::string earlyExit = root.GetString("EarlyExit", "");
	const std::string mainCode  = root.GetString("MainCode" , "");

	shaderSrc = fmt::sprintf(shaderSrc,
		dataStructSS.str(),
		dataToFieldsSS.str(),
		earlyExitAutoSS.str(),
		earlyExit,
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
	shader->SetUniform("frameInfo", 0.0f, 0.0f, 0.0f, 0.0f);
	shader->SetUniform("camPos", 0.0f, 0.0f, 0.0f);
	shader->SetUniform("camDir[0]", 0.0f, 0.0f, 0.0f);
	shader->SetUniform("camDir[1]", 0.0f, 0.0f, 0.0f);
	shader->SetUniform("camDir[2]", 0.0f, 0.0f, 0.0f);
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

	particles[pos] = data;
	particlesUpdateList.SetUpdate(pos);
}

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::Del(size_t pos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(pos < particles.size());

	auto& data = particles[pos];
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
inline ParticleDataType& ParticleGenerator<ParticleDataType, ParticleGenType>::Get(size_t pos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(pos < particles.size());

	particlesUpdateList.SetUpdate(pos);
	return particles[pos];
}

template<typename ParticleDataType, typename ParticleGenType>
inline void ParticleGenerator<ParticleDataType, ParticleGenType>::SetMaxNumQuads()
{
	maxNumQuads = std::accumulate(particles.begin(), particles.end(), 0, [](int32_t prev, const auto& p) { return prev + p.GetMaxNumQuads(); });
}

template<typename ParticleDataType, typename ParticleGenType>
inline const ParticleDataType& ParticleGenerator<ParticleDataType, ParticleGenType>::Get(size_t pos) const
{
	assert(pos < particles.size());
	return particles[pos];
}