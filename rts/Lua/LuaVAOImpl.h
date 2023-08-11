/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_VAO_IMPL_H
#define LUA_VAO_IMPL_H

#include <map>
#include <string>
#include <memory>
#include <vector>

#include "lib/sol2/forward.hpp"
#include "Rendering/GL/myGL.h"
#include "Rendering/Models/3DModelVAO.h"
#include "System/UnorderedMap.hpp"

class VAO;
class VBO;
class LuaVBOImpl;

class LuaVAOImpl {
public:
	class Bins;

	LuaVAOImpl();

	LuaVAOImpl(const LuaVAOImpl& lva) = delete;
	LuaVAOImpl(LuaVAOImpl&& lva) = default;

	void Delete();
	~LuaVAOImpl();
public:
	static bool Supported();
public:
	using LuaVBOImplSP = std::shared_ptr<LuaVBOImpl>; //my workaround to https://github.com/ThePhD/sol2/issues/1206
public:
	void AttachVertexBuffer(const LuaVBOImplSP& luaVBO);
	void AttachInstanceBuffer(const LuaVBOImplSP& luaVBO);
	void AttachIndexBuffer(const LuaVBOImplSP& luaVBO);

	void DrawArrays(GLenum mode, sol::optional<int> vertCountOpt, sol::optional<int> vertexFirstOpt, sol::optional<int> instanceCountOpt, sol::optional<int> instanceFirstOpt);
	void DrawElements(GLenum mode, sol::optional<int> indCountOpt, sol::optional<int> indElemOffsetOpt, sol::optional<int> instanceCountOpt, sol::optional<int> baseVertexOpt, sol::optional<int> instanceFirstOpt);

	void ClearSubmission();
	int AddUnitsToSubmission(int id);
	int AddUnitsToSubmission(const sol::stack_table& ids);
	int AddFeaturesToSubmission(int id);
	int AddFeaturesToSubmission(const sol::stack_table& ids);
	int AddUnitDefsToSubmission(int id);
	int AddUnitDefsToSubmission(const sol::stack_table& ids);
	int AddFeatureDefsToSubmission(int id);
	int AddFeatureDefsToSubmission(const sol::stack_table& ids);
	void RemoveFromSubmission(int idx);
	void Submit();

	void UpdateUnitBins(const sol::stack_table& removedUnits, const sol::stack_table& addedUnits, sol::optional<size_t> removedCount, sol::optional<size_t> addedCount);
	void UpdateFeatureBins(const sol::stack_table& removedFeatures, const sol::stack_table& addedFeatures, sol::optional<size_t> removedCount, sol::optional<size_t> addedCount);
	void SubmitBins();
private:
	template<typename T>
	struct DrawCheckType {
		DrawCheckType() = default;
		DrawCheckType(T drawCount_, T baseVertex_, T baseIndex_, T instCount_, T baseInstance_)
			: drawCount{ std::move(drawCount_) }
			, baseVertex{ std::move(baseVertex_) }
			, baseIndex{ std::move(baseIndex_) }
			, instCount{ std::move(instCount_) }
			, baseInstance{ std::move(baseInstance_) }
		{};
		T drawCount;
		T baseVertex;
		T baseIndex;
		T instCount;
		T baseInstance;
	};
	using DrawCheckInput  = DrawCheckType<sol::optional<int>>;
	using DrawCheckResult = DrawCheckType<int>;

	[[maybe_unused]] DrawCheckResult DrawCheck(GLenum mode, const DrawCheckInput& inputs, bool indexed);
	void CondInitVAO();
	void CheckDrawPrimitiveType(GLenum mode) const;
	void AttachBufferImpl(const std::shared_ptr<LuaVBOImpl>& luaVBO, std::shared_ptr<LuaVBOImpl>& thisLuaVBO, GLenum reqTarget);

	void EnsureBinsInit();
private:
	template <typename TObj>
	int AddObjectsToSubmissionImpl(int id);
	template <typename TObj>
	int AddObjectsToSubmissionImpl(const sol::stack_table& ids);
	template <typename TObj>
	SDrawElementsIndirectCommand DrawObjectGetCmdImpl(int id);
	template <typename TObj>
	static const SIndexAndCount GetDrawIndicesImpl(int id);
	template <typename TObj>
	static const SIndexAndCount GetDrawIndicesImpl(const TObj* obj);
private:
	std::unique_ptr<VAO> vao = nullptr;

	std::shared_ptr<LuaVBOImpl> vertLuaVBO;
	std::shared_ptr<LuaVBOImpl> instLuaVBO;
	std::shared_ptr<LuaVBOImpl> indxLuaVBO;

	uint32_t oldVertVBOId = 0;
	uint32_t oldInstVBOId = 0;
	uint32_t oldIndxVBOId = 0;

	std::unique_ptr<Bins> bins = nullptr;

	uint32_t baseInstance;
	std::vector<SDrawElementsIndirectCommand> submitCmds;
};

class LuaVAOImpl::Bins {
public:
	inline Bins(std::vector<SDrawElementsIndirectCommand>& submitCmds_)
	:	submitCmds(submitCmds_) {}

	struct Bin {
		inline Bin(int modelId) : modelId(modelId) {}

		int modelId;
		std::vector<int> objIds;
		std::vector<SInstanceData> instanceData;
	};
	std::vector<Bin> bins;

	spring::unordered_map<int, size_t> modelIdToBinIndex;
	spring::unordered_map<int, size_t> objIdToLocalInstance;

	std::vector<SInstanceData> instanceData;
	bool requireInstanceDataUpload = false;
	size_t firstChangedInstance;
	std::vector<SDrawElementsIndirectCommand>& submitCmds;

	template <typename TObj>
	void UpdateImpl(const sol::stack_table& removedObjects, const sol::stack_table& addedObjects, sol::optional<size_t> removedCount, sol::optional<size_t> addedCount);
};

#endif //LUA_VAO_IMPL_H