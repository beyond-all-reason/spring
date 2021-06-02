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

	struct IndexCount {
		IndexCount(uint32_t index_, uint32_t count_)
			: index{ index_ }
			, count{ count_ }
		{}
		bool operator==(const IndexCount& o) const { return index == o.index && count == o.count; }
		uint32_t index;
		uint32_t count;
	};

	struct IndexCountHash {
		size_t operator()(const IndexCount& p) const {
			return (uint64_t)p.index << 32 | p.count;
		}
	};

	std::unordered_map<IndexCount, std::vector<SInstanceData>, IndexCountHash> modelDataToInstance;
};