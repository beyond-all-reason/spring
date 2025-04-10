/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DebugVisibilityDrawer.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/GL/glExtra.h"
#include "Sim/Misc/QuadField.h"
#include "System/Color.h"

static constexpr float4 PASS_QUAD_COLOR = float4(0.00f, 0.75f, 0.00f, 0.45f);
static constexpr float4 CULL_QUAD_COLOR = float4(0.75f, 0.00f, 0.00f, 0.45f);

static constexpr float HM_SQUARE_SIZE = float(CQuadField::BASE_QUAD_SIZE) / SQUARE_SIZE;
static constexpr float WM_SQUARE_SIZE = HM_SQUARE_SIZE * SQUARE_SIZE;

struct CDebugVisibilityDrawer : public CReadMap::IQuadDrawer {
public:
	int numQuadsX;
	int numQuadsZ;
	std::vector<bool> visibleQuads;

	void ResetState()
	{
		numQuadsX = static_cast<size_t>(mapDims.mapx * SQUARE_SIZE / WM_SQUARE_SIZE);
		numQuadsZ = static_cast<size_t>(mapDims.mapy * SQUARE_SIZE / WM_SQUARE_SIZE);
		visibleQuads.resize(numQuadsX * numQuadsZ);
		std::fill(visibleQuads.begin(), visibleQuads.end(), false);
	}

	void DrawQuad(int x, int z)
	{
		assert(x < numQuadsX);
		assert(z < numQuadsZ);
		visibleQuads[z * numQuadsX + x] = true;
	}
};

CDebugVisibilityDrawer DebugVisibilityDrawer::drawer = CDebugVisibilityDrawer{};

void DebugVisibilityDrawer::DrawWorld()
{
	if (!enable)
		return;

	drawer.ResetState();
	readMap->GridVisibility(nullptr, &drawer, 1e9, HM_SQUARE_SIZE);

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_0>();
	auto& sh = rb.GetShader();
	rb.AssertSubmission();

	for (int z = 0; z < drawer.numQuadsZ; ++z) {
		for (int x = 0; x < drawer.numQuadsX; ++x) {
			if (!drawer.visibleQuads[z * drawer.numQuadsX + x])
				continue;

			AABB aabbPatch = {
			    float3{(x + 0) * WM_SQUARE_SIZE, 0.0f, (z + 0) * WM_SQUARE_SIZE},
			    float3{(x + 1) * WM_SQUARE_SIZE, 0.0f, (z + 1) * WM_SQUARE_SIZE},
			};
			float3 c = aabbPatch.CalcCenter();
			float h = CGround::GetHeightReal(c.x, c.z, false);
			aabbPatch.mins.y = h + 1.0f;
			aabbPatch.maxs.y = aabbPatch.mins.y + 50.0f;

			std::array<float3, 8> pcs;
			aabbPatch.CalcCorners(pcs);
			// bottom
			// mmm, mmM, Mmm, MmM
			// top
			// mMm, mMM, MMm, MMM

			const int32_t baseVert = rb.GetBaseVertex();

			rb.AddVertex({pcs[1]}); // 0
			rb.AddVertex({pcs[3]}); // 1
			rb.AddVertex({pcs[2]}); // 2
			rb.AddVertex({pcs[0]}); // 3

			rb.AddVertex({pcs[5]}); // 4
			rb.AddVertex({pcs[7]}); // 5
			rb.AddVertex({pcs[6]}); // 6
			rb.AddVertex({pcs[4]}); // 7

			rb.AddIndices(
			    {// bottom cap
			        3, 0, 1, 3, 1, 2,

			        // top cap
			        7, 4, 5, 7, 5, 6,

			        // sides
			        3, 7, 4, 3, 4, 0,

			        2, 6, 5, 2, 5, 1,

			        0, 4, 5, 0, 5, 1,

			        3, 7, 6, 3, 6, 2},
			    baseVert);
		}
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	sh.Enable();
	sh.SetUniform4v("ucolor", &PASS_QUAD_COLOR[0]);
	rb.DrawElements(GL_TRIANGLES);
	sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);
	sh.Disable();

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void DebugVisibilityDrawer::DrawMinimap()
{
	if (!enable)
		return;

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();
	auto& sh = rb.GetShader();
	rb.AssertSubmission();

	for (int z = 0; z < drawer.numQuadsZ; ++z) {
		for (int x = 0; x < drawer.numQuadsX; ++x) {
			SColor currCol =
			    SColor(drawer.visibleQuads[z * drawer.numQuadsX + x] ? &PASS_QUAD_COLOR.r : &CULL_QUAD_COLOR.r);

			AABB aabbPatch = {
			    float3{(x + 0) * WM_SQUARE_SIZE, 0.0f, (z + 0) * WM_SQUARE_SIZE},
			    float3{(x + 1) * WM_SQUARE_SIZE, 0.0f, (z + 1) * WM_SQUARE_SIZE},
			};
			float3 c = aabbPatch.CalcCenter();
			const auto& uhmi = readMap->GetUnsyncedHeightInfo(
			    c.x / (SQUARE_SIZE * WM_SQUARE_SIZE), c.z / (SQUARE_SIZE * WM_SQUARE_SIZE));
			aabbPatch.mins.y = uhmi.y + 1.0f;

			std::array<float3, 8> pcs;
			aabbPatch.CalcCorners(pcs);

			rb.AddQuadTriangles({pcs[1], currCol}, {pcs[3], currCol}, {pcs[2], currCol}, {pcs[0], currCol});
		}
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	sh.Enable();
	rb.DrawElements(GL_TRIANGLES);
	sh.Disable();

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}
