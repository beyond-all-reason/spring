/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DModelVAO.h"

#include <algorithm>
#include <iterator>

#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/MatrixUploader.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

void S3DModelVAO::SubmitImmediatelyImpl(const SDrawElementsIndirectCommand* scmd, const uint32_t ssboOffset, const uint32_t teamID, const GLenum mode, const bool bindUnbind)
{
	// do not increment base instance
	SInstanceData instanceData{ ssboOffset, teamID };

	instVBO->Bind();
	instVBO->SetBufferSubData(baseInstance * sizeof(SInstanceData), sizeof(SInstanceData), &instanceData);
	instVBO->Unbind();

	if (bindUnbind)
		Bind();

	glDrawElementsIndirect(mode, GL_UNSIGNED_INT, scmd);

	if (bindUnbind)
		Unbind();
}

void S3DModelVAO::EnableAttribs(bool inst) const
{
	if (!inst) {
		for (int i = 0; i <= 5; ++i) {
			glEnableVertexAttribArray(i);
			glVertexAttribDivisor(i, 0);
		}

		glVertexAttribPointer (0, 3, GL_FLOAT       , false, sizeof(SVertexData), (const void*)offsetof(SVertexData, pos         ));
		glVertexAttribPointer (1, 3, GL_FLOAT       , false, sizeof(SVertexData), (const void*)offsetof(SVertexData, normal      ));
		glVertexAttribPointer (2, 3, GL_FLOAT       , false, sizeof(SVertexData), (const void*)offsetof(SVertexData, sTangent    ));
		glVertexAttribPointer (3, 3, GL_FLOAT       , false, sizeof(SVertexData), (const void*)offsetof(SVertexData, tTangent    ));
		glVertexAttribPointer (4, 4, GL_FLOAT       , false, sizeof(SVertexData), (const void*)offsetof(SVertexData, texCoords[0]));
		glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT,        sizeof(SVertexData), (const void*)offsetof(SVertexData, pieceIndex  ));
	}
	else {
		for (int i = 6; i <= 6; ++i) {
			glEnableVertexAttribArray(i);
			glVertexAttribDivisor(i, 1);
		}

		// covers all 4 uints of SInstanceData
		glVertexAttribIPointer(6, 4, GL_UNSIGNED_INT, sizeof(SInstanceData), (const void*)offsetof(SInstanceData, ssboOffset));
	}
}

void S3DModelVAO::DisableAttribs() const
{
	for (int i = 0; i <= 6; ++i) {
		glDisableVertexAttribArray(i);
		glVertexAttribDivisor(i, 0);
	}
}

void S3DModelVAO::Init()
{
	baseInstance = 0u;
	std::vector<SVertexData> vertData; vertData.reserve(2 << 21);
	std::vector<uint32_t   > indxData; indxData.reserve(2 << 22);

	//populate content of the common buffers
	{
		auto& allModels = modelLoader.GetModelsVec();
		for (auto& model : allModels) {

			//models should know their index offset
			model.indxStart = std::distance(indxData.cbegin(), indxData.cend());

			for (auto modelPiece : model.pieceObjects) { //vec of pointers
				if (!modelPiece->HasGeometryData())
					continue;

				const auto& modelPieceVerts = modelPiece->GetVerticesVec();
				const auto& modelPieceIndcs = modelPiece->GetIndicesVec();

				const uint32_t indexOffsetVertNum = vertData.size();

				vertData.insert(vertData.end(), modelPieceVerts.begin(), modelPieceVerts.end()); //append
				indxData.insert(indxData.end(), modelPieceIndcs.begin(), modelPieceIndcs.end()); //append

				const auto endIdx = indxData.end();
				const auto begIdx = endIdx - modelPieceIndcs.size();

				std::for_each(begIdx, endIdx, [indexOffsetVertNum](uint32_t& indx) { indx += indexOffsetVertNum; }); // add per piece vertex offset to indices

				//model pieces should know their index offset
				modelPiece->indxStart = std::distance(indxData.begin(), begIdx);

				//model pieces should know their index count
				modelPiece->indxCount = modelPieceIndcs.size();
			}

			//models should know their index count
			model.indxCount = indxData.size() - model.indxStart;
		}
	}

	//LOG("S3DModelVAO::Init() indxData.size() %u vertData.size() %u", static_cast<uint32_t>(indxData.size()), static_cast<uint32_t>(vertData.size()));

	//OpenGL stuff
	{
		vertVBO = std::make_unique<VBO>(GL_ARRAY_BUFFER, false);
		vertVBO->Bind();
		vertVBO->New(vertData);
		vertVBO->Unbind();

		indxVBO = std::make_unique<VBO>(GL_ELEMENT_ARRAY_BUFFER, false);
		indxVBO->Bind();
		indxVBO->New(indxData);
		indxVBO->Unbind();
	}
	{
		vao = std::make_unique<VAO>();
		vao->Bind();

		vertVBO = std::make_unique<VBO>(GL_ARRAY_BUFFER, false);
		vertVBO->Bind();
		vertVBO->New(vertData);

		indxVBO = std::make_unique<VBO>(GL_ELEMENT_ARRAY_BUFFER, false);
		indxVBO->Bind();
		indxVBO->New(indxData);
		EnableAttribs(false);

		vertVBO->Unbind();

		instVBO = std::make_unique<VBO>(GL_ARRAY_BUFFER, false);
		instVBO->Bind();
		instVBO->New(S3DModelVAO::INSTANCE_BUFFER_NUM_ELEMS * sizeof(SInstanceData), GL_STREAM_DRAW);
		EnableAttribs(true);

		vao->Unbind();
		DisableAttribs();

		indxVBO->Unbind();
		instVBO->Unbind();
	}
}

void S3DModelVAO::Bind() const
{
	assert(vao);
	vao->Bind();
}

void S3DModelVAO::Unbind() const
{
	assert(vao);
	vao->Unbind();
}

void S3DModelVAO::AddToSubmission(const CUnit* unit)
{
	const auto ssboIndex = MatrixUploader::GetInstance().GetElemOffset(unit);
	if (ssboIndex == MatricesMemStorage::INVALID_INDEX)
		return;

	const auto indxCount = IndexCount(unit->model->indxStart, unit->model->indxCount);
	auto& modelInstanceData = modelDataToInstance[indxCount];
	modelInstanceData.emplace_back(SInstanceData(ssboIndex, unit->team));
}

void S3DModelVAO::AddToSubmission(const UnitDef* unitDef, const int teamID)
{
	const auto ssboIndex = MatrixUploader::GetInstance().GetElemOffset(unitDef);
	if (ssboIndex == MatricesMemStorage::INVALID_INDEX)
		return;

	const auto indxCount = IndexCount(unitDef->model->indxStart, unitDef->model->indxCount);
	auto& modelInstanceData = modelDataToInstance[indxCount];
	modelInstanceData.emplace_back(SInstanceData(ssboIndex, teamID));
}

void S3DModelVAO::AddToSubmission(const S3DModel* model, const int teamID)
{
	const auto ssboIndex = MatrixUploader::GetInstance().GetElemOffset(model);
	if (ssboIndex == MatricesMemStorage::INVALID_INDEX)
		return;

	const auto indxCount = IndexCount(model->indxStart, model->indxCount);
	auto& modelInstanceData = modelDataToInstance[indxCount];
	modelInstanceData.emplace_back(SInstanceData(ssboIndex, teamID));
}

void S3DModelVAO::Submit(const GLenum mode, const bool bindUnbind)
{
	static std::vector<SDrawElementsIndirectCommand> submitCmds;
	submitCmds.clear();

	baseInstance = 0u;

	static std::vector<SInstanceData> allRenderModelData;
	allRenderModelData.reserve(INSTANCE_BUFFER_NUM_ELEMS);
	allRenderModelData.clear();

	for (const auto& [indxCount, renderModelData] : modelDataToInstance) {
		//model
		SDrawElementsIndirectCommand scmd{
			indxCount.count,
			static_cast<uint32_t>(renderModelData.size()),
			indxCount.index,
			0u,
			baseInstance
		};

		submitCmds.emplace_back(scmd);

		allRenderModelData.insert(allRenderModelData.end(), renderModelData.cbegin(), renderModelData.cend());
		baseInstance += renderModelData.size();
	}

	if (submitCmds.empty())
		return;

	instVBO->Bind();
	instVBO->SetBufferSubData(allRenderModelData);
	instVBO->Unbind();

	if (bindUnbind)
		Bind();

	glMultiDrawElementsIndirect(mode, GL_UNSIGNED_INT, submitCmds.data(), submitCmds.size(), sizeof(SDrawElementsIndirectCommand));

	if (bindUnbind)
		Unbind();
}

void S3DModelVAO::SubmitImmediately(const CUnit* unit, const GLenum mode, const bool bindUnbind)
{
	const auto ssboIndex = MatrixUploader::GetInstance().GetElemOffset(unit);
	if (ssboIndex == MatricesMemStorage::INVALID_INDEX)
		return;

	const auto* model = unit->model;

	SDrawElementsIndirectCommand scmd{
		model->indxCount,
		1,
		model->indxStart,
		0u,
		baseInstance
	};

	SubmitImmediatelyImpl(&scmd, ssboIndex, unit->team, mode, bindUnbind);
}

void S3DModelVAO::SubmitImmediately(const UnitDef* unitDef, const int teamID, const GLenum mode, const bool bindUnbind)
{
	const auto ssboIndex = MatrixUploader::GetInstance().GetElemOffset(unitDef);
	if (ssboIndex == MatricesMemStorage::INVALID_INDEX)
		return;

	const auto* model = unitDef->model;

	SDrawElementsIndirectCommand scmd{
		model->indxCount,
		1,
		model->indxStart,
		0u,
		baseInstance
	};

	SubmitImmediatelyImpl(&scmd, ssboIndex, teamID, mode, bindUnbind);
}

void S3DModelVAO::SubmitImmediately(const S3DModel* model, const int teamID, const GLenum mode, const bool bindUnbind)
{
	const auto ssboIndex = MatrixUploader::GetInstance().GetElemOffset(model);
	if (ssboIndex == MatricesMemStorage::INVALID_INDEX)
		return;

	SDrawElementsIndirectCommand scmd{
		model->indxCount,
		1,
		model->indxStart,
		0u,
		baseInstance
	};

	SubmitImmediatelyImpl(&scmd, ssboIndex, teamID, mode, bindUnbind);
}