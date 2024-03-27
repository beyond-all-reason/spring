/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "BasicWater.h"
#include "ISky.h"
#include "WaterRendering.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

#include <tracy/Tracy.hpp>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CBasicWater::InitResources(bool loadShader)
{
	//ZoneScoped;
	CBitmap waterTexBM;
	if (!waterTexBM.Load(waterRendering->texture)) {
		LOG_L(L_WARNING, "[%s] could not read water texture from file \"%s\"", __FUNCTION__, waterRendering->texture.c_str());

		// fallback
		waterTexBM.AllocDummy(SColor(0,0,255,255));
	}

	// create mipmapped texture
	textureID = waterTexBM.CreateMipMapTexture();
	xsize = waterTexBM.xsize;
	ysize = waterTexBM.ysize;

	GenWaterQuadsRB();
}

void CBasicWater::FreeResources()
{
	//ZoneScoped;
	glDeleteTextures(1, &textureID);
}

void CBasicWater::GenWaterQuadsRB()
{
	//ZoneScoped;
	static constexpr float div16 = 1.0f / 16.0f;

	const float mapSizeX = mapDims.mapx * SQUARE_SIZE;
	const float mapSizeY = mapDims.mapy * SQUARE_SIZE;

	// Calculate number of times texture should repeat over the map,
	// taking aspect ratio into account.
	float repeatX = 65536.0f / mapDims.mapx;
	float repeatY = 65536.0f / mapDims.mapy * xsize / ysize;

	// Use better repeat setting of 1 repeat per 4096 mapx/mapy for the new
	// ocean.jpg while retaining backward compatibility with old maps relying
	// on 1 repeat per 1024 mapx/mapy. (changed 16/05/2007)
	if (waterRendering->texture == "bitmaps/ocean.jpg") {
		repeatX /= 4;
		repeatY /= 4;
	}

	repeatX = (waterRendering->repeatX != 0 ? waterRendering->repeatX : repeatX) / 16;
	repeatY = (waterRendering->repeatY != 0 ? waterRendering->repeatY : repeatY) / 16;

	rb = TypedRenderBuffer<VA_TYPE_T>(4 * 16 * 16, 6 * 16 * 16, IStreamBufferConcept::Types::SB_BUFFERDATA);
	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			rb.AddQuadTriangles(
				{ { (x + 0) * mapSizeX * div16, 0, (y + 0) * mapSizeY * div16 }, (x + 0) * repeatX, (y + 0) * repeatY },
				{ { (x + 0) * mapSizeX * div16, 0, (y + 1) * mapSizeY * div16 }, (x + 0) * repeatX, (y + 1) * repeatY },
				{ { (x + 1) * mapSizeX * div16, 0, (y + 1) * mapSizeY * div16 }, (x + 1) * repeatX, (y + 1) * repeatY },
				{ { (x + 1) * mapSizeX * div16, 0, (y + 0) * mapSizeY * div16 }, (x + 1) * repeatX, (y + 0) * repeatY }
			);
		}
	}
	rb.SetReadonly();
}



void CBasicWater::Draw()
{
	//ZoneScoped;
	if (!waterRendering->forceRendering && !readMap->HasVisibleWater())
		return;

	glPushAttrib(GL_FOG_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT);

	glDisable(GL_ALPHA_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_TEXTURE_2D);

	const auto& sky = ISky::GetSky();
	sky->SetupFog();
	glPolygonMode(GL_FRONT_AND_BACK, mix(GL_FILL, GL_LINE, static_cast<int>(wireFrameMode)));

	glBindTexture(GL_TEXTURE_2D, textureID);

	auto& sh = rb.GetShader();
	sh.Enable();
	sh.SetUniform("ucolor", 0.7f, 0.7f, 0.7f, 0.5f);
	rb.DrawElements(GL_TRIANGLES, false);
	sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);
	sh.Disable();

	glBindTexture(GL_TEXTURE_2D,         0);

	glPopAttrib();
}
