/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include <algorithm>
#include <tuple>


// Must pass expectedValuesN to convert from other integer types to GLint
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

template<class ResultTupleType, GLenum... GLParamNames>
inline ResultTupleType FetchActiveStateAttribValues()
{
	static constexpr GLenum GLParamNameFirst = std::get<0>(std::make_tuple(GLParamNames...));
	static constexpr GLenum GLParamNameSecond = (sizeof...(GLParamNames) > 1? std::get<sizeof...(GLParamNames)-1>(std::make_tuple(GLParamNames...)) : 0);
	static constexpr size_t ValuesPerParam = std::tuple_size_v<ResultTupleType>/sizeof...(GLParamNames);

	ResultTupleType resultTuple;
	glGetAny(GLParamNameFirst, &std::get<0>(resultTuple), ValuesPerParam);
	if constexpr(GLParamNameSecond) glGetAny(GLParamNameSecond, &std::get<ValuesPerParam-1>(resultTuple), ValuesPerParam);
	return resultTuple;
}

}

template<auto DedicatedGLFuncPtrPtr, GLenum... GLParamName, class AttribValuesTupleType>
inline void glSetAny(AttribValuesTupleType newValues)
{
	if constexpr(DedicatedGLFuncPtrPtr)
	{
		std::apply(*DedicatedGLFuncPtrPtr, newValues);
	}
	else
	{
		(std::get<0>(newValues) == GL_TRUE? glEnable : glDisable)(GLParamName...);
	}
}
