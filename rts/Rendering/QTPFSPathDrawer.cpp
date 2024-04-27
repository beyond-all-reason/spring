// #undef NDEBUG

#include <limits>
#include <memory>

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/UI/MiniMap.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"

#include "Sim/Path/QTPFS/Path.h"
#include "Sim/Path/QTPFS/Node.h"
#include "Sim/Path/QTPFS/NodeLayer.h"
#include "Sim/Path/QTPFS/PathCache.h"
#include "Sim/Path/QTPFS/PathManager.h"

#include "Sim/Path/QTPFS/Components/Path.h"
#include "Sim/Path/QTPFS/Components/PathSpeedModInfo.h"
#include "Sim/Path/QTPFS/Registry.h"

#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/QTPFSPathDrawer.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Map/InfoTexture/Legacy/LegacyInfoTextureHandler.h"
#include "System/StringUtil.h"

static std::vector<const QTPFS::QTNode*> visibleNodes;

static constexpr unsigned char LINK_COLOR[4] = {1 * 255, 0 * 255, 1 * 255, 1 * 128};
static constexpr unsigned char PATH_COLOR[4] = {0 * 255, 0 * 255, 1 * 255, 1 * 255};
static constexpr unsigned char BAD_PATH_COLOR[4] = {1 * 255, 0 * 255, 0 * 255, 1 * 255};
static constexpr unsigned char INCOMPLETE_PATH_COLOR[4] = {1 * 255, 1 * 255, 0 * 255, 1 * 128};
static constexpr unsigned char NODE_COLORS[][4] = {
	{1 * 255, 0 * 255, 0 * 255, 1 * 255}, // red --> blocked
	{0 * 255, 1 * 255, 0 * 255, 1 * 255}, // green --> passable
	{1 * 255, 1 * 127, 0 * 255, 1 * 255}, // orange --> exit only
	{0 * 255, 0 * 255, 1 *  64, 1 *  64}, // light blue --> pushed
};


QTPFSPathDrawer::QTPFSPathDrawer() {
	pm = dynamic_cast<QTPFS::PathManager*>(pathManager);
}

void QTPFSPathDrawer::DrawAll() const {
	const MoveDef* md = GetSelectedMoveDef();

	if (md == nullptr)
		return;

	if (!enabled)
		return;

	if (!gs->cheatEnabled && !gu->spectating)
		return;

	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	visibleNodes.clear();
	visibleNodes.reserve(256);

	auto& nodeLayer = pm->GetNodeLayer(md->pathType);
	for (int i = 0; i < nodeLayer.GetRootNodeCount(); ++i){
		auto curRootNode = nodeLayer.GetPoolNode(i);
		GetVisibleNodes(curRootNode, nodeLayer, visibleNodes);
	}

	if (!visibleNodes.empty()) {
		auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();
		auto& sh = rb.GetShader();

		sh.Enable();

		DrawNodes(md, rb, visibleNodes, nodeLayer);
		DrawPaths(md, rb);

		sh.Disable();

		// text has its own shader, draw it last
		DrawCosts(visibleNodes);
	}

	glPopAttrib();
}

void QTPFSPathDrawer::DrawNodes(const MoveDef* md, TypedRenderBuffer<VA_TYPE_C>& rb, const std::vector<const QTPFS::QTNode*>& nodes, const QTPFS::NodeLayer& nodeLayer) const {
	for (const QTPFS::QTNode* node: nodes) {
		int nodeColour = node->AllSquaresImpassable() ? 0 : node->IsExitOnly() ? 2 : 1;
		DrawNodeW(md, node, rb, &NODE_COLORS[nodeColour][0], 0.f);
	}

	glLineWidth(2.0f);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	rb.DrawArrays(GL_QUADS);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glLineWidth(1.0f);
}

void QTPFSPathDrawer::DrawCosts(const std::vector<const QTPFS::QTNode*>& nodes) const {
	#define xmidw (node->xmid() * SQUARE_SIZE)
	#define zmidw (node->zmid() * SQUARE_SIZE)

	for (const QTPFS::QTNode* node: nodes) {
		const float3 pos = {xmidw * 1.0f, CGround::GetHeightReal(xmidw, zmidw, false) + 4.0f, zmidw * 1.0f};

		if (pos.SqDistance(camera->GetPos()) >= Square(1000.0f))
			continue;

		font->SetTextColor(0.0f, 0.0f, 0.0f, 1.0f);
		// font->glWorldPrint(pos, 5.0f, FloatToString(node->GetMoveCost(), "%8.2f"));
		// font->glWorldPrint(pos, 5.0f, IntToString(node->GetNodeNumber(), "%08x"));
		font->glWorldPrint(pos, 5.0f, IntToString(node->GetIndex(), "%d"));
	}

	font->DrawWorldBuffered();
	#undef zmidw
	#undef xmidw
}



void QTPFSPathDrawer::GetVisibleNodes(const QTPFS::QTNode* nt, const QTPFS::NodeLayer& nl, std::vector<const QTPFS::QTNode*>& nodes) const {
	if (nt->IsLeaf()) {
		nodes.push_back(nt);
		return;
	}

	for (unsigned int i = 0; i < QTNODE_CHILD_COUNT; i++) {
		const QTPFS::QTNode* cn = nl.GetPoolNode(nt->GetChildBaseIndex() + i);
		const float3 mins = float3(cn->xmin() * SQUARE_SIZE, 0.0f, cn->zmin() * SQUARE_SIZE);
		const float3 maxs = float3(cn->xmax() * SQUARE_SIZE, 0.0f, cn->zmax() * SQUARE_SIZE);

		if (!camera->InView(mins, maxs))
			continue;

		GetVisibleNodes(cn, nl, nodes);
	}
}


void QTPFSPathDrawer::DrawPaths(const MoveDef* md, TypedRenderBuffer<VA_TYPE_C>& rb) const {
	glLineWidth(4.0f);

	const auto pathView = QTPFS::registry.view<QTPFS::IPath>();
	for (const auto& pathEntity : pathView) {
		const auto* path = &pathView.get<QTPFS::IPath>(pathEntity);
		if (path->GetPathType() == md->pathType)
			DrawPath(md, path, rb);
	}

	{
		#ifdef QTPFS_DRAW_WAYPOINT_GROUND_CIRCLES
		constexpr SColor color = {0.0f, 0.0f, 1.0f, 1.0f};

		for (const auto& pathEntity : pathView) {
			const auto* path = &pathView.get<QTPFS::IPath>(pathEntity);

			for (unsigned int n = 0; n < path->NumPoints(); n++) {
				glSurfaceCircle(path->GetPoint(n), path->GetRadius(), color, 16);
				// glSurfaceCircleW(wla, {path->GetPoint(n), path->GetRadius()}, color, 16);
			}
		}
		#endif
	}

	glLineWidth(1.0f);

	#ifdef QTPFS_TRACE_PATH_SEARCHES
	const auto& pathTraces = pm->GetPathTraces();

	for (const auto& pathEntity : pathView) {
		const auto* path = &pathView.get<QTPFS::IPath>(pathEntity);
		const auto traceIt = pathTraces.find(entt::to_integral(pathEntity));

		if (traceIt == pathTraces.end())
			continue;
		// this only happens if source-node was equal to target-node
		if (traceIt->second == nullptr)
			continue;

		DrawSearchExecution(path->GetPathType(), traceIt->second, rb);
	}
	#endif
}

void QTPFSPathDrawer::DrawPath(const MoveDef* md, const QTPFS::IPath* path, TypedRenderBuffer<VA_TYPE_C>& rb) const {
	constexpr auto getHeight = [](const MoveDef* md, float x, float z) {
		if (md->FloatOnWater())
			return CGround::GetHeightAboveWater(x, z, false);
		else
			return CGround::GetHeightReal(x, z, false);
	};

	for (unsigned int n = 0; n < path->NumPoints() - 1; n++) {
		float3 p0 = path->GetPoint(n + 0);
		float3 p1 = path->GetPoint(n + 1);

		assert(p1.x >= 0.f);
		assert(p1.z >= 0.f);
		assert(p1.x / SQUARE_SIZE < mapDims.mapx);
		assert(p1.z / SQUARE_SIZE < mapDims.mapy);

		assert(p0 != float3());
		assert(p1 != float3());

		if (!camera->InView(p0) && !camera->InView(p1))
			continue;

		p0.y = getHeight(md, p0.x, p0.z);
		p1.y = getHeight(md, p1.x, p1.z);

		// if (path->GetSearchTime().toMilliSecsi() < 10LL) {
			rb.AddVertex({p0, PATH_COLOR});
			rb.AddVertex({p1, PATH_COLOR});
		// } else {
		// 	rb.AddVertex({p0, BAD_PATH_COLOR});
		// 	rb.AddVertex({p1, BAD_PATH_COLOR});
		// }
	}
	{
		float3 p0 = path->GetPoint(path->NumPoints() - 1);
		float3 p1 = path->GetGoalPosition();
		if (p0 != p1) {
			assert(p1.x >= 0.f);
			assert(p1.z >= 0.f);
			assert(p1.x / SQUARE_SIZE < mapDims.mapx);
			assert(p1.z / SQUARE_SIZE < mapDims.mapy);

			assert(p0 != float3());
			assert(p1 != float3());

			if (camera->InView(p0) || camera->InView(p1)) {
				p0.y = getHeight(md, p0.x, p0.z);
				p1.y = getHeight(md, p1.x, p1.z);

				rb.AddVertex({p0, INCOMPLETE_PATH_COLOR});
				rb.AddVertex({p1, INCOMPLETE_PATH_COLOR});
			}
		}
	}

	rb.Submit(GL_LINES);
}

void QTPFSPathDrawer::DrawSearchExecution(unsigned int pathType, const QTPFS::PathSearchTrace::Execution* se, TypedRenderBuffer<VA_TYPE_C>& rb) const {
	// TODO: prevent overdraw so oft-visited nodes do not become darker
	const std::vector<QTPFS::PathSearchTrace::Iteration>& searchIters = se->GetIterations();
	      std::vector<QTPFS::PathSearchTrace::Iteration>::const_iterator it;
	const unsigned searchFrame = se->GetFrame();

	// number of frames reserved to draw the entire trace
	constexpr unsigned int MAX_DRAW_TIME = GAME_SPEED * 5;

	unsigned int itersPerFrame = (searchIters.size() / MAX_DRAW_TIME) + 1;
	unsigned int curSearchIter = 0;
	unsigned int maxSearchIter = ((gs->frameNum - searchFrame) + 1) * itersPerFrame;

	for (it = searchIters.begin(); it != searchIters.end(); ++it) {
		if (curSearchIter++ >= maxSearchIter)
			break;

		const QTPFS::PathSearchTrace::Iteration& searchIter = *it;
		const std::vector<unsigned int>& nodeIndices = searchIter.GetNodeIndices();

		DrawSearchIteration(pathType, nodeIndices, rb);
	}
}

void QTPFSPathDrawer::DrawSearchIteration(unsigned int pathType, const std::vector<unsigned int>& nodeIndices, TypedRenderBuffer<VA_TYPE_C>& rb) const {
	unsigned int hmx = nodeIndices[0] % mapDims.mapx;
	unsigned int hmz = nodeIndices[0] / mapDims.mapx;

	const QTPFS::NodeLayer& nodeLayer = pm->GetNodeLayer(pathType);

	const QTPFS::QTNode* pushedNode = nullptr;
	const QTPFS::QTNode* poppedNode = static_cast<const QTPFS::QTNode*>(nodeLayer.GetNode(hmx, hmz));

	{
		// popped node
		DrawNode(poppedNode, rb, &NODE_COLORS[2][0]);

		// pushed nodes
		for (size_t i = 1, n = nodeIndices.size(); i < n; i++) {
			hmx = nodeIndices[i] % mapDims.mapx;
			hmz = nodeIndices[i] / mapDims.mapx;

			DrawNode(pushedNode = static_cast<const QTPFS::QTNode*>(nodeLayer.GetNode(hmx, hmz)), rb, &NODE_COLORS[2][0]);
		}

		rb.DrawElements(GL_TRIANGLES);
	}
	{
		glLineWidth(2.0f);

		for (size_t i = 1, n = nodeIndices.size(); i < n; i++) {
			hmx = nodeIndices[i] % mapDims.mapx;
			hmz = nodeIndices[i] / mapDims.mapx;

			DrawNodeLink(pushedNode = static_cast<const QTPFS::QTNode*>(nodeLayer.GetNode(hmx, hmz)), poppedNode, rb);
		}

		rb.DrawArrays(GL_LINES);
		glLineWidth(1.0f);
	}
}


#define xminw (node->xmin() * SQUARE_SIZE)
#define xmaxw (node->xmax() * SQUARE_SIZE)
#define zminw (node->zmin() * SQUARE_SIZE)
#define zmaxw (node->zmax() * SQUARE_SIZE)

void QTPFSPathDrawer::DrawNode(const QTPFS::QTNode* node, TypedRenderBuffer<VA_TYPE_C>& rb, const unsigned char* color) const {
	const float3 v0 = float3(xminw, CGround::GetHeightReal(xminw, zminw, false) + 4.0f, zminw);
	const float3 v1 = float3(xmaxw, CGround::GetHeightReal(xmaxw, zminw, false) + 4.0f, zminw);
	const float3 v2 = float3(xmaxw, CGround::GetHeightReal(xmaxw, zmaxw, false) + 4.0f, zmaxw);
	const float3 v3 = float3(xminw, CGround::GetHeightReal(xminw, zmaxw, false) + 4.0f, zmaxw);

	rb.AddQuadTriangles(
		{ v0, color },
		{ v1, color },
		{ v2, color },
		{ v3, color }
	);
}

void QTPFSPathDrawer::DrawNodeW(const MoveDef* md, const QTPFS::QTNode* node, TypedRenderBuffer<VA_TYPE_C>& rb, const unsigned char* color, float sizeAdj) const {
	constexpr auto getHeight = [](const MoveDef* md, float x, float z) {
		if (md->FloatOnWater())
			return CGround::GetHeightAboveWater(x, z, false);
		else
			return CGround::GetHeightReal(x, z, false);
	};
	
	const float3 v0 = float3(xminw - sizeAdj, getHeight(md, xminw, zminw) + 4.0f, zminw - sizeAdj);
	const float3 v1 = float3(xmaxw + sizeAdj, getHeight(md, xmaxw, zminw) + 4.0f, zminw - sizeAdj);
	const float3 v2 = float3(xmaxw + sizeAdj, getHeight(md, xmaxw, zmaxw) + 4.0f, zmaxw + sizeAdj);
	const float3 v3 = float3(xminw - sizeAdj, getHeight(md, xminw, zmaxw) + 4.0f, zmaxw + sizeAdj);

	rb.AddVertex({v0, color});
	rb.AddVertex({v1, color});
	rb.AddVertex({v2, color});
	rb.AddVertex({v3, color});
}

#undef xminw
#undef xmaxw
#undef zminw
#undef zmaxw


void QTPFSPathDrawer::DrawNodeLink(const QTPFS::QTNode* pushedNode, const QTPFS::QTNode* poppedNode, TypedRenderBuffer<VA_TYPE_C>& rb) const {
	#define xmidw(n) (n->xmid() * SQUARE_SIZE)
	#define zmidw(n) (n->zmid() * SQUARE_SIZE)

	const float3 v0 = {xmidw(pushedNode) * 1.0f, CGround::GetHeightReal(xmidw(pushedNode), zmidw(pushedNode), false) + 4.0f, zmidw(pushedNode) * 1.0f};
	const float3 v1 = {xmidw(poppedNode) * 1.0f, CGround::GetHeightReal(xmidw(poppedNode), zmidw(poppedNode), false) + 4.0f, zmidw(poppedNode) * 1.0f};


	if (!camera->InView(v0) && !camera->InView(v1))
		return;

	rb.AddVertex({v0, LINK_COLOR});
	rb.AddVertex({v1, LINK_COLOR});

	#undef xmidw
	#undef zmidw
}



#if 1
// part of LegacyInfoTexHandler, no longer called
void QTPFSPathDrawer::UpdateExtraTexture(int extraTex, int starty, int endy, int offset, unsigned char* texMem) const {
	switch (extraTex) {
		case CLegacyInfoTextureHandler::drawPathTrav: {
			const MoveDef* md = GetSelectedMoveDef();

			if (md != nullptr) {
				const QTPFS::NodeLayer& nl = pm->GetNodeLayer(md->pathType);

				auto& speedModComp = QTPFS::systemGlobals.GetSystemComponent<QTPFS::PathSpeedModInfoSystemComponent>();
				const float smr = 1.0f / ( speedModComp.relSpeedModinfos[nl.GetNodelayer()].max );
				const bool los = (gs->cheatEnabled || gu->spectating);

				for (int ty = starty; ty < endy; ++ty) {
					for (int tx = 0; tx < mapDims.hmapx; ++tx) {
						const int sqx = (tx << 1);
						const int sqz = (ty << 1);
						const int texIdx = ((ty * (mapDims.pwr2mapx >> 1)) + tx) * 4 - offset;
						const bool losSqr = losHandler->InLos(SquareToFloat3(sqx, sqz), gu->myAllyTeam);

						#if 1
						// use node-modifiers as baseline so visualisation is in sync with alt+B
						const QTPFS::QTNode* node = static_cast<const QTPFS::QTNode*>(nl.GetNode(sqx, sqz));

						const float sm = CMoveMath::GetPosSpeedMod(*md, sqx, sqz);
						const SColor& smc = GetSpeedModColor((los || losSqr)? node->GetSpeedMod() * smr: sm);
						#else
						float scale = 1.0f;

						if (los || losSqr) {
							if (CMoveMath::IsBlocked(*md, sqx,     sqz    ) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
							if (CMoveMath::IsBlocked(*md, sqx + 1, sqz    ) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
							if (CMoveMath::IsBlocked(*md, sqx,     sqz + 1) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
							if (CMoveMath::IsBlocked(*md, sqx + 1, sqz + 1) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
						}

						const float sm = CMoveMath::GetPosSpeedMod(md, sqx, sqz);
						const SColor& smc = GetSpeedModColor(sm * scale);
						#endif

						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_R] = smc.r;
						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_G] = smc.g;
						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_B] = smc.b;
						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_A] = smc.a;
					}
				}
			} else {
				// we have nothing to show -> draw a dark red overlay
				for (int ty = starty; ty < endy; ++ty) {
					for (int tx = 0; tx < mapDims.hmapx; ++tx) {
						const int texIdx = ((ty * (mapDims.pwr2mapx >> 1)) + tx) * 4 - offset;

						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_R] = 100;
						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_G] = 0;
						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_B] = 0;
						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_A] = 255;
					}
				}
			}
		} break;

		case CLegacyInfoTextureHandler::drawPathCost: {
		} break;
	}
}
#else
void QTPFSPathDrawer::UpdateExtraTexture(int extraTex, int starty, int endy, int offset, unsigned char* texMem) const {}
#endif

void QTPFSPathDrawer::DrawInMiniMap()
{
	auto mdt = pm->GetMapDamageTrack();

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

	const int blockSize = QTPFS::PathManager::DAMAGE_MAP_BLOCK_SIZE;

	auto width = mdt.width;
	auto height = mdt.height;
	float maxStrength = mdt.mapChangeTrackers.size();

	std::vector<float> mapDamageStrength;
	mapDamageStrength.resize(width*height, 0.f);

	for (auto& track : mdt.mapChangeTrackers) {
		for (auto mapQuad: track.damageQueue) {
			assert(mapQuad < mapDamageStrength.size());
			mapDamageStrength[mapQuad]++;
		}
	}

	for (int i = 0; i < mapDamageStrength.size(); ++i) {
		if (mapDamageStrength[i] == 0.f) { continue; }
		const int blockIdxX = (i % width) * blockSize;
		const int blockIdxY = (i / width) * blockSize;
		const float drawStrength = 0.2f + 0.55f*(mapDamageStrength[i] / maxStrength);
		glColor4f(1.0f, 1.0f, 0.0f, drawStrength);
		glRectf(blockIdxX, blockIdxY, blockIdxX + blockSize, blockIdxY + blockSize);
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
}

