/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LineDrawer.h"

#include <cmath>
#include <array>
#include <algorithm>

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Game/UI/CommandColors.h"
#include "System/SpringMath.h"
#include "System/SpringHash.h"

CLineDrawer lineDrawer;

struct LinePairHash {
	uint64_t operator()(const CLineDrawer::LinePair& lp) const {
		return static_cast<uint64_t>(spring::LiteHash(lp.p0)) << 32 | spring::LiteHash(lp.p1);
	}
};

void CLineDrawer::DrawLine(const float3& endPos, const SColor& color)
{
	SColor beginColor;

	if (forceRestart)
		beginColor = lastColor;
	else if (useColorRestarts && !useRestartColor)
		beginColor = SColor{ color.r, color.g, color.b, static_cast<uint8_t>(color.a * restartAlpha) };
	else if (useColorRestarts &&  useRestartColor)
		beginColor = restartColor;
	else
		beginColor = color;

	LinePair lp;
	LinePairHash lph;
	lp.p0 = VA_TYPE_C{ lastPos, beginColor };
	lp.p1 = VA_TYPE_C{ endPos ,      color };
	lp.hash = lph(lp);

	static auto sortPred = [](const LinePair& a, const LinePair& b) {
		return (a.hash < b.hash);
	};

	auto& vertexCache = vertexCaches[lineStipple];

	const auto it = spring::binary_search(vertexCache.begin(), vertexCache.end(), lp, sortPred);
	//const auto it = std::find_if(vertexCache.begin(), vertexCache.end(), [hash = lp.hash](const LinePair& lp) { return hash == lp.hash; });
	if (it == vertexCache.end()) {
		//vertexCache.emplace_back(lp);
		spring::VectorInsertSorted(vertexCache, lp, sortPred);
	}

	lastPos = endPos;
	lastColor = color;

	forceRestart = false;
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
	if (vertexCaches[0].empty() && vertexCaches[1].empty())
		return;
	
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();
	auto& sh = TypedRenderBuffer<VA_TYPE_C>::GetShader();
	sh.Enable();

	if (!vertexCaches[1].empty()) { //strippled
		for (auto& lp : vertexCaches[1]) {
			rb.AddVertices({ std::move(lp.p0), std::move(lp.p1) });
		}

		glEnable(GL_LINE_STIPPLE);
		rb.DrawArrays(GL_LINES);
	}
	if (!vertexCaches[0].empty()) { //solid
		for (auto& lp : vertexCaches[0]) {
			rb.AddVertices({ std::move(lp.p0), std::move(lp.p1) });
		}

		glDisable(GL_LINE_STIPPLE);
		rb.DrawArrays(GL_LINES);
	}

	sh.Disable();
	glPopAttrib();

	vertexCaches[0].clear();
	vertexCaches[1].clear();
}
