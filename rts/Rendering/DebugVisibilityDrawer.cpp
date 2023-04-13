/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DebugVisibilityDrawer.h"

#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Map/Ground.h"
#include "Rendering/GL/glExtra.h"
#include "Sim/Misc/QuadField.h"

static constexpr float4 DEFAULT_QUAD_COLOR = float4(0.00f, 0.75f, 0.00f, 0.45f);
static unsigned int volumeDisplayListIDs[] = {0, 0, 0};
static const float squareSize = float(CQuadField::BASE_QUAD_SIZE) / SQUARE_SIZE;

static const void glWireCubeFill(uint32_t* listID) {
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
	glPolygonMode(GL_FRONT_AND_BACK, GL_POLYGON);

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

class CDebugVisibilityDrawer : public CReadMap::IQuadDrawer {
	public:
		void ResetState() {}

		void DrawQuad(int x, int y) {
			glEnable(GL_DEPTH_TEST);

			glColorf4(DEFAULT_QUAD_COLOR);
			glPushMatrix();
			const float qx = (x + 0.5) * CQuadField::BASE_QUAD_SIZE;
			const float qz = (y + 0.5) * CQuadField::BASE_QUAD_SIZE;
			const float3 normal = CGround::GetNormal(qx, qz, false);
			const float3 mid = float3(qx, CGround::GetHeightReal(qx, qz, false), qz);
			glTranslatef3(mid);
			glRotatef(asin(normal.Length())*180/math::PI, normal.x, normal.y, normal.z);
			glScalef(CQuadField::BASE_QUAD_SIZE, CQuadField::BASE_QUAD_SIZE / 4.0f, CQuadField::BASE_QUAD_SIZE);
			glWireCubeFill(&volumeDisplayListIDs[0]);
			glPopMatrix();
		}
};

namespace DebugVisibilityDrawer {
	bool enable = false;

	void Draw() {
		if (!enable)
			return;

		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHT1);
		glDisable(GL_CULL_FACE);
		glDisable(GL_TEXTURE_2D);
		// glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_FOG);
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glLineWidth(2.0f);
		glDepthMask(GL_TRUE);

		static CDebugVisibilityDrawer drawer;

		drawer.ResetState();
		readMap->GridVisibility(nullptr, &drawer, 1e9, squareSize);

		glLineWidth(1.0f);
		glPopAttrib();
	}

} // namespace DebugVisibilityDrawer
