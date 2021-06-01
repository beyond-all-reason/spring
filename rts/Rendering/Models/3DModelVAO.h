/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <memory>

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"

struct S3DModel;
struct S3DModelPiece;
struct SInstanceData;

struct CUnit;
struct UnitDef;

struct SDrawElementsIndirectCommand;

// singleton
class S3DModelVAO {
public:
	static S3DModelVAO& GetInstance() {
		static S3DModelVAO instance;
		return instance;
	};
public:
	static constexpr size_t INSTANCE_BUFFER_NUM_ELEMS = 2 << 15;
public:
	S3DModelVAO() = default;

	void Init();

	void Bind() const;
	void Unbind() const;

	void AddToSubmission(const CUnit* unit);
	void AddToSubmission(const UnitDef* unitDef, const int teamID);
	void AddToSubmission(const S3DModel* model, const int teamID);
	void Submit(const GLenum mode = GL_TRIANGLES, const bool bindUnbind = false);

	void SubmitImmediately(const CUnit* unit, const GLenum mode = GL_TRIANGLES, const bool bindUnbind = false);
	void SubmitImmediately(const UnitDef* unitDef, const int teamID, const GLenum mode = GL_TRIANGLES, const bool bindUnbind = false);
	void SubmitImmediately(const S3DModel* model, const int teamID, const GLenum mode = GL_TRIANGLES, const bool bindUnbind = false);

	const VBO* GetVertVBO() const { return vertVBO.get(); }
	      VBO* GetVertVBO()       { return vertVBO.get(); }
	const VBO* GetIndxVBO() const { return indxVBO.get(); }
	      VBO* GetIndxVBO()       { return indxVBO.get(); }
private:
	void SubmitImmediatelyImpl(
		const SDrawElementsIndirectCommand* scmd,
		const uint32_t ssboOffset,
		const uint32_t teamID,
		const GLenum mode = GL_TRIANGLES,
		const bool bindUnbind = false
	);
	void EnableAttribs(bool inst) const;
	void DisableAttribs() const;
private:
	uint32_t baseInstance = 0u;

	std::unique_ptr<VBO> vertVBO;
	std::unique_ptr<VBO> indxVBO;

	std::unique_ptr<VBO> instVBO;
	std::unique_ptr<VAO> vao;

	std::unordered_map<const S3DModel*, std::vector<SInstanceData>> renderDataModels;
	std::unordered_map<const S3DModelPiece*, std::vector<SInstanceData>> renderDataModelPieces;
};

struct SDrawElementsIndirectCommand {
	SDrawElementsIndirectCommand(uint32_t indexCount_, uint32_t instanceCount_, uint32_t firstIndex_, uint32_t baseVertex_, uint32_t baseInstance_)
		: indexCount{ indexCount_ }
		, instanceCount{ instanceCount_ }
		, firstIndex{ firstIndex_ }
		, baseVertex{ baseVertex_ }
		, baseInstance{ baseInstance_ }
	{};

	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	uint32_t baseVertex;
	uint32_t baseInstance;
};

struct SInstanceData {
	SInstanceData() = default;
	SInstanceData(uint32_t ssboOffset_, uint32_t teamIndex_, uint64_t piecesMask_ = uint64_t(-1))
		: ssboOffset{ ssboOffset_ }
		, teamIndex{ teamIndex_ }
		, piecesMask{ piecesMask_ }
	{}

	uint32_t ssboOffset;
	uint32_t teamIndex;
	uint64_t piecesMask; //support up to 64 pieces
};