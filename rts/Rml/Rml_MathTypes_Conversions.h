// This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

//
// Created by ChrisFloofyKitsune on 2/10/2024.
// Based on the example code at the end of Include/RmlUi/Config/Config.h
//

#ifndef RML_MATH_TYPES_CONVERSIONS_H
#define RML_MATH_TYPES_CONVERSIONS_H

#include "System/Color.h"
#include "System/Matrix44f.h"
#include "System/float3.h"
#include "System/float4.h"
#include "System/type2.h"

// The following defines should be used for inserting custom type cast operators for conversion of
// RmlUi types to user types. RmlUi uses template math types, therefore conversion operators to
// non-templated types should be done using SFINAE as in example below.

// Extra code to be inserted into RmlUi::Color<> class body. Note: be mindful of colorspaces used by
// different color types. RmlUi assumes that float colors are interpreted in linear colorspace while
// byte colors are interpreted as sRGB.
#define RMLUI_COLOUR_USER_EXTRA                                               \
	template <typename U = ColourType,                                        \
	          typename std::enable_if_t<std::is_same_v<U, byte>>* = nullptr>  \
	operator SColor() const                                                   \
	{                                                                         \
		return SColor(red, green, blue, alpha);                               \
	}                                                                         \
	template <typename U = ColourType,                                        \
	          typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr> \
	operator SColor() const                                                   \
	{                                                                         \
		return SColor(red, green, blue, alpha);                               \
	}

// Extra code to be inserted into RmlUi::Vector2<> class body.
#define RMLUI_VECTOR2_USER_EXTRA                                                                 \
	template <typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>   \
	operator int2() const                                                                        \
	{                                                                                            \
		return int2(x, y);                                                                       \
	}                                                                                            \
	template <typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr> \
	operator float2() const                                                                      \
	{                                                                                            \
		return float2(x, y);                                                                     \
	}                                                                                            \
	template <typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>   \
	Vector2(int2 value)                                                                          \
		: Vector2(value.x, value.y)                                                              \
	{                                                                                            \
	}                                                                                            \
	template <typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr> \
	Vector2(float2 value)                                                                        \
		: Vector2(value.x, value.y)                                                              \
	{                                                                                            \
	}

// Extra code to be inserted into RmlUi::Vector3<> class body.
#define RMLUI_VECTOR3_USER_EXTRA                                                                 \
	template <typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr> \
	operator float3() const                                                                      \
	{                                                                                            \
		return float3(x, y, z);                                                                  \
	}                                                                                            \
	template <typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr> \
	Vector3(float3 value)                                                                        \
		: Vector3(value.x, value.y, value.z)                                                     \
	{                                                                                            \
	}

// Extra code to be inserted into RmlUi::Vector4<> class body.
#define RMLUI_VECTOR4_USER_EXTRA                                                                 \
	template <typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr> \
	operator float4() const                                                                      \
	{                                                                                            \
		return float4(x, y, z, w);                                                               \
	}                                                                                            \
	template <typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr> \
	Vector4(float4 value)                                                                        \
		: Vector4(value.x, value.y, value.z, value.w)                                            \
	{                                                                                            \
	}

// Extra code to be inserted into RmlUi::Matrix4<> class body.
#define RMLUI_MATRIX4_USER_EXTRA                                              \
	template <typename U = ComponentType,                                     \
	          typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr> \
	operator CMatrix44f() const                                               \
	{                                                                         \
		return CMatrix44f(data());                                            \
	}                                                                         \
	template <typename U = ComponentType,                                     \
	          typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr> \
	Matrix4(CMatrix44f value)                                                 \
		: Matrix4(Matrix4::FromColumnMajor(value.m))                          \
	{                                                                         \
	}

#endif  // RML_MATH_TYPES_CONVERSIONS_H
