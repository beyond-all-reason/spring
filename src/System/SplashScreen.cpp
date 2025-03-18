/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdint>

#include <SDL.h>

#include "SplashScreen.hpp"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/Platform/Watchdog.h"

#ifndef HEADLESS
void ShowSplashScreen(
	const std::string& splashScreenFile,
	const std::string& springVersionStr,
	const std::function<bool()>& testDoneFunc
) {
	CBitmap bmp;

	VA_TYPE_2DT quadElems[] = {
		{0.0f, 1.0f,  0.0f, 0.0f},
		{0.0f, 0.0f,  0.0f, 1.0f},
		{1.0f, 0.0f,  1.0f, 1.0f},
		{1.0f, 1.0f,  1.0f, 0.0f},
	};

	// passing an empty name would cause bmp FileHandler to also
	// search inside the VFS since its default mode is RAW_FIRST
	if (splashScreenFile.empty() || !bmp.Load(splashScreenFile)) {
		bmp.AllocDummy({0, 0, 0, 255});
		quadElems[0].x = 0.5f - 0.125f * 0.5f; quadElems[0].y = 0.5f + 0.125f * 0.5f * globalRendering->aspectRatio;
		quadElems[1].x = 0.5f - 0.125f * 0.5f; quadElems[1].y = 0.5f - 0.125f * 0.5f * globalRendering->aspectRatio;
		quadElems[2].x = 0.5f + 0.125f * 0.5f; quadElems[2].y = 0.5f - 0.125f * 0.5f * globalRendering->aspectRatio;
		quadElems[3].x = 0.5f + 0.125f * 0.5f; quadElems[3].y = 0.5f + 0.125f * 0.5f * globalRendering->aspectRatio;
	}

	static constexpr const char* fmtStrs[] = {
		"[Initializing Virtual File System]",
		"* archives scanned: %u",
		"* scantime elapsed: %.1fms",
		"Recoil %s",
		"This program is distributed under the GNU General Public License, see doc/LICENSE for more information.",
	};

	char versionStrBuf[512];

	memset(versionStrBuf, 0, sizeof(versionStrBuf));
	snprintf(versionStrBuf, sizeof(versionStrBuf), fmtStrs[3], springVersionStr.c_str());

	const unsigned int splashTex = bmp.CreateTexture();
	const unsigned int fontFlags = FONT_NORM | FONT_SCALE;

	const float4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	const float4 coors = {0.5f, 0.175f, 0.8f, 0.04f}; // x, y, scale, space

	const float textWidth[3] = {font->GetTextWidth(fmtStrs[0]), font->GetTextWidth(fmtStrs[4]), font->GetTextWidth(versionStrBuf)};
	const float normWidth[3] = {
		textWidth[0] * globalRendering->pixelX * font->GetSize() * coors.z,
		textWidth[1] * globalRendering->pixelX * font->GetSize() * coors.z,
		textWidth[2] * globalRendering->pixelX * font->GetSize() * coors.z,
	};

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DT>();
	rb.AssertSubmission();
	auto& sh = rb.GetShader();

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);

	for (spring_time t0 = spring_now(), t1 = t0; !testDoneFunc(); t1 = spring_now()) {
		glClear(GL_COLOR_BUFFER_BIT);

		glBindTexture(GL_TEXTURE_2D, splashTex);

		rb.AddQuadTriangles(
			{ quadElems[0].x, quadElems[0].y, quadElems[0].s, quadElems[0].t },
			{ quadElems[1].x, quadElems[1].y, quadElems[1].s, quadElems[1].t },
			{ quadElems[2].x, quadElems[2].y, quadElems[2].s, quadElems[2].t },
			{ quadElems[3].x, quadElems[3].y, quadElems[3].s, quadElems[3].t }
		);

		sh.Enable();
		rb.DrawElements(GL_TRIANGLES);
		sh.Disable();

		font->Begin();
		font->SetTextColor(color.x, color.y, color.z, color.w);
		font->glFormat(coors.x - (normWidth[0] * 0.500f), coors.y                             , coors.z, fontFlags, fmtStrs[0]);
		font->glFormat(coors.x - (normWidth[0] * 0.475f), coors.y - (coors.w * coors.z * 1.0f), coors.z, fontFlags, fmtStrs[1], CArchiveScanner::GetNumScannedArchives());
		font->glFormat(coors.x - (normWidth[0] * 0.475f), coors.y - (coors.w * coors.z * 2.0f), coors.z, fontFlags, fmtStrs[2], (t1 - t0).toMilliSecsf());
		font->End();

		// always render Spring's license notice
		font->Begin();
		font->SetOutlineColor(0.0f, 0.0f, 0.0f, 0.65f);
		font->SetTextColor(color.x, color.y, color.z, color.w);
		font->glFormat(coors.x - (normWidth[2] * 0.5f), coors.y * 0.5f - (coors.w * coors.z * 1.0f), coors.z, fontFlags | FONT_OUTLINE, versionStrBuf);
		font->glFormat(coors.x - (normWidth[1] * 0.5f), coors.y * 0.5f - (coors.w * coors.z * 2.0f), coors.z, fontFlags | FONT_OUTLINE, fmtStrs[4]);
		font->End();

		globalRendering->SwapBuffers(true, true);

		// prevent WM's from assuming the window is unresponsive and
		// generating a kill-request
		SDL_Event event;
		while (SDL_PollEvent(&event)) {}
		Watchdog::ClearTimer(WDT_MAIN);
	}

	glPopAttrib();
	glDeleteTextures(1, &splashTex);
}
#endif

