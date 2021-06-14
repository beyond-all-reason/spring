#ifndef FIXED_PIPELINE_STATE_H
#define FIXED_PIPELINE_STATE_H

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

	class NamedSingleState {
	public:
		template<typename F, typename Args> NamedSingleState(F func, const Args& args) :
			object ( new ObjectModel<F, Args>(func, args) ) {
		}

		NamedSingleState() = delete;
		NamedSingleState(const NamedSingleState& rhs) = default;

		bool operator !=(const NamedSingleState& rhs) const {
			return !object->isSame(*rhs.object);
		}
		bool operator ==(const NamedSingleState& rhs) const {
			return object->isSame(*rhs.object);
		}

		void apply() const {
			object->apply();
		}

	private:
		struct ObjectConcept {
			virtual ~ObjectConcept() = default;
			virtual void apply() const = 0;
			virtual const std::type_info& typeInfo() const = 0;
			virtual bool isSame(const ObjectConcept& rhs) const = 0;
		};

		template<typename F, typename T> struct ObjectModel : ObjectConcept {
			ObjectModel(F func_, const T& args_) :
				func( func_ ), args( args_ )

			{}

			void apply() const override {
				std::apply(func, args);
			}
			const std::type_info& typeInfo() const override {
				return typeid(args);
			}
			bool isSame(const ObjectConcept& rhs) const override {
				if (typeInfo() != rhs.typeInfo())
					return false;

				return args == static_cast<const ObjectModel&>(rhs).args;
			}
		private:
			F func;
			T args;
		};

		std::shared_ptr<ObjectConcept> object;
	};

	#define DEBUG_PIPELINE_STATE 1
	class FixedPipelineState {
	private:
		//template <typename Result, typename ...Args> using vararg_function = std::function< Result(Args...) >;
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

		FixedPipelineState& InferState();
	public:
		FixedPipelineState& operator=(const FixedPipelineState& other) = default; //copy
		FixedPipelineState& operator=(FixedPipelineState&& other) = default; //move
	public:
		void Bind() const { BindUnbind(true); }
		void Unbind() const { BindUnbind(false); }
	private:
		void BindUnbind(const bool bind) const;

		template <typename... T>
		void DumpState(std::tuple<T...> tuple);
	private:
		static void BindTextureProxy(GLenum texUnit, GLenum texType, GLuint texID) { glActiveTexture(texUnit); glBindTexture(texType, texID); }
	private:
		FixedPipelineState& CommonBinaryState(const GLenum state, bool b) {
			const auto argsTuple = std::make_tuple(state, static_cast<GLboolean>(b));
			DumpState(std::tuple_cat(std::make_tuple("EnableDisable"), argsTuple));
			binaryStates[state] = b;
			return *this;
		}

		template <class F, class ...Args>
		FixedPipelineState& CommonNamedState(const char* funcName, F func, Args... args) {
			auto argsTuple = std::make_tuple(args...);
			DumpState(std::tuple_cat(std::make_tuple(funcName), argsTuple));
			namedStates.emplace(std::string(funcName), NamedSingleState(func, argsTuple));
			return *this;
		}

	private:
		std::unordered_map<GLenum, bool> binaryStates;
		std::unordered_map<std::string, NamedSingleState> namedStates;

		std::vector<std::pair<bool, onBindUnbindFunc>> customOnBindUnbind;

		GLuint lastActiveTexture = ~0u;
	private:
		static std::stack<FixedPipelineState> statesChain;
	};

	class ScopedState {
	public:
		ScopedState(const FixedPipelineState& state) : s{ state } { s.Bind(); }
		~ScopedState() { s.Unbind(); }
	private:
		const FixedPipelineState s;
	};
}

#endif