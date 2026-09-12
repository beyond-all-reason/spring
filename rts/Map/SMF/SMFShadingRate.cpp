/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <array>

#include "SMFShadingRate.h"
#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"

#include "System/Misc/TracyDefs.h"

void CSMFShadingRate::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (texID != 0) {
		glDeleteTextures(1, &texID);
		texID = 0;
	}

	texSize = {0, 0};
}


bool CSMFShadingRate::Enable()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!globalRendering->supportShadingRateImage || elmosPerPixel <= 0.0f)
		return false;
	// the rate image is indexed in framebuffer coordinates, offset viewports would misalign
	if (globalRendering->viewPosX != 0 || globalRendering->viewPosY != 0)
		return false;
	// nearby cliffs could then show up on above-horizon rays (which get the coarsest rate) and every downward ray degrades to full rate anyway, so skip the frame
	if (camera->GetPos().y <= readMap->GetCurrMaxHeight())
		return false;

	UpdateImage();

	static constexpr std::array<GLenum, 3> palette = {
		GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV,
		GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV,
		GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV,
	};

	glBindShadingRateImageNV(texID);
	glShadingRateImagePaletteNV(0, 0, palette.size(), palette.data());
	glEnable(GL_SHADING_RATE_IMAGE_NV);
	return true;
}

void CSMFShadingRate::Disable()
{
	RECOIL_DETAILED_TRACY_ZONE;
	glDisable(GL_SHADING_RATE_IMAGE_NV);
	glBindShadingRateImageNV(0);
}


void CSMFShadingRate::UpdateImage()
{
	RECOIL_DETAILED_TRACY_ZONE;
	{
		GLint texelWidth = 16;
		GLint texelHeight = 16;
		glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV, &texelWidth);
		glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV, &texelHeight);
		texelSize = {std::max(texelWidth, 1), std::max(texelHeight, 1)};
	}

	const int2 wantedSize = {
		(globalRendering->viewSizeX + texelSize.x - 1) / texelSize.x,
		(globalRendering->viewSizeY + texelSize.y - 1) / texelSize.y,
	};

	if (wantedSize != texSize) {
		Kill();

		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		// the extension requires an immutable-format R8UI texture
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, wantedSize.x, wantedSize.y);

		texSize = wantedSize;
		rateData.resize(texSize.x * texSize.y);
	} else {
		glBindTexture(GL_TEXTURE_2D, texID);
	}

	const float3 camPos = camera->GetPos();
	const float3 fwdDir = camera->GetDir();
	const float3 upDir = camera->GetUp();
	const float3 rgtDir = camera->GetRight();

	const float vsx = std::max(globalRendering->viewSizeX, 1);
	const float vsy = std::max(globalRendering->viewSizeY, 1);

	// view-plane extent of one pixel at unit ray distance; times the ray distance this is the ground footprint in elmos
	const float pixelSize = (camera->GetTanHalfFov() * 2.0f) / vsy;
	// conservative near bound, a plane at the terrain's max height is close enough
	// since the rate thresholds sit thousands of elmos out (Enable guarantees > 0)
	const float camHeight = camPos.y - readMap->GetCurrMaxHeight();

	for (int ty = 0; ty < texSize.y; ty++) {
		// rate-image texels map to framebuffer pixels, i.e. bottom-up
		const float dy = ((ty + 0.5f) * texelSize.y - vsy * 0.5f) * pixelSize;

		for (int tx = 0; tx < texSize.x; tx++) {
			const float dx = ((tx + 0.5f) * texelSize.x - vsx * 0.5f) * pixelSize;

			const float3 rayDir = (fwdDir + upDir * dy + rgtDir * dx).Normalize();

			// rays at or above the horizon can only contain even more distant terrain
			uint8_t rate = 2;

			if (rayDir.y < 0.0f) {
				const float elmos = (camHeight / -rayDir.y) * pixelSize;

				rate = (elmos >= elmosPerPixel * 2.0f) + (elmos >= elmosPerPixel * 4.0f);
			}

			rateData[ty * texSize.x + tx] = rate;
		}
	}

	// rows are tightly packed and not necessarily 4-byte multiples
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RED_INTEGER, GL_UNSIGNED_BYTE, rateData.data());
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glBindTexture(GL_TEXTURE_2D, 0);

	glShadingRateImageBarrierNV(GL_TRUE);
}
