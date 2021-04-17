#ifndef FIXED_STATE_H
#define FIXED_STATE_H

#include <functional>
#include <string>
#include <unordered_map>
#include <stack>
#include <vector>
#include <tuple>
#include <cstring>

#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"
#include "System/StringHash.h"

namespace GL {
	template<typename GLgetFunc, typename GLparamType, typename ReturnType>
	static ReturnType glGetT(GLgetFunc glGetFunc, GLenum param) {
		ReturnType ret;
		constexpr size_t arrSize = sizeof(ret);
		GLparamType* arr = static_cast<GLparamType*>(malloc(arrSize));

		memset(arr, 0, arrSize);
		glGetError();
		glGetFunc(param, &arr[0]);
		assert(glGetError() == GL_NO_ERROR);
		memcpy(reinterpret_cast<GLparamType*>(&ret), &arr[0], arrSize);

		free(arr);

		return ret;
	}

	template<typename ReturnType = int>
	static ReturnType glGetIntT(GLenum param) {
		return (glGetT<decltype(&glGetIntegerv), GLint, ReturnType>(glGetIntegerv, param));
	}

	template<typename ReturnType = float, size_t numFloats = 1>
	static ReturnType glGetFloatT(GLenum param) {
		return (glGetT<decltype(&glGetFloatv), GLfloat, ReturnType>(glGetFloatv, param));
	};

	class FixedPipelineState {
	private:
		//template <typename Result, typename ...Args> using vararg_function = std::function< Result(Args...) >;
		using glB1Func = std::function<void(GLboolean)>;
		using glB4Func = std::function<void(GLboolean, GLboolean, GLboolean, GLboolean)>;
		using glE1Func = std::function<void(GLenum)>;
		using glE2Func = std::function<void(GLenum, GLenum)>;
		using glE3Func = std::function<void(GLenum, GLenum, GLenum)>;
		using glI4Func = std::function<void(GLint, GLint, GLint, GLint)>;
		using glE1F1Func  = std::function<void(GLenum, GLfloat)>;
		using glF1Func = std::function<void(GLfloat)>;
		using glF2Func = std::function<void(GLfloat, GLfloat)>;
		using glF4Func = std::function<void(GLfloat, GLfloat, GLfloat, GLfloat)>;

		using onBindUnbindFunc = std::function<void()>;
	public:
		FixedPipelineState();
		FixedPipelineState(FixedPipelineState&& rh) = default; //move
		FixedPipelineState(const FixedPipelineState& rh) = default; //copy

		FixedPipelineState& PolygonMode(GLenum mode) { return CommonNamedState(__func__, glPolygonMode, GL_FRONT_AND_BACK, mode); }

		FixedPipelineState& PolygonOffset(GLfloat factor, GLfloat units) { return CommonNamedState(__func__, glPolygonOffset, factor, units); }
		FixedPipelineState& PolygonOffsetFill(bool b)  { return CommonBinaryState(GL_POLYGON_OFFSET_FILL , b); }
		FixedPipelineState& PolygonOffsetLine(bool b)  { return CommonBinaryState(GL_POLYGON_OFFSET_LINE , b); }
		FixedPipelineState& PolygonOffsetPoint(bool b) { return CommonBinaryState(GL_POLYGON_OFFSET_POINT, b); }

		FixedPipelineState& FrontFace(GLenum mode) { return CommonNamedState(__func__, glFrontFace, mode); }
		FixedPipelineState& Culling(bool b) { return CommonBinaryState(GL_CULL_FACE_MODE, b); }
		FixedPipelineState& CullFace(GLenum mode) { return CommonNamedState(__func__, glCullFace, mode); }

		FixedPipelineState& DepthMask(bool b) { return CommonNamedState(__func__, glDepthMask, b); }
		FixedPipelineState& DepthRange(GLfloat n, GLfloat f) { return CommonNamedState(__func__, glDepthRangef, n, f); }
		FixedPipelineState& DepthClamp(bool b) { return CommonBinaryState(GL_DEPTH_CLAMP, b); }
		FixedPipelineState& DepthTest(bool b) { return CommonBinaryState(GL_DEPTH_TEST, b); }
		FixedPipelineState& DepthFunc(GLenum func) { return CommonNamedState(__func__, glDepthFunc, func); }

		// compat profile only
		FixedPipelineState& AlphaTest(bool b) { return CommonBinaryState(GL_ALPHA_TEST, b); }
		FixedPipelineState& AlphaFunc(GLenum func, GLfloat ref) { return CommonNamedState(__func__, glAlphaFunc, func, ref); }

		// TODO : expand
		FixedPipelineState& Blending(bool b) { return CommonBinaryState(GL_BLEND, b); }
		FixedPipelineState& BlendFunc(GLenum sfactor, GLenum dfactor) { return CommonNamedState(__func__, glBlendFunc, sfactor, dfactor); }
		FixedPipelineState& BlendColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { return CommonNamedState(__func__, glBlendColor, r, g, b, a); }

		FixedPipelineState& StencilTest(bool b) { return CommonBinaryState(GL_STENCIL_TEST, b); }

		FixedPipelineState& ColorMask(bool r, bool g, bool b, bool a) { return CommonNamedState(__func__, glColorMask, r, g, b, a); }

		//FixedPipelineState& Multisampling(bool b) { return CommonBinaryState(GL_MULTISAMPLE, b); }
		/// TODO

		FixedPipelineState& ScissorTest(bool b) { return CommonBinaryState(GL_SCISSOR_TEST, b); }
		FixedPipelineState& ScissorRect(GLint x, GLint y, GLint w, GLint h) { return CommonNamedState(__func__, glScissor, x, y, w, h); }

		FixedPipelineState& Viewport(GLint x, GLint y, GLint w, GLint h) { return CommonNamedState(__func__, glViewport, x, y, w, h); }

		FixedPipelineState& BindTexture(GLenum texRelUnit, GLenum texType, GLuint texID) { return CommonNamedState(__func__, BindTextureProxy, texRelUnit + GL_TEXTURE0, texType, texID); };
		FixedPipelineState& LastActiveTexture(GLenum texRelUnit) { lastActiveTexture = texRelUnit + GL_TEXTURE0; return *this; }

		//FixedPipelineState& BindShader(Shader::IProgramObject* po) {  }

		/// applied in the end
		FixedPipelineState& OnBind(onBindUnbindFunc func) { customOnBindUnbind.emplace_back(std::make_pair(true, func)); return *this; };
		/// applied in the beginning
		FixedPipelineState& OnUnbind(onBindUnbindFunc func) { customOnBindUnbind.emplace_back(std::make_pair(false, func)); return *this; };
	public:
		FixedPipelineState& operator=(const FixedPipelineState& other) = default; //copy
		FixedPipelineState& operator=(FixedPipelineState&& other) = default; //move
	public:
		void Bind() const { BindUnbind(true); }
		void Unbind() const { BindUnbind(false); }
	private:
		void BindUnbind(const bool bind) const;
	private:
		static void BindTextureProxy(GLenum texUnit, GLenum texType, GLuint texID) { glActiveTexture(texUnit); glBindTexture(texType, texID); }
	private:
		FixedPipelineState& CommonBinaryState(const GLenum state, bool b) {
			binaryStates[state] = b;
			return *this;
		}

		FixedPipelineState& CommonNamedState(const char* func, glB1Func&& f, bool b) {
			b1States[hashString(func)] = std::make_pair(f, std::make_tuple(static_cast<GLboolean>(b)));
			return *this;
		}

		FixedPipelineState& CommonNamedState(const char* func, glB4Func&& f, bool b1, bool b2, bool b3, bool b4) {
			b4States[hashString(func)] = std::make_pair(f, std::make_tuple(
				static_cast<GLboolean>(b1),
				static_cast<GLboolean>(b2),
				static_cast<GLboolean>(b3),
				static_cast<GLboolean>(b4)
			));

			return *this;
		}

		FixedPipelineState& CommonNamedState(const char* func, glE1Func&& f, GLenum v) {
			e1States[hashString(func)] = std::make_pair(f, std::make_tuple(v));
			return *this;
		}

		FixedPipelineState& CommonNamedState(const char* func, glE2Func&& f, GLenum v1, GLenum v2) {
			e2States[hashString(func)] = std::make_pair(f, std::make_tuple(v1, v2));
			return *this;
		}

		FixedPipelineState& CommonNamedState(const char* func, glE3Func&& f, GLenum v1, GLenum v2, GLenum v3) {
			e3States[hashString(func)] = std::make_pair(f, std::make_tuple(v1, v2, v3));
			return *this;
		}

		FixedPipelineState& CommonNamedState(const char* func, glI4Func&& f, GLint v1, GLint v2, GLint v3, GLint v4) {
			i4States[hashString(func)] = std::make_pair(f, std::make_tuple(v1, v2, v2, v4));
			return *this;
		}

		FixedPipelineState& CommonNamedState(const char* func, glE1F1Func&& f, GLenum v1, GLfloat v2) {
			e1f1States[hashString(func)] = std::make_pair(f, std::make_tuple(v1, v2));
			return *this;
		}

		FixedPipelineState& CommonNamedState(const char* func, glF1Func&& f, GLfloat v) {
			f1States[hashString(func)] = std::make_pair(f, std::make_tuple(v));
			return *this;
		}

		FixedPipelineState& CommonNamedState(const char* func, glF2Func&& f, GLfloat v1, GLfloat v2) {
			f2States[hashString(func)] = std::make_pair(f, std::make_tuple(v1, v2));
			return *this;
		}

		FixedPipelineState& CommonNamedState(const char* func, glF4Func&& f, GLfloat v1, GLfloat v2, GLfloat v3, GLfloat v4) {
			f4States[hashString(func)] = std::make_pair(f, std::make_tuple(v1, v2, v3, v4));
			return *this;
		}

	private:
		std::unordered_map<GLenum, bool> binaryStates;

		std::unordered_map<uint32_t, std::pair<glB1Func, std::tuple<GLboolean>>> b1States;
		std::unordered_map<uint32_t, std::pair<glB4Func, std::tuple<GLboolean, GLboolean, GLboolean, GLboolean>>> b4States;

		std::unordered_map<uint32_t, std::pair<glE1Func, std::tuple<GLenum>>> e1States;
		std::unordered_map<uint32_t, std::pair<glE2Func, std::tuple<GLenum, GLenum>>> e2States;
		std::unordered_map<uint32_t, std::pair<glE3Func, std::tuple<GLenum, GLenum, GLenum>>> e3States;

		std::unordered_map<uint32_t, std::pair<glI4Func, std::tuple<GLint, GLint, GLint, GLint>>> i4States;

		std::unordered_map<uint32_t, std::pair<glE1F1Func, std::tuple<GLenum, GLenum>>> e1f1States;

		std::unordered_map<uint32_t, std::pair<glF1Func, std::tuple<GLfloat>>> f1States;
		std::unordered_map<uint32_t, std::pair<glF2Func, std::tuple<GLfloat, GLfloat>>> f2States;
		std::unordered_map<uint32_t, std::pair<glF4Func, std::tuple<GLfloat, GLfloat, GLfloat, GLfloat>>> f4States;

		GLuint lastActiveTexture = ~0u;

		std::vector<std::pair<bool, onBindUnbindFunc>> customOnBindUnbind;
	private:
		static std::stack<FixedPipelineState> statesChain;
	};
	std::stack<FixedPipelineState> FixedPipelineState::statesChain = {};

	class ScopedState {
	public:
		ScopedState(const FixedPipelineState& state) : s{ state } { s.Bind(); }
		~ScopedState() { s.Unbind(); }
	private:
		const FixedPipelineState s;
	};
}

#endif