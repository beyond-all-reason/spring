#include "ParticleGeneratorHandler.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArrayTypes.h"


void ParticleGeneratorHandler::Init()
{
	static auto Initialize = [](auto& gen) {
		using T = std::remove_reference_t<decltype(*gen.get())>;
		gen = std::make_unique<T>();
	};
	
	std::apply([](auto& ... gen) {
		(Initialize(gen), ...);
	}, generators);

	vertVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	indcVBO = VBO{ GL_SHADER_STORAGE_BUFFER };

	atomicCntVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	atomicCntVBO.Bind();
	atomicCntVBO.New(sizeof(uint32_t), GL_STREAM_COPY);
	atomicCntVBO.Unbind();
}

void ParticleGeneratorHandler::Kill()
{
	static auto Nullify = [](auto& gen) {
		gen = nullptr;
	};
	
	std::apply([](auto& ... gens) {
		(Nullify(gens), ...);
	}, generators);

	vertVBO.Release();
	indcVBO.Release();
	atomicCntVBO.Release();
}

void ParticleGeneratorHandler::GenerateAll()
{
	numQuads = std::apply([](auto& ... gen) {
		return (0 + ... + gen->GetNumQuads());
	}, generators);

	if (numQuads <= 0)
		return;

	struct TriangleData
	{
		float4 pos;
		float4 uvw;
		float4 uvInfo;
		float4 apAndCol;
	};

	ParticleGeneratorGlobal::atomicCntVal = { 0 };
	{
		static constexpr uint32_t ZERO = 0u;
		atomicCntVBO.Bind();
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &ZERO);
		atomicCntVBO.Unbind();
	}

	{
		vertVBO.Bind();
		vertVBO.ReallocToFit(4u * numQuads * sizeof(TriangleData)); // TODO replace with VA_TYPE_PROJ
		vertVBO.Unbind();
	}

	{
		indcVBO.Bind();
		indcVBO.ReallocToFit(6u * numQuads * sizeof(uint32_t));
		indcVBO.Unbind();
	}

	vertVBO.BindBufferRange(ParticleGeneratorDefs::VERT_SSBO_BINDING_IDX);
	indcVBO.BindBufferRange(ParticleGeneratorDefs::IDCS_SSBO_BINDING_IDX);
	atomicCntVBO.BindBufferRange(ParticleGeneratorDefs::ATOM_SSBO_BINDING_IDX);
	
	std::apply([](auto& ... gen) {
		(gen->Generate(), ...);
	}, generators);

	vertVBO.UnbindBufferRange(ParticleGeneratorDefs::VERT_SSBO_BINDING_IDX);
	indcVBO.UnbindBufferRange(ParticleGeneratorDefs::IDCS_SSBO_BINDING_IDX);
	atomicCntVBO.UnbindBufferRange(ParticleGeneratorDefs::ATOM_SSBO_BINDING_IDX);

#if 0
	{
		// Debug
		atomicCntVBO.Bind();
		uint32_t val;
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(val), &val);
		atomicCntVBO.Unbind();

		if (val > 0) {
			vertVBO.Bind();
			std::vector<TriangleData> trData(4 * val);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(TriangleData) * trData.size(), trData.data());
			vertVBO.Unbind();

			indcVBO.Bind();
			std::vector<uint32_t> idData(4 * val);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * idData.size(), idData.data());
			indcVBO.Unbind();
		}
	}
#endif
}