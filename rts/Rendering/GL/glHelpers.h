/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include <tuple>
#include "Rendering/Textures/TextureFormat.h"


template<class GLType>
inline void glGetAny(GLenum paramName, GLType* data)
{
	glGetIntegerv(paramName, static_cast<GLint*>(data));
}
template<>
inline void glGetAny<GLboolean>(GLenum paramName, GLboolean* data)
{
	glGetBooleanv(paramName, data);
}
template<>
inline void glGetAny<GLfloat>(GLenum paramName, GLfloat* data)
{
	glGetFloatv(paramName, data);
}
template<>
inline void glGetAny<GLdouble>(GLenum paramName, GLdouble* data)
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

	ResultTupleType resultTuple;
	glGetAny(GLParamNameFirst, &std::get<0>(resultTuple));
	if constexpr(GLParamNameSecond) glGetAny(GLParamNameSecond, &std::get<1>(resultTuple));
	return resultTuple;
}

}

template<auto DedicatedGLFuncPtrPtr, GLenum GLParamName, class AttribValuesTupleType>
inline void glSetAny(AttribValuesTupleType newValues)
{
	if constexpr(DedicatedGLFuncPtrPtr)
	{
		std::apply(*DedicatedGLFuncPtrPtr, newValues);
	}
	else
	{
		(std::get<0>(newValues) == GL_TRUE? glEnable : glDisable)(GLParamName);
	}
}
