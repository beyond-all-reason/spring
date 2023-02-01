/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// TODO: move this out of Sim, this is rendering code!

#include "LineDrawer.h"

#include <cmath>

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Game/UI/CommandColors.h"

CLineDrawer lineDrawer;

void CLineDrawer::DrawLine(const float3& endPos, const float* color)
{
	LinePair& p = allLines[lineStipple].emplace_back();

	if (!useColorRestarts) {
		p.colors.push_back(color[0]);
		p.colors.push_back(color[1]);
		p.colors.push_back(color[2]);
		p.colors.push_back(color[3]);
		p.verts.push_back(endPos.x);
		p.verts.push_back(endPos.y);
		p.verts.push_back(endPos.z);
	}
	else {
		if (useRestartColor) {
			p.colors.push_back(restartColor[0]);
			p.colors.push_back(restartColor[1]);
			p.colors.push_back(restartColor[2]);
			p.colors.push_back(restartColor[3]);
		}
		else {
			p.colors.push_back(color[0]);
			p.colors.push_back(color[1]);
			p.colors.push_back(color[2]);
			p.colors.push_back(color[3] * restartAlpha);
		}
		p.verts.push_back(lastPos.x);
		p.verts.push_back(lastPos.y);
		p.verts.push_back(lastPos.z);

		p.colors.push_back(color[0]);
		p.colors.push_back(color[1]);
		p.colors.push_back(color[2]);
		p.colors.push_back(color[3]);
		p.verts.push_back(endPos.x);
		p.verts.push_back(endPos.y);
		p.verts.push_back(endPos.z);
	}

	lastPos = endPos;
	lastColor = color;
}


void CLineDrawer::Restart()
{
	LinePair& p = allLines[lineStipple].emplace_back();

	if (!useColorRestarts) {
		p.type = GL_LINE_STRIP;
		p.colors.push_back(lastColor[0]);
		p.colors.push_back(lastColor[1]);
		p.colors.push_back(lastColor[2]);
		p.colors.push_back(lastColor[3]);
		p.verts.push_back(lastPos[0]);
		p.verts.push_back(lastPos[1]);
		p.verts.push_back(lastPos[2]);
	}
	else {
		p.type = GL_LINES;
	}
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
	if (allLines[0].empty() && allLines[1].empty())
		return;
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_STIPPLE);

	auto& solid    = allLines[0];
	auto& stippled = allLines[1];

	for (int i = 0; i<solid.size(); ++i) {
		int size = solid[i].colors.size();
		if(size > 0) {
			glColorPointer(4, GL_FLOAT, 0, &solid[i].colors[0]);
			glVertexPointer(3, GL_FLOAT, 0, &solid[i].verts[0]);
			glDrawArrays(solid[i].type, 0, size/4);
		}
	}

	if (!stippled.empty()) {
		glEnable(GL_LINE_STIPPLE);
		for (int i = 0; i<stippled.size(); ++i) {
			int size = stippled[i].colors.size();
			if(size > 0) {
				glColorPointer(4, GL_FLOAT, 0, &stippled[i].colors[0]);
				glVertexPointer(3, GL_FLOAT, 0, &stippled[i].verts[0]);
				glDrawArrays(stippled[i].type, 0, size/4);
			}
		}
		glDisable(GL_LINE_STIPPLE);
	}

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glPopAttrib();

	solid.clear();
	stippled.clear();
}
