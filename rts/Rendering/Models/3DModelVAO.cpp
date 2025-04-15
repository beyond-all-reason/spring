/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DModelVAO.h"

#include <algorithm>
#include <iterator>

#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/ModelsDataUploader.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Features/Feature.h"

#include "System/Misc/TracyDefs.h"


void S3DModelVAO::EnableAttribs(bool inst) const
{
	RECOIL_DETAILED_TRACY_ZONE;
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
		glVertexAttribIPointer(5, 3, GL_UNSIGNED_INT,        sizeof(SVertexData), (const void*)offsetof(SVertexData, boneIDsLow  ));
	}
	else {
		for (int i = 6; i <= 6; ++i) {
			glEnableVertexAttribArray(i);
			glVertexAttribDivisor(i, 1);
		}

		// covers all 4 uints of SInstanceData
		glVertexAttribIPointer(6, 4, GL_UNSIGNED_INT, sizeof(SInstanceData), (const void*)offsetof(SInstanceData, matOffset));
	}
}

void S3DModelVAO::DisableAttribs() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (int i = 0; i <= 6; ++i) {
		glDisableVertexAttribArray(i);
		glVertexAttribDivisor(i, 0);
	}
}

S3DModelVAO::S3DModelVAO()
{
	RECOIL_DETAILED_TRACY_ZONE;
	vertData.reserve(VERT_SIZE0);
	indxData.reserve(INDX_SIZE0);

	vertVBO = VBO{ GL_ARRAY_BUFFER        , false };
	indxVBO = VBO{ GL_ELEMENT_ARRAY_BUFFER, false };
	instVBO = VBO{ GL_ARRAY_BUFFER        , false };

	//no better place to init it
	instVBO.Bind();
	instVBO.New(S3DModelVAO::INSTANCE_BUFFER_NUM_ELEMS * sizeof(SInstanceData), GL_STREAM_DRAW);
	instVBO.Unbind();
}

void S3DModelVAO::ProcessVertices(const S3DModel* model)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(model);
	assert(model->loadStatus == S3DModel::LoadStatus::LOADING);

	if (const auto* root = model->GetRootPiece(); root->vertIndex != ~0u)
		return;

	uint32_t vertIndex = static_cast<uint32_t>(vertData.size());
	for (auto* modelPiece : model->pieceObjects) {
		modelPiece->vertIndex = vertIndex;
		const auto& modelPieceVerts = modelPiece->GetVerticesVec();
		vertIndex += modelPieceVerts.size();
		vertData.insert(vertData.end(), modelPieceVerts.begin(), modelPieceVerts.end()); //append
	}
}

void S3DModelVAO::ProcessIndicies(S3DModel* model)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(model);
	if (model->indxStart != ~0u)
		return;

	//models should know their index offset
	model->indxStart = static_cast<uint32_t>(std::distance(indxData.cbegin(), indxData.cend()));

	for (auto* modelPiece : model->pieceObjects) {
		if (!modelPiece->HasGeometryData()) {
			modelPiece->indxStart = static_cast<uint32_t>(indxData.size());
			modelPiece->indxCount = 0;
			continue;
		}

		const auto& modelPieceIndcs = modelPiece->GetIndicesVec();
		indxData.insert(indxData.end(), modelPieceIndcs.begin(), modelPieceIndcs.end()); //append

		const auto endIdx = indxData.end();
		const auto begIdx = endIdx - modelPieceIndcs.size();

		std::for_each(begIdx, endIdx, [offset = modelPiece->vertIndex](uint32_t& indx) { indx += offset; }); // add per piece vertex offset to indices

		//model pieces should know their index offset
		modelPiece->indxStart = static_cast<uint32_t>(std::distance(indxData.begin(), begIdx));

		//model pieces should know their index count
		modelPiece->indxCount = static_cast<uint32_t>(modelPieceIndcs.size());
	}
	//models should know their index count
	model->indxCount = static_cast<uint32_t>(indxData.size() - model->indxStart);

	//add shatter indices to the end of indxData
	for (const auto* modelPiece : model->pieceObjects) {
		if (!modelPiece->HasGeometryData())
			continue;

		const auto& mdlPcsShatIndcs = modelPiece->GetShatterIndicesVec();

		indxData.insert(indxData.end(), mdlPcsShatIndcs.begin(), mdlPcsShatIndcs.end()); //append

		const auto endIdx = indxData.end();
		const auto begIdx = endIdx - mdlPcsShatIndcs.size();

		std::for_each(begIdx, endIdx, [offset = modelPiece->vertIndex](uint32_t& indx) { indx += offset; }); // add per piece vertex offset to indices
	}
}

void S3DModelVAO::CreateVAO()
{
	RECOIL_DETAILED_TRACY_ZONE;
	vao = VAO{};
	vao.Bind();

	vertVBO.Bind();
	indxVBO.Bind();
	EnableAttribs(false); // vertex attribs
	vertVBO.Unbind();

	instVBO.Bind();
	EnableAttribs(true); // instance attribs

	vao.Unbind();
	DisableAttribs();

	indxVBO.Unbind();
	instVBO.Unbind();
}

void S3DModelVAO::UploadVBOs()
{
	RECOIL_DETAILED_TRACY_ZONE;
	static constexpr size_t MEM_STEP = 8 * 1024 * 1024;
	bool reinitVAO = (vao.GetIdRaw() == 0);

	if (vertData.size() > vertUploadIndex) {
		assert(!safeToDeleteVectors);
		vertVBO.Bind();
		const size_t reqSize = AlignUp(std::max(vertData.size(), S3DModelVAO::VERT_SIZE0) * sizeof(SVertexData), MEM_STEP);
		reinitVAO |= (reqSize > vertVBO.GetSize());
		vertVBO.Resize(reqSize, GL_STATIC_DRAW); //noop if size hasn't changed, will copy data if changed
		vertVBO.SetBufferSubData(vertUploadIndex * sizeof(SVertexData), (vertData.size() - vertUploadIndex) * sizeof(SVertexData), vertData.data() + vertUploadIndex);
		vertVBO.Unbind();
		vertUploadIndex = vertData.size();
		vertUploadSize = vertUploadIndex;
	}

	if (indxData.size() > indxUploadIndex) {
		assert(!safeToDeleteVectors);
		indxVBO.Bind();
		const size_t reqSize = AlignUp(std::max(indxData.size(), S3DModelVAO::INDX_SIZE0) * sizeof(   uint32_t), MEM_STEP);
		reinitVAO |= (reqSize > indxVBO.GetSize());
		indxVBO.Resize(reqSize, GL_STATIC_DRAW); //noop if size hasn't changed, will copy data if changed
		indxVBO.SetBufferSubData(indxUploadIndex * sizeof(   uint32_t), (indxData.size() - indxUploadIndex) * sizeof(   uint32_t), indxData.data() + indxUploadIndex);
		indxVBO.Unbind();
		indxUploadIndex = indxData.size();
		indxUploadSize = indxUploadIndex;
	}

	if (reinitVAO)
		CreateVAO();

	if (safeToDeleteVectors && !vertData.empty()) {
		// all models have been uploaded in the calls above
		// safe to clear CPU copy of the data
		vertData.clear();
		indxData.clear();
		vertUploadIndex = 0;
		indxUploadIndex = 0;
	}
}

void S3DModelVAO::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	Kill();
	instance = std::make_unique<S3DModelVAO>();
}

void S3DModelVAO::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	instance = nullptr;
}

void S3DModelVAO::Bind() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(vao.GetIdRaw() > 0);
	vao.Bind();
}

void S3DModelVAO::Unbind() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(vao.GetIdRaw() > 0);
	vao.Unbind();
}

void S3DModelVAO::BindLegacyVertexAttribsAndVBOs() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	vertVBO.Bind();
	indxVBO.Bind();

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(SVertexData), vertVBO.GetPtr(offsetof(SVertexData, pos)));

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(SVertexData), vertVBO.GetPtr(offsetof(SVertexData, normal)));

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SVertexData), vertVBO.GetPtr(offsetof(SVertexData, texCoords[0])));

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SVertexData), vertVBO.GetPtr(offsetof(SVertexData, texCoords[1])));

	glClientActiveTexture(GL_TEXTURE5);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(3, GL_FLOAT, sizeof(SVertexData), vertVBO.GetPtr(offsetof(SVertexData, sTangent)));

	glClientActiveTexture(GL_TEXTURE6);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(3, GL_FLOAT, sizeof(SVertexData), vertVBO.GetPtr(offsetof(SVertexData, tTangent)));
}

void S3DModelVAO::UnbindLegacyVertexAttribsAndVBOs() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glClientActiveTexture(GL_TEXTURE6);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE5);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	indxVBO.Unbind();
	vertVBO.Unbind();
}

void S3DModelVAO::DrawElements(GLenum prim, uint32_t vboIndxStart, uint32_t vboIndxCount) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glDrawElements(prim, vboIndxCount, GL_UNSIGNED_INT, indxVBO.GetPtr(vboIndxStart * sizeof(uint32_t)));
}

template<typename TObj>
bool S3DModelVAO::AddToSubmissionImpl(const TObj* obj, uint32_t indexStart, uint32_t indexCount, uint8_t teamID, uint8_t drawFlags)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto matIndex = transformsUploader.GetElemOffset(obj);
	if (matIndex == TransformsMemStorage::INVALID_INDEX)
		return false;

	const auto uniIndex = modelUniformsStorage.GetObjOffset(obj); //doesn't need to exist for defs and models. Don't check for validity

	uint16_t numPieces = 0;
	size_t bposeIndex = 0;
	if constexpr (std::is_same<TObj, S3DModel>::value) {
		numPieces = static_cast<uint16_t>(obj->numPieces);
		bposeIndex = transformsUploader.GetElemOffset(obj);
	}
	else {
		numPieces = static_cast<uint16_t>(obj->model->numPieces);
		bposeIndex = transformsUploader.GetElemOffset(obj->model);
	}

	if (bposeIndex == TransformsMemStorage::INVALID_INDEX)
		return false;

	auto& modelInstanceData = modelDataToInstance[SIndexAndCount{ indexStart, indexCount }];
	modelInstanceData.emplace_back(SInstanceData(
		static_cast<uint32_t>(matIndex),
		teamID,
		drawFlags,
		numPieces,
		static_cast<uint32_t>(uniIndex),
		static_cast<uint32_t>(bposeIndex)
	));

	return true;
}

bool S3DModelVAO::AddToSubmission(const S3DModel* model, uint8_t teamID, uint8_t drawFlags)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(model);

	return AddToSubmissionImpl(model, model->indxStart, model->indxCount, teamID, drawFlags);
}

bool S3DModelVAO::AddToSubmission(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(unit);

	const S3DModel* model = unit->model;
	assert(model);

	return AddToSubmissionImpl(unit, model->indxStart, model->indxCount, unit->team, unit->drawFlag);
}

bool S3DModelVAO::AddToSubmission(const CFeature* feature)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(feature);

	const S3DModel* model = feature->model;
	assert(model);

	return AddToSubmissionImpl(feature, model->indxStart, model->indxCount, feature->team, feature->drawFlag);
}

bool S3DModelVAO::AddToSubmission(const UnitDef* unitDef, uint8_t teamID)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(unitDef);

	const S3DModel* model = unitDef->model;
	assert(model);

	return AddToSubmissionImpl(unitDef, model->indxStart, model->indxCount, teamID, 0);
}


void S3DModelVAO::Submit(GLenum mode, bool bindUnbind)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static std::vector<SDrawElementsIndirectCommand> submitCmds;
	submitCmds.clear();

	batchedBaseInstance = 0u;

	static std::vector<SInstanceData> allRenderModelData;
	allRenderModelData.reserve(INSTANCE_BUFFER_NUM_BATCHED);
	allRenderModelData.clear();

	for (const auto& [indxCount, renderModelData] : modelDataToInstance) {
		if (allRenderModelData.size() + renderModelData.size() >= INSTANCE_BUFFER_NUM_BATCHED)
			continue;

		SDrawElementsIndirectCommand scmd{
			indxCount.count,
			static_cast<uint32_t>(renderModelData.size()),
			indxCount.index,
			0u,
			batchedBaseInstance
		};

		submitCmds.emplace_back(scmd);

		allRenderModelData.insert(allRenderModelData.end(), renderModelData.cbegin(), renderModelData.cend());
		batchedBaseInstance += renderModelData.size();
	}

	if (submitCmds.empty())
		return;

	instVBO.Bind();
	instVBO.SetBufferSubData(allRenderModelData);
	instVBO.Unbind();

	if (bindUnbind)
		Bind();

	glMultiDrawElementsIndirect(mode, GL_UNSIGNED_INT, submitCmds.data(), submitCmds.size(), sizeof(SDrawElementsIndirectCommand));

	if (bindUnbind)
		Unbind();

	modelDataToInstance.clear();
}

template<typename TObj>
bool S3DModelVAO::SubmitImmediatelyImpl(const TObj* obj, uint32_t indexStart, uint32_t indexCount, uint8_t teamID, uint8_t drawFlags, GLenum mode, bool bindUnbind)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::size_t matIndex = transformsUploader.GetElemOffset(obj);
	if (matIndex == TransformsMemStorage::INVALID_INDEX)
		return false;

	const auto uniIndex = modelUniformsStorage.GetObjOffset(obj); //doesn't need to exist for defs. Don't check for validity

	uint16_t numPieces = 0;
	size_t bposeIndex = 0;
	if constexpr (std::is_same<TObj, S3DModel>::value) {
		numPieces = static_cast<uint16_t>(obj->numPieces);
		bposeIndex = transformsUploader.GetElemOffset(obj);
	}
	else {
		numPieces = static_cast<uint16_t>(obj->model->numPieces);
		bposeIndex = transformsUploader.GetElemOffset(obj->model);
	}

	SInstanceData instanceData(static_cast<uint32_t>(matIndex), teamID, drawFlags, numPieces, uniIndex, bposeIndex);
	const uint32_t immediateBaseInstanceAbs = INSTANCE_BUFFER_NUM_BATCHED + immediateBaseInstance;

	static SDrawElementsIndirectCommand scmd;
	scmd = {
		indexCount,
		1,
		indexStart,
		0u,
		immediateBaseInstanceAbs
	};

	instVBO.Bind();
	instVBO.SetBufferSubData(immediateBaseInstanceAbs * sizeof(SInstanceData), sizeof(SInstanceData), &instanceData);
	instVBO.Unbind();

	immediateBaseInstance = (immediateBaseInstance + 1) % INSTANCE_BUFFER_NUM_IMMEDIATE;

	if (bindUnbind)
		Bind();

	// As of 01.05.2023 AMD Windows drivers do not support baseInstance field of SDrawElementsIndirectCommand
	// therefore can't use glDrawElementsIndirect
	// At the same time AMD Windows drivers sometimes crash on glDrawElementsInstancedBaseInstance
	// can't use it either
	// Revert to glMultiDrawElementsIndirect as it works reliably

	glMultiDrawElementsIndirect(mode, GL_UNSIGNED_INT, &scmd, 1u, sizeof(SDrawElementsIndirectCommand));

	if (bindUnbind)
		Unbind();

	return true;
}

bool S3DModelVAO::SubmitImmediately(const S3DModel* model, uint8_t teamID, uint8_t drawFlags, GLenum mode, bool bindUnbind)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(model);
	return SubmitImmediatelyImpl(model, model->indxStart, model->indxCount, teamID, drawFlags, mode, bindUnbind);
}

bool S3DModelVAO::SubmitImmediately(const CUnit* unit, const GLenum mode, bool bindUnbind)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(unit);

	const S3DModel* model = unit->model;
	assert(model);

	return SubmitImmediatelyImpl(unit, model->indxStart, model->indxCount, unit->team, unit->drawFlag, mode, bindUnbind);
}

bool S3DModelVAO::SubmitImmediately(const CFeature* feature, GLenum mode, bool bindUnbind)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(feature);

	const S3DModel* model = feature->model;
	assert(model);

	return SubmitImmediatelyImpl(feature, model->indxStart, model->indxCount, feature->team, feature->drawFlag, mode, bindUnbind);
}

bool S3DModelVAO::SubmitImmediately(const UnitDef* unitDef, int teamID, GLenum mode, bool bindUnbind)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(unitDef);

	const S3DModel* model = unitDef->model;
	assert(model);

	return SubmitImmediatelyImpl(unitDef, model->indxStart, model->indxCount, teamID, 0, mode, bindUnbind);
}
