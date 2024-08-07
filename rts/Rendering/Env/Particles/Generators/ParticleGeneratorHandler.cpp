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

	counterVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	counterVBO.Bind();
	counterVBO.New(ParticleGeneratorDefs::ATOM_SSBO_NUM_ELEMENTS * sizeof(uint32_t), GL_STREAM_COPY);
	counterVBO.Unbind();
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
	counterVBO.Release();
}

void ParticleGeneratorHandler::GenerateAll()
{
	SCOPED_TIMER("ParticleGeneratorHandler::GenerateAll");

	numQuads = std::apply([](auto& ... gen) {
		return (0 + ... + gen->GetMaxNumQuads());
	}, generators);

	if (numQuads <= 0)
		return;

	ParticleGeneratorGlobal::atomicCntVal = { 0 };
	{
		static constexpr uint32_t ZERO = 0u;
		counterVBO.Bind();
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &ZERO);
		counterVBO.Unbind();
	}

	{
		vertVBO.Bind();
		vertVBO.ReallocToFit(4u * numQuads * sizeof(VA_TYPE_PROJ));
		vertVBO.Unbind();
	}

	{
		indcVBO.Bind();
		indcVBO.ReallocToFit(6u * numQuads * sizeof(uint32_t));
		indcVBO.Unbind();
	}

	vertVBO.BindBufferRange(ParticleGeneratorDefs::VERT_SSBO_BINDING_IDX);
	indcVBO.BindBufferRange(ParticleGeneratorDefs::IDCS_SSBO_BINDING_IDX);
	counterVBO.BindBufferRange(ParticleGeneratorDefs::ATOM_SSBO_BINDING_IDX);
	
	std::apply([this](auto& ... gen) {
		(gen->Generate(numQuads), ...);
	}, generators);

	vertVBO.UnbindBufferRange(ParticleGeneratorDefs::VERT_SSBO_BINDING_IDX);
	indcVBO.UnbindBufferRange(ParticleGeneratorDefs::IDCS_SSBO_BINDING_IDX);
	counterVBO.UnbindBufferRange(ParticleGeneratorDefs::ATOM_SSBO_BINDING_IDX);

#if 0
	{
		// Debug
		counterVBO.Bind();
		std::array<uint32_t, ParticleGeneratorDefs::ATOM_SSBO_NUM_ELEMENTS> vals;
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * vals.size(), vals.data());
		counterVBO.Unbind();

		if (vals[0] > 0) {
			vertVBO.Bind();
			std::vector<VA_TYPE_PROJ> trData(6 * vals[0]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(VA_TYPE_PROJ) * trData.size(), trData.data());
			vertVBO.Unbind();

			indcVBO.Bind();
			std::vector<uint32_t> idData(4 * vals[0]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * idData.size(), idData.data());
			indcVBO.Unbind();
		}
	}
#endif
}