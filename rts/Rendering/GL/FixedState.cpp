#include "FixedState.h"

#include <array>
#include <stdexcept>

#include "System/type2.h"
#include "System/float4.h"

using namespace GL;
FixedPipelineState::FixedPipelineState()
{
	if (statesChain.empty()) { //default state
		{
			PolygonMode(GL_FILL);
		}
		{
			PolygonOffset(glGetFloatT(GL_POLYGON_OFFSET_FACTOR), glGetFloatT(GL_POLYGON_OFFSET_UNITS));
			PolygonOffsetFill (glIsEnabled(GL_POLYGON_OFFSET_FILL ));
			PolygonOffsetLine (glIsEnabled(GL_POLYGON_OFFSET_LINE ));
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
			auto viewPort = glGetIntT<std::array<int32_t, 4>>(GL_VIEWPORT);
			Viewport(viewPort[0], viewPort[1], viewPort[2], viewPort[3]);
		}
	}
	else {
		*this = statesChain.top(); //copy&paste previous state
	}
}

void FixedPipelineState::BindUnbind(const bool bind) const
{
	if (!bind)
		statesChain.pop();

	#define APPLY_STATES(states) \
	do { \
		for (const auto [strhash, funcArgs] : states) { \
			if (statesChain.empty()) { /*default state*/ \
				std::apply(funcArgs.first, funcArgs.second); \
				continue; \
			} \
			const auto prev = statesChain.top().states; \
			const auto prevStateIt = prev.find(strhash); \
			if (prevStateIt == prev.cend()) { /*haven't seen this state in previous states*/ \
				/* TODO: do something to save revert state */ \
				std::apply(funcArgs.first, funcArgs.second); \
				continue; \
			} \
			if (prevStateIt->second.second != funcArgs.second) { /*previous state's args are different to this state args*/ \
				std::apply(funcArgs.first, funcArgs.second); \
				continue; \
			} \
		} \
	} while (false)

	#define REVERT_STATES(states) \
	do { \
		for (const auto [strhash, funcArgs] : states) { \
			if (statesChain.empty()) { /*default state*/ \
				continue; \
			} \
			const auto prev = statesChain.top().states; \
			const auto prevStateIt = prev.find(strhash); \
			if (prevStateIt == prev.cend()) { /*haven't seen this state in previous states*/ \
				/* TODO: do something to save revert state */ \
				throw std::runtime_error("[FixedPipelineState::BindUnbind] Cannot revert state that has no default value"); \
			} \
			if (prevStateIt->second.second != funcArgs.second) { /*previous state's args are different to this state args*/ \
				std::apply(prevStateIt->second.first, prevStateIt->second.second); \
				continue; \
			} \
		} \
	} while (false)



	if (bind) {
		APPLY_STATES(b1States);
		APPLY_STATES(b4States);

		APPLY_STATES(e1States);
		APPLY_STATES(e2States);

		APPLY_STATES(i4States);

		APPLY_STATES(e1f1States);

		APPLY_STATES(f1States);
		APPLY_STATES(f2States);
		APPLY_STATES(f4States);
	}
	else
	{
		REVERT_STATES(b1States);
		REVERT_STATES(b4States);

		REVERT_STATES(e1States);
		REVERT_STATES(e2States);

		REVERT_STATES(i4States);

		REVERT_STATES(e1f1States);

		REVERT_STATES(f1States);
		REVERT_STATES(f2States);
		REVERT_STATES(f4States);
	}

	//now enable/disable states
	for (const auto [state, status] : binaryStates) {
		/*
		  b, s ==> e
		  1, 1 ==> 1
		  0, 1 ==> 0
		  1, 1 ==> 0
		  0, 0 ==> 1
		*/
		const bool en = !(bind ^ status);
		if (en)
			glEnable(state);
		else
			glDisable(state);
	}

	if (bind)
		statesChain.push(*this);
}
