/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/MiniMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"

#include "Sim/Path/Default/IPath.h"
#include "Sim/Path/Default/PathFinder.h"
#include "Sim/Path/Default/PathFinderDef.h"
#include "Sim/Path/Default/PathEstimator.h"
#include "Sim/Path/Default/PathManager.h"
#include "Sim/Path/Default/PathHeatMap.hpp"
#include "Sim/Path/Default/PathFlowMap.hpp"

#include "Rendering/Fonts/glFont.h"
#include "Rendering/DefaultPathDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/VertexArray.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"

#define PE_EXTRA_DEBUG_OVERLAYS 1

static const SColor buildColors[] = {
	SColor(138, 138, 138), // nolos
	SColor(  0, 210,   0), // free
	SColor(190, 180,   0), // objblocked
	SColor(210,   0,   0), // terrainblocked
};

static inline const SColor& GetBuildColor(const DefaultPathDrawer::BuildSquareStatus& status) {
	return buildColors[status];
}




DefaultPathDrawer::DefaultPathDrawer(): IPathDrawer()
{
	pm = dynamic_cast<CPathManager*>(pathManager);
}

void DefaultPathDrawer::DrawAll() const {
	// CPathManager is not thread-safe
	if (enabled && (gs->cheatEnabled || gu->spectating)) {
		glPushAttrib(GL_ENABLE_BIT);

		Draw(); // draw paths and goals
		Draw(pm->GetMaxResPF()); // draw PF grid-overlay
		Draw(pm->GetMedResPE()); // draw PE grid-overlay (med-res)
		Draw(pm->GetLowResPE()); // draw PE grid-overlay (low-res)

		glPopAttrib();
	}
}


void DefaultPathDrawer::DrawInMiniMap()
{
	const CPathEstimator* pe = pm->GetMedResPE();

	if (!IsEnabled() || (!gs->cheatEnabled && !gu->spectatingFullView))
		return;

	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.0, -1.0);
		minimap->ApplyConstraintsMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef3(UpVector);
		glScalef(1.0f / mapDims.mapx, -1.0f / mapDims.mapy, 1.0f);

	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 0.0f, 0.7f);

	for (const int2& sb: pe->GetUpdatedBlocks()) {
		const int blockIdxX = sb.x * pe->GetBlockSize();
		const int blockIdxY = sb.y * pe->GetBlockSize();
		glRectf(blockIdxX, blockIdxY, blockIdxX + pe->GetBlockSize(), blockIdxY + pe->GetBlockSize());
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
}

void DefaultPathDrawer::Draw() const {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glLineWidth(3);

	for (const auto& p: pm->GetPathMap()) {
		const CPathManager::MultiPath& multiPath = p.second;

		glBegin(GL_LINE_STRIP);

			// draw low-res segments of <path> (green)
			glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
			for (auto pvi = multiPath.lowResPath.path.begin(); pvi != multiPath.lowResPath.path.end(); ++pvi) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

			// draw med-res segments of <path> (blue)
			glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
			for (auto pvi = multiPath.medResPath.path.begin(); pvi != multiPath.medResPath.path.end(); ++pvi) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

			// draw max-res segments of <path> (red)
			glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
			for (auto pvi = multiPath.maxResPath.path.begin(); pvi != multiPath.maxResPath.path.end(); ++pvi) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

		glEnd();
	}

	// draw path definitions (goal, radius)
	for (const auto& p: pm->GetPathMap()) {
		Draw(&p.second.peDef);
	}

	glLineWidth(1);
}



void DefaultPathDrawer::Draw(const CPathFinderDef* pfd) const {
	if (pfd->synced) {
		glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
	} else {
		glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
	}
	glSurfaceCircle(pfd->wsGoalPos, std::sqrt(pfd->sqGoalRadius), 20);
}

void DefaultPathDrawer::Draw(const CPathFinder* pf) const {
	glColor3f(0.7f, 0.2f, 0.2f);
	glDisable(GL_TEXTURE_2D);
	CVertexArray* va = GetVertexArray();
	va->Initialize();

	for (unsigned int idx = 0; idx < pf->openBlockBuffer.GetSize(); idx++) {
		const PathNode* os = pf->openBlockBuffer.GetNode(idx);
		const int2 sqr = os->nodePos;
		const int square = os->nodeNum;
		float3 p1;
			p1.x = sqr.x * SQUARE_SIZE;
			p1.z = sqr.y * SQUARE_SIZE;
			p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 15.0f;

		const unsigned int dir = pf->blockStates.nodeMask[square] & PATHOPT_CARDINALS;
		const int2 obp = sqr - PF_DIRECTION_VECTORS_2D[dir];
		float3 p2;
			p2.x = obp.x * SQUARE_SIZE;
			p2.z = obp.y * SQUARE_SIZE;
			p2.y = CGround::GetHeightAboveWater(p2.x, p2.z, false) + 15.0f;

		if (!camera->InView(p1) && !camera->InView(p2))
			continue;

		va->AddVertex0(p1);
		va->AddVertex0(p2);
	}

	va->DrawArray0(GL_LINES);
}



void DefaultPathDrawer::Draw(const CPathEstimator* pe) const {
	const MoveDef* md = GetSelectedMoveDef();
	const PathNodeStateBuffer& blockStates = pe->blockStates;

	if (md == nullptr)
		return;

	glDisable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 0.0f);

	#if (PE_EXTRA_DEBUG_OVERLAYS == 1)
	const int overlayPeriod = GAME_SPEED * 5;
	const int overlayNumber = (gs->frameNum % (overlayPeriod * 2)) / overlayPeriod;

	const bool drawLowResPE = (overlayNumber == 1 && pe == pm->GetLowResPE());
	const bool drawMedResPE = (overlayNumber == 0 && pe == pm->GetMedResPE());

	// alternate between the extra debug-overlays
	// (normally TMI, but useful to keep the code
	// compiling)
	if (drawLowResPE || drawMedResPE) {
		glBegin(GL_LINES);

		const int2 peNumBlocks = pe->GetNumBlocks();
		const int vertexBaseNr = md->pathType * peNumBlocks.x * peNumBlocks.y * PATH_DIRECTION_VERTICES;

		for (int z = 0; z < peNumBlocks.y; z++) {
			for (int x = 0; x < peNumBlocks.x; x++) {
				const int blockNr = pe->BlockPosToIdx(int2(x, z));

				float3 p1;
					p1.x = (blockStates.peNodeOffsets[md->pathType][blockNr].x) * SQUARE_SIZE;
					p1.z = (blockStates.peNodeOffsets[md->pathType][blockNr].y) * SQUARE_SIZE;
					p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 10.0f;

				if (!camera->InView(p1))
					continue;

				glColor3f(1.0f, 1.0f, 0.75f * drawLowResPE);
				glVertexf3(p1);
				glVertexf3(p1 - UpVector * 10.0f);

				for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
					const int obx = x + PE_DIRECTION_VECTORS[dir].x;
					const int obz = z + PE_DIRECTION_VECTORS[dir].y;

					if (obx <              0) continue;
					if (obz <              0) continue;
					if (obx >= peNumBlocks.x) continue;
					if (obz >= peNumBlocks.y) continue;

					const int obBlockNr = obz * peNumBlocks.x + obx;
					const int vertexNr = vertexBaseNr + blockNr * PATH_DIRECTION_VERTICES + GetBlockVertexOffset(dir, peNumBlocks.x);

					const float rawCost = pe->GetVertexCosts()[vertexNr];
					const float nrmCost = (rawCost * PATH_NODE_SPACING) / pe->BLOCK_SIZE;

					if (rawCost >= PATHCOST_INFINITY)
						continue;

					float3 p2;
						p2.x = (blockStates.peNodeOffsets[md->pathType][obBlockNr].x) * SQUARE_SIZE;
						p2.z = (blockStates.peNodeOffsets[md->pathType][obBlockNr].y) * SQUARE_SIZE;
						p2.y = CGround::GetHeightAboveWater(p2.x, p2.z, false) + 10.0f;

					glColor3f(1.0f / std::sqrt(nrmCost), 1.0f / nrmCost, 0.75f * drawLowResPE);
					glVertexf3(p1);
					glVertexf3(p2);
				}
			}
		}

		glEnd();

		for (int z = 0; z < peNumBlocks.y; z++) {
			for (int x = 0; x < peNumBlocks.x; x++) {
				const int blockNr = pe->BlockPosToIdx(int2(x, z));

				float3 p1;
					p1.x = (blockStates.peNodeOffsets[md->pathType][blockNr].x) * SQUARE_SIZE;
					p1.z = (blockStates.peNodeOffsets[md->pathType][blockNr].y) * SQUARE_SIZE;
					p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 10.0f;

				if (!camera->InView(p1))
					continue;

				for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
					const int obx = x + PE_DIRECTION_VECTORS[dir].x;
					const int obz = z + PE_DIRECTION_VECTORS[dir].y;

					if (obx <              0) continue;
					if (obz <              0) continue;
					if (obx >= peNumBlocks.x) continue;
					if (obz >= peNumBlocks.y) continue;

					const int obBlockNr = obz * peNumBlocks.x + obx;
					const int vertexNr = vertexBaseNr + blockNr * PATH_DIRECTION_VERTICES + GetBlockVertexOffset(dir, peNumBlocks.x);

					// rescale so numbers remain near 1.0 (more readable)
					const float rawCost = pe->GetVertexCosts()[vertexNr];
					const float nrmCost = (rawCost * PATH_NODE_SPACING) / pe->BLOCK_SIZE;

					if (rawCost >= PATHCOST_INFINITY)
						continue;

					float3 p2;
						p2.x = (blockStates.peNodeOffsets[md->pathType][obBlockNr].x) * SQUARE_SIZE;
						p2.z = (blockStates.peNodeOffsets[md->pathType][obBlockNr].y) * SQUARE_SIZE;
						p2.y = CGround::GetHeightAboveWater(p2.x, p2.z, false) + 10.0f;

					// draw cost at middle of edge
					p2 = (p1 + p2) * 0.5f;

					if (!camera->InView(p2))
						continue;
					if (camera->GetPos().SqDistance(p2) >= (1000.0f * 1000.0f))
						continue;

					font->SetTextColor(1.0f, 1.0f / nrmCost, 0.75f * drawLowResPE, 1.0f);
					font->glWorldPrint(p2, 5.0f, FloatToString(nrmCost, "f(%.2f)"));
				}
			}
		}
	}
	#endif

	// [0] := low-res, [1] := med-res
	const SColor colors[2] = {SColor(0.2f, 0.7f, 0.7f, 1.0f), SColor(0.7f, 0.2f, 0.7f, 1.0f)};
	const SColor& color = colors[pe == pm->GetMedResPE()];

	glColor3ub(color.r, color.g, color.b);

	{
		CVertexArray* va = GetVertexArray();
		va->Initialize();

		for (unsigned int idx = 0; idx < pe->openBlockBuffer.GetSize(); idx++) {
			const PathNode* ob = pe->openBlockBuffer.GetNode(idx);
			const int blockNr = ob->nodeNum;

			auto pathOptDir = blockStates.nodeMask[blockNr] & PATHOPT_CARDINALS;
			auto pathDir = PathOpt2PathDir(pathOptDir);
			const int2 obp = pe->BlockIdxToPos(blockNr) - PE_DIRECTION_VECTORS[pathDir];
			const int obBlockNr = pe->BlockPosToIdx(obp);

			if (obBlockNr < 0)
				continue;

			float3 p1;
				p1.x = (blockStates.peNodeOffsets[md->pathType][blockNr].x) * SQUARE_SIZE;
				p1.z = (blockStates.peNodeOffsets[md->pathType][blockNr].y) * SQUARE_SIZE;
				p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 15.0f;
			float3 p2;
				p2.x = (blockStates.peNodeOffsets[md->pathType][obBlockNr].x) * SQUARE_SIZE;
				p2.z = (blockStates.peNodeOffsets[md->pathType][obBlockNr].y) * SQUARE_SIZE;
				p2.y = CGround::GetHeightAboveWater(p2.x, p2.z, false) + 15.0f;

			if (!camera->InView(p1) && !camera->InView(p2))
				continue;

			va->AddVertex0(p1);
			va->AddVertex0(p2);
		}
		va->DrawArray0(GL_LINES);
	}

	#if (PE_EXTRA_DEBUG_OVERLAYS == 1)
	if (drawLowResPE || drawMedResPE) {
		return; // TMI

		const PathNodeBuffer& openBlockBuffer = pe->openBlockBuffer;
		char blockCostsStr[32];

		for (unsigned int blockIdx = 0; blockIdx < openBlockBuffer.GetSize(); blockIdx++) {
			const PathNode* ob = openBlockBuffer.GetNode(blockIdx);
			const int blockNr = ob->nodeNum;

			float3 p1;
				p1.x = (blockStates.peNodeOffsets[md->pathType][blockNr].x) * SQUARE_SIZE;
				p1.z = (blockStates.peNodeOffsets[md->pathType][blockNr].y) * SQUARE_SIZE;
				p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 35.0f;

			if (!camera->InView(p1))
				continue;
			if (camera->GetPos().SqDistance(p1) >= (4000.0f * 4000.0f))
				continue;

			SNPRINTF(blockCostsStr, sizeof(blockCostsStr), "f(%.2f) g(%.2f)", ob->fCost, ob->gCost);
			font->SetTextColor(1.0f, 0.7f, 0.75f * drawLowResPE, 1.0f);
			font->glWorldPrint(p1, 5.0f, blockCostsStr);
		}
	}
	#endif
}

