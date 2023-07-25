#pragma once

#include <string>
#include <stack>
#include <vector>
#include <tuple>
#include <cstring>
#include <sstream>
#include <type_traits>
#include <memory>
#include <bitset>

#include "System/TemplateUtils.hpp"
#include "System/Log/ILog.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"

namespace GL {
	#define NAME_GL(name) gl##name
	#define NAME_STATE(name) name##State
	#define SET_NAMED_STATE(name, ...)  CommonNamedState<NAME_STATE(name)>(#name, __VA_ARGS__)
	#define SET_BINARY_STATE(name, ena) CommonBinaryState<NAME_STATE(name)>(#name, ena)
	#define SET_BINARY_COMPLEX_STATE(name, ...) CommonBinaryState<NAME_STATE(name)>(#name, __VA_ARGS__)

	class FixedPipelineState {
	public:
		static void InitStatic();
		static void KillStatic();
		FixedPipelineState();
		FixedPipelineState(FixedPipelineState&& rh) = default; //move
		FixedPipelineState(const FixedPipelineState& rh) = default; //copy

		FixedPipelineState& PolygonMode(GLenum mode) { return SET_NAMED_STATE(PolygonMode, GL_FRONT_AND_BACK, mode); }

		FixedPipelineState& PolygonOffset(GLfloat factor, GLfloat units) { return SET_NAMED_STATE(PolygonOffset, factor, units); }
		FixedPipelineState& PolygonOffsetFill(bool b) { return SET_BINARY_STATE(PolygonOffsetFill, b); }
		FixedPipelineState& PolygonOffsetLine(bool b) { return SET_BINARY_STATE(PolygonOffsetLine, b); }
		FixedPipelineState& PolygonOffsetPoint(bool b) { return SET_BINARY_STATE(PolygonOffsetPoint, b); }

		FixedPipelineState& FrontFace(GLenum mode) { return SET_NAMED_STATE(FrontFace, mode); }
		FixedPipelineState& Culling(bool b) { return SET_BINARY_STATE(Culling, b); }
		FixedPipelineState& CullFace(GLenum mode) { return SET_NAMED_STATE(CullFace, mode); }

		FixedPipelineState& DepthMask(bool b) { return SET_NAMED_STATE(DepthMask, b); }
		FixedPipelineState& DepthRange(GLfloat n, GLfloat f) { return SET_NAMED_STATE(DepthRangef, n, f); }
		FixedPipelineState& DepthClamp(bool b) { return SET_BINARY_STATE(DepthClamp, b); }
		FixedPipelineState& DepthTest(bool b) { return SET_BINARY_STATE(DepthTest, b); }
		FixedPipelineState& DepthFunc(GLenum func) { return SET_NAMED_STATE(DepthFunc, func); }

		// compat profile only
		FixedPipelineState& AlphaTest(bool b) { return SET_BINARY_STATE(AlphaTest, b); }
		FixedPipelineState& AlphaFunc(GLenum func, GLfloat ref) { return SET_NAMED_STATE(AlphaFunc, func, ref); }

		// TODO : expand
		FixedPipelineState& Blending(bool b) { return SET_BINARY_STATE(Blending, b); }
		FixedPipelineState& BlendFunc(GLenum sfactor, GLenum dfactor) { return SET_NAMED_STATE(BlendFunc, sfactor, dfactor); }
		FixedPipelineState& BlendColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { return SET_NAMED_STATE(BlendColor, r, g, b, a); }

		// TODO: Add more on stencils
		FixedPipelineState& StencilTest(bool b) { return SET_BINARY_STATE(StencilTest, b); }

		FixedPipelineState& ColorMask(bool r, bool g, bool b, bool a) { return SET_NAMED_STATE(ColorMask, r, static_cast<GLboolean>(g), static_cast<GLboolean>(b), static_cast<GLboolean>(a)); }

		FixedPipelineState& Multisampling(bool b) { return SET_BINARY_STATE(Multisampling, b); }
		FixedPipelineState& AlphaToCoverage(bool b) { return SET_BINARY_STATE(AlphaToCoverage, b); }
		FixedPipelineState& AlphaToOne(bool b) { return SET_BINARY_STATE(AlphaToOne, b); }

		FixedPipelineState& PrimitiveRestart(bool b) { return SET_BINARY_STATE(PrimitiveRestart, b); }
		FixedPipelineState& PrimitiveRestartIndex(GLuint index) { return SET_NAMED_STATE(PrimitiveRestartIndex, index); }

		FixedPipelineState& CubemapSeamless(bool b) { return SET_BINARY_STATE(CubemapSeamless, b); }

		FixedPipelineState& PointSize(bool b) { return SET_BINARY_STATE(PointSize, b); }

		FixedPipelineState& ClipDistance(GLenum relClipSp, bool b) { return SET_BINARY_COMPLEX_STATE(ClipDistance, GL_CLIP_DISTANCE0 + relClipSp, b); }

		FixedPipelineState& ScissorTest(bool b) { return SET_BINARY_STATE(ScissorTest, b); }
		FixedPipelineState& ScissorRect(GLint x, GLint y, GLint w, GLint h) { return SET_NAMED_STATE(Scissor, x, y, w, h); }

		FixedPipelineState& Viewport(GLint x, GLint y, GLint w, GLint h) { return SET_NAMED_STATE(Viewport, x, y, w, h); }

		FixedPipelineState& BindTexture(GLenum texRelUnit, GLenum texType, GLuint texID) { return SET_NAMED_STATE(BindTexture, texRelUnit + GL_TEXTURE0, texType, texID); };
		FixedPipelineState& LastActiveTexture(GLenum texRelUnit) { lastActiveTexture = texRelUnit + GL_TEXTURE0; return *this; }

		//FixedPipelineState& BindShader(Shader::IProgramObject* po) {  }

		FixedPipelineState& InferState();
	public:
		FixedPipelineState& operator=(const FixedPipelineState& other) = default; //copy
		FixedPipelineState& operator=(FixedPipelineState&& other) = default; //move
	public:
		void Bind() const { BindUnbind<true>(); }
		void Unbind() const { BindUnbind<false>(); }
	private:
		template<bool bind>
		void BindUnbind() const;

		template <typename... T>
		inline void DumpState(const std::tuple<T...>& tuple);
	private:
		static void BindTextureProxy(GLenum texUnit, GLenum texType, GLuint texID) { glActiveTexture(texUnit); glBindTexture(texType, texID); }
	private:
		template<typename StateType, typename ... Args>
		FixedPipelineState& CommonNamedState(const char* funcName, Args&&... args) {
			static constexpr auto db_index = 0 + spring::tuple_type_index_v<StateType, decltype(namedStates)>;
			dirtyBits.set(db_index, true);
			std::get<StateType>(namedStates) = {
				std::make_tuple(args...)
			};
			return *this;
		}

		template<typename StateType>
		FixedPipelineState& CommonBinaryState(const char* funcName, bool enabled) {
			static constexpr auto db_index = std::tuple_size_v<decltype(namedStates)> + spring::tuple_type_index_v<StateType, decltype(binaryStates)>;
			dirtyBits.set(db_index, true);
			std::get<StateType>(binaryStates) = {
				static_cast<GLboolean>(enabled)
			};
			return *this;
		}
		template<typename StateType>
		FixedPipelineState& CommonBinaryState(const char* funcName, GLenum cap, bool enabled) {
			static_assert(std::is_base_of_v<BinaryComplexState, StateType>, "Invalid dispatch");
			static constexpr auto db_index = std::tuple_size_v<decltype(namedStates)> + spring::tuple_type_index_v<StateType, decltype(binaryStates)>;
			dirtyBits.set(db_index, true);
			std::get<StateType>(binaryStates) = {
				cap,
				static_cast<GLboolean>(enabled)
			};
			return *this;
		}
	private:
		template<GLenum cap>
		struct BinaryState {
			static constexpr auto capability = cap;
			GLboolean enabled;
		};
		struct BinaryComplexState {
			GLenum capability;
			GLboolean enabled;
		};
		/////
		#define DEFINE_BINARY_STATE(name, cap) using NAME_STATE(name) = BinaryState<cap>;
		#define DEFINE_BINARY_COMPLEX_STATE(name) struct NAME_STATE(name) : public BinaryComplexState{};
		#define DEFINE_NAMED_STATE(name) struct NAME_STATE(name)\
		{\
			using FuncType = std::remove_pointer_t<decltype(NAME_GL(name))>;\
			using ArgsType = decltype(spring::arg_types_tuple_t(*(NAME_GL(name))));\
			inline static const FuncType* func = (NAME_GL(name));\
			ArgsType  args;\
		}
		#define DEFINE_NAMED_STATE_CUSTOM(name, funcName) struct NAME_STATE(name)\
		{\
			using FuncType = std::remove_pointer_t<decltype(funcName)>;\
			using ArgsType = decltype(spring::arg_types_tuple_t(*(funcName)));\
			inline static const FuncType* func = funcName;\
			ArgsType  args;\
		}
		DEFINE_NAMED_STATE(PolygonMode);
		DEFINE_NAMED_STATE(PolygonOffset);
		DEFINE_NAMED_STATE(FrontFace);
		DEFINE_NAMED_STATE(CullFace);
		DEFINE_NAMED_STATE(DepthMask);
		DEFINE_NAMED_STATE(DepthRangef);
		DEFINE_NAMED_STATE(DepthFunc);
		DEFINE_NAMED_STATE(AlphaFunc);
		DEFINE_NAMED_STATE(BlendFunc);
		DEFINE_NAMED_STATE(BlendColor);
		DEFINE_NAMED_STATE(ColorMask);
		DEFINE_NAMED_STATE(PrimitiveRestartIndex);
		DEFINE_NAMED_STATE(Scissor);
		DEFINE_NAMED_STATE(Viewport);
		DEFINE_NAMED_STATE_CUSTOM(BindTexture, BindTextureProxy);

		DEFINE_BINARY_STATE(PolygonOffsetFill, GL_POLYGON_OFFSET_FILL);
		DEFINE_BINARY_STATE(PolygonOffsetLine, GL_POLYGON_OFFSET_LINE);
		DEFINE_BINARY_STATE(PolygonOffsetPoint, GL_POLYGON_OFFSET_POINT);
		DEFINE_BINARY_STATE(Culling, GL_CULL_FACE_MODE);
		DEFINE_BINARY_STATE(DepthClamp, GL_DEPTH_CLAMP);
		DEFINE_BINARY_STATE(DepthTest, GL_DEPTH_TEST);
		DEFINE_BINARY_STATE(AlphaTest, GL_ALPHA_TEST);
		DEFINE_BINARY_STATE(Blending, GL_BLEND);
		DEFINE_BINARY_STATE(StencilTest, GL_STENCIL_TEST);
		DEFINE_BINARY_STATE(Multisampling, GL_MULTISAMPLE);
		DEFINE_BINARY_STATE(AlphaToCoverage, GL_SAMPLE_ALPHA_TO_COVERAGE);
		DEFINE_BINARY_STATE(AlphaToOne, GL_SAMPLE_ALPHA_TO_ONE);
		DEFINE_BINARY_STATE(PrimitiveRestart, GL_PRIMITIVE_RESTART);
		DEFINE_BINARY_STATE(CubemapSeamless, GL_TEXTURE_CUBE_MAP_SEAMLESS);
		DEFINE_BINARY_STATE(PointSize, GL_PROGRAM_POINT_SIZE);
		DEFINE_BINARY_COMPLEX_STATE(ClipDistance);
		DEFINE_BINARY_STATE(ScissorTest, GL_SCISSOR_TEST);

#undef DEFINE_NAMED_STATE
#undef DEFINE_BINARY_STATE

		std::tuple <
			NAME_STATE(PolygonMode),
			NAME_STATE(PolygonOffset),
			NAME_STATE(FrontFace),
			NAME_STATE(CullFace),
			NAME_STATE(DepthMask),
			NAME_STATE(DepthRangef),
			NAME_STATE(DepthFunc),
			NAME_STATE(AlphaFunc),
			NAME_STATE(BlendFunc),
			NAME_STATE(BlendColor),
			NAME_STATE(ColorMask),
			NAME_STATE(PrimitiveRestartIndex),
			NAME_STATE(Scissor),
			NAME_STATE(Viewport),
			NAME_STATE(BindTexture)
		> namedStates;

		std::tuple <
			NAME_STATE(PolygonOffsetFill),
			NAME_STATE(PolygonOffsetLine),
			NAME_STATE(PolygonOffsetPoint),
			NAME_STATE(Culling),
			NAME_STATE(DepthClamp),
			NAME_STATE(DepthTest),
			NAME_STATE(AlphaTest),
			NAME_STATE(Blending),
			NAME_STATE(StencilTest),
			NAME_STATE(Multisampling),
			NAME_STATE(AlphaToCoverage),
			NAME_STATE(AlphaToOne),
			NAME_STATE(PrimitiveRestart),
			NAME_STATE(CubemapSeamless),
			NAME_STATE(PointSize),
			NAME_STATE(ClipDistance),
			NAME_STATE(ScissorTest)
		> binaryStates;

		std::bitset<std::tuple_size_v<decltype(namedStates)> + std::tuple_size_v<decltype(binaryStates)>> dirtyBits;

		GLuint lastActiveTexture = ~0u;
	private:
		static std::stack<FixedPipelineState> statesStack;
	};

	class ScopedState {
	public:
		ScopedState(const FixedPipelineState& state) : s{ state } { s.Bind(); }
		~ScopedState() { s.Unbind(); }
	private:
		const FixedPipelineState s;
	};


	template<typename ...T>
	inline void FixedPipelineState::DumpState(const std::tuple<T...>& tuple)
	{
		/*
		std::ostringstream ss;
		ss << "[ ";
		std::apply([&ss](auto&&... args) {((ss << +args << ", "), ...); }, tuple);
		ss.seekp(-2, ss.cur);
		ss << " ]";

		LOG_L(L_NOTICE, "[FixedPipelineState::DumpState] %s", ss.str().c_str());
		*/
	}

}
