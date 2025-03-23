/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DModel.h"

#include "3DModelVAO.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"

#include "System/Log/ILog.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "System/Misc/TracyDefs.h"

CR_BIND(LocalModelPiece, (nullptr))
CR_REG_METADATA(LocalModelPiece, (
	CR_MEMBER(prevModelSpaceTra),
	CR_MEMBER(pos),
	CR_MEMBER(rot),
	CR_MEMBER(dir),
	CR_MEMBER(colvol),
	CR_MEMBER(scriptSetVisible),
	CR_MEMBER(blockScriptAnims),
	CR_MEMBER(lmodelPieceIndex),
	CR_MEMBER(scriptPieceIndex),
	CR_MEMBER(parent),
	CR_MEMBER(localModel),
	CR_MEMBER(children),

	// reload
	CR_IGNORED(original),

	CR_IGNORED(dirty),
	CR_IGNORED(modelSpaceTra),
	CR_IGNORED(pieceSpaceTra),

	CR_IGNORED(lodDispLists) //FIXME GL idx!
))

CR_BIND(LocalModel, )
CR_REG_METADATA(LocalModel, (
	CR_MEMBER(pieces),

	CR_MEMBER(boundingVolume),
	CR_IGNORED(luaMaterialData),
	CR_MEMBER(needsBoundariesRecalc)
))

static_assert(sizeof(SVertexData) == (3 + 3 + 3 + 3 + 4 + 2 + 1) * 4);

void S3DModelHelpers::BindLegacyAttrVBOs()
{
	RECOIL_DETAILED_TRACY_ZONE;
	S3DModelVAO::GetInstance().BindLegacyVertexAttribsAndVBOs();
}
void S3DModelHelpers::UnbindLegacyAttrVBOs()
{
	RECOIL_DETAILED_TRACY_ZONE;
	S3DModelVAO::GetInstance().UnbindLegacyVertexAttribsAndVBOs();
}

/** ****************************************************************************************************
 * S3DModelPiece
 */

void S3DModelPiece::DrawStaticLegacy(bool bind, bool bindPosMat) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasGeometryData())
		return;

	if (bind) S3DModelHelpers::BindLegacyAttrVBOs();

	if (bindPosMat) {
		glPushMatrix();
		glMultMatrixf(bposeTransform.ToMatrix());
		DrawElements();
		glPopMatrix();
	}
	else {
		DrawElements();
	}

	if (bind) S3DModelHelpers::UnbindLegacyAttrVBOs();
}

// only used by projectiles with the PF_Recursive flag
void S3DModelPiece::DrawStaticLegacyRec() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	S3DModelHelpers::BindLegacyAttrVBOs();

	DrawStaticLegacy(false, false);

	for (const S3DModelPiece* childPiece : children) {
		childPiece->DrawStaticLegacy(false, false);
	}

	S3DModelHelpers::UnbindLegacyAttrVBOs();
}


float3 S3DModelPiece::GetEmitPos() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (vertices.size()) {
		case 0:
		case 1: { return ZeroVector; } break;
		default: { return GetVertexPos(0); } break;
	}
}

float3 S3DModelPiece::GetEmitDir() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (vertices.size()) {
		case 0: { return FwdVector; } break;
		case 1: { return GetVertexPos(0); } break;
		default: { return (GetVertexPos(1) - GetVertexPos(0)); } break;
	}
}


void S3DModelPiece::CreateShatterPieces()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasGeometryData())
		return;

	shatterIndices.reserve(S3DModelPiecePart::SHATTER_VARIATIONS * indices.size());

	for (int i = 0; i < S3DModelPiecePart::SHATTER_VARIATIONS; ++i) {
		CreateShatterPiecesVariation(i);
	}
}


void S3DModelPiece::CreateShatterPiecesVariation(int num)
{
	RECOIL_DETAILED_TRACY_ZONE;
	using ShatterPartDataPair = std::pair<S3DModelPiecePart::RenderData, std::vector<uint32_t>>;
	using ShatterPartsBuffer  = std::array<ShatterPartDataPair, S3DModelPiecePart::SHATTER_MAX_PARTS>;

	ShatterPartsBuffer shatterPartsBuf;

	for (auto& [rd, idcs] : shatterPartsBuf) {
		rd.dir = (guRNG.NextVector()).ANormalize();
	}

	// helper
	const auto GetPolygonDir = [&](size_t idx)
	{
		float3 midPos;
		midPos += GetVertexPos(indices[idx + 0]);
		midPos += GetVertexPos(indices[idx + 1]);
		midPos += GetVertexPos(indices[idx + 2]);
		midPos /= 3.0f;
		return midPos.ANormalize();
	};

	// add vertices to splitter parts
	for (size_t i = 0; i < indices.size(); i += 3) {
		const float3& dir = GetPolygonDir(i);

		// find the closest shatter part (the one that points into same dir)
		float md = -2.0f;

		ShatterPartDataPair* mcp = nullptr;
		const S3DModelPiecePart::RenderData* rd = nullptr;

		for (ShatterPartDataPair& cp: shatterPartsBuf) {
			rd = &cp.first;

			if (rd->dir.dot(dir) < md)
				continue;

			md = rd->dir.dot(dir);
			mcp = &cp;
		}

		assert(mcp);

		//  + vertIndex will be added in void S3DModelVAO::ProcessIndicies(S3DModel* model)
		(mcp->second).push_back(indices[i + 0]);
		(mcp->second).push_back(indices[i + 1]);
		(mcp->second).push_back(indices[i + 2]);
	}

	{
		const size_t mapSize = indices.size();

		uint32_t indxPos = 0;

		for (auto& [rd, idcs] : shatterPartsBuf) {
			rd.indexCount = static_cast<uint32_t>(idcs.size());
			rd.indexStart = static_cast<uint32_t>(num * mapSize) + indxPos;

			if (rd.indexCount > 0) {
				shatterIndices.insert(shatterIndices.end(), idcs.begin(), idcs.end());
				indxPos += rd.indexCount;
			}
		}
	}

	{
		// delete empty splitter parts
		size_t backIdx = shatterPartsBuf.size() - 1;

		for (size_t j = 0; j < shatterPartsBuf.size() && j < backIdx; ) {
			const auto& [rd, idcs] = shatterPartsBuf[j];

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
	RECOIL_DETAILED_TRACY_ZONE;
	const float2  pieceParams = {float3::max(float3::fabs(maxs), float3::fabs(mins)).Length(), pieceChance};
	const   int2 renderParams = {texType, team};

	projectileHandler.AddFlyingPiece(modelType, this, m, pos, speed, pieceParams, renderParams);
}

void S3DModelPiece::SetPieceTransform(const Transform& parentTra)
{
	bposeTransform = parentTra * Transform{
		CQuaternion(),
		offset,
		scale
	};

	for (S3DModelPiece* c : children) {
		c->SetPieceTransform(bposeTransform);
	}
}

Transform S3DModelPiece::ComposeTransform(const float3& t, const float3& r, float s) const
{
	// NOTE:
	//   ORDER MATTERS (T(baked + script) * R(baked) * R(script) * S(baked))
	//   translating + rotating + scaling is faster than matrix-multiplying
	//   m is identity so m.SetPos(t)==m.Translate(t) but with fewer instrs
	Transform tra;
	tra.t = t;

	if (hasBakedTra)
		tra *= bakedTransform;

	tra *= Transform(CQuaternion::FromEulerYPRNeg(-r), ZeroVector, s);
	return tra;
}


void S3DModelPiece::PostProcessGeometry(uint32_t pieceIndex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasGeometryData())
		return;


	for (auto& v : vertices) {
		if (v.boneIDsLow == SVertexData::DEFAULT_BONEIDS_LOW && v.boneIDsHigh == SVertexData::DEFAULT_BONEIDS_HIGH) {
			v.boneIDsLow [0] = static_cast<uint8_t>((pieceIndex     ) & 0xFF);
			v.boneIDsHigh[0] = static_cast<uint8_t>((pieceIndex >> 8) & 0xFF);
		}
	}
}

void S3DModelPiece::DrawElements(GLuint prim) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (indxCount == 0)
		return;
	assert(indxCount != ~0u);

	S3DModelVAO::GetInstance().DrawElements(prim, indxStart, indxCount);
}

void S3DModelPiece::DrawShatterElements(uint32_t vboIndxStart, uint32_t vboIndxCount, GLuint prim)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (vboIndxCount == 0)
		return;

	S3DModelVAO::GetInstance().DrawElements(prim, vboIndxStart, vboIndxCount);
}

void S3DModelPiece::ReleaseShatterIndices()
{
	RECOIL_DETAILED_TRACY_ZONE;
	shatterIndices.clear();
}

/** ****************************************************************************************************
 * LocalModel
 */

void LocalModel::DrawPieces() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (const auto& p : pieces) {
		p.Draw();
	}
}

void LocalModel::DrawPiecesLOD(uint32_t lod) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!luaMaterialData.ValidLOD(lod))
		return;

	for (const auto& p: pieces) {
		p.DrawLOD(lod);
	}
}

void LocalModel::SetLODCount(uint32_t lodCount)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(Initialized());

	luaMaterialData.SetLODCount(lodCount);
	pieces[0].SetLODCount(lodCount);
}


void LocalModel::SetModel(const S3DModel* model, bool initialize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// make sure we do not get called for trees, etc
	assert(model != nullptr);
	assert(model->numPieces >= 1);

	if (!initialize) {
		assert(pieces.size() == model->numPieces);

		// PostLoad; only update the pieces
		for (size_t n = 0; n < pieces.size(); n++) {
			S3DModelPiece* omp = model->GetPiece(n);

			pieces[n].original = omp;
		}

		pieces[0].UpdateChildTransformRec(true);
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
	pieces[0].UpdateChildTransformRec(false);

	for (auto& piece : pieces) {
		piece.SavePrevModelSpaceTransform();
	}

	UpdateBoundingVolume();

	assert(pieces.size() == model->numPieces);
}

LocalModelPiece* LocalModel::CreateLocalModelPieces(const S3DModelPiece* mpParent)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LocalModelPiece* lmpChild = nullptr;

	// construct an LMP(mp) in-place
	pieces.emplace_back(mpParent);
	LocalModelPiece* lmpParent = &pieces.back();

	lmpParent->SetLModelPieceIndex(pieces.size() - 1);
	lmpParent->SetScriptPieceIndex(pieces.size() - 1);
	lmpParent->SetLocalModel(this);

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
	ZoneScoped;

	// bounding-box extrema (local space)
	float3 bbMins = DEF_MIN_SIZE;
	float3 bbMaxs = DEF_MAX_SIZE;

	for (const auto& lmPiece: pieces) {
		const auto& tra = lmPiece.GetModelSpaceTransform();
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
			const float3 vertex = tra * v;

			bbMins = float3::min(bbMins, vertex);
			bbMaxs = float3::max(bbMaxs, vertex);
		}
	}

	// note: offset is relative to object->pos
	boundingVolume.InitBox(bbMaxs - bbMins, (bbMaxs + bbMins) * 0.5f);

	needsBoundariesRecalc = false;
}

/** ****************************************************************************************************
 * LocalModelPiece
 */

LocalModelPiece::LocalModelPiece(const S3DModelPiece* piece)
	: colvol(piece->GetCollisionVolume())
	, dirty(true)
	, wasUpdated(true)

	, scriptSetVisible(true)
	, blockScriptAnims(false)

	, lmodelPieceIndex(-1)
	, scriptPieceIndex(-1)

	, original(piece)
	, parent(nullptr) // set later
{
	assert(piece != nullptr);

	pos = piece->offset;
	dir = piece->GetEmitDir(); // warning investigated, seems fake

	pieceSpaceTra = CalcPieceSpaceTransform(pos, rot, original->scale);
	prevModelSpaceTra = Transform{ };

	children.reserve(piece->children.size());
}

void LocalModelPiece::SetDirty() {
	RECOIL_DETAILED_TRACY_ZONE;
	dirty = true;

	for (LocalModelPiece* child: children) {
		if (child->dirty)
			continue;
		child->SetDirty();
	}
}

void LocalModelPiece::SetPosOrRot(const float3& src, float3& dst) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (blockScriptAnims)
		return;
	if (!dirty && !dst.same(src)) {
		SetDirty();
		assert(localModel);
		localModel->SetBoundariesNeedsRecalc();
	}

	dst = src;
}

void LocalModelPiece::SetScriptVisible(bool b)
{
	scriptSetVisible = b;
	wasUpdated = true;
}

void LocalModelPiece::UpdateChildTransformRec(bool updateChildTransform) const
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (dirty) {
		dirty = false;
		wasUpdated = true;
		updateChildTransform = true;

		pieceSpaceTra = CalcPieceSpaceTransform(pos, rot, original->scale);
	}

	if (updateChildTransform) {
		if (parent != nullptr)
			modelSpaceTra = parent->modelSpaceTra * pieceSpaceTra;
		else
			modelSpaceTra = pieceSpaceTra;

		modelSpaceMat = modelSpaceTra.ToMatrix();
	}

	for (auto& child : children) {
		child->UpdateChildTransformRec(updateChildTransform);
	}
}

void LocalModelPiece::UpdateParentMatricesRec() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (parent != nullptr && parent->dirty)
		parent->UpdateParentMatricesRec();

	dirty = false;
	wasUpdated = true;

	pieceSpaceTra = CalcPieceSpaceTransform(pos, rot, original->scale);

	if (parent != nullptr)
		modelSpaceTra = parent->modelSpaceTra * pieceSpaceTra;
	else
		modelSpaceTra = pieceSpaceTra;

	modelSpaceMat = modelSpaceTra.ToMatrix();
}


void LocalModelPiece::Draw() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!scriptSetVisible)
		return;

	if (!original->HasGeometryData())
		return;

	assert(original);

	glPushMatrix();
	glMultMatrixf(GetModelSpaceMatrix());
	S3DModelHelpers::BindLegacyAttrVBOs();
	original->DrawElements();
	S3DModelHelpers::UnbindLegacyAttrVBOs();
	glPopMatrix();
}

void LocalModelPiece::DrawLOD(uint32_t lod) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!scriptSetVisible)
		return;

	if (!original->HasGeometryData())
		return;

	glPushMatrix();
	glMultMatrixf(GetModelSpaceMatrix());
	if (const auto ldl = lodDispLists[lod]; ldl == 0) {
		S3DModelHelpers::BindLegacyAttrVBOs();
		original->DrawElements();
		S3DModelHelpers::UnbindLegacyAttrVBOs();
	} else {
		glCallList(ldl);
	}
	glPopMatrix();
}



void LocalModelPiece::SetLODCount(uint32_t count)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// any new LOD's get null-lists first
	lodDispLists.resize(count, 0);

	for (uint32_t i = 0; i < children.size(); i++) {
		children[i]->SetLODCount(count);
	}
}


bool LocalModelPiece::GetEmitDirPos(float3& emitPos, float3& emitDir) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (original == nullptr)
		return false;

	// note: actually OBJECT_TO_WORLD but transform is the same
	emitPos = GetModelSpaceTransform() *        original->GetEmitPos()        * WORLD_TO_OBJECT_SPACE;
	emitDir = GetModelSpaceTransform() * float4(original->GetEmitDir(), 0.0f) * WORLD_TO_OBJECT_SPACE;
	return true;
}

/******************************************************************************/
/******************************************************************************/

S3DModelPiece* S3DModel::FindPiece(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = std::find_if(pieceObjects.begin(), pieceObjects.end(), [&name](const S3DModelPiece* piece) {
		return piece->name == name;
	});
	if (it == pieceObjects.end())
		return nullptr;

	return *it;
}

const S3DModelPiece* S3DModel::FindPiece(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = std::find_if(pieceObjects.begin(), pieceObjects.end(), [&name](const S3DModelPiece* piece) {
		return piece->name == name;
		});
	if (it == pieceObjects.end())
		return nullptr;

	return *it;
}

size_t S3DModel::FindPieceOffset(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = std::find_if(pieceObjects.begin(), pieceObjects.end(), [&name](const S3DModelPiece* piece) {
		return piece->name == name;
	});

	if (it == pieceObjects.end())
		return size_t(-1);

	return std::distance(pieceObjects.begin(), it);
}

void S3DModel::SetPieceMatrices()
{
	pieceObjects[0]->SetPieceTransform(Transform());

	// use this occasion and copy bpose matrices
	for (size_t i = 0; i < pieceObjects.size(); ++i) {
		const auto* po = pieceObjects[i];
		traAlloc.UpdateForced(i, po->bposeTransform);
	}
}
