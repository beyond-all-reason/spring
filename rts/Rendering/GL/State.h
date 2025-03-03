/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include "glHelpers.h"
#include "System/TemplateUtils.hpp"
#include <tuple>
#include <type_traits>
#include <utility>

/*
	GL::State doesn't represent active GL state.
	It represents GL state, that was last applied by GL::SubState objects, or any other object that performs state change via this interface.

	If a state was not set by a SubState object, it is internally set to Unknown.
	When a state is Unknown and SubState object requests it, the state is fetched from GL.
	When the last existing SubState object reverts a state, it is set back to Unknown.
	Setting back to Unknown after the last SubState object enforces fetch from GL every time the first SubState object is created,
		but makes it possible to insert SubState object in any part of the code without serious restrictions to the whole prior code.

	User must make sure that ordinary GL commands that are not tracked do not collide with SubState object's modifications during its lifetime.
*/

namespace GL
{

template<auto DedicatedGLFuncPtrPtr, GLenum... GLParamNames>
class StateAttribute {
private:
	using GLParamsType = spring::func_ptr_signature_t<DedicatedGLFuncPtrPtr, GLboolean>;
	using StatusType = bool;
	static constexpr StatusType Unknown = false;
	static constexpr StatusType WasUnknown = false;

public:
	using ValueType = std::pair<StatusType, GLParamsType>;

	inline StateAttribute() : value(Unknown, GLParamsType()) {};

	inline operator ValueType() const
	{
		return (value.first != Unknown)
			? value
			: ValueType(WasUnknown, FetchEffectualStateAttribValues<GLParamsType>(GLParamNames...));
	}
	inline StateAttribute& operator=(const ValueType& newValue)
	{
		if (value != newValue) {
			value = newValue;
			glSetAny<DedicatedGLFuncPtrPtr, GLParamNames...>(newValue.second);
		}
		return *this;
	}

private:
	ValueType value;
};

template<>
class StateAttribute<&glPolygonMode, GL_POLYGON_MODE> {
private:
	using StatusType = bool;
	static constexpr StatusType Unknown = false;
	static constexpr StatusType WasUnknown = false;

public:
	using ValueType = std::pair<StatusType, GLenum>;

	inline StateAttribute() : value(Unknown, 0) {};

	inline operator ValueType() const
	{
		return (value.first != Unknown)
			? value
			: ValueType(WasUnknown, FetchEffectualStateAttribValue<GLenum>(GL_POLYGON_MODE));
	}
	inline StateAttribute& operator=(const ValueType& newValue)
	{
		if (value != newValue) {
			value = newValue;
			glPolygonMode(GL_FRONT_AND_BACK, newValue.second);
		}
		return *this;
	}

private:
	ValueType value;
};

template<class AttributeT> struct UniqueStateAttributeValueType {
public:
	using AttributeType = AttributeT;
	using AttributeValueType = typename AttributeType::ValueType;

	template<class... ArgTypes>
	inline UniqueStateAttributeValueType(ArgTypes&&... valueConstructionArgs)
	: 	value(true, typename AttributeValueType::second_type(std::forward<ArgTypes>(valueConstructionArgs)...))
	{}
	template<class Type, std::enable_if_t<std::is_convertible_v<std::remove_reference_t<Type>, AttributeValueType>, bool> = true>
	inline UniqueStateAttributeValueType(Type&& valueSample)
	:	value(valueSample)
	{}

	inline operator const AttributeValueType&() const { return value; }

private:
	AttributeValueType value;
};

#define ATTRIBUTE(name) name##Attribute
#define ATTRIBUTE_TYPE_DEFS(name, ...) \
	using ATTRIBUTE(name) = StateAttribute<&(gl##name), __VA_ARGS__>; \
	using name = UniqueStateAttributeValueType<ATTRIBUTE(name)>;
#define CAPABILITY_ATTRIBUTE_TYPE_DEFS(name, glParamName) \
	using ATTRIBUTE(name) = StateAttribute<nullptr, glParamName>; \
	using name = UniqueStateAttributeValueType<ATTRIBUTE(name)>;
#define MULTI_CAPABILITY_ATTRIBUTE_TYPE_DEFS(name, glParamName) \
	template<GLenum Index> using ATTRIBUTE(name) = StateAttribute<nullptr, glParamName+Index>; \
	template<GLenum Index> using name = UniqueStateAttributeValueType<ATTRIBUTE(name)<Index>>;

namespace State {
	ATTRIBUTE_TYPE_DEFS            (PolygonMode, GL_POLYGON_MODE);
	ATTRIBUTE_TYPE_DEFS            (PolygonOffset, GL_POLYGON_OFFSET_FACTOR, GL_POLYGON_OFFSET_UNITS);
	CAPABILITY_ATTRIBUTE_TYPE_DEFS (PolygonOffsetFill, GL_POLYGON_OFFSET_FILL);
	CAPABILITY_ATTRIBUTE_TYPE_DEFS (PolygonOffsetLine, GL_POLYGON_OFFSET_LINE);
	CAPABILITY_ATTRIBUTE_TYPE_DEFS (PolygonOffsetPoint, GL_POLYGON_OFFSET_POINT);

	ATTRIBUTE_TYPE_DEFS            (LineWidth, GL_LINE_WIDTH);

	ATTRIBUTE_TYPE_DEFS            (Viewport, GL_VIEWPORT);

	ATTRIBUTE_TYPE_DEFS            (FrontFace, GL_FRONT_FACE);
	CAPABILITY_ATTRIBUTE_TYPE_DEFS (Culling, GL_CULL_FACE);
	ATTRIBUTE_TYPE_DEFS            (CullFace, GL_CULL_FACE_MODE);
	MULTI_CAPABILITY_ATTRIBUTE_TYPE_DEFS (ClipDistance, GL_CLIP_DISTANCE0);

	CAPABILITY_ATTRIBUTE_TYPE_DEFS (ScissorTest, GL_SCISSOR_TEST);
	ATTRIBUTE_TYPE_DEFS            (Scissor, GL_SCISSOR_BOX);

	CAPABILITY_ATTRIBUTE_TYPE_DEFS (DepthTest, GL_DEPTH_TEST);
	ATTRIBUTE_TYPE_DEFS            (DepthFunc, GL_DEPTH_FUNC);
	ATTRIBUTE_TYPE_DEFS            (DepthRangef, GL_DEPTH_RANGE);
	CAPABILITY_ATTRIBUTE_TYPE_DEFS (DepthClamp, GL_DEPTH_CLAMP);
	ATTRIBUTE_TYPE_DEFS            (DepthMask, GL_DEPTH_WRITEMASK);

	CAPABILITY_ATTRIBUTE_TYPE_DEFS (StencilTest, GL_STENCIL_TEST);
	ATTRIBUTE_TYPE_DEFS            (StencilFunc, GL_STENCIL_FUNC, GL_STENCIL_REF, GL_STENCIL_VALUE_MASK);
	ATTRIBUTE_TYPE_DEFS            (StencilMask, GL_STENCIL_WRITEMASK);
	ATTRIBUTE_TYPE_DEFS            (StencilOp, GL_STENCIL_FAIL, GL_STENCIL_PASS_DEPTH_FAIL, GL_STENCIL_PASS_DEPTH_PASS);
	ATTRIBUTE_TYPE_DEFS            (ClearStencil, GL_STENCIL_CLEAR_VALUE);

	CAPABILITY_ATTRIBUTE_TYPE_DEFS (AlphaTest, GL_ALPHA_TEST);
	ATTRIBUTE_TYPE_DEFS            (AlphaFunc, GL_ALPHA_TEST_FUNC, GL_ALPHA_TEST_REF);

	CAPABILITY_ATTRIBUTE_TYPE_DEFS (Blending, GL_BLEND);
	ATTRIBUTE_TYPE_DEFS            (BlendFunc, GL_BLEND_SRC, GL_BLEND_DST);
	ATTRIBUTE_TYPE_DEFS            (BlendEquation, GL_BLEND_EQUATION_RGB);
	ATTRIBUTE_TYPE_DEFS            (BlendEquationSeparate, GL_BLEND_EQUATION_RGB, GL_BLEND_EQUATION_ALPHA);
	ATTRIBUTE_TYPE_DEFS            (BlendColor, GL_BLEND_COLOR);

	ATTRIBUTE_TYPE_DEFS            (ColorMask, GL_COLOR_WRITEMASK);
	ATTRIBUTE_TYPE_DEFS            (ClearColor, GL_COLOR_CLEAR_VALUE);

	CAPABILITY_ATTRIBUTE_TYPE_DEFS (PrimitiveRestart, GL_PRIMITIVE_RESTART);
	ATTRIBUTE_TYPE_DEFS            (PrimitiveRestartIndex, GL_PRIMITIVE_RESTART_INDEX);

	ATTRIBUTE_TYPE_DEFS            (MinSampleShading, GL_MIN_SAMPLE_SHADING_VALUE);

	CAPABILITY_ATTRIBUTE_TYPE_DEFS (Multisampling, GL_MULTISAMPLE);
	CAPABILITY_ATTRIBUTE_TYPE_DEFS (SampleShading, GL_SAMPLE_SHADING);
	CAPABILITY_ATTRIBUTE_TYPE_DEFS (AlphaToCoverage, GL_SAMPLE_ALPHA_TO_COVERAGE);
	CAPABILITY_ATTRIBUTE_TYPE_DEFS (AlphaToOne, GL_SAMPLE_ALPHA_TO_ONE);

	CAPABILITY_ATTRIBUTE_TYPE_DEFS (CubemapSeamless, GL_TEXTURE_CUBE_MAP_SEAMLESS);
	CAPABILITY_ATTRIBUTE_TYPE_DEFS (PointSize, GL_PROGRAM_POINT_SIZE);

	CAPABILITY_ATTRIBUTE_TYPE_DEFS (FrameBufferSRBG, GL_FRAMEBUFFER_SRGB);

	extern std::tuple<
		ATTRIBUTE(PolygonMode),
		ATTRIBUTE(PolygonOffsetFill),
		ATTRIBUTE(PolygonOffsetLine),
		ATTRIBUTE(PolygonOffsetPoint),
		ATTRIBUTE(LineWidth),
		ATTRIBUTE(Viewport),
		ATTRIBUTE(FrontFace),
		ATTRIBUTE(Culling),
		ATTRIBUTE(CullFace),
		ATTRIBUTE(ClipDistance)<0>,
		ATTRIBUTE(ClipDistance)<1>,
		ATTRIBUTE(ClipDistance)<2>,
		ATTRIBUTE(ClipDistance)<3>,
		ATTRIBUTE(ClipDistance)<4>,
		ATTRIBUTE(ClipDistance)<5>,
		ATTRIBUTE(ClipDistance)<6>,
		ATTRIBUTE(ClipDistance)<7>,
		ATTRIBUTE(ScissorTest),
		ATTRIBUTE(Scissor),
		ATTRIBUTE(DepthTest),
		ATTRIBUTE(DepthFunc),
		ATTRIBUTE(DepthRangef),
		ATTRIBUTE(DepthClamp),
		ATTRIBUTE(DepthMask),
		ATTRIBUTE(StencilTest),
		ATTRIBUTE(StencilFunc),
		ATTRIBUTE(StencilMask),
		ATTRIBUTE(StencilOp),
		ATTRIBUTE(ClearStencil),
		ATTRIBUTE(AlphaTest),
		ATTRIBUTE(AlphaFunc),
		ATTRIBUTE(Blending),
		ATTRIBUTE(BlendFunc),
		ATTRIBUTE(BlendEquation),
		ATTRIBUTE(BlendEquationSeparate),
		ATTRIBUTE(BlendColor),
		ATTRIBUTE(ColorMask),
		ATTRIBUTE(ClearColor),
		ATTRIBUTE(PrimitiveRestart),
		ATTRIBUTE(PrimitiveRestartIndex),
		ATTRIBUTE(Multisampling),
		ATTRIBUTE(SampleShading),
		ATTRIBUTE(MinSampleShading),
		ATTRIBUTE(AlphaToCoverage),
		ATTRIBUTE(AlphaToOne),
		ATTRIBUTE(CubemapSeamless),
		ATTRIBUTE(PointSize),
		ATTRIBUTE(FrameBufferSRBG)
	> Attributes;
};

#undef ATTRIBUTE
#undef ATTRIBUTE_TYPE_DEFS
#undef CAPABILITY_ATTRIBUTE_TYPE_DEFS
#undef MULTI_CAPABILITY_ATTRIBUTE_TYPE_DEFS

}