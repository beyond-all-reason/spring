/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include <algorithm>
#include <tuple>


// Get gl parameter values into a homogenous GL-typed variable (single or array)
// Must pass expectedValuesN to convert from GLint to other integer types (GLenum, GLsizei and such)
template<class GLType>
inline void glGetAny(GLenum paramName, GLType* data, const int expectedValuesN = -1)
{
	GLint ints[expectedValuesN];
	glGetIntegerv(paramName, ints);
	std::move(ints, ints+expectedValuesN, data);
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

template<class ResultTupleType, GLenum GLParamName>
inline ResultTupleType FetchActiveStateAttribValues()
{
	ResultTupleType resultTuple;
	glGetAny(GLParamName, &std::get<0>(resultTuple), std::tuple_size_v<ResultTupleType>);
	return resultTuple;
}
template<class ResultTupleType, GLenum GLFirstParamName, GLenum GLSecondParamName>
inline ResultTupleType FetchActiveStateAttribValues()
{
	ResultTupleType resultTuple;
	glGetAny(GLFirstParamName, &std::get<0>(resultTuple), std::tuple_size_v<ResultTupleType>/2);
	glGetAny(GLSecondParamName, &std::get<std::tuple_size_v<ResultTupleType>/2-1>(resultTuple), std::tuple_size_v<ResultTupleType>/2);
	return resultTuple;
}

}

// Set gl attribute, whether it is capability (glEnable/glDisable) or dedicated, via this single interface
// Pass DedicatedGLFuncPtrPtr *if* it exists, nullptr if it doesn't
template<auto DedicatedGLFuncPtrPtr, GLenum... GLParamName, class AttribValuesTupleType>
inline void glSetAny(AttribValuesTupleType newValues)
{
	if constexpr(DedicatedGLFuncPtrPtr)
	{
		std::apply(*DedicatedGLFuncPtrPtr, newValues);
	}
	else // glEnable/glDisable(attribute)
	{
		(std::get<0>(newValues) == GL_TRUE? glEnable : glDisable)(GLParamName...);
	}
}
