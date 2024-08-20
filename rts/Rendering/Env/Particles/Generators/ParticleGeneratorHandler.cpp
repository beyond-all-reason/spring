#include "ParticleGeneratorHandler.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArrayTypes.h"

struct IndirectBufferIndices {
	static constexpr int32_t INDR_SSBO_BINDING_IDX = 2;
	static constexpr int32_t SIZE_SSBO_BINDING_IDX = 3;

	static constexpr int32_t KVAL_SSBO_INDRCT_X = 0;
	static constexpr int32_t KVAL_SSBO_INDRCT_Y = 1;
	static constexpr int32_t KVAL_SSBO_INDRCT_Z = 2;

	static constexpr int32_t HIST_SSBO_INDRCT_X = 3;
	static constexpr int32_t HIST_SSBO_INDRCT_Y = 4;
	static constexpr int32_t HIST_SSBO_INDRCT_Z = 5;

	static constexpr int32_t DRAW_SSBO_INDSC = 6;
	static constexpr int32_t DRAW_SSBO_INSTC = 7;
	static constexpr int32_t DRAW_SSBO_FIRSI = 8;
	static constexpr int32_t DRAW_SSBO_BASEV = 9;
	static constexpr int32_t DRAW_SSBO_BASEI = 10;

	static constexpr int32_t INDR_SSBO_ELEM_SIZE = 11;
};

struct KeyValStorageBindings {
	// bindings 0 and 1 are occupied by MATRIX_SSBO_BINDING_IDX and MATUNI_SSBO_BINDING_IDX
	static constexpr int32_t VERT_SSBO_BINDING_IDX = 2;
	static constexpr int32_t SIZE_SSBO_BINDING_IDX = 3;
	static constexpr int32_t KEYO_SSBO_BINDING_IDX = 4;
	static constexpr int32_t VALO_SSBO_BINDING_IDX = 5;
};

struct RadixSortStorageBindings {
	// bindings 0 and 1 are occupied by MATRIX_SSBO_BINDING_IDX and MATUNI_SSBO_BINDING_IDX
	static constexpr int32_t HIST_SSBO_BINDING_IDX = 2;
	static constexpr int32_t SIZE_SSBO_BINDING_IDX = 3;
	static constexpr int32_t KEYI_SSBO_BINDING_IDX = 4;
	static constexpr int32_t VALI_SSBO_BINDING_IDX = 5;
	static constexpr int32_t KEYO_SSBO_BINDING_IDX = 6;
	static constexpr int32_t VALO_SSBO_BINDING_IDX = 7;
};

struct IndicesProducerStorageBindings {
	// bindings 0 and 1 are occupied by MATRIX_SSBO_BINDING_IDX and MATUNI_SSBO_BINDING_IDX
	static constexpr int32_t VALI_SSBO_BINDING_IDX = 3;
	static constexpr int32_t IDCS_SSBO_BINDING_IDX = 4;
	static constexpr int32_t SIZE_SSBO_BINDING_IDX = 5;
};

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
	vertVBO.SetUsage(GL_STREAM_COPY);

	indcVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	indcVBO.SetUsage(GL_STREAM_COPY);

	indrVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	indrVBO.Bind();
	indrVBO.New(IndirectBufferIndices::INDR_SSBO_ELEM_SIZE * sizeof(uint32_t), GL_STREAM_COPY);
	indrVBO.Unbind();

	cntrVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	cntrVBO.Bind();
	cntrVBO.New(ParticleGeneratorDefs::SIZE_SSBO_NUM_ELEMENTS * sizeof(uint32_t), GL_STREAM_COPY);
	cntrVBO.Unbind();

	histVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	histVBO.SetUsage(GL_STREAM_COPY);

	keysInVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	keysInVBO.SetUsage(GL_STREAM_COPY);

	valsInVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	valsInVBO.SetUsage(GL_STREAM_COPY);

	keysOutVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	keysOutVBO.SetUsage(GL_STREAM_COPY);

	valsOutVBO = VBO{ GL_SHADER_STORAGE_BUFFER };
	valsOutVBO.SetUsage(GL_STREAM_COPY);

	assert(ParticleGeneratorDefs::WORKGROUP_SIZE >= (1 << RADIX_BIN_BIT_SIZE));
	{
		auto& shader = indirParamsShader;
		shader = shaderHandler->CreateProgramObject("ParticleGeneratorHandler", "IndirectParameters");
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ParticleSetIndirectParamsCS.glsl", "", GL_COMPUTE_SHADER));
		shader->SetFlag("SIZE_SSBO_BINDING_IDX", IndirectBufferIndices::SIZE_SSBO_BINDING_IDX);
		shader->SetFlag("INDR_SSBO_BINDING_IDX", IndirectBufferIndices::INDR_SSBO_BINDING_IDX);
		shader->SetFlag("SIZE_SSBO_QUAD_IDX", ParticleGeneratorDefs::SIZE_SSBO_QUAD_IDX);
		shader->SetFlag("KVAL_SSBO_INDRCT_X", IndirectBufferIndices::KVAL_SSBO_INDRCT_X);
		shader->SetFlag("KVAL_SSBO_INDRCT_Y", IndirectBufferIndices::KVAL_SSBO_INDRCT_Y);
		shader->SetFlag("KVAL_SSBO_INDRCT_Z", IndirectBufferIndices::KVAL_SSBO_INDRCT_Z);
		shader->SetFlag("HIST_SSBO_INDRCT_X", IndirectBufferIndices::HIST_SSBO_INDRCT_X);
		shader->SetFlag("HIST_SSBO_INDRCT_Y", IndirectBufferIndices::HIST_SSBO_INDRCT_Y);
		shader->SetFlag("HIST_SSBO_INDRCT_Z", IndirectBufferIndices::HIST_SSBO_INDRCT_Z);
		shader->SetFlag("DRAW_SSBO_INDSC", IndirectBufferIndices::DRAW_SSBO_INDSC);
		shader->SetFlag("DRAW_SSBO_INSTC", IndirectBufferIndices::DRAW_SSBO_INSTC);
		shader->SetFlag("DRAW_SSBO_FIRSI", IndirectBufferIndices::DRAW_SSBO_FIRSI);
		shader->SetFlag("DRAW_SSBO_BASEV", IndirectBufferIndices::DRAW_SSBO_BASEV);
		shader->SetFlag("DRAW_SSBO_BASEI", IndirectBufferIndices::DRAW_SSBO_BASEI);
		shader->SetFlag("SIZE_SSBO_NUM_ELEM", ParticleGeneratorDefs::SIZE_SSBO_NUM_ELEM);
		shader->SetFlag("KEYVAL_SORTING_KEYVAL_WG_SIZE", ParticleGeneratorDefs::WORKGROUP_SIZE);
		shader->SetFlag("RADIX_SHADER_WG_SIZE", ParticleGeneratorDefs::WORKGROUP_SIZE);
		shader->SetFlag("PROCESS_TRIANGLES", PROCESS_TRIANGLES);
		shader->Link();

		shader->Enable();
		shader->SetUniform("sortElemsPerThread", 32);
		shader->Disable();

		shader->Validate();
	}
	{
		auto& shader = keyValShader;
		shader = shaderHandler->CreateProgramObject("ParticleGeneratorHandler", "KeyValueGenerator");
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ParticleSortingKeyValCS.glsl", "", GL_COMPUTE_SHADER));
		shader->SetFlag("WORKGROUP_SIZE", ParticleGeneratorDefs::WORKGROUP_SIZE);
		shader->SetFlag("SIZE_SSBO_QUAD_IDX", ParticleGeneratorDefs::SIZE_SSBO_QUAD_IDX);
		shader->SetFlag("VERT_SSBO_BINDING_IDX", KeyValStorageBindings::VERT_SSBO_BINDING_IDX);
		shader->SetFlag("SIZE_SSBO_BINDING_IDX", KeyValStorageBindings::SIZE_SSBO_BINDING_IDX);
		shader->SetFlag("KEYO_SSBO_BINDING_IDX", KeyValStorageBindings::KEYO_SSBO_BINDING_IDX);
		shader->SetFlag("VALO_SSBO_BINDING_IDX", KeyValStorageBindings::VALO_SSBO_BINDING_IDX);
		shader->SetFlag("PROCESS_TRIANGLES", PROCESS_TRIANGLES);
		shader->SetFlag("USE_PROJECTED_DISTANCE", USE_PROJECTED_DISTANCE);
		shader->Link();

		shader->Enable();
		shader->SetUniform("cameraPos", 0.0f, 0.0f, 0.0f);
		shader->SetUniform("cameraFwd", 0.0f, 0.0f, 1.0f);
		shader->SetUniform("cameraNearFar", 0.0f, 1.0f);
		shader->Disable();

		shader->Validate();
	}
	{
		auto& shader = radixHistShader;
		shader = shaderHandler->CreateProgramObject("ParticleGeneratorHandler", "RadixHist");
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/RadixHistCS.glsl", "", GL_COMPUTE_SHADER));
		shader->SetFlag("INDIRECT_EXECUTION", true);
		shader->SetFlag("WORKGROUP_SIZE", ParticleGeneratorDefs::WORKGROUP_SIZE);
		shader->SetFlag("HIST_SSBO_BINDING_IDX", RadixSortStorageBindings::HIST_SSBO_BINDING_IDX);
		shader->SetFlag("SIZE_SSBO_BINDING_IDX", RadixSortStorageBindings::SIZE_SSBO_BINDING_IDX);
		shader->SetFlag("KEYI_SSBO_BINDING_IDX", RadixSortStorageBindings::KEYI_SSBO_BINDING_IDX);
		shader->SetFlag("GET_NUM_ELEMS", fmt::format("atomicCounters[{}]", ParticleGeneratorDefs::SIZE_SSBO_NUM_ELEM).c_str());
		shader->SetFlag("BIN_BIT_SIZE", RADIX_BIN_BIT_SIZE);
		shader->Link();

		shader->Enable();
		shader->Disable();

		shader->Validate();
	}
	{
		assert(ParticleGeneratorDefs::WORKGROUP_SIZE >= HIST_BIN_SIZE);

		auto& shader = radixSortShader;
		shader = shaderHandler->CreateProgramObject("ParticleGeneratorHandler", "RadixSort");
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/RadixSortCS.glsl", "", GL_COMPUTE_SHADER));
		shader->SetFlag("INDIRECT_EXECUTION", true);
		shader->SetFlag("WORKGROUP_SIZE", ParticleGeneratorDefs::WORKGROUP_SIZE);
		shader->SetFlag("HIST_SSBO_BINDING_IDX", RadixSortStorageBindings::HIST_SSBO_BINDING_IDX);
		shader->SetFlag("SIZE_SSBO_BINDING_IDX", RadixSortStorageBindings::SIZE_SSBO_BINDING_IDX);
		shader->SetFlag("KEYI_SSBO_BINDING_IDX", RadixSortStorageBindings::KEYI_SSBO_BINDING_IDX);
		shader->SetFlag("VALI_SSBO_BINDING_IDX", RadixSortStorageBindings::VALI_SSBO_BINDING_IDX);
		shader->SetFlag("KEYO_SSBO_BINDING_IDX", RadixSortStorageBindings::KEYO_SSBO_BINDING_IDX);
		shader->SetFlag("VALO_SSBO_BINDING_IDX", RadixSortStorageBindings::VALO_SSBO_BINDING_IDX);
		shader->SetFlag("GET_NUM_ELEMS", fmt::format("atomicCounters[{}]", ParticleGeneratorDefs::SIZE_SSBO_NUM_ELEM).c_str());
		shader->SetFlag("SUBGROUP_SIZE", globalRendering->csWarpSize);
		shader->SetFlag("BIN_BIT_SIZE", RADIX_BIN_BIT_SIZE);
		shader->Link();

		shader->Enable();
		shader->Disable();

		shader->Validate();
	}
	{
		auto& shader = indcsProdShader;
		shader = shaderHandler->CreateProgramObject("ParticleGeneratorHandler", "TriangleIndicesProducer");
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ParticleProduceIndcsCS.glsl", "", GL_COMPUTE_SHADER));
		shader->SetFlag("WORKGROUP_SIZE", ParticleGeneratorDefs::WORKGROUP_SIZE);
		shader->SetFlag("VALI_SSBO_BINDING_IDX", IndicesProducerStorageBindings::VALI_SSBO_BINDING_IDX);
		shader->SetFlag("SIZE_SSBO_BINDING_IDX", IndicesProducerStorageBindings::SIZE_SSBO_BINDING_IDX);
		shader->SetFlag("IDCS_SSBO_BINDING_IDX", IndicesProducerStorageBindings::IDCS_SSBO_BINDING_IDX);
		shader->SetFlag("SIZE_SSBO_NUM_ELEM", ParticleGeneratorDefs::SIZE_SSBO_NUM_ELEM);
		shader->SetFlag("SIZE_SSBO_OOBC_IDX", ParticleGeneratorDefs::SIZE_SSBO_OOBC_IDX);
		shader->SetFlag("PROCESS_TRIANGLES", PROCESS_TRIANGLES);
		shader->SetFlag("GET_NUM_ELEMS", fmt::format("atomicCounters[{}]", ParticleGeneratorDefs::SIZE_SSBO_NUM_ELEM).c_str());
		shader->Link();

		shader->Enable();
		shader->Disable();

		shader->Validate();
	}
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
	cntrVBO.Release();
	indrVBO.Release();
	keysInVBO.Release();
	valsInVBO.Release();
	keysOutVBO.Release();
	valsOutVBO.Release();
	histVBO.Release();
}

void ParticleGeneratorHandler::ReallocateBuffersPre()
{
	bool reInitVAO = (vertVBO.GetIdRaw() == 0 || indcVBO.GetIdRaw() == 0);
	{
		auto bindingToken = vertVBO.BindScoped();
		reInitVAO |= vertVBO.ReallocToFit(4 * numQuads * sizeof(VA_TYPE_PROJ));
	}
	{
		auto bindingToken = indcVBO.BindScoped();
		reInitVAO |= indcVBO.ReallocToFit(6 * numQuads * sizeof(uint32_t));
	}

	if (reInitVAO) {
		vao.Bind();
		vertVBO.Bind(GL_ARRAY_BUFFER);
		indcVBO.Bind(GL_ELEMENT_ARRAY_BUFFER);
		VA_TYPE_PROJ::BindVertexAtrribs();

		vao.Unbind();
		vertVBO.Unbind(); vertVBO.SetCurrTargetRaw(GL_SHADER_STORAGE_BUFFER);
		indcVBO.Unbind(); indcVBO.SetCurrTargetRaw(GL_SHADER_STORAGE_BUFFER);
		VA_TYPE_PROJ::UnbindVertexAtrribs();
	}
}

void ParticleGeneratorHandler::ReallocateBuffersPost()
{
	const uint32_t allocSize = (1 + PROCESS_TRIANGLES) * numQuads * sizeof(uint32_t);
	{
		auto bindingToken = keysInVBO.BindScoped();
		keysInVBO.ReallocToFit(allocSize);
	}
	{
		auto bindingToken = valsInVBO.BindScoped();
		valsInVBO.ReallocToFit(allocSize);
	}
	{
		auto bindingToken = keysOutVBO.BindScoped();
		keysOutVBO.ReallocToFit(allocSize);
	}
	{
		auto bindingToken = valsOutVBO.BindScoped();
		valsOutVBO.ReallocToFit(allocSize);
	}
	{
		static constexpr uint32_t OPTIMAL_NUM_WG = 64u;

		const uint32_t numKeyData = 4u * numQuads;
		const uint32_t elemsPerWorkGroup = (numKeyData + ParticleGeneratorDefs::WORKGROUP_SIZE - 1) / ParticleGeneratorDefs::WORKGROUP_SIZE;
		sortElemsPerThread = (elemsPerWorkGroup + OPTIMAL_NUM_WG - 1) / OPTIMAL_NUM_WG;
		sortElemsPerThread = 1 << static_cast<uint32_t>(std::round(std::log2(sortElemsPerThread)));
		sortElemsPerThread = std::clamp(sortElemsPerThread, 1, 32);
		sortElemsPerThread = 32;

		const uint32_t elemsPerWorkGroupExt = ParticleGeneratorDefs::WORKGROUP_SIZE * sortElemsPerThread;
		sortHistNumWorkGroups = (numKeyData + elemsPerWorkGroupExt - 1) / elemsPerWorkGroupExt;
		const uint32_t histElemCount = HIST_BIN_SIZE * sortHistNumWorkGroups;

		auto bindingToken = histVBO.BindScoped();
		histVBO.ReallocToFit(histElemCount * sizeof(uint32_t));
	}
}

void ParticleGeneratorHandler::GenerateAll()
{
	SCOPED_TIMER("ParticleGeneratorHandler::GenerateAll");

	numQuads = std::apply([](auto& ... gen) {
		return (0 + ... + gen->GetMaxNumQuads());
	}, generators);

	if (numQuads <= 0)
		return;

	ReallocateBuffersPre();

	ParticleGeneratorGlobal::atomicCntVal = { 0 };
	static constexpr uint32_t ZERO = 0u;
	{
		auto bindingToken = cntrVBO.BindScoped();
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &ZERO);
	}
	{
		auto bindingToken1 = vertVBO.BindBufferRangeScoped(ParticleGeneratorDefs::VERT_SSBO_BINDING_IDX);
		auto bindingToken2 = cntrVBO.BindBufferRangeScoped(ParticleGeneratorDefs::SIZE_SSBO_BINDING_IDX);

		std::apply([this](auto& ... gen) {
			(gen->Generate(numQuads), ...);
		}, generators);
	}

	// TODO check if Generate() used CPU or GPU
	ReallocateBuffersPost();

	/*
	{
		auto token = indcVBO.BindScoped();
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &ZERO);
	}
	*/

	// form indirect dispatch buffer + set constants
	{
		auto bindingToken1 = cntrVBO.BindBufferRangeScoped(IndirectBufferIndices::SIZE_SSBO_BINDING_IDX);
		auto bindingToken2 = indrVBO.BindBufferRangeScoped(IndirectBufferIndices::INDR_SSBO_BINDING_IDX);
		auto shaderToken = indirParamsShader->EnableScoped();
		indirParamsShader->SetUniform("sortElemsPerThread", sortElemsPerThread);

		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
	}
	{
		// Debug
		cntrVBO.Bind();
		std::array<uint32_t, ParticleGeneratorDefs::SIZE_SSBO_NUM_ELEMENTS> cntrs;
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * cntrs.size(), cntrs.data());
		cntrVBO.Unbind();

		indrVBO.Bind();
		std::array<uint32_t, IndirectBufferIndices::INDR_SSBO_ELEM_SIZE> indir;
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * indir.size(), indir.data());
		indrVBO.Unbind();
	}

	// keep indirect draw buffer manually bound
	indrVBO.Bind(GL_DISPATCH_INDIRECT_BUFFER);

	// produce keys and values
	{
		auto bindingToken1 = vertVBO.BindBufferRangeScoped(KeyValStorageBindings::VERT_SSBO_BINDING_IDX);
		auto bindingToken2 = cntrVBO.BindBufferRangeScoped(KeyValStorageBindings::SIZE_SSBO_BINDING_IDX);
		auto bindingToken3 = keysInVBO.BindBufferRangeScoped(KeyValStorageBindings::KEYO_SSBO_BINDING_IDX);
		auto bindingToken4 = valsInVBO.BindBufferRangeScoped(KeyValStorageBindings::VALO_SSBO_BINDING_IDX);

		auto enToken = keyValShader->EnableScoped();
		const auto& camPos = camera->GetPos();
		const auto& camFwd = camera->GetForward();
		keyValShader->SetUniform("cameraPos", camPos.x, camPos.y, camPos.z);
		keyValShader->SetUniform("cameraFwd", camFwd.x, camFwd.y, camFwd.z);
		keyValShader->SetUniform("cameraNearFar", camera->GetNearPlaneDist(), camera->GetFarPlaneDist());
#if 1
		glDispatchComputeIndirect(static_cast<GLintptr>(IndirectBufferIndices::KVAL_SSBO_INDRCT_X * sizeof(int32_t)));
#else
		const int32_t numInvocations = (numQuads * (1 + PROCESS_TRIANGLES) + ParticleGeneratorDefs::WORKGROUP_SIZE - 1) / ParticleGeneratorDefs::WORKGROUP_SIZE;
		glDispatchCompute(numInvocations, 1, 1);
#endif
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Debug
		keysInVBO.Bind();
		std::vector<uint32_t> keys(numQuads * (1 + PROCESS_TRIANGLES));
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * keys.size(), keys.data());
		keysInVBO.Unbind();

		valsInVBO.Bind();
		std::vector<uint32_t> vals(numQuads * (1 + PROCESS_TRIANGLES));
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * vals.size(), vals.data());
		valsInVBO.Unbind();
	}
	// radix sorting
	std::array<VBO*, 2> outBufs;
	{
		auto bindingToken1 = histVBO.BindBufferRangeScoped(RadixSortStorageBindings::HIST_SSBO_BINDING_IDX);
		auto bindingToken2 = cntrVBO.BindBufferRangeScoped(RadixSortStorageBindings::SIZE_SSBO_BINDING_IDX);

		for (int passNum = 0; passNum < (8u * sizeof(uint32_t)) / RADIX_BIN_BIT_SIZE; ++passNum) {
			auto bindingToken3 = keysInVBO.BindBufferRangeScoped(RadixSortStorageBindings::KEYI_SSBO_BINDING_IDX);
			auto bindingToken4 = valsInVBO.BindBufferRangeScoped(RadixSortStorageBindings::VALI_SSBO_BINDING_IDX);
			auto bindingToken5 = keysOutVBO.BindBufferRangeScoped(RadixSortStorageBindings::KEYO_SSBO_BINDING_IDX);
			auto bindingToken6 = valsOutVBO.BindBufferRangeScoped(RadixSortStorageBindings::VALO_SSBO_BINDING_IDX);
			{
				auto enToken = radixHistShader->EnableScoped();
				// not needed because of the information provide in SSBO
				//radixHistShader->SetUniform("numElements", numQuads * (1 + PROCESS_TRIANGLES));
				radixHistShader->SetUniform("numElemsPerThread", sortElemsPerThread);
				radixHistShader->SetUniform("passNum", passNum);
				glDispatchComputeIndirect(static_cast<GLintptr>(IndirectBufferIndices::HIST_SSBO_INDRCT_X * sizeof(int32_t)));
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}
			{
				auto enToken = radixSortShader->EnableScoped();
				// not needed because of the information provide in SSBO
				//radixSortShader->SetUniform("numElements", numQuads * (1 + PROCESS_TRIANGLES));
				radixSortShader->SetUniform("numElemsPerThread", sortElemsPerThread);
				radixSortShader->SetUniform("numWorkGroups", sortHistNumWorkGroups);
				radixSortShader->SetUniform("passNum", passNum);
				glDispatchComputeIndirect(static_cast<GLintptr>(IndirectBufferIndices::HIST_SSBO_INDRCT_X * sizeof(int32_t)));
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}

			outBufs = { &keysOutVBO, &valsOutVBO };

			std::swap(keysInVBO, keysOutVBO);
			std::swap(valsInVBO, valsOutVBO);
		}
	}
	{
		// Debug
		outBufs[0]->Bind();
		std::vector<uint32_t> keys(numQuads* (1 + PROCESS_TRIANGLES));
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t)* keys.size(), keys.data());
		outBufs[0]->Unbind();

		outBufs[1]->Bind();
		std::vector<uint32_t> vals(numQuads* (1 + PROCESS_TRIANGLES));
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t)* vals.size(), vals.data());
		outBufs[1]->Unbind();
	}

	{
		auto bindingToken1 = outBufs[1]->BindBufferRangeScoped(IndicesProducerStorageBindings::VALI_SSBO_BINDING_IDX);
		auto bindingToken2 = indcVBO.BindBufferRangeScoped(IndicesProducerStorageBindings::IDCS_SSBO_BINDING_IDX);
		auto bindingToken3 = cntrVBO.BindBufferRangeScoped(IndicesProducerStorageBindings::SIZE_SSBO_BINDING_IDX);

		auto enToken = indcsProdShader->EnableScoped();

		glDispatchComputeIndirect(static_cast<GLintptr>(IndirectBufferIndices::KVAL_SSBO_INDRCT_X * sizeof(int32_t)));
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	{
		// Debug
		indcVBO.Bind();
		std::vector<uint32_t> indcs(6 * numQuads);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * indcs.size(), indcs.data());
		indcVBO.Unbind();
	}

	// Unbind manually
	indrVBO.Unbind();
	indrVBO.SetCurrTargetRaw(GL_SHADER_STORAGE_BUFFER); //reset so it can be bound to a slot in the shader

#if 1
	{
		// Debug
		cntrVBO.Bind();
		std::array<uint32_t, ParticleGeneratorDefs::SIZE_SSBO_NUM_ELEMENTS> vals;
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * vals.size(), vals.data());
		cntrVBO.Unbind();

		if (auto numQ = vals[ParticleGeneratorDefs::SIZE_SSBO_QUAD_IDX]; numQ > 0) {
			vertVBO.Bind();
			std::vector<VA_TYPE_PROJ> trData(4 * numQ);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(VA_TYPE_PROJ) * trData.size(), trData.data());
			vertVBO.Unbind();

			indcVBO.Bind();
			std::vector<uint32_t> idData(6 * numQ);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * idData.size(), idData.data());
			indcVBO.Unbind();
		}
	}
#endif
}

void ParticleGeneratorHandler::RenderAll()
{
	if (numQuads == 0)
		return;

	indrVBO.Bind(GL_DRAW_INDIRECT_BUFFER);
	vao.Bind();
	//vertVBO.Bind();
	//indcVBO.Bind();

	glDrawElementsIndirect(
		GL_TRIANGLES,
		GL_UNSIGNED_INT,
		reinterpret_cast<const void*>(
		IndirectBufferIndices::DRAW_SSBO_INDSC * sizeof(uint32_t))
	);

	vao.Unbind();
	//vertVBO.Unbind();
	//indcVBO.Unbind();

	indrVBO.Unbind();
	indrVBO.SetCurrTargetRaw(GL_SHADER_STORAGE_BUFFER); //reset so it can be bound to a slot in the shader
}

