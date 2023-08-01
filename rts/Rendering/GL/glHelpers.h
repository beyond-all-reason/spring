/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include "Rendering/Textures/TextureFormat.h"
#include <algorithm>
#include <tuple>


// Get gl parameter values into a homogenous GL-typed variable (single or array)
// Must pass expectedValuesN to convert from GLint to other integer types (GLenum, GLsizei and such)
template<class GLType>
inline void glGetAny(GLenum paramName, GLType* data, const int expectedValuesN = -1)
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

template<class ResultTupleType>
inline ResultTupleType FetchEffectualStateAttribValues(GLenum paramName)
{
	ResultTupleType resultTuple;
	glGetAny(paramName, &std::get<0>(resultTuple), std::tuple_size_v<ResultTupleType>);
	return resultTuple;
}
template<class ResultTupleType>
inline ResultTupleType FetchEffectualStateAttribValues(GLenum firstParamName, GLenum secondParamName)
{
	ResultTupleType resultTuple;
	glGetAny(firstParamName, &std::get<0>(resultTuple), std::tuple_size_v<ResultTupleType>/2);
	glGetAny(secondParamName, &std::get<std::tuple_size_v<ResultTupleType>/2-1>(resultTuple), std::tuple_size_v<ResultTupleType>/2);
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
inline void glSetAny(AttribValuesTupleType newValues)
{
	if constexpr(DedicatedGLFuncPtrPtr)
	{
		std::apply(DedicatedGLFuncPtrPtr, newValues);
	}
	else // glEnable/glDisable(attribute)
	{
		(std::get<0>(newValues) == GL_TRUE? glEnable : glDisable)(GLParamName...);
	}
}
