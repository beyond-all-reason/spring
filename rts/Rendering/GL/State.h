/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include <type_traits>
#include <tuple>

// HELPERS, this section will be moved elsewhere:
namespace GL
{
	template<class FuncType>
	struct FuncSignature;

	template<class ReturnType, class... ArgTypes>
	struct FuncSignature<ReturnType(ArgTypes...)> {
		using type = std::tuple<ArgTypes...>;
	};

    template<
        typename F,
        std::enable_if_t<std::is_function<F>::value, bool> = true
    >
    auto FuncArgs(const F&) -> typename FuncSignature<F>::type;
    template<
        typename F,
        std::enable_if_t<std::is_function<F>::value, bool> = true
    >
    auto FuncArgs(const F*) -> typename FuncSignature<F>::type;

	template<class GLType>
	inline void glGetAny(GLenum attribute, GLType* data)
	{
		glGetIntegerv(attribute, static_cast<GLint*>(data));
	}
	template<>
	inline void glGetAny<GLboolean>(GLenum attribute, GLboolean* data)
	{
		glGetBooleanv(attribute, data);
	}
	template<>
	inline void glGetAny<GLfloat>(GLenum attribute, GLfloat* data)
	{
		glGetFloatv(attribute, data);
	}
	template<>
	inline void glGetAny<GLdouble>(GLenum attribute, GLdouble* data)
	{
		glGetDoublev(attribute, data);
	}

	template<class ResultTupleType, GLenum... GLGetNames>
	inline ResultTupleType FetchActiveStateAttribValues()
	{
		static constexpr GLenum GLGetNameFirst = std::get<0>(std::make_tuple(GLGetNames...));
		static constexpr GLenum GLGetNameSecond = (sizeof...(GLGetNames) > 1? std::get<sizeof...(GLGetNames)-1>(std::make_tuple(GLGetNames...)) : 0);

		ResultTupleType resultTuple;
		glGetAny(GLGetNameFirst, &std::get<0>(resultTuple));
		if constexpr(GLGetNameSecond) glGetAny(GLGetNameSecond, &std::get<1>(resultTuple));
		return resultTuple;
	}
}

// STATE.h:

/*
	GL::State doesn't represent active GL state.
	It represents GL state, that was last applied by GL::StateChange objects, or any other object that performs state change via this interface.

	If a state was not set by a StateChange object, it is internally set to Unknown.
	When a state is Unknown and StateChange object requests it, the state is fetched from GL.
	When the last existing StateChange object reverts a state, it is set back to Unknown.
	Setting back to Unknown after the last StateChange object enforces fetch from GL every time the first StateChange object is created,
		but makes it possible to insert StateChange object in any part of the code without serious restrictions to the whole prior code.

	User must make sure that ordinary GL commands that are not tracked do not collide with StateChange object's modifications during their lifetime.
*/

namespace GL
{

template<auto GLFuncPtrPtr, GLenum... GLGetNames> struct StateAttribute {
public:
	using ValuesType = decltype(FuncArgs(**GLFuncPtrPtr));

	inline StateAttribute() : status(Unknown) {}

	inline operator ValuesType() const
	{
		return (status != Unknown)
			? vals
			: FetchActiveStateAttribValues<ValuesType, GLGetNames...>();
	}
	inline StateAttribute& operator=(const ValuesType& newVals)
	{
		if (vals != newVals) {
			vals = newVals;
			std::apply(*GLFuncPtrPtr, newVals);
		}
		return *this;
	}

private:
	static constexpr uint32_t Unknown = -1;

	union {
		ValuesType vals;
		uint32_t status;
	};
};

namespace State::Impl
{
template<class ValuesT, class AttributeT> struct StateAttributeValuesUniqueType {
public:
	using ValuesType = ValuesT;
	using AttributeType = AttributeT;
	template<class... ArgTypes> inline StateAttributeValuesUniqueType(ArgTypes&&... args) : values(args...) {}
	inline operator const ValuesType&() const { return values; }
private:
	ValuesType values;
};
}

#define ATTRIBUTE(name) name##Attribute
#define ATTRIBUTE_TYPE_DEFS(name, ...) \
	using ATTRIBUTE(name) = StateAttribute<&(gl##name), __VA_ARGS__>; \
	using name = Impl::StateAttributeValuesUniqueType<typename ATTRIBUTE(name)::ValuesType, ATTRIBUTE(name)>;

namespace State {
	ATTRIBUTE_TYPE_DEFS(PolygonMode, GL_POLYGON_MODE);
	ATTRIBUTE_TYPE_DEFS(FrontFace, GL_FRONT_FACE);
	ATTRIBUTE_TYPE_DEFS(CullFace, GL_CULL_FACE_MODE);
	ATTRIBUTE_TYPE_DEFS(DepthMask, GL_DEPTH_WRITEMASK);
	ATTRIBUTE_TYPE_DEFS(DepthRange, GL_DEPTH_RANGE);
	ATTRIBUTE_TYPE_DEFS(DepthFunc, GL_DEPTH_FUNC);
	ATTRIBUTE_TYPE_DEFS(AlphaFunc, GL_ALPHA_TEST_FUNC, GL_ALPHA_TEST_REF);
	ATTRIBUTE_TYPE_DEFS(BlendFunc, GL_BLEND_SRC, GL_BLEND_DST);
	ATTRIBUTE_TYPE_DEFS(PrimitiveRestartIndex, GL_PRIMITIVE_RESTART_INDEX);
	ATTRIBUTE_TYPE_DEFS(Scissor, GL_SCISSOR_BOX);
	ATTRIBUTE_TYPE_DEFS(Viewport, GL_VIEWPORT);

	using AttributesType = std::tuple<
		ATTRIBUTE(PolygonMode),
		ATTRIBUTE(FrontFace),
		ATTRIBUTE(CullFace),
		ATTRIBUTE(DepthMask),
		ATTRIBUTE(DepthRange),
		ATTRIBUTE(DepthFunc),
		ATTRIBUTE(AlphaFunc),
		ATTRIBUTE(BlendFunc),
		ATTRIBUTE(PrimitiveRestartIndex),
		ATTRIBUTE(Scissor),
		ATTRIBUTE(Viewport)
	>;
	extern AttributesType Attributes;
};

#undef ATTRIBUTE
#undef ATTRIBUTE_TYPE_DEFS

}