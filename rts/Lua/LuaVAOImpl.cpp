#include "LuaVAOImpl.h"

#include <algorithm>
#include <type_traits>

#include "lib/fmt/format.h"
#include "lib/fmt/printf.h"

#include "lib/sol2/sol.hpp"

#include "System/SafeUtil.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"
#include "LuaVBOImpl.h"

#include "LuaUtils.h"

/***
 * Vertex Array Object
 * 
 * @class VAO
 * @table VAO
 * @see LuaVAO.GetVAO
 * @see rts/Lua/LuaVAOImpl.cpp
 */


LuaVAOImpl::LuaVAOImpl()
	: vao{nullptr}

	, vertLuaVBO{nullptr}
	, instLuaVBO{nullptr}
	, indxLuaVBO{nullptr}

	, baseInstance{0u}
{}


/***
 *
 * @function VAO:Delete
 * @return nil
 */
void LuaVAOImpl::Delete()
{
	vertLuaVBO = nullptr;
	instLuaVBO = nullptr;
	indxLuaVBO = nullptr;

	vao = nullptr;
}

LuaVAOImpl::~LuaVAOImpl()
{
	Delete();
}

bool LuaVAOImpl::Supported()
{
	static bool supported = VBO::IsSupported(GL_ARRAY_BUFFER) && VAO::IsSupported() && GLAD_GL_ARB_instanced_arrays && GLAD_GL_ARB_draw_elements_base_vertex && GLAD_GL_ARB_multi_draw_indirect;
	return supported;
}


void LuaVAOImpl::AttachBufferImpl(const std::shared_ptr<LuaVBOImpl>& luaVBO, std::shared_ptr<LuaVBOImpl>& thisLuaVBO, GLenum reqTarget)
{
	if (thisLuaVBO) {
		LuaUtils::SolLuaError("[LuaVAOImpl::%s] LuaVBO already attached", __func__);
	}

	if (luaVBO->defTarget != reqTarget) {
		LuaUtils::SolLuaError("[LuaVAOImpl::%s] LuaVBO should have been created with [%u] target, got [%u] target instead", __func__, reqTarget, luaVBO->defTarget);
	}

	if (!luaVBO->vbo) {
		LuaUtils::SolLuaError("[LuaVAOImpl::%s] LuaVBO is invalid. Did you sucessfully call vbo:Define()?", __func__);
	}

	thisLuaVBO = luaVBO;

	if (vertLuaVBO && instLuaVBO) {
		for (const auto& v : vertLuaVBO->bufferAttribDefs) {
			for (const auto& i : instLuaVBO->bufferAttribDefs) {
				if (v.first == i.first) {
					LuaUtils::SolLuaError("[LuaVAOImpl::%s] Vertex and Instance LuaVBO have defined a duplicate attribute [%d]", __func__, v.first);
				}
			}
		}
	}
}


/*** Attaches a VBO to be used as a vertex buffer
 *
 * @function VAO:AttachVertexBuffer
 * @param vbo VBO
 * @return nil
 */
void LuaVAOImpl::AttachVertexBuffer(const LuaVBOImplSP& luaVBO)
{
	AttachBufferImpl(luaVBO, vertLuaVBO, GL_ARRAY_BUFFER);
}


/*** Attaches a VBO to be used as an instance buffer
 *
 * @function VAO:AttachInstanceBuffer
 * @param vbo VBO
 * @return nil
 */
void LuaVAOImpl::AttachInstanceBuffer(const LuaVBOImplSP& luaVBO)
{
	AttachBufferImpl(luaVBO, instLuaVBO, GL_ARRAY_BUFFER);
}


/*** Attaches a VBO to be used as an index buffer
 *
 * @function VAO:AttachIndexBuffer
 * @param vbo VBO
 * @return nil
 */
void LuaVAOImpl::AttachIndexBuffer(const LuaVBOImplSP& luaVBO)
{
	AttachBufferImpl(luaVBO, indxLuaVBO, GL_ELEMENT_ARRAY_BUFFER);
}

template<typename TObj>
const SIndexAndCount LuaVAOImpl::GetDrawIndicesImpl(int id)
{
	const TObj* obj = LuaUtils::SolIdToObject<TObj>(id, __func__); //wrong ids are handles in LuaUtils::SolIdToObject<>()
	return GetDrawIndicesImpl<TObj>(obj);
}

template<typename TObj>
const SIndexAndCount LuaVAOImpl::GetDrawIndicesImpl(const TObj* obj)
{
	static_assert(std::is_base_of_v<CSolidObject, TObj> || std::is_base_of_v<SolidObjectDef, TObj>);

	S3DModel* model = obj->model;
	assert(model);
	return SIndexAndCount(model->indxStart, model->indxCount);
}

template<typename TObj>
int LuaVAOImpl::AddObjectsToSubmissionImpl(int id)
{
	DrawCheckInput inputs{
		std::nullopt,
		std::nullopt,
		std::nullopt,
		static_cast<int>(submitCmds.size() + 1),
		std::nullopt
	};

	DrawCheck(GL_TRIANGLES, inputs, true);
	submitCmds.emplace_back(DrawObjectGetCmdImpl<TObj>(id));

	return submitCmds.size() - 1;
}

template<typename TObj>
int LuaVAOImpl::AddObjectsToSubmissionImpl(const sol::stack_table& ids)
{
	const std::size_t idsSize = ids.size(); //size() is very costly to do in the loop

	DrawCheckInput inputs{
		std::nullopt,
		std::nullopt,
		std::nullopt,
		static_cast<int>(submitCmds.size() + idsSize),
		std::nullopt
	};

	DrawCheck(GL_TRIANGLES, inputs, true);

	constexpr auto defaultValue = static_cast<lua_Number>(0);
	for (std::size_t i = 0u; i < idsSize; ++i) {
		lua_Number idLua = ids.raw_get_or<lua_Number>(i + 1, defaultValue);
		int id = spring::SafeCast<int, lua_Number>(idLua);

		submitCmds.emplace_back(DrawObjectGetCmdImpl<TObj>(id));
	}

	return submitCmds.size() - idsSize;
}

template<typename TObj>
SDrawElementsIndirectCommand LuaVAOImpl::DrawObjectGetCmdImpl(int id)
{
	const auto& indexAndCount = LuaVAOImpl::GetDrawIndicesImpl<TObj>(id);

	return SDrawElementsIndirectCommand {
		indexAndCount.count,
		1u,
		indexAndCount.index,
		0u,
		baseInstance++
	};
}

void LuaVAOImpl::CheckDrawPrimitiveType(GLenum mode) const
{
	switch (mode) {
	case GL_POINTS:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
	case GL_LINES:
	case GL_LINE_STRIP_ADJACENCY:
	case GL_LINES_ADJACENCY:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP_ADJACENCY:
	case GL_TRIANGLES_ADJACENCY:
	case GL_PATCHES:
		break;
	default:
		LuaUtils::SolLuaError("[LuaVAOImpl::%s]: Using deprecated or incorrect primitive type (%d)", __func__, mode);
	}
}

void LuaVAOImpl::CondInitVAO()
{
	if (vao &&
		(vertLuaVBO && vertLuaVBO->GetId() == oldVertVBOId) &&
		(indxLuaVBO && indxLuaVBO->GetId() == oldIndxVBOId) &&
		(instLuaVBO && instLuaVBO->GetId() == oldInstVBOId))
		return; //already init and all VBOs still have same IDs

	vao = nullptr;
	vao = std::make_unique<VAO>();
	vao->Bind();

	if (vertLuaVBO) {
		vertLuaVBO->vbo->Bind(GL_ARRAY_BUFFER); //type is needed cause same buffer could have been rebounded as something else using LuaVBOs functions
		oldVertVBOId = vertLuaVBO->GetId();
	}

	if (indxLuaVBO) {
		indxLuaVBO->vbo->Bind(GL_ELEMENT_ARRAY_BUFFER);
		oldIndxVBOId = indxLuaVBO->GetId();
	}

	#define INT2PTR(x) (reinterpret_cast<void*>(static_cast<intptr_t>(x)))

	GLenum indMin = ~0u;
	GLenum indMax =  0u;

	const auto glVertexAttribPointerFunc = [](GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) {
		if (type == GL_FLOAT || normalized)
			glVertexAttribPointer(index, size, type, normalized, stride, pointer);
		else //assume int types
			glVertexAttribIPointer(index, size, type, stride, pointer);
	};

	if (vertLuaVBO)
		for (const auto& va : vertLuaVBO->bufferAttribDefsVec) {
			const auto& attr = va.second;
			glEnableVertexAttribArray(va.first);
			glVertexAttribPointerFunc(va.first, attr.size, attr.type, attr.normalized, vertLuaVBO->elemSizeInBytes, INT2PTR(attr.pointer));
			glVertexAttribDivisor(va.first, 0);
			indMin = std::min(indMin, static_cast<GLenum>(va.first));
			indMax = std::max(indMax, static_cast<GLenum>(va.first));
		}

	if (instLuaVBO) {
		if (vertLuaVBO)
			vertLuaVBO->vbo->Unbind();

		instLuaVBO->vbo->Bind(GL_ARRAY_BUFFER);
		oldInstVBOId = instLuaVBO->GetId();

		for (const auto& va : instLuaVBO->bufferAttribDefsVec) {
			const auto& attr = va.second;
			glEnableVertexAttribArray(va.first);
			glVertexAttribPointerFunc(va.first, attr.size, attr.type, attr.normalized, instLuaVBO->elemSizeInBytes, INT2PTR(attr.pointer));
			glVertexAttribDivisor(va.first, 1);
			indMin = std::min(indMin, static_cast<GLenum>(va.first));
			indMax = std::max(indMax, static_cast<GLenum>(va.first));
		}
	}

	#undef INT2PTR

	vao->Unbind();

	if (vertLuaVBO && vertLuaVBO->vbo->bound)
		vertLuaVBO->vbo->Unbind();

	if (instLuaVBO && instLuaVBO->vbo->bound)
		instLuaVBO->vbo->Unbind();

	if (indxLuaVBO && indxLuaVBO->vbo->bound)
		indxLuaVBO->vbo->Unbind();

	//restore default state
	for (GLuint index = indMin; index <= indMax; ++index) {
		glVertexAttribDivisor(index, 0);
		glDisableVertexAttribArray(index);
	}
}

LuaVAOImpl::DrawCheckResult LuaVAOImpl::DrawCheck(GLenum mode, const DrawCheckInput& inputs, bool indexed)
{
	LuaVAOImpl::DrawCheckResult result{};

	if (vertLuaVBO)
		vertLuaVBO->UpdateModelsVBOElementCount(); //need to update elements count because underlyiing VBO could have been updated

	if (indexed) {
		if (!indxLuaVBO)
			LuaUtils::SolLuaError("[LuaVAOImpl::%s]: No index buffer is attached. Did you succesfully call vao:AttachIndexBuffer()?", __func__);

		indxLuaVBO->UpdateModelsVBOElementCount(); //need to update elements count because underlyiing VBO could have been updated

		result.baseIndex  = std::max(inputs.baseIndex.value_or(0) , 0);
		result.baseVertex = std::max(inputs.baseVertex.value_or(0), 0); //can't be checked easily

		result.drawCount = inputs.drawCount.value_or(indxLuaVBO->elementsCount);
		if (!inputs.drawCount.has_value() || inputs.drawCount.value() <= 0)
			result.drawCount -= result.baseIndex; //adjust automatically

		if (result.drawCount <= 0)
			LuaUtils::SolLuaError("[LuaVAOImpl::%s]: Non-positive number of draw elements %d is requested", __func__, result.drawCount);

		if (result.drawCount > indxLuaVBO->elementsCount - result.baseIndex)
			LuaUtils::SolLuaError("[LuaVAOImpl::%s]: Requested number of elements %d with offset %d exceeds buffer size %u", __func__, result.drawCount, result.baseIndex, indxLuaVBO->elementsCount);

	} else {
		if (!vertLuaVBO) {
			if (!inputs.drawCount.has_value())
				LuaUtils::SolLuaError("[LuaVAOImpl::%s]: In case vertex buffer is not attached, the drawCount param should be set explicitly", __func__);

			result.drawCount = inputs.drawCount.value();
		}
		else {
			result.drawCount = inputs.drawCount.value_or(vertLuaVBO->elementsCount);

			if (!inputs.drawCount.has_value())
				result.drawCount -= result.baseIndex; //note baseIndex not baseVertex

			if (result.drawCount > vertLuaVBO->elementsCount - result.baseIndex)
				LuaUtils::SolLuaError("[LuaVAOImpl::%s]: Requested number of vertices %d with offset %d exceeds buffer size %u", __func__, result.drawCount, result.baseIndex, vertLuaVBO->elementsCount);
		}
	}

	result.baseInstance = std::max(inputs.baseInstance.value_or(0), 0);
	result.instCount = std::max(inputs.instCount.value_or(0), 0); // 0 - forces ordinary version, while 1 - calls *Instanced()

	if (result.instCount > 0) {
		if (instLuaVBO && (result.instCount > instLuaVBO->elementsCount - result.baseInstance))
			LuaUtils::SolLuaError("[LuaVAOImpl::%s]: Requested number of instances %d with offset %d exceeds buffer size %u", __func__, result.instCount, result.baseInstance, instLuaVBO->elementsCount);
	}
	else {
		if (result.baseInstance > 0)
			LuaUtils::SolLuaError("[LuaVAOImpl::%s]: Requested baseInstance [%d] and but zero instance count", __func__, result.baseInstance);
	}

	CheckDrawPrimitiveType(mode);
	CondInitVAO();

	return result;
}


/***
 *
 * @function VAO:DrawArrays
 * @param glEnum number primitivesMode
 * @param vertexCount number?
 * @param vertexFirst number?
 * @param instanceCount number?
 * @param instanceFirst number?
 * @return nil
 */
void LuaVAOImpl::DrawArrays(GLenum mode, sol::optional<int> vertCountOpt, sol::optional<int> vertexFirstOpt, sol::optional<int> instanceCountOpt, sol::optional<int> instanceFirstOpt)
{
	DrawCheckInput inputs{
		vertCountOpt,
		std::nullopt,
		vertexFirstOpt,
		instanceCountOpt,
		instanceFirstOpt
	};

	const auto result = DrawCheck(mode, inputs, false);

	vao->Bind();

	if (result.instCount == 0)
		glDrawArrays(mode, result.baseIndex, result.drawCount);
	else {
		if (result.baseInstance > 0)
			glDrawArraysInstancedBaseInstance(mode, result.baseIndex, result.drawCount, result.instCount, result.baseInstance);
		else
			glDrawArraysInstanced(mode, result.baseIndex, result.drawCount, result.instCount);
	}

	vao->Unbind();
}


/***
 *
 * @function VAO:DrawElements
 * @param glEnum number primitivesMode
 * @param drawCount number?
 * @param baseIndex number?
 * @param instanceCount number?
 * @param baseVertex number?
 * @param baseInstance number?
 * @return nil
 */
void LuaVAOImpl::DrawElements(GLenum mode, sol::optional<int> indCountOpt, sol::optional<int> indElemOffsetOpt, sol::optional<int> instanceCountOpt, sol::optional<int> baseVertexOpt, sol::optional<int> instanceFirstOpt)
{
	DrawCheckInput inputs{
		indCountOpt,
		baseVertexOpt,
		indElemOffsetOpt,
		instanceCountOpt,
		instanceFirstOpt
	};

	const auto result = DrawCheck(mode, inputs, true);

	const auto indElemOffsetInBytes = result.baseIndex * indxLuaVBO->elemSizeInBytes;

	const auto indexType = indxLuaVBO->bufferAttribDefsVec[0].second.type;

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(indxLuaVBO->primitiveRestartIndex);

	vao->Bind();

	#define INT2PTR(x) (reinterpret_cast<void*>(static_cast<intptr_t>(x)))
	if (result.instCount == 0) {
		if (result.baseVertex == 0)
			glDrawElements(mode, result.drawCount, indexType, INT2PTR(indElemOffsetInBytes));
		else
			glDrawElementsBaseVertex(mode, result.drawCount, indexType, INT2PTR(indElemOffsetInBytes), result.baseVertex);
	} else {
		if (result.baseInstance > 0)
			glDrawElementsInstancedBaseVertexBaseInstance(mode, result.drawCount, indexType, INT2PTR(indElemOffsetInBytes), result.instCount, result.baseVertex, result.baseInstance);
		else {
			if (result.baseVertex == 0)
				glDrawElementsInstanced(mode, result.drawCount, indexType, INT2PTR(indElemOffsetInBytes), result.instCount);
			else
				glDrawElementsInstancedBaseVertex(mode, result.drawCount, indexType, INT2PTR(indElemOffsetInBytes), result.instCount, result.baseVertex);
		}
	}
	#undef INT2PTR

	vao->Unbind();

	glDisable(GL_PRIMITIVE_RESTART);
}

void LuaVAOImpl::ClearSubmission()
{
	baseInstance = 0u;
	submitCmds.clear();
}


/***
 *
 * @function VAO:AddUnitsToSubmission
 * @param unitIDs number|number[]
 * @return number submittedCount
 */
int LuaVAOImpl::AddUnitsToSubmission(int id) { return AddObjectsToSubmissionImpl<CUnit>(id); }
int LuaVAOImpl::AddUnitsToSubmission(const sol::stack_table& ids) { return  AddObjectsToSubmissionImpl<CUnit>(ids); }


/***
 *
 * @function VAO:AddFeaturesToSubmission
 * @param featureIDs number|number[]
 * @return number submittedCount
 */
int LuaVAOImpl::AddFeaturesToSubmission(int id) { return AddObjectsToSubmissionImpl<CFeature>(id); }
int LuaVAOImpl::AddFeaturesToSubmission(const sol::stack_table& ids) { return AddObjectsToSubmissionImpl<CFeature>(ids); }


/***
 *
 * @function VAO:AddUnitDefsToSubmission
 * @param unitDefIDs number|number[]
 * @return number submittedCount
 */
int LuaVAOImpl::AddUnitDefsToSubmission(int id) { return AddObjectsToSubmissionImpl<UnitDef>(id); }
int LuaVAOImpl::AddUnitDefsToSubmission(const sol::stack_table& ids) { return AddObjectsToSubmissionImpl<UnitDef>(ids); }


/***
 *
 * @function VAO:AddFeatureDefsToSubmission
 * @param featureDefIDs number|number[]
 * @return number submittedCount
 */
int LuaVAOImpl::AddFeatureDefsToSubmission(int id) { return AddObjectsToSubmissionImpl<FeatureDef>(id); }
int LuaVAOImpl::AddFeatureDefsToSubmission(const sol::stack_table& ids) { return AddObjectsToSubmissionImpl<FeatureDef>(ids); }


/***
 *
 * @function VAO:RemoveFromSubmission
 * @param index number
 * @return nil
 */
void LuaVAOImpl::RemoveFromSubmission(int idx)
{
	if (idx < 0 || idx >= submitCmds.size()) {
		LuaUtils::SolLuaError("[LuaVAOImpl::%s] wrong submitCmds index [%d]", __func__, idx);
		return;
	}

	if (idx != submitCmds.size() - 1)
		submitCmds[idx] = submitCmds.back();

	submitCmds.pop_back();
	for (baseInstance = 0; baseInstance < submitCmds.size(); ++baseInstance) {
		submitCmds[baseInstance].baseInstance = baseInstance;
	}
}


/***
 *
 * @function VAO:Submit
 * @return nil
 */
void LuaVAOImpl::Submit()
{
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(indxLuaVBO->primitiveRestartIndex);

	vao->Bind();
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, submitCmds.data(), submitCmds.size(), sizeof(SDrawElementsIndirectCommand));
	vao->Unbind();

	glDisable(GL_PRIMITIVE_RESTART);
}
