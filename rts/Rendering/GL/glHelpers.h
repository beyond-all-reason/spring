/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include "System/TemplateUtils.hpp"
#include "Rendering/Textures/TextureFormat.h"
#include <algorithm>
#include <tuple>


// Get gl parameter values into a homogenous GL-typed variable (single or array)
// Must pass expectedValuesN to convert from GLint to other integer types (GLenum, GLsizei and such)
template<class GLType>
inline void glGetAny(GLenum paramName, GLType* data, const int expectedValuesN = 1)
{
	std::array<GLType, 1024> values;
	assert(expectedValuesN > 0 && expectedValuesN < 1024);
	if constexpr (std::is_same_v<GLType, int32_t>)
		glGetIntegerv(paramName, values.data());
	else if constexpr (std::is_same_v<GLType, uint32_t>)
		glGetIntegerv(paramName, reinterpret_cast<GLint*>(values.data()));
	else if constexpr (std::is_same_v<GLType, GLboolean>)
		glGetBooleanv(paramName, values.data());
	else if constexpr (std::is_same_v<GLType, GLfloat>)
		glGetFloatv(paramName, values.data());
	else if constexpr (std::is_same_v<GLType, GLdouble>)
		glGetDoublev(paramName, values.data());
	else
		assert(false);

	std::copy(values.begin(), values.begin() + expectedValuesN, data);
}

namespace GL
{

// Fetch value of a single value parameter, not more than that
template<class ResultType>
inline ResultType FetchEffectualStateAttribValue(GLenum paramName)
{
	ResultType resultValue;
	glGetAny(paramName, &resultValue, 1);
	return resultValue;
}

template <typename T> concept glEnumType = std::is_same_v<T, GLenum>;

template<class ResultTupleType, glEnumType... ParamNames>
inline ResultTupleType FetchEffectualStateAttribValues(ParamNames... paramNames)
{
	static_assert(sizeof...(paramNames) == std::tuple_size_v<ResultTupleType>);
	ResultTupleType resultTuple;

	auto IndexDispatcher = spring::make_index_dispatcher<std::tuple_size_v<ResultTupleType>>();

	IndexDispatcher([args = std::forward_as_tuple(paramNames...), &resultTuple](auto idx) {
		glGetAny(std::get<idx>(args), &std::get<idx>(resultTuple));
	});

	return resultTuple;
}

inline GLuint FetchCurrentSlotTextureID(GLenum target) {
	GLenum query = GL::GetBindingQueryFromTarget(target);
	assert(query);
	GLuint currentSlotTextureID;
	glGetAny(query, &currentSlotTextureID, 1);
	return currentSlotTextureID;
}

}

// Set gl attribute, whether it is capability (glEnable/glDisable) or dedicated, via this single interface
// Pass DedicatedGLFuncPtrPtr *if* it exists, nullptr if it doesn't
template<auto DedicatedGLFuncPtrPtr, GLenum... GLParamName, class AttribValuesTupleType>
inline void glSetAny(AttribValuesTupleType&& newValues)
{
	if constexpr(DedicatedGLFuncPtrPtr)
	{
		static auto HelperFunc = [](auto&& ... p) {
			(*DedicatedGLFuncPtrPtr)(std::forward<decltype(p)>(p)...);
		};
		std::apply(HelperFunc, std::forward<AttribValuesTupleType>(newValues));
	}
	else // glEnable/glDisable(attribute)
	{
		(std::get<0>(newValues) == GL_TRUE? glEnable : glDisable)(GLParamName...);
	}
}
