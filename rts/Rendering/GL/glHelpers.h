/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include "System/TemplateUtils.hpp"
#include "Rendering/Textures/TextureFormat.h"
#include <algorithm>
#include <tuple>
#include <array>


// Get gl parameter values into a homogeneous GL-typed variable (single or array)
// Must pass expectedValuesN to convert from GLint to other integer types (GLenum, GLsizei and such)
template<class GLType>
inline void glGetAny(GLenum paramName, GLType* data, const int expectedValuesN = 1)
{
	GLint ints[1024];
	assert(expectedValuesN > 0 && expectedValuesN < 1024);
	glGetIntegerv(paramName, ints);
	std::copy(ints, ints+expectedValuesN, data);
}
template<>
inline void glGetAny<GLint>(GLenum paramName, GLint* data, const int)
{
	glGetIntegerv(paramName, data);
}
template<>
inline void glGetAny<GLboolean>(GLenum paramName, GLboolean* data, const int)
{
	glGetBooleanv(paramName, data);
}
template<>
inline void glGetAny<GLfloat>(GLenum paramName, GLfloat* data, const int)
{
	glGetFloatv(paramName, data);
}
template<>
inline void glGetAny<GLdouble>(GLenum paramName, GLdouble* data, const int)
{
	glGetDoublev(paramName, data);
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

template<class ResultTupleType, glEnumType ParamName>
inline ResultTupleType FetchEffectualStateAttribValues(ParamName paramName)
{
	ResultTupleType resultTuple;

	using ArrType = std::tuple_element_t<0, ResultTupleType>;
	std::array<ArrType, std::tuple_size_v<ResultTupleType>> arr;
	glGetAny(paramName, arr.data(), arr.size());

	auto IndexDispatcher = spring::make_index_dispatcher<std::tuple_size_v<ResultTupleType>>();
	IndexDispatcher([&arr, &resultTuple](auto idx) {
		std::get<idx>(resultTuple) = arr[idx];
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
