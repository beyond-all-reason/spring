/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include "RenderBuffers.h"

/*
 *  Draw a circle / rectangle on top of the top surface (ground/water).
 *  Note: Uses the current color.
 */

class CWeapon;
struct WeaponDef;

extern void glSurfaceCircle(const float3& center, float radius, const SColor& col, uint32_t res);

// params.x := radius, params.y := slope, params.z := gravity
extern void glBallisticCircle(const CWeapon* weapon     , const SColor& color, uint32_t resolution, const float3& center, const float3& params);
extern void glBallisticCircle(const WeaponDef* weaponDef, const SColor& color, uint32_t resolution, const float3& center, const float3& params);

using DrawVolumeFunc = void (*)(const void* data);
extern void glDrawVolume(DrawVolumeFunc drawFunc, const void* data);

extern void glWireCube(uint32_t* listID);
extern void glWireCylinder(uint32_t* listID, uint32_t numDivs, float zSize);
extern void glWireSphere(uint32_t* listID, uint32_t numRows, uint32_t numCols);

template<typename TQuad, typename TColor, typename TRenderBuffer> void gleDrawQuadC(const TQuad& quad, const TColor& color, TRenderBuffer& rb) {
	rb.SafeAppend({ {quad.x1, quad.y1, 0.0f}, color }); // tl
	rb.SafeAppend({ {quad.x1, quad.y2, 0.0f}, color }); // bl
	rb.SafeAppend({ {quad.x2, quad.y2, 0.0f}, color }); // br

	rb.SafeAppend({ {quad.x2, quad.y2, 0.0f}, color }); // br
	rb.SafeAppend({ {quad.x2, quad.y1, 0.0f}, color }); // tr
	rb.SafeAppend({ {quad.x1, quad.y1, 0.0f}, color }); // tl
}
