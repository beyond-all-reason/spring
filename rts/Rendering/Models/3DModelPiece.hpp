#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <optional>

#include "3DModelDefs.hpp"
#include "3DModelMisc.hpp"
#include "VertexData.hpp"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Matrix44f.h"
#include "System/Transform.hpp"

struct S3DModel;
struct S3DModelPiece {
	S3DModelPiece() = default;
	virtual ~S3DModelPiece() = default;

	virtual void Clear() {
		name.clear();
		children.clear();

		for (S3DModelPiecePart& p : shatterParts) {
			p.renderData.clear();
		}

		vertices.clear();
		indices.clear();
		shatterIndices.clear();

		parent = nullptr;
		colvol = {};

		bposeTransform.LoadIdentity();

		offset = ZeroVector;
		goffset = ZeroVector;
		scale = 1.0f;

		mins = DEF_MIN_SIZE;
		maxs = DEF_MAX_SIZE;

		vertIndex = ~0u;
		indxStart = ~0u;
		indxCount = ~0u;
	}

	virtual float3 GetEmitPos() const;
	virtual float3 GetEmitDir() const;

	// internal use
	const float3& GetVertexPos(const int idx) const { return vertices[idx].pos; }
	const float3& GetNormal(const int idx) const { return vertices[idx].normal; }

	virtual void PostProcessGeometry(uint32_t pieceIndex);


	void DrawElements(uint32_t prim = 0x0004/*GL_TRIANGLES*/) const;
	static void DrawShatterElements(uint32_t vboIndxStart, uint32_t vboIndxCount, uint32_t prim = 0x0004/*GL_TRIANGLES*/);
public:
	void DrawStaticLegacy(bool bind, bool bindPosMat) const;
	void DrawStaticLegacyRec() const;

	void CreateShatterPieces();
	void Shatter(float, int, int, int, const float3, const float3, const CMatrix44f&) const;

	void SetPieceTransform(const Transform& parentTra);
	void SetBakedTransform(const Transform& tra) {
		if (tra.IsIdentity())
			bakedTransform = std::nullopt;
		else
			bakedTransform = tra;
	}

	Transform ComposeTransform(const float3& t, const float3& r, float s) const;

	void SetCollisionVolume(const CollisionVolume& cv) { colvol = cv; }
	const CollisionVolume* GetCollisionVolume() const { return &colvol; }
	      CollisionVolume* GetCollisionVolume()       { return &colvol; }

	bool HasGeometryData() const { return indices.size() >= 3; }
	void SetParentModel(S3DModel* model_) { model = model_; }
	const S3DModel* GetParentModel() const { return model; }

	void ReleaseShatterIndices();

	const std::vector<SVertexData>& GetVerticesVec() const { return vertices; }
	const std::vector<uint32_t>& GetIndicesVec() const { return indices; }
	const std::vector<uint32_t>& GetShatterIndicesVec() const { return shatterIndices; }

	std::vector<SVertexData>& GetVerticesVec() { return vertices; }
	std::vector<uint32_t>& GetIndicesVec() { return indices; }
	std::vector<uint32_t>& GetShatterIndicesVec() { return shatterIndices; }

	bool HasBackedTra() const { return bakedTransform.has_value(); }
private:
	void CreateShatterPiecesVariation(int num);
	void DrawStaticLegacyRecImpl(const float3& rootT) const;
public:
	std::string name;
	std::vector<S3DModelPiece*> children;
	std::array<S3DModelPiecePart, S3DModelPiecePart::SHATTER_VARIATIONS> shatterParts;

	S3DModelPiece* parent = nullptr;
	CollisionVolume colvol;

	// bind-pose transform, including baked rots
	Transform bposeTransform;

	// baked local-space rotations
	std::optional<Transform> bakedTransform;

	float3 offset;      /// local (piece-space) offset wrt. parent piece
	float3 goffset;     /// global (model-space) offset wrt. root piece
	float scale{1.0f};  /// baked uniform scaling factor (assimp-only)

	float3 mins = DEF_MIN_SIZE;
	float3 maxs = DEF_MAX_SIZE;

	uint32_t vertIndex = ~0u; // global vertex number offset
	uint32_t indxStart = ~0u; // global Index VBO offset
	uint32_t indxCount = ~0u;
protected:
	std::vector<SVertexData> vertices;
	std::vector<uint32_t> indices;
	std::vector<uint32_t> shatterIndices;

	S3DModel* model;
public:
	friend class CAssParser;
};