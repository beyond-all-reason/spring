#include "FixedPipelineState.h"

#include <array>
#include <stdexcept>

#include "System/type2.h"
#include "System/float4.h"
#include "System/StringHash.h"
#include "Rendering/GlobalRendering.h"

using namespace GL;
std::stack<FixedPipelineState> FixedPipelineState::statesStack = {};

namespace {
	template<typename GLgetFunc, typename GLparamType, typename ReturnType>
	static ReturnType glGetT(GLgetFunc glGetFunc, GLenum param) {
		ReturnType ret;

		constexpr size_t arrSize = sizeof(ret);
		static std::array<uint8_t, arrSize> arr;
		arr = { 0 };

		glGetError();
		glGetFunc(param, reinterpret_cast<GLparamType*>(arr.data()));
		assert(glGetError() == GL_NO_ERROR);
		memcpy(reinterpret_cast<GLparamType*>(&ret), arr.data(), arrSize);

		return ret;
	}

	template<typename ReturnType = int>
	static ReturnType glGetIntT(GLenum param) {
		return (glGetT<decltype(&glGetIntegerv), GLint, ReturnType>(glGetIntegerv, param));
	}

	template<typename ReturnType = float>
	static ReturnType glGetFloatT(GLenum param) {
		return (glGetT<decltype(&glGetFloatv), GLfloat, ReturnType>(glGetFloatv, param));
	};
};


void GL::FixedPipelineState::InitStatic()
{
	assert(statesStack.empty());
	statesStack.push(FixedPipelineState().InferState());
	auto ps2 = FixedPipelineState().AlphaToCoverage(true);
	ps2.Bind();
	ps2.Unbind();
}

void GL::FixedPipelineState::KillStatic()
{
	assert(statesStack.size() == 1);
	statesStack.pop();
}

GL::FixedPipelineState::FixedPipelineState()
{
	if unlikely(FixedPipelineState::statesStack.empty())
		return;

	*this = statesStack.top();
	this->dirtyBits.reset();
}

FixedPipelineState& GL::FixedPipelineState::InferState()
{
	{
		PolygonMode(GL_FILL);
	}
	{
		PolygonOffset(glGetFloatT(GL_POLYGON_OFFSET_FACTOR), glGetFloatT(GL_POLYGON_OFFSET_UNITS));
		PolygonOffsetFill(glIsEnabled(GL_POLYGON_OFFSET_FILL));
		PolygonOffsetLine(glIsEnabled(GL_POLYGON_OFFSET_LINE));
		PolygonOffsetPoint(glIsEnabled(GL_POLYGON_OFFSET_POINT));
	}
	{
		FrontFace(glGetIntT(GL_FRONT_FACE)); // GL_CCW
		CullFace(glGetIntT(GL_CULL_FACE_MODE)); // GL_BACK
		Culling(glIsEnabled(GL_CULL_FACE));
	}
	{
		float2 nf = glGetFloatT<float2>(GL_DEPTH_RANGE);
		DepthRange(nf.x, nf.y);
		DepthClamp(glIsEnabled(GL_DEPTH_CLAMP));
		DepthTest(glIsEnabled(GL_DEPTH_TEST));
		DepthMask(glGetIntT(GL_DEPTH_WRITEMASK));
		DepthFunc(glGetIntT(GL_DEPTH_FUNC));
	}
	{
		AlphaTest(glIsEnabled(GL_ALPHA_TEST));
		AlphaFunc(glGetIntT(GL_ALPHA_TEST_FUNC), glGetFloatT(GL_ALPHA_TEST_REF));
	}
	{
		Blending(glIsEnabled(GL_BLEND));
		// Does not return proper values until glBlendFunc has been called
		// PushBlendFunc(glGetIntT(GL_BLEND_SRC_ALPHA), glGetIntT(GL_BLEND_DST_ALPHA));
		BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		float4 blendCol = glGetFloatT<float4>(GL_BLEND_COLOR);
		BlendColor(blendCol.r, blendCol.g, blendCol.b, blendCol.a);
	}
	{
		ScissorTest(glIsEnabled(GL_SCISSOR_TEST));
		auto scissorRect = glGetIntT<std::array<int32_t, 4>>(GL_SCISSOR_BOX);
		ScissorRect(scissorRect[0], scissorRect[1], scissorRect[2], scissorRect[3]);
	}
	{
		StencilTest(glIsEnabled(GL_STENCIL_TEST));
	}
	{
		auto colorMask = glGetIntT<std::array<uint32_t, 4>>(GL_COLOR_WRITEMASK);
		ColorMask(
			static_cast<bool>(colorMask[0]),
			static_cast<bool>(colorMask[1]),
			static_cast<bool>(colorMask[2]),
			static_cast<bool>(colorMask[3])
		);
	}
	{
		Multisampling(glIsEnabled(GL_MULTISAMPLE));
		Multisampling(glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE));
		Multisampling(glIsEnabled(GL_SAMPLE_ALPHA_TO_ONE));
	}
	{
		PrimitiveRestart(glIsEnabled(GL_PRIMITIVE_RESTART));
		PrimitiveRestartIndex(glGetIntT<GLuint>(GL_PRIMITIVE_RESTART_INDEX));
	}
	{
		CubemapSeamless(glIsEnabled(GL_TEXTURE_CUBE_MAP_SEAMLESS));
	}
	{
		PointSize(glIsEnabled(GL_PROGRAM_POINT_SIZE));
	}
	{
		auto maxClipDists = glGetIntT(GL_MAX_CLIP_DISTANCES);
		for (int i = 0; i < maxClipDists; ++i)
			ClipDistance(i, glIsEnabled(GL_CLIP_DISTANCE0 + i));
	}
	{
		auto viewPort = glGetIntT<std::array<int32_t, 4>>(GL_VIEWPORT);
		Viewport(viewPort[0], viewPort[1], viewPort[2], viewPort[3]);
	}
	{
		lastActiveTexture = glGetIntT(GL_ACTIVE_TEXTURE) - GL_TEXTURE0;
	}

	// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTextures.xhtml
#define MAKE_TEX_QUERY_BIND_PAIR(type) std::make_pair(GL_TEXTURE_BINDING_##type, GL_TEXTURE_##type)
	static constexpr std::array<std::pair<GLenum, GLenum>, 11> texTypes = {
		MAKE_TEX_QUERY_BIND_PAIR(1D),
		MAKE_TEX_QUERY_BIND_PAIR(2D),
		MAKE_TEX_QUERY_BIND_PAIR(3D),
		MAKE_TEX_QUERY_BIND_PAIR(1D_ARRAY),
		MAKE_TEX_QUERY_BIND_PAIR(2D_ARRAY),
		MAKE_TEX_QUERY_BIND_PAIR(RECTANGLE),
		MAKE_TEX_QUERY_BIND_PAIR(BUFFER),
		MAKE_TEX_QUERY_BIND_PAIR(CUBE_MAP),
		MAKE_TEX_QUERY_BIND_PAIR(CUBE_MAP_ARRAY),
		MAKE_TEX_QUERY_BIND_PAIR(2D_MULTISAMPLE),
		MAKE_TEX_QUERY_BIND_PAIR(2D_MULTISAMPLE_ARRAY)
	};
#undef MAKE_TEX_QUERY_BIND_PAIR

	for (int texRelUnit = 0; texRelUnit < CGlobalRendering::MAX_TEXTURE_UNITS; ++texRelUnit)
	{
		bool found = false;
		for (const auto [queryType, bindType] : texTypes) {
			glActiveTexture(GL_TEXTURE0 + texRelUnit);
			GLint texID = glGetIntT(queryType);
			if (texID > 0) {
				BindTexture(texRelUnit, bindType, texID);
				found = true;
				break;
			}
		}

		if (found)
			continue;

		//BindTexture(texRelUnit, GL_TEXTURE_2D, 0u);
	}
	glActiveTexture(lastActiveTexture + GL_TEXTURE0); //revert just in case

	return *this;
}

template<bool bind>
void FixedPipelineState::BindUnbind() const
{
	if constexpr(!bind)
		statesStack.pop();

	static constexpr std::add_pointer_t<decltype(glEnable)> EnableDisableFunc[] = { glDisable, glEnable };
	assert(statesStack.size() >= 1);
	if constexpr(bind) {
		// named
		std::apply([this, prev = &statesStack.top()](auto&& ... states) {
			const auto CompareFunc = [this, prev](auto&& state) {
				using StateType = std::decay_t<decltype(state)>;
				constexpr auto db_index = 0 + spring::tuple_type_index_v<StateType, decltype(namedStates)>;
				if (!dirtyBits[db_index])
					return;
				if (state.args != std::get<StateType>(prev->namedStates).args)
					std::apply(state.func, state.args);
			};
			((CompareFunc(states)), ...);
		}, namedStates);
	}
	else {
		// named
		std::apply([this, prev = &statesStack.top()](auto&& ... states) {
			const auto CompareFunc = [this, prev](auto&& state) {
				using StateType = std::decay_t<decltype(state)>;
				constexpr auto db_index = 0 + spring::tuple_type_index_v<StateType, decltype(namedStates)>;
				if (!dirtyBits[db_index])
					return;
				if (state.args != std::get<StateType>(prev->namedStates).args)
					std::apply(state.func, std::get<StateType>(prev->namedStates).args);
			};
			((CompareFunc(states)), ...);
		}, namedStates);
	}

	// binary
	std::apply([this, prev = &statesStack.top()](auto&& ... states) {
		const auto CompareFunc = [this, prev](auto&& state) {
			using StateType = std::decay_t<decltype(state)>;
			constexpr auto db_index = std::tuple_size_v<decltype(namedStates)> +spring::tuple_type_index_v<StateType, decltype(binaryStates)>;
			if (!dirtyBits[db_index])
				return;
			assert(state.capability == std::get<StateType>(prev->binaryStates).capability);
			bool curEn = !(bind ^ static_cast<bool>(                                  state.enabled));
			bool preEn = !(bind ^ static_cast<bool>(std::get<StateType>(prev->binaryStates).enabled));
			if (curEn != preEn) {
				EnableDisableFunc[curEn](state.capability);
			}
		};
		((CompareFunc(states)), ...);
	}, binaryStates);


	if (lastActiveTexture < CGlobalRendering::MAX_TEXTURE_UNITS)
		glActiveTexture(lastActiveTexture + GL_TEXTURE0);
	else {
		if (!bind && !statesStack.empty()) {
			const auto prevLastActiveTexture = statesStack.top().lastActiveTexture;
			if (prevLastActiveTexture < CGlobalRendering::MAX_TEXTURE_UNITS)
				glActiveTexture(prevLastActiveTexture + GL_TEXTURE0);
		}
	}

	if constexpr(bind)
		statesStack.push(*this);
	/*
	if (!bind)
		statesChain.pop();

	for (const auto& [onbind, func] : customOnBindUnbind) {
		if (bind == onbind)
			func();
	}

	if (bind) {
		for (const auto& [funcName, namedState] : namedStates) {
            if (statesChain.empty()) { //default state
                namedState.apply();
                continue;
            }
            const auto& prev = statesChain.top().namedStates;
            const auto& prevStateIt = prev.find(funcName);
            if (prevStateIt == prev.cend()) { //haven't seen this state in previous states
                // TODO: do something to save revert state
                namedState.apply();
                continue;
            }
            if (prevStateIt->second != namedState) { // previous state's args are different to this state args
                namedState.apply();
                continue;
            }
        }
	} else {
        for (const auto& [funcName, namedState] : namedStates) {
            if (statesChain.empty()) { // default state
                continue;
            }
            const auto& prev = statesChain.top().namedStates;
            const auto& prevStateIt = prev.find(funcName);
            if (prevStateIt == prev.cend()) { //haven't seen this state in previous states
                // TODO: do something to save revert state
                throw std::runtime_error("[FixedPipelineState::BindUnbind] Cannot revert state that has no default value");
            }
            if (prevStateIt->second != namedState) { // previous state's args are different to this state args
                prevStateIt->second.apply();
                continue;
            }
        }
	}

	//now enable/disable states
	const auto& prev = statesChain.top().binaryStates;
	for (const auto [state, status] : binaryStates) {
		const auto& prevStateIt = prev.find(state);
		//  b, s ==> e
		//  1, 1 ==> 1
		//  0, 1 ==> 0
		//  1, 1 ==> 0
		//  0, 0 ==> 1
		const bool en  =                              !(bind ^ status             )      ;
		const bool pEn = prevStateIt != prev.cend() ? !(bind ^ prevStateIt->second) : !en;

		if (en != pEn) {
			if (en)
				glEnable(state);
			else
				glDisable(state);
		}
	}

	if (lastActiveTexture < CGlobalRendering::MAX_TEXTURE_UNITS)
		glActiveTexture(lastActiveTexture + GL_TEXTURE0);
	else {
		if (!bind && !statesChain.empty()) {
			const auto prevLastActiveTexture = statesChain.top().lastActiveTexture;
			if (prevLastActiveTexture < CGlobalRendering::MAX_TEXTURE_UNITS)
				glActiveTexture(prevLastActiveTexture + GL_TEXTURE0);
		}
	}

	for (const auto& [onbind, func] : customOnBindUnbind) {
		if (bind == onbind)
			func();
	}

	if (bind) {
		statesChain.push(*this);
	}
	*/
}