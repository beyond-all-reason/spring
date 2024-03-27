/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "glExtra.h"
#include "RenderBuffers.h"
#include "VertexArray.h"
#include "Map/Ground.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/SpringMath.h"
#include "System/Threading/ThreadPool.h"

#include <tracy/Tracy.hpp>


/**
 *  Draws a trigonometric circle in 'resolution' steps.
 */
namespace {
	template<typename Func>
	void glSurfaceCircleImpl(const float3& center, float radius, const SColor& col, uint32_t res, Func&& func)
	{
		for (uint32_t i = 0; i < res; ++i) {
			const float radians = math::TWOPI * (float)i / (float)res;
			float3 pos;
			pos.x = center.x + (fastmath::sin(radians) * radius);
			pos.z = center.z + (fastmath::cos(radians) * radius);
			pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + 5.0f;
			func(std::forward<float3>(pos), col);
		}
	}
}
void glSurfaceCircle(const float3& center, float radius, const SColor& col, uint32_t res)
{
	//ZoneScoped;
	const float4 fColor = col;

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_0>();
	rb.AssertSubmission();
	auto& sh = rb.GetShader();

	const auto addFunc = [&rb](auto&& pos, const auto& col) {
		rb.AddVertex({ std::forward<float3>(pos) });
	};

	glSurfaceCircleImpl(center, radius, col, res, addFunc);

	sh.Enable();
	sh.SetUniform("ucolor", fColor.x, fColor.y, fColor.z, fColor.w);
	rb.DrawArrays(GL_LINE_LOOP);
	sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);
	sh.Disable();
}

void glSurfaceCircleLua(const float3& center, float radius, const SColor& col, uint32_t res)
{
	//ZoneScoped;
	CVertexArray* va = GetVertexArray();
	va->Initialize();

	const auto addFunc = [va](auto&& pos, const auto& col) {
		va->AddVertexC(pos, col);
	};

	glSurfaceCircleImpl(center, radius, col, res, addFunc);

	va->DrawArrayC(GL_LINE_LOOP);
}

static constexpr float (*weaponRangeFuncs[])(const CWeapon*, const WeaponDef*, float, float) = {
	CWeapon::GetStaticRange2D,
	CWeapon::GetLiveRange2D,
};

namespace {
	std::vector<VA_TYPE_0> glBallisticCircleImpl(const CWeapon* weapon, const WeaponDef* weaponDef, uint32_t resolution, const float3& center, const float3& params)
	{
		static constexpr int resDiv = 50;

		std::vector<VA_TYPE_0> vertices;
		vertices.resize(resolution);

		const float radius = params.x;
		const float slope = params.y;

		const float wdHeightMod = weaponDef->heightmod;
		const float wdProjGravity = mix(params.z, -weaponDef->myGravity, weaponDef->myGravity != 0.0f);

		for_mt(0, resolution, [&](const int i) {
			const float radians = math::TWOPI * (float)i / (float)resolution;

			const float sinR = fastmath::sin(radians);
			const float cosR = fastmath::cos(radians);

			float maxWeaponRange = radius;

			float3 pos;
			pos.x = center.x + (sinR * maxWeaponRange);
			pos.z = center.z + (cosR * maxWeaponRange);
			pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false);

			float posHeightDelta = (pos.y - center.y) * 0.5f;
			float posWeaponRange = weaponRangeFuncs[weapon != nullptr](weapon, weaponDef, posHeightDelta* wdHeightMod, wdProjGravity);
			float rangeIncrement = (maxWeaponRange -= (posHeightDelta * slope)) * 0.5f;
			float ydiff = 0.0f;

			// "binary search" for the maximum positional range per angle, accounting for terrain height
			for (int j = 0; j < resDiv && (std::fabs(posWeaponRange - maxWeaponRange) + ydiff) >(0.01f * maxWeaponRange); j++) {
				if (posWeaponRange > maxWeaponRange) {
					maxWeaponRange += rangeIncrement;
				}
				else {
					// overshot, reduce step-size
					maxWeaponRange -= rangeIncrement;
					rangeIncrement *= 0.5f;
				}

				pos.x = center.x + (sinR * maxWeaponRange);
				pos.z = center.z + (cosR * maxWeaponRange);

				const float newY = CGround::GetHeightAboveWater(pos.x, pos.z, false);
				ydiff = std::fabs(pos.y - newY);
				pos.y = newY;

				posHeightDelta = pos.y - center.y;
				posWeaponRange = weaponRangeFuncs[weapon != nullptr](weapon, weaponDef, posHeightDelta* wdHeightMod, wdProjGravity);
			}

			pos.x = center.x + (sinR * posWeaponRange);
			pos.z = center.z + (cosR * posWeaponRange);
			pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + 5.0f;

			vertices[i].pos = std::move(pos);
		});

		return vertices;
	}
}

/*
 *  Draws a trigonometric circle in 'resolution' steps, with a slope modifier
 */

void glBallisticCircle(const CWeapon* weapon, const WeaponDef* weaponDef, const SColor& color, uint32_t resolution, const float3& center, const float3& params)
{
	//ZoneScoped;
	const float4 fColor = color;

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_0>();
	rb.AssertSubmission();


	auto vertices = glBallisticCircleImpl(weapon, weaponDef, resolution, center, params);
	rb.AddVertices(vertices);

	auto& sh = rb.GetShader();
	sh.Enable();
	sh.SetUniform("ucolor", fColor.x, fColor.y, fColor.z, fColor.w);
	rb.DrawArrays(GL_LINE_LOOP);
	sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);
	sh.Disable();
}

void glBallisticCircleLua(const CWeapon* weapon, const WeaponDef* weaponDef, const SColor& color, uint32_t resolution, const float3& center, const float3& params)
{
	//ZoneScoped;
	CVertexArray* va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(resolution, 0, VA_SIZE_C);

	auto vertices = glBallisticCircleImpl(weapon, weaponDef, resolution, center, params);
	auto* vaVertices = va->GetTypedVertexArray<VA_TYPE_C>(resolution);

	for (auto&& vert : vertices) {
		*vaVertices = VA_TYPE_C {
			std::forward<float3>(vert.pos),
			color
		};
		vaVertices++;
	}

	va->DrawArrayC(GL_LINE_LOOP);
}

void glBallisticCircle(const CWeapon* weapon     , const SColor& color, uint32_t resolution, const float3& center, const float3& params)
{
	//ZoneScoped;
	glBallisticCircle(weapon, weapon->weaponDef, color, resolution, center, params);
}

void glBallisticCircle(const WeaponDef* weaponDef, const SColor& color, uint32_t resolution, const float3& center, const float3& params)
{
	//ZoneScoped;
	glBallisticCircle(nullptr, weaponDef, color, resolution, center, params);
}

void glBallisticCircleLua(const CWeapon* weapon, const SColor& color, uint32_t resolution, const float3& center, const float3& params)
{
	//ZoneScoped;
	glBallisticCircleLua(weapon, weapon->weaponDef, color, resolution, center, params);
}

void glBallisticCircleLua(const WeaponDef* weaponDef, const SColor& color, uint32_t resolution, const float3& center, const float3& params)
{
	//ZoneScoped;
	glBallisticCircleLua(nullptr, weaponDef, color, resolution, center, params);
}


/******************************************************************************/

void glDrawVolume(DrawVolumeFunc drawFunc, const void* data)
{
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		glEnable(GL_STENCIL_TEST);
		glStencilMask(0x1);
		glStencilFunc(GL_ALWAYS, 0, 0x1);
		glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
		drawFunc(data); // draw

	glDisable(GL_DEPTH_TEST);

	glStencilFunc(GL_NOTEQUAL, 0, 0x1);
	glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO); // clear as we go

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	drawFunc(data);   // draw

	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}



/******************************************************************************/

void glWireCube(uint32_t* listID) {
	//ZoneScoped;
	static constexpr float3 vertices[8] = {
		{ 0.5f,  0.5f,  0.5f},
		{ 0.5f, -0.5f,  0.5f},
		{-0.5f, -0.5f,  0.5f},
		{-0.5f,  0.5f,  0.5f},

		{ 0.5f,  0.5f, -0.5f},
		{ 0.5f, -0.5f, -0.5f},
		{-0.5f, -0.5f, -0.5f},
		{-0.5f,  0.5f, -0.5f},
	};

	if ((*listID) != 0) {
		glCallList(*listID);
		return;
	}

	glNewList(((*listID) = glGenLists(1)), GL_COMPILE);
	glPushAttrib(GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glBegin(GL_QUADS);
		glVertex3f(vertices[0].x, vertices[0].y, vertices[0].z);
		glVertex3f(vertices[1].x, vertices[1].y, vertices[1].z);
		glVertex3f(vertices[2].x, vertices[2].y, vertices[2].z);
		glVertex3f(vertices[3].x, vertices[3].y, vertices[3].z);

		glVertex3f(vertices[4].x, vertices[4].y, vertices[4].z);
		glVertex3f(vertices[5].x, vertices[5].y, vertices[5].z);
		glVertex3f(vertices[6].x, vertices[6].y, vertices[6].z);
		glVertex3f(vertices[7].x, vertices[7].y, vertices[7].z);
	glEnd();
	glBegin(GL_QUAD_STRIP);
		glVertex3f(vertices[4].x, vertices[4].y, vertices[4].z);
		glVertex3f(vertices[0].x, vertices[0].y, vertices[0].z);

		glVertex3f(vertices[5].x, vertices[5].y, vertices[5].z);
		glVertex3f(vertices[1].x, vertices[1].y, vertices[1].z);

		glVertex3f(vertices[6].x, vertices[6].y, vertices[6].z);
		glVertex3f(vertices[2].x, vertices[2].y, vertices[2].z);

		glVertex3f(vertices[7].x, vertices[7].y, vertices[7].z);
		glVertex3f(vertices[3].x, vertices[3].y, vertices[3].z);

		glVertex3f(vertices[4].x, vertices[4].y, vertices[4].z);
		glVertex3f(vertices[0].x, vertices[0].y, vertices[0].z);
	glEnd();

	glPopAttrib();
	glEndList();
}

void glWireCylinder(uint32_t* listID, uint32_t numDivs, float zSize) {
	//ZoneScoped;
	if ((*listID) != 0) {
		glCallList(*listID);
		return;
	}

	assert(numDivs > 2);
	assert(zSize > 0.0f);

	std::vector<float3> vertices(numDivs * 2);

	glNewList(((*listID) = glGenLists(1)), GL_COMPILE);
	glPushAttrib(GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// front end-cap
	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(0.0f, 0.0f, 0.0f);

		for (uint32_t n = 0; n <= numDivs; n++) {
			const uint32_t i = n % numDivs;

			vertices[i].x = std::cos(i * (math::TWOPI / numDivs));
			vertices[i].y = std::sin(i * (math::TWOPI / numDivs));
			vertices[i].z = 0.0f;

			glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
		}
	glEnd();

	// back end-cap
	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(0.0f, 0.0f, zSize);

		for (uint32_t n = 0; n <= numDivs; n++) {
			const uint32_t i = n % numDivs;

			vertices[i + numDivs].x = vertices[i].x;
			vertices[i + numDivs].y = vertices[i].y;
			vertices[i + numDivs].z = zSize;

			glVertex3f(vertices[i + numDivs].x, vertices[i + numDivs].y, vertices[i + numDivs].z);
		}
	glEnd();

	glBegin(GL_QUAD_STRIP);
		for (uint32_t n = 0; n < numDivs; n++) {
			glVertex3f(vertices[n          ].x, vertices[n          ].y, vertices[n          ].z);
			glVertex3f(vertices[n + numDivs].x, vertices[n + numDivs].y, vertices[n + numDivs].z);
		}

		glVertex3f(vertices[0          ].x, vertices[0          ].y, vertices[0          ].z);
		glVertex3f(vertices[0 + numDivs].x, vertices[0 + numDivs].y, vertices[0 + numDivs].z);
	glEnd();

	glPopAttrib();
	glEndList();
}

void glWireSphere(uint32_t* listID, uint32_t numRows, uint32_t numCols) {
	//ZoneScoped;
	if ((*listID) != 0) {
		glCallList(*listID);
		return;
	}

	assert(numRows > 2);
	assert(numCols > 2);

	std::vector<float3> vertices((numRows + 1) * numCols);

	for (uint32_t row = 0; row <= numRows; row++) {
		for (uint32_t col = 0; col < numCols; col++) {
			const float a = (col * (math::TWOPI / numCols));
			const float b = (row * (math::PI    / numRows));

			float3& v = vertices[row * numCols + col];

			v.x = std::cos(a) * std::sin(b);
			v.y = std::sin(a) * std::sin(b);
			v.z = std::cos(b);
		}
	}

	glNewList(((*listID) = glGenLists(1)), GL_COMPILE);
	glPushAttrib(GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	const float3& v0 = vertices.front();
	const float3& v1 = vertices.back();

	// top slice
	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(v0.x, v0.y, v0.z);

		for (uint32_t col = 0; col <= numCols; col++) {
			const uint32_t i = 1 * numCols + (col % numCols);
			const float3& v = vertices[i];

			glVertex3f(v.x, v.y, v.z);
		}
	glEnd();

	// bottom slice
	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(v1.x, v1.y, v1.z);

		for (uint32_t col = 0; col <= numCols; col++) {
			const uint32_t i = ((numRows - 1) * numCols) + (col % numCols);
			const float3& v = vertices[i];

			glVertex3f(v.x, v.y, v.z);
		}
	glEnd();

	for (uint32_t row = 1; row < numRows - 1; row++) {
		glBegin(GL_QUAD_STRIP);

		for (uint32_t col = 0; col < numCols; col++) {
			const float3& v0 = vertices[(row + 0) * numCols + col];
			const float3& v1 = vertices[(row + 1) * numCols + col];

			glVertex3f(v0.x, v0.y, v0.z);
			glVertex3f(v1.x, v1.y, v1.z);
		}

		const float3& v0 = vertices[(row + 0) * numCols];
		const float3& v1 = vertices[(row + 1) * numCols];

		glVertex3f(v0.x, v0.y, v0.z);
		glVertex3f(v1.x, v1.y, v1.z);
		glEnd();
	}

	glPopAttrib();
	glEndList();
}
