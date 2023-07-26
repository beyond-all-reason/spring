/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DebugVisibilityDrawer.h"

#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Map/Ground.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/RenderBuffers.h"
#include "System/Color.h"
#include "Sim/Misc/QuadField.h"

static constexpr float4 PASS_QUAD_COLOR = float4(0.00f, 0.75f, 0.00f, 0.45f);
static constexpr float4 CULL_QUAD_COLOR = float4(0.75f, 0.00f, 0.00f, 0.45f);

static constexpr float HM_SQUARE_SIZE = float(CQuadField::BASE_QUAD_SIZE) / SQUARE_SIZE;
static constexpr float WM_SQUARE_SIZE = HM_SQUARE_SIZE * SQUARE_SIZE;

struct CDebugVisibilityDrawer : public CReadMap::IQuadDrawer {
	public:
		CDebugVisibilityDrawer(VisibleQuadData& data) : data{data} {}
		void ResetState() {
			auto& q = data;
			q.numQuadsX = static_cast<size_t>(mapDims.mapx * SQUARE_SIZE / WM_SQUARE_SIZE);
			q.numQuadsZ = static_cast<size_t>(mapDims.mapy * SQUARE_SIZE / WM_SQUARE_SIZE);
			q.quads.resize(q.numQuadsX * q.numQuadsZ);
			std::fill(q.quads.begin(), q.quads.end(), false);
		}
		void DrawQuad(int x, int z) {
			auto& q = data;
			assert(!q.visibleQuads.empty());
			assert(x < q.numQuadsX);
			assert(z < q.numQuadsZ);
			q.quads[z * q.numQuadsX + x] = true;
		}

		VisibleQuadData& data;
};

void VisibleQuadData::Init() {
	CDebugVisibilityDrawer{*this}.ResetState();
}

void VisibleQuadData::Update() {
	auto drawer = CDebugVisibilityDrawer{*this};
	drawer.ResetState();
	readMap->GridVisibility(nullptr, &drawer, 1e9, HM_SQUARE_SIZE);
}

bool VisibleQuadData::isInQuads(const float3& pos) {
	auto quadId = quadField.WorldPosToQuadFieldIdx(pos);
	assert(!quads.empty());
	return quads[quadId];
}

void DebugVisibilityDrawer::DrawWorld()
{
	if (!enable)
		return;

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_0>();
	auto& sh = rb.GetShader();
	rb.AssertSubmission();

	auto& q = CamVisibleQuads;
	for (int z = 0; z < q.GetNumQuadsZ(); ++z) {
		for (int x = 0; x < q.GetNumQuadsX(); ++x) {
			if (!q.GetQuads()[z * q.GetNumQuadsX() + x])
				continue;

			AABB aabbPatch = {
				float3{ (x + 0) * WM_SQUARE_SIZE, 0.0f, (z + 0) * WM_SQUARE_SIZE },
				float3{ (x + 1) * WM_SQUARE_SIZE, 0.0f, (z + 1) * WM_SQUARE_SIZE },
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

			rb.AddVertex({ pcs[1] }); //0
			rb.AddVertex({ pcs[3] }); //1
			rb.AddVertex({ pcs[2] }); //2
			rb.AddVertex({ pcs[0] }); //3

			rb.AddVertex({ pcs[5] }); //4
			rb.AddVertex({ pcs[7] }); //5
			rb.AddVertex({ pcs[6] }); //6
			rb.AddVertex({ pcs[4] }); //7

			rb.AddIndices({
				// bottom cap
				3, 0, 1,
				3, 1, 2,

				// top cap
				7, 4, 5,
				7, 5, 6,

				// sides
				3, 7, 4,
				3, 4, 0,

				2, 6, 5,
				2, 5, 1,

				0, 4, 5,
				0, 5, 1,

				3, 7, 6,
				3, 6, 2
			}, baseVert);
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

	auto& q = CamVisibleQuads;
	for (int z = 0; z < q.GetNumQuadsZ(); ++z) {
		for (int x = 0; x < q.GetNumQuadsX(); ++x) {
			SColor currCol = SColor(q.GetQuads()[z * q.GetNumQuadsX() + x] ? &PASS_QUAD_COLOR.r : &CULL_QUAD_COLOR.r);
			
			AABB aabbPatch = {
				float3{ (x + 0) * WM_SQUARE_SIZE, 0.0f, (z + 0) * WM_SQUARE_SIZE },
				float3{ (x + 1) * WM_SQUARE_SIZE, 0.0f, (z + 1) * WM_SQUARE_SIZE },
			};
			float3 c = aabbPatch.CalcCenter();
			const auto& uhmi = readMap->GetUnsyncedHeightInfo(c.x / (SQUARE_SIZE * WM_SQUARE_SIZE), c.z / (SQUARE_SIZE * WM_SQUARE_SIZE));
			aabbPatch.mins.y = uhmi.y + 1.0f;

			std::array<float3, 8> pcs;
			aabbPatch.CalcCorners(pcs);

			rb.AddQuadTriangles(
				{ pcs[1], currCol},
				{ pcs[3], currCol},
				{ pcs[2], currCol},
				{ pcs[0], currCol}
			);
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
