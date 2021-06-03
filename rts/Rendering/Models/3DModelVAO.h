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

	bool AddToSubmission(const S3DModel* model, int teamID);
	bool AddToSubmission(const CUnit* unit);
	bool AddToSubmission(const UnitDef* unitDef, int teamID);
	void Submit(GLenum mode = GL_TRIANGLES, bool bindUnbind = false);

	bool SubmitImmediately(const S3DModel* model, int teamID, GLenum mode = GL_TRIANGLES, bool bindUnbind = false);
	bool SubmitImmediately(const CUnit* unit, GLenum mode = GL_TRIANGLES, bool bindUnbind = false);
	bool SubmitImmediately(const UnitDef* unitDef, int teamID, GLenum mode = GL_TRIANGLES, bool bindUnbind = false);

	const VBO* GetVertVBO() const { return &vertVBO; }
	      VBO* GetVertVBO()       { return &vertVBO; }
	const VBO* GetIndxVBO() const { return &indxVBO; }
	      VBO* GetIndxVBO()       { return &indxVBO; }
private:
	template<typename TObj>
	bool SubmitImmediatelyImpl(
		const TObj* obj,
		uint32_t indexStart,
		uint32_t indexCount,
		uint32_t teamID,
		GLenum mode = GL_TRIANGLES,
		bool bindUnbind = false
	);
	template<typename TObj>
	bool AddToSubmissionImpl(
		const TObj* obj,
		uint32_t indexStart,
		uint32_t indexCount,
		uint32_t teamID
	);
	void EnableAttribs(bool inst) const;
	void DisableAttribs() const;
private:
	uint32_t baseInstance = 0u;

	VBO vertVBO;
	VBO indxVBO;

	VBO instVBO;
	VAO vao;

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