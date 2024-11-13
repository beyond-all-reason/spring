/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <tuple>

#include "myGL.h"
#include "VAO.h"
#include "VBO.h"
#include "RenderBuffers.h"
#include "System/UnorderedMap.hpp"

/*
 *  Draw a circle / rectangle on top of the top surface (ground/water).
 *  Note: Uses the current color.
 */

class CWeapon;
struct WeaponDef;

extern void glSurfaceCircle(const float3& center, float radius, const SColor& col, uint32_t res);
extern void glSurfaceCircleLua(const float3& center, float radius, const SColor& col, uint32_t res);

// params.x := radius, params.y := slope, params.z := gravity
extern void glBallisticCircle(const CWeapon* weapon     , const SColor& color, uint32_t resolution, const float3& center, const float3& params);
extern void glBallisticCircle(const WeaponDef* weaponDef, const SColor& color, uint32_t resolution, const float3& center, const float3& params);
extern void glBallisticCircleLua(const CWeapon* weapon, const SColor& color, uint32_t resolution, const float3& center, const float3& params);
extern void glBallisticCircleLua(const WeaponDef* weaponDef, const SColor& color, uint32_t resolution, const float3& center, const float3& params);

using DrawVolumeFunc = void (*)(const void* data);
extern void glDrawVolume(DrawVolumeFunc drawFunc, const void* data);

template<typename TQuad, typename TColor, typename TRenderBuffer> void gleDrawQuadC(const TQuad& quad, const TColor& color, TRenderBuffer& rb) {
	rb.AddQuadTriangles(
		{ {quad.x1, quad.y1, 0.0f}, color }, //tl
		{ {quad.x2, quad.y1, 0.0f}, color }, //tr
		{ {quad.x2, quad.y2, 0.0f}, color }, //br
		{ {quad.x1, quad.y2, 0.0f}, color }  //bl
	);
}

namespace GL {
	class Shapes {
	public:
		void Init();
		void Kill();
		Shader::IProgramObject* GetShader();
		void DrawSolidSphere(uint32_t numRows, uint32_t numCols, const CMatrix44f& m, const float* color);
		void DrawWireSphere(uint32_t numRows, uint32_t numCols, const CMatrix44f& m, const float* color);
		void DrawWireCylinder(uint32_t numDivs, const CMatrix44f& m, const float* color);
		void DrawWireBox(const CMatrix44f& m, const float* color);

		void DrawSolidSphere(uint32_t numRows, uint32_t numCols);
		void DrawWireSphere(uint32_t numRows, uint32_t numCols);
		void DrawWireCylinder(uint32_t numDivs);
		void DrawWireBox();
	private:
		// add other shapes if needed
		spring::unordered_map<std::tuple<uint32_t, uint32_t>, size_t> solidSpheresMap;
		spring::unordered_map<std::tuple<uint32_t, uint32_t>, size_t> wireSpheresMap;
		spring::unordered_map<uint32_t, size_t> wireCylindersMap;
		size_t wireBoxIdx{ size_t(-1) };
		std::vector<std::tuple<VAO, VBO, VBO>> allObjects;
		Shader::IProgramObject* shader = nullptr;
	private:
		Shader::ShaderEnabledToken FillShaderUniforms(const CMatrix44f& m, const float* color) const;
		static void EnableAttribs();
		static void DisableAttribs();
		size_t CreateGLObjects(const std::vector<float3>& verts, const std::vector<uint32_t>& indcs);
		auto CreateSolidSphere(uint32_t numRows, uint32_t numCols) -> decltype(solidSpheresMap)::iterator;
		auto CreateWireSphere(uint32_t numRows, uint32_t numCols) -> decltype(wireSpheresMap)::iterator;
		auto CreateWireCylinder(uint32_t numDivs) -> decltype(wireCylindersMap)::iterator;
	};

	extern Shapes shapes;
}