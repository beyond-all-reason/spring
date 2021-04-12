/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DModel.h"

#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/MatrixUploader.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "lib/meshoptimizer/src/meshoptimizer.h"

#include "System/Log/ILog.h"

#include <algorithm>
#include <cctype>
#include <cstring>

CR_BIND(LocalModelPiece, (nullptr))
CR_REG_METADATA(LocalModelPiece, (
	CR_MEMBER(pos),
	CR_MEMBER(rot),
	CR_MEMBER(dir),
	CR_MEMBER(colvol),
	CR_MEMBER(scriptSetVisible),
	CR_MEMBER(blockScriptAnims),
	CR_MEMBER(lmodelPieceIndex),
	CR_MEMBER(scriptPieceIndex),
	CR_MEMBER(parent),
	CR_MEMBER(children),

	// reload
	CR_IGNORED(dispListID),
	CR_IGNORED(original),

	CR_IGNORED(dirty),
	CR_IGNORED(modelSpaceMat),
	CR_IGNORED(pieceSpaceMat),

	CR_IGNORED(lodDispLists) //FIXME GL idx!
))

CR_BIND(LocalModel, )
CR_REG_METADATA(LocalModel, (
	CR_MEMBER(pieces),

	CR_IGNORED(boundingVolume),
	CR_IGNORED(luaMaterialData)
))


/** ****************************************************************************************************
 * S3DModelPiece
 */

void S3DModelPiece::CreateDispList()
{
	glNewList(dispListID = glGenLists(1), GL_COMPILE);
	DrawForList();
	glEndList();
}

void S3DModelPiece::DeleteDispList()
{
	glDeleteLists(dispListID, 1);
	dispListID = 0;
}

void S3DModelPiece::DrawStatic() const
{
	if (!HasGeometryData())
		return;

	glPushMatrix();
	glMultMatrixf(bposeMatrix);
	glCallList(dispListID);
	glPopMatrix();
}


float3 S3DModelPiece::GetEmitPos() const
{
	switch (GetVertexCount()) {
		case 0:
		case 1: { return ZeroVector; } break;
		default: { return GetVertexPos(0); } break;
	}
}

float3 S3DModelPiece::GetEmitDir() const
{
	switch (GetVertexCount()) {
		case 0: { return FwdVector; } break;
		case 1: { return GetVertexPos(0); } break;
		default: { return (GetVertexPos(1) - GetVertexPos(0)); } break;
	}
}


void S3DModelPiece::CreateShatterPieces()
{
	if (!HasGeometryData())
		return;

	vboShatterIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
	vboShatterIndices.Resize(S3DModelPiecePart::SHATTER_VARIATIONS * GetVertexDrawIndexCount() * sizeof(uint32_t));

	for (int i = 0; i < S3DModelPiecePart::SHATTER_VARIATIONS; ++i) {
		CreateShatterPiecesVariation(i);
	}

	vboShatterIndices.Unbind();
}


void S3DModelPiece::CreateShatterPiecesVariation(const int num)
{
	typedef  std::pair<S3DModelPiecePart::RenderData, std::vector<uint32_t> >  ShatterPartDataPair;
	typedef  std::array< ShatterPartDataPair, S3DModelPiecePart::SHATTER_MAX_PARTS>  ShatterPartsBuffer;

	// operate on a buffer; indices are not needed once VBO has been created
	ShatterPartsBuffer shatterPartsBuf;

	for (ShatterPartDataPair& cp: shatterPartsBuf) {
		cp.first.dir = (guRNG.NextVector()).ANormalize();
	}

	// helper
	const auto GetPolygonDir = [&](const size_t idx) -> float3
	{
		float3 midPos;
		midPos += GetVertexPos(indices[idx + 0]);
		midPos += GetVertexPos(indices[idx + 1]);
		midPos += GetVertexPos(indices[idx + 2]);
		midPos *= 0.333f;
		return (midPos.ANormalize());
	};

	// add vertices to splitter parts
	for (size_t i = 0; i < indices.size(); i += 3) {
		const float3& dir = GetPolygonDir(i);

		// find the closest shatter part (the one that points into same dir)
		float md = -2.0f;

		ShatterPartDataPair* mcp = nullptr;
		S3DModelPiecePart::RenderData* rd = nullptr;

		for (ShatterPartDataPair& cp: shatterPartsBuf) {
			rd = &cp.first;

			if (rd->dir.dot(dir) < md)
				continue;

			md = rd->dir.dot(dir);
			mcp = &cp;
		}

		(mcp->second).push_back(indices[i + 0] + vboVertStart);
		(mcp->second).push_back(indices[i + 1] + vboVertStart);
		(mcp->second).push_back(indices[i + 2] + vboVertStart);
	}

	{
		// fill the vertex index vbo
		const size_t mapSize = indices.size() * sizeof(uint32_t);
		size_t vboPos = 0;

		for (auto* vboMem = vboShatterIndices.MapBuffer(num * mapSize, mapSize, GL_WRITE_ONLY); vboMem != nullptr; vboMem = nullptr) {
			for (ShatterPartDataPair& cp: shatterPartsBuf) {
				S3DModelPiecePart::RenderData& rd = cp.first;

				rd.indexCount = (cp.second).size();
				rd.vboOffset  = num * mapSize + vboPos;

				if (rd.indexCount > 0) {
					memcpy(vboMem + vboPos, &(cp.second)[0], rd.indexCount * sizeof(uint32_t));
					vboPos += (rd.indexCount * sizeof(uint32_t));
				}
			}
		}

		vboShatterIndices.UnmapBuffer();
	}

	{
		// delete empty splitter parts
		size_t backIdx = shatterPartsBuf.size() - 1;

		for (size_t j = 0; j < shatterPartsBuf.size() && j < backIdx; ) {
			const ShatterPartDataPair& cp = shatterPartsBuf[j];
			const S3DModelPiecePart::RenderData& rd = cp.first;

			if (rd.indexCount == 0) {
				std::swap(shatterPartsBuf[j], shatterPartsBuf[backIdx--]);
				continue;
			}

			j++;
		}

		shatterParts[num].renderData.clear();
		shatterParts[num].renderData.reserve(backIdx + 1);

		// finish: copy buffer to actual memory
		for (size_t n = 0; n <= backIdx; n++) {
			shatterParts[num].renderData.push_back(shatterPartsBuf[n].first);
		}
	}
}


void S3DModelPiece::Shatter(float pieceChance, int modelType, int texType, int team, const float3 pos, const float3 speed, const CMatrix44f& m) const
{
	const float2  pieceParams = {float3::max(float3::fabs(maxs), float3::fabs(mins)).Length(), pieceChance};
	const   int2 renderParams = {texType, team};

	projectileHandler.AddFlyingPiece(modelType, this, m, pos, speed, pieceParams, renderParams);
}


void S3DModelPiece::PostProcessGeometry()
{
	if (!HasGeometryData())
		return;

	vboVertStart = model->curVertStartIndx;
	vboIndxStart = model->curIndxStartIndx;

	MeshOptimize();

	indicesVBO.resize(indices.size());
	std::transform(indices.cbegin(), indices.cend(), indicesVBO.begin(), [this](uint32_t indx) { return indx + this->vboVertStart; });
}

void S3DModelPiece::UploadToVBO()
{
	if (!HasGeometryData())
		return;

	assert(model);
	model->UploadToVBO(vertices, indicesVBO, vboVertStart, vboIndxStart);

	indicesVBO.clear(); //no longer needed
}

void S3DModelPiece::MeshOptimize()
{
	if (!HasGeometryData())
		return;

#if 0
	return;
#endif

	decltype(indices)  optIndices = indices;
	decltype(vertices) optVertices = vertices;
	{
		// First, generate a remap table from your existing vertex (and, optionally, index) data:
		std::vector<uint32_t> remap(vertices.size()); // allocate temporary memory for the remap table
		size_t vertexCount = meshopt_generateVertexRemap(remap.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(SVertexData));

		// After generating the remap table, you can allocate space for the target vertex buffer (vertex_count elements) and index buffer (index_count elements) and generate them:
		optVertices.resize(vertexCount);

		meshopt_remapIndexBuffer(optIndices.data(), indices.data(), indices.size(), remap.data());
		meshopt_remapVertexBuffer(optVertices.data(), vertices.data(), vertices.size(), sizeof(SVertexData), remap.data());
	}

	// Vertex cache optimization
	meshopt_optimizeVertexCache(optIndices.data(), optIndices.data(), optIndices.size(), optVertices.size());

	// Vertex fetch optimization
	optVertices.resize(meshopt_optimizeVertexFetch(optVertices.data(), optIndices.data(), optIndices.size(), optVertices.data(), optVertices.size(), sizeof(SVertexData)));
#if 0 //uncomment when Lod system will need to be implemented
	{
		const float2 optTarget{ 0.7f, 0.05f };

		size_t target_index_count = size_t(optIndices.size() * optTarget.x);
		float result_error = 0.0f;

		decltype(indices)  lodIndices  = optIndices;
		decltype(vertices) lodVertices = optVertices;

		lodIndices.resize(meshopt_simplify(lodIndices.data(), optIndices.data(), optIndices.size(), &optVertices[0].pos.x, optVertices.size(), sizeof(SVertexData), target_index_count, optTarget.y, &result_error));
		lodVertices.resize(meshopt_optimizeVertexFetch(lodVertices.data(), lodIndices.data(), lodIndices.size(), lodVertices.data(), lodVertices.size(), sizeof(SVertexData)));

		LOG("[%s] vertices.size() = %u, indices.size() = %u || optVertices.size() = %u, optIndices.size() = %u || lodIndices.size() = %u, lodVertices = %u || result_error = %f",
			model->name.c_str(),
			static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(indices.size()),
			static_cast<uint32_t>(optVertices.size()), static_cast<uint32_t>(optIndices.size()),
			static_cast<uint32_t>(lodVertices.size()), static_cast<uint32_t>(lodIndices.size()),
			static_cast<double>(result_error)
		);

		if (lodIndices.size() < optIndices.size() || lodVertices.size() < optVertices.size()) {
			optIndices  = lodIndices;
			optVertices = lodVertices;
		}
	}
#endif

	if (optIndices.size() < indices.size() || optVertices.size() < vertices.size()) {
#if 0 //uncomment to get reports
		LOG("[%s %s] (deltaVertices, deltaIndices) = (%u, %u)",
			model->name.c_str(),
			this->name.c_str(),
			static_cast<uint32_t>(vertices.size() - optVertices.size()),
			static_cast<uint32_t>(indices.size()  - optIndices.size())
		);
#endif
		indices  = optIndices;
		vertices = optVertices;
	}
}

void S3DModelPiece::BindVertexAttribVBOs() const
{
	assert(model);
	model->BindVertexAttribs();
}

void S3DModelPiece::UnbindVertexAttribVBOs() const
{
	assert(model);
	model->UnbindVertexAttribs();
}

void S3DModelPiece::BindIndexVBO() const
{
	assert(model);
	model->BindIndexVBO();
}

void S3DModelPiece::UnbindIndexVBO() const
{
	assert(model);
	model->UnbindIndexVBO();
}

void S3DModelPiece::DrawElements(GLuint prim) const
{
	assert(model);
	model->DrawElements(prim, vboIndxStart, vboIndxStart + indices.size());
}



/** ****************************************************************************************************
 * LocalModel
 */

void LocalModel::DrawPieces() const
{
	for (const auto& p: pieces) {
		p.Draw();
	}
}

void LocalModel::DrawPiecesLOD(uint32_t lod) const
{
	if (!luaMaterialData.ValidLOD(lod))
		return;

	for (const auto& p: pieces) {
		p.DrawLOD(lod);
	}
}

void LocalModel::SetLODCount(uint32_t lodCount)
{
	assert(Initialized());

	luaMaterialData.SetLODCount(lodCount);
	pieces[0].SetLODCount(lodCount);
}


void LocalModel::SetModel(const S3DModel* model, bool initialize)
{
	// make sure we do not get called for trees, etc
	assert(model != nullptr);
	assert(model->numPieces >= 1);

	if (!initialize) {
		assert(pieces.size() == model->numPieces);

		// PostLoad; only update the pieces
		for (size_t n = 0; n < pieces.size(); n++) {
			S3DModelPiece* omp = model->GetPiece(n);

			pieces[n].original = omp;
			pieces[n].dispListID = omp->GetDisplayListID();
		}

		pieces[0].UpdateChildMatricesRec(true);
		UpdateBoundingVolume();
		return;
	}

	assert(pieces.empty());

	pieces.clear();
	pieces.reserve(model->numPieces);

	CreateLocalModelPieces(model->GetRootPiece());

	// must recursively update matrices here too: for features
	// LocalModel::Update is never called, but they might have
	// baked piece rotations (in the case of .dae)
	pieces[0].UpdateChildMatricesRec(false);
	UpdateBoundingVolume();

	assert(pieces.size() == model->numPieces);
}

LocalModelPiece* LocalModel::CreateLocalModelPieces(const S3DModelPiece* mpParent)
{
	LocalModelPiece* lmpChild = nullptr;

	// construct an LMP(mp) in-place
	pieces.emplace_back(mpParent);
	LocalModelPiece* lmpParent = &pieces.back();

	lmpParent->SetLModelPieceIndex(pieces.size() - 1);
	lmpParent->SetScriptPieceIndex(pieces.size() - 1);

	// the mapping is 1:1 for Lua scripts, but not necessarily for COB
	// CobInstance::MapScriptToModelPieces does the remapping (if any)
	assert(lmpParent->GetLModelPieceIndex() == lmpParent->GetScriptPieceIndex());

	for (const S3DModelPiece* mpChild: mpParent->children) {
		lmpChild = CreateLocalModelPieces(mpChild);
		lmpChild->SetParent(lmpParent);
		lmpParent->AddChild(lmpChild);
	}

	return lmpParent;
}


void LocalModel::UpdateBoundingVolume()
{
	// bounding-box extrema (local space)
	float3 bbMins = DEF_MIN_SIZE;
	float3 bbMaxs = DEF_MAX_SIZE;

	for (const auto& lmPiece: pieces) {
		const CMatrix44f& matrix = lmPiece.GetModelSpaceMatrix();
		const S3DModelPiece* piece = lmPiece.original;

		// skip empty pieces or bounds will not be sensible
		if (!piece->HasGeometryData())
			continue;

		// transform only the corners of the piece's bounding-box
		const float3 pMins = piece->mins;
		const float3 pMaxs = piece->maxs;
		const float3 verts[8] = {
			// bottom
			float3(pMins.x,  pMins.y,  pMins.z),
			float3(pMaxs.x,  pMins.y,  pMins.z),
			float3(pMaxs.x,  pMins.y,  pMaxs.z),
			float3(pMins.x,  pMins.y,  pMaxs.z),
			// top
			float3(pMins.x,  pMaxs.y,  pMins.z),
			float3(pMaxs.x,  pMaxs.y,  pMins.z),
			float3(pMaxs.x,  pMaxs.y,  pMaxs.z),
			float3(pMins.x,  pMaxs.y,  pMaxs.z),
		};

		for (const float3& v: verts) {
			const float3 vertex = matrix * v;

			bbMins = float3::min(bbMins, vertex);
			bbMaxs = float3::max(bbMaxs, vertex);
		}
	}

	// note: offset is relative to object->pos
	boundingVolume.InitBox(bbMaxs - bbMins, (bbMaxs + bbMins) * 0.5f);
}

void S3DModel::CreateVBOs()
{
	{
		vertVBO = std::make_unique<VBO>(GL_ARRAY_BUFFER, false);
		vertVBO->Bind();
		vertVBO->New(curVertStartIndx * sizeof(SVertexData), GL_STATIC_DRAW, nullptr);
		vertVBO->Unbind();
	}
	{
		indxVBO = std::make_unique<VBO>(GL_ELEMENT_ARRAY_BUFFER, false);
		indxVBO->Bind();
		indxVBO->New(curIndxStartIndx * sizeof(uint32_t), GL_STATIC_DRAW, nullptr);
		indxVBO->Unbind();
	}
}

/** ****************************************************************************************************
 * S3DModel
 */
void S3DModel::BindVertexAttribs() const
{
	BindVertexVBO();
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(SVertexData), vertVBO->GetPtr(offsetof(SVertexData, pos)));

		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(SVertexData), vertVBO->GetPtr(offsetof(SVertexData, normal)));

		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(SVertexData), vertVBO->GetPtr(offsetof(SVertexData, texCoords[0])));

		glClientActiveTexture(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(SVertexData), vertVBO->GetPtr(offsetof(SVertexData, texCoords[1])));

		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(SVertexData), vertVBO->GetPtr(offsetof(SVertexData, sTangent)));

		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(SVertexData), vertVBO->GetPtr(offsetof(SVertexData, tTangent)));
	UnbindVertexVBO();
}


void S3DModel::UnbindVertexAttribs() const
{
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
}

void S3DModel::BindIndexVBO() const
{
	indxVBO->Bind(GL_ELEMENT_ARRAY_BUFFER);
}

void S3DModel::UnbindIndexVBO() const
{
	assert(indxVBO);
	indxVBO->Unbind();
}

void S3DModel::BindVertexVBO() const
{
	vertVBO->Bind(GL_ARRAY_BUFFER);
}

void S3DModel::UnbindVertexVBO() const
{
	vertVBO->Unbind();
}

void S3DModel::DrawElements(GLenum prim, uint32_t vboIndxStart, uint32_t vboIndxEnd) const
{
	assert(vboIndxEnd - vboIndxStart > 0);
	glDrawElements(prim, vboIndxEnd - vboIndxStart, GL_UNSIGNED_INT, indxVBO->GetPtr(vboIndxStart * sizeof(uint32_t)));
}

void S3DModel::UploadToVBO(const std::vector<SVertexData>& vertices, const std::vector<uint32_t>& indices, const uint32_t vertStart, const uint32_t indxStart) const
{
	{
		vertVBO->Bind();
		auto* map = vertVBO->MapBuffer(vertStart * sizeof(SVertexData), vertices.size() * sizeof(SVertexData), GL_WRITE_ONLY);
		memcpy(map, vertices.data(), vertices.size() * sizeof(SVertexData));
		vertVBO->UnmapBuffer();
		vertVBO->Unbind();
	}
	{
		indxVBO->Bind();
		auto* map = indxVBO->MapBuffer(indxStart * sizeof(uint32_t), indices.size() * sizeof(uint32_t), GL_WRITE_ONLY);
		memcpy(map, indices.data(), indices.size() * sizeof(uint32_t));
		indxVBO->UnmapBuffer();
		indxVBO->Unbind();
	}

}

/** ****************************************************************************************************
 * LocalModelPiece
 */

LocalModelPiece::LocalModelPiece(const S3DModelPiece* piece)
	: colvol(piece->GetCollisionVolume())

	, dirty(true)

	, scriptSetVisible(piece->HasGeometryData())
	, blockScriptAnims(false)

	, lmodelPieceIndex(-1)
	, scriptPieceIndex(-1)

	, original(piece)
	, parent(nullptr) // set later
{
	assert(piece != nullptr);

	pos = piece->offset;
	dir = piece->GetEmitDir();

	pieceSpaceMat = std::move(CalcPieceSpaceMatrix(pos, rot, original->scales));
	dispListID = piece->GetDisplayListID();

	children.reserve(piece->children.size());
}

void LocalModelPiece::SetDirty() {
	dirty = true;

	for (LocalModelPiece* child: children) {
		if (child->dirty)
			continue;
		child->SetDirty();
	}
}

void LocalModelPiece::SetPosOrRot(const float3& src, float3& dst) {
	if (blockScriptAnims)
		return;
	if (!dirty && !dst.same(src))
		SetDirty();

	dst = src;
}


void LocalModelPiece::UpdateChildMatricesRec(bool updateChildMatrices) const
{
	if (dirty) {
		dirty = false;
		updateChildMatrices = true;

		pieceSpaceMat = CalcPieceSpaceMatrix(pos, rot, original->scales);
	}

	if (updateChildMatrices) {
		modelSpaceMat = pieceSpaceMat;

		if (parent != nullptr) {
			modelSpaceMat >>= parent->modelSpaceMat;
		}
	}

	for (auto& child : children) {
		child->UpdateChildMatricesRec(updateChildMatrices);
	}
}

void LocalModelPiece::UpdateParentMatricesRec() const
{
	if (parent != nullptr && parent->dirty)
		parent->UpdateParentMatricesRec();

	dirty = false;

	pieceSpaceMat = CalcPieceSpaceMatrix(pos, rot, original->scales);
	modelSpaceMat = pieceSpaceMat;

	if (parent != nullptr)
		modelSpaceMat >>= parent->modelSpaceMat;
}


void LocalModelPiece::Draw() const
{
	if (!scriptSetVisible)
		return;

	glPushMatrix();
	glMultMatrixf(GetModelSpaceMatrix());
	glCallList(dispListID);
	glPopMatrix();
}

void LocalModelPiece::DrawLOD(uint32_t lod) const
{
	if (!scriptSetVisible)
		return;

	glPushMatrix();
	glMultMatrixf(GetModelSpaceMatrix());
	glCallList(lodDispLists[lod]);
	glPopMatrix();
}



void LocalModelPiece::SetLODCount(uint32_t count)
{
	// any new LOD's get null-lists first
	lodDispLists.resize(count, 0);

	for (uint32_t i = 0; i < children.size(); i++) {
		children[i]->SetLODCount(count);
	}
}


bool LocalModelPiece::GetEmitDirPos(float3& emitPos, float3& emitDir) const
{
	if (original == nullptr)
		return false;

	// note: actually OBJECT_TO_WORLD but transform is the same
	emitPos = GetModelSpaceMatrix() *        original->GetEmitPos()        * WORLD_TO_OBJECT_SPACE;
	emitDir = GetModelSpaceMatrix() * float4(original->GetEmitDir(), 0.0f) * WORLD_TO_OBJECT_SPACE;
	return true;
}

/******************************************************************************/
/******************************************************************************/

void S3DModelVAO::SubmitImmediatelyImpl(const SDrawElementsIndirectCommand* scmd, const uint32_t ssboOffset, const uint32_t teamID, const GLenum mode, const bool bindUnbind)
{
	// do not increment base instance
	SInstanceData instanceData{ssboOffset, teamID};

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
		for (int i = 0; i <= 6; ++i) {
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

		glVertexAttribIPointer(6, 4, GL_UNSIGNED_INT, sizeof(SInstanceData),      (const void*)offsetof(SInstanceData, ssboOffset));
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

void S3DModelVAO::Bind()
{
	assert(vao);
	vao->Bind();
}

void S3DModelVAO::Unbind()
{
	assert(vao);
	vao->Unbind();
}

void S3DModelVAO::AddToSubmission(const CUnit* unit)
{
	const auto ssboIndex = MatrixUploader::GetInstance().GetElemOffset(unit);
	if (ssboIndex == ~0u)
		return;

	auto& renderModelData = renderDataModels[unit->model];
	renderModelData.emplace_back(SInstanceData(ssboIndex, unit->team));
}

void S3DModelVAO::AddToSubmission(const UnitDef* unitDef, const int teamID)
{
	const auto ssboIndex = MatrixUploader::GetInstance().GetElemOffset(unitDef);
	if (ssboIndex == ~0u)
		return;

	auto& renderModelData = renderDataModels[unitDef->model];
	renderModelData.emplace_back(SInstanceData(ssboIndex, teamID));
}

void S3DModelVAO::AddToSubmission(const S3DModel* model, const int teamID)
{
	const auto ssboIndex = MatrixUploader::GetInstance().GetElemOffset(model);
	if (ssboIndex == ~0u)
		return;

	auto& renderModelData = renderDataModels[model];
	renderModelData.emplace_back(SInstanceData(ssboIndex, teamID));
}

void S3DModelVAO::Submit(const GLenum mode, const bool bindUnbind)
{
	static std::vector<SDrawElementsIndirectCommand> submitCmds;
	submitCmds.clear();

	baseInstance = 0u;

	static std::vector<SInstanceData> allRenderData;
	allRenderData.reserve(INSTANCE_BUFFER_NUM_ELEMS);
	allRenderData.clear();

	//models
	for (const auto& [model, renderModelData] : renderDataModels) {
		//model
		SDrawElementsIndirectCommand scmd{
			model->indxCount,
			static_cast<uint32_t>(renderModelData.size()),
			model->indxStart,
			0u,
			baseInstance
		};

		submitCmds.emplace_back(scmd);

		allRenderData.insert(allRenderData.end(), renderModelData.cbegin(), renderModelData.cend());

		baseInstance += renderModelData.size();
	};

	//modelPieces
	for (const auto& [modelPiece, renderModelPieceData] : renderDataModelPieces) {
		//model
		SDrawElementsIndirectCommand scmd{
			modelPiece->indxCount,
			static_cast<uint32_t>(renderModelPieceData.size()),
			modelPiece->indxStart,
			0u,
			baseInstance
		};

		submitCmds.emplace_back(scmd);

		allRenderData.insert(allRenderData.end(), renderModelPieceData.cbegin(), renderModelPieceData.cend());

		baseInstance += renderModelPieceData.size();
	};
	//TODO modelPieceParts?

	renderDataModels.clear();
	renderDataModelPieces.clear();

	if (submitCmds.empty())
		return;

	instVBO->Bind();
	instVBO->SetBufferSubData(allRenderData);
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
	if (ssboIndex == ~0u)
		return;

	const auto* model = unit->model;

	SDrawElementsIndirectCommand scmd {
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
	if (ssboIndex == ~0u)
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
	if (ssboIndex == ~0u)
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
