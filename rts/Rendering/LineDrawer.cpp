/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LineDrawer.h"

#include <cmath>
#include <array>

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Game/UI/CommandColors.h"

CLineDrawer lineDrawer;

CLineDrawer::CLineDrawer()
{
	static constexpr size_t RESERVED_NUM_OF_VERTICES = 1 << 12;
	for (auto& rb : rbs) {
		rb = std::make_unique<TypedRenderBuffer<VA_TYPE_C>>(RESERVED_NUM_OF_VERTICES, 0u);
	}
}

void CLineDrawer::DrawLine(const float3& endPos, const SColor& color)
{
	const size_t arrayIndex = useColorRestarts * 2 + lineStipple * 1;
	auto& rb = rbs[arrayIndex];

	if (useColorRestarts) {
		auto beginColor = useRestartColor ? restartColor : SColor{ color.r, color.g, color.b, static_cast<uint8_t>(color.a * restartAlpha) };
		rb->AddVertex({ lastPos, beginColor });
	}

	rb->AddVertex({ endPos, color });

	lastPos = endPos;
	lastColor = color;
}


void CLineDrawer::Restart()
{
	//will have to restart anyway in DrawLine
	if (useColorRestarts)
		return;

	const size_t arrayIndex = /* useColorRestarts * 2 = 0*/ lineStipple * 1;
	auto& rb = rbs[arrayIndex];

	rb->AddVertex({ lastPos, lastColor });
}

void CLineDrawer::UpdateLineStipple()
{
	stippleTimer += (globalRendering->lastFrameTime * 0.001f * cmdColors.StippleSpeed());
	stippleTimer = std::fmod(stippleTimer, (16.0f / 20.0f));
}

void CLineDrawer::SetupLineStipple()
{
	const unsigned int stipPat = (0xffff & cmdColors.StipplePattern());
	if ((stipPat != 0x0000) && (stipPat != 0xffff)) {
		lineStipple = true;
	} else {
		lineStipple = false;
		return;
	}
	const unsigned int fullPat = (stipPat << 16) | (stipPat & 0x0000ffff);
	const int shiftBits = 15 - (int(stippleTimer * 20.0f) % 16);
	glLineStipple(cmdColors.StippleFactor(), (fullPat >> shiftBits));
}

void CLineDrawer::DrawAll()
{
	std::array<bool, 5> draw = {false};
	for (size_t i = 0; i < rbs.size(); ++i) {
		draw[i]  = rbs[i]->ShouldSubmit(false);
		draw[4] |= draw[i];
	}

	if (!draw[4])
		return;
	
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	auto& sh = TypedRenderBuffer<VA_TYPE_C>::GetShader();
	sh.Enable();

	if (draw[1] || draw[3]) { //strippled
		glEnable(GL_LINE_STIPPLE);
		if (draw[1]) { //strippled, no color restart
			rbs[1]->DrawArrays(GL_LINE_STRIP);
		}
		if (draw[3]) { //strippled, color restart
			rbs[3]->DrawArrays(GL_LINES);
		}
	}
	if (draw[0] || draw[2]) { //solid
		glDisable(GL_LINE_STIPPLE);
		if (draw[0]) { //solid, no color restart
			rbs[0]->DrawArrays(GL_LINE_STRIP);
		}
		if (draw[2]) { //solid, color restart
			rbs[2]->DrawArrays(GL_LINES);
		}
	}

	sh.Disable();

	glPopAttrib();
}
