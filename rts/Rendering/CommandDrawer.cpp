/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandDrawer.h"
#include "LineDrawer.h"
#include "Game/GameHelper.h"
#include "Game/UI/CommandColors.h"
#include "Game/WaitCommandsAI.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandQueue.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/AirCAI.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/SpringMath.h"
#include "System/Color.h"
#include "System/Log/ILog.h"

static const CUnit* GetTrackableUnit(const CUnit* caiOwner, const CUnit* cmdUnit)
{
	if (cmdUnit == nullptr)
		return nullptr;
	if ((cmdUnit->losStatus[caiOwner->allyteam] & (LOS_INLOS | LOS_INRADAR)) == 0)
		return nullptr;

	return cmdUnit;
}

CommandDrawer* CommandDrawer::GetInstance() {
	// luaQueuedUnitSet gets cleared each frame, so this is fine wrt. reloading
	static CommandDrawer drawer;
	return &drawer;
}



void CommandDrawer::Draw(const CCommandAI* cai, int queueDrawDepth) const {
	// note: {Air,Builder}CAI inherit from MobileCAI, so test that last
	if ((dynamic_cast<const     CAirCAI*>(cai)) != nullptr) {     DrawAirCAICommands(static_cast<const     CAirCAI*>(cai), queueDrawDepth); return; }
	if ((dynamic_cast<const CBuilderCAI*>(cai)) != nullptr) { DrawBuilderCAICommands(static_cast<const CBuilderCAI*>(cai), queueDrawDepth); return; }
	if ((dynamic_cast<const CFactoryCAI*>(cai)) != nullptr) { DrawFactoryCAICommands(static_cast<const CFactoryCAI*>(cai), queueDrawDepth); return; }
	if ((dynamic_cast<const  CMobileCAI*>(cai)) != nullptr) {  DrawMobileCAICommands(static_cast<const  CMobileCAI*>(cai), queueDrawDepth); return; }

	DrawCommands(cai, queueDrawDepth);
}



void CommandDrawer::AddLuaQueuedUnit(const CUnit* unit, int queueDrawDepth) {
	// needs to insert by id, pointers can become dangling
	luaQueuedUnitSet.insert({ unit->id, queueDrawDepth });
}

void CommandDrawer::DrawLuaQueuedUnitSetCommands() const
{
	if (luaQueuedUnitSet.empty())
		return;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	lineDrawer.Configure(cmdColors.UseColorRestarts(),
	                     cmdColors.UseRestartColor(),
	                     cmdColors.restart,
	                     cmdColors.RestartAlpha());
	lineDrawer.SetupLineStipple();

	glEnable(GL_BLEND);
	glBlendFunc((GLenum)cmdColors.QueuedBlendSrc(),
	            (GLenum)cmdColors.QueuedBlendDst());

	glLineWidth(cmdColors.QueuedLineWidth());

	for (const auto& [unitID, qDrawDepth] : luaQueuedUnitSet) {
		const CUnit* unit = unitHandler.GetUnit(unitID);

		if (unit == nullptr || unit->commandAI == nullptr)
			continue;

		Draw(unit->commandAI, qDrawDepth);
	}

	glLineWidth(1.0f);
	glEnable(GL_DEPTH_TEST);
}

void CommandDrawer::DrawCommands(const CCommandAI* cai, int queueDrawDepth) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	if (queueDrawDepth <= 0)
		queueDrawDepth = commandQue.size();

	lineDrawer.StartPath(owner->GetObjDrawMidPos(), cmdColors.start);

	if (owner->selfDCountdown != 0)
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);

	for (auto ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		if (std::distance(commandQue.begin(), ci) > queueDrawDepth)
			break;

		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_ATTACK:
			case CMD_MANUALFIRE: {
				if (ci->GetNumParams() == 1) {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

					if (unit != nullptr)
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.attack);

				} else {
					assert(ci->GetNumParams() >= 3);

					const float x = ci->GetParam(0);
					const float z = ci->GetParam(2);
					const float y = CGround::GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
			} break;

			case CMD_WAIT: {
				DrawWaitIcon(*ci);
			} break;
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(cmdID);
			} break;

			default: {
				DrawDefaultCommand(*ci, owner);
			} break;
		}
	}

	lineDrawer.FinishPath();
}



void CommandDrawer::DrawAirCAICommands(const CAirCAI* cai, int queueDrawDepth) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	if (queueDrawDepth <= 0)
		queueDrawDepth = commandQue.size();

	lineDrawer.StartPath(owner->GetObjDrawMidPos(), cmdColors.start);

	if (owner->selfDCountdown != 0)
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);

	for (auto ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		if (std::distance(commandQue.begin(), ci) > queueDrawDepth)
			break;

		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_MOVE: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.move);
			} break;
			case CMD_FIGHT: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.fight);
			} break;
			case CMD_PATROL: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.patrol);
			} break;

			case CMD_ATTACK: {
				if (ci->GetNumParams() == 1) {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

					if (unit != nullptr)
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.attack);

				} else {
					assert(ci->GetNumParams() >= 3);

					const float x = ci->GetParam(0);
					const float z = ci->GetParam(2);
					const float y = CGround::GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
			} break;

			case CMD_AREA_ATTACK: {
				const float3& endPos = ci->GetPos(0);

				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.attack);
				lineDrawer.Break(endPos, cmdColors.attack);

				glSurfaceCircle(endPos, ci->GetParam(3), { cmdColors.attack }, cmdCircleResolution);

				lineDrawer.RestartWithColor(cmdColors.attack);
			} break;

			case CMD_GUARD: {
				const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

				if (unit != nullptr)
					lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.guard);

			} break;

			case CMD_WAIT: {
				DrawWaitIcon(*ci);
			} break;
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(cmdID);
			} break;

			default: {
				DrawDefaultCommand(*ci, owner);
			} break;
		}
	}

	lineDrawer.FinishPath();
}



void CommandDrawer::DrawBuilderCAICommands(const CBuilderCAI* cai, int queueDrawDepth) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	if (queueDrawDepth <= 0)
		queueDrawDepth = commandQue.size();

	lineDrawer.StartPath(owner->GetObjDrawMidPos(), cmdColors.start);

	if (owner->selfDCountdown != 0)
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);

	for (auto ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		if (std::distance(commandQue.begin(), ci) > queueDrawDepth)
			break;

		const int cmdID = ci->GetID();

		if (cmdID < 0) {
			#if 0
			if (std::find(cai->buildOptions.begin(), cai->buildOptions.end(), cmdID) != cai->buildOptions.end())
			#endif
			{
				BuildInfo bi;

				if (!bi.Parse(*ci))
					continue;

				cursorIcons.AddBuildIcon(cmdID, bi.pos, owner->team, bi.buildFacing);
				lineDrawer.DrawLine(bi.pos, cmdColors.build);

				// draw metal extraction range
				if (bi.def->extractRange > 0.0f) {
					lineDrawer.Break(bi.pos, cmdColors.build);
					glSurfaceCircle(bi.pos, bi.def->extractRange, { cmdColors.rangeExtract }, 40.0f);
					lineDrawer.Restart();
				}
			}
			continue;
		}

		switch (cmdID) {
			case CMD_MOVE: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.move);
			} break;
			case CMD_FIGHT:{
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.fight);
			} break;
			case CMD_PATROL: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.patrol);
			} break;

			case CMD_GUARD: {
				const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

				if (unit != nullptr)
					lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.guard);

			} break;

			case CMD_RESTORE: {
				const float3& endPos = ci->GetPos(0);

				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.restore);
				lineDrawer.Break(endPos, cmdColors.restore);

				glSurfaceCircle(endPos, ci->GetParam(3), { cmdColors.restore }, cmdCircleResolution);

				lineDrawer.RestartWithColor(cmdColors.restore);
			} break;

			case CMD_ATTACK:
			case CMD_MANUALFIRE: {
				if (ci->GetNumParams() == 1) {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

					if (unit != nullptr)
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.attack);

				} else {
					assert(ci->GetNumParams() >= 3);

					const float x = ci->GetParam(0);
					const float z = ci->GetParam(2);
					const float y = CGround::GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
			} break;

			case CMD_RECLAIM:
			case CMD_RESURRECT: {
				const float* color = (cmdID == CMD_RECLAIM) ? cmdColors.reclaim
				                                             : cmdColors.resurrect;
				if (ci->GetNumParams() == 4) {
					const float3& endPos = ci->GetPos(0);

					lineDrawer.DrawLineAndIcon(cmdID, endPos, color);
					lineDrawer.Break(endPos, color);

					glSurfaceCircle(endPos, ci->GetParam(3), { color }, cmdCircleResolution);

					lineDrawer.RestartWithColor(color);
				} else {
					assert(ci->GetParam(0) >= 0.0f);

					const unsigned int id = std::max(0.0f, ci->GetParam(0));

					if (id >= unitHandler.MaxUnits()) {
						const CFeature* feature = featureHandler.GetFeature(id - unitHandler.MaxUnits());

						if (feature != nullptr)
							lineDrawer.DrawLineAndIcon(cmdID, feature->GetObjDrawMidPos(), color);

					} else {
						const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(id));

						if (unit != nullptr && unit != owner)
							lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), color);

					}
				}
			} break;

			case CMD_REPAIR:
			case CMD_CAPTURE: {
				const float* color = (ci->GetID() == CMD_REPAIR) ? cmdColors.repair: cmdColors.capture;

				if (ci->GetNumParams() == 4) {
					const float3& endPos = ci->GetPos(0);

					lineDrawer.DrawLineAndIcon(cmdID, endPos, color);
					lineDrawer.Break(endPos, color);

					glSurfaceCircle(endPos, ci->GetParam(3), { color }, cmdCircleResolution);

					lineDrawer.RestartWithColor(color);
				} else {
					if (ci->GetNumParams() >= 1) {
						const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

						if (unit != nullptr)
							lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), color);

					}
				}
			} break;

			case CMD_LOAD_ONTO: {
				const CUnit* unit = unitHandler.GetUnitUnsafe(ci->GetParam(0));
				lineDrawer.DrawLineAndIcon(cmdID, unit->pos, cmdColors.load);
			} break;
			case CMD_WAIT: {
				DrawWaitIcon(*ci);
			} break;
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(ci->GetID());
			} break;

			default: {
				DrawDefaultCommand(*ci, owner);
			} break;
		}
	}

	lineDrawer.FinishPath();
}



void CommandDrawer::DrawFactoryCAICommands(const CFactoryCAI* cai, int queueDrawDepth) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;
	const CCommandQueue& newUnitCommands = cai->newUnitCommands;

	if (queueDrawDepth <= 0)
		queueDrawDepth = newUnitCommands.size();

	lineDrawer.StartPath(owner->GetObjDrawMidPos(), cmdColors.start);

	if (owner->selfDCountdown != 0)
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);

	if (!commandQue.empty() && (commandQue.front().GetID() == CMD_WAIT))
		DrawWaitIcon(commandQue.front());

	for (auto ci = newUnitCommands.begin(); ci != newUnitCommands.end(); ++ci) {
		if (std::distance(newUnitCommands.begin(), ci) > queueDrawDepth)
			break;

		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_MOVE: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0) + UpVector * 3.0f, cmdColors.move);
			} break;
			case CMD_FIGHT: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0) + UpVector * 3.0f, cmdColors.fight);
			} break;
			case CMD_PATROL: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0) + UpVector * 3.0f, cmdColors.patrol);
			} break;

			case CMD_ATTACK: {
				if (ci->GetNumParams() == 1) {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

					if (unit != nullptr)
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.attack);

				} else {
					assert(ci->GetNumParams() >= 3);

					const float x = ci->GetParam(0);
					const float z = ci->GetParam(2);
					const float y = CGround::GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
			} break;

			case CMD_GUARD: {
				const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

				if (unit != nullptr)
					lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.guard);

			} break;

			case CMD_WAIT: {
				DrawWaitIcon(*ci);
			} break;
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(cmdID);
			} break;

			default: {
				DrawDefaultCommand(*ci, owner);
			} break;
		}

		if ((cmdID < 0) && (ci->GetNumParams() >= 3)) {
			BuildInfo bi;

			if (!bi.Parse(*ci))
				continue;

			cursorIcons.AddBuildIcon(cmdID, bi.pos, owner->team, bi.buildFacing);
			lineDrawer.DrawLine(bi.pos, cmdColors.build);

			// draw metal extraction range
			if (bi.def->extractRange > 0.0f) {
				lineDrawer.Break(bi.pos, cmdColors.build);
				glSurfaceCircle(bi.pos, bi.def->extractRange, { cmdColors.rangeExtract }, 40.0f);
				lineDrawer.Restart();
			}
		}
	}

	lineDrawer.FinishPath();
}



void CommandDrawer::DrawMobileCAICommands(const CMobileCAI* cai, int queueDrawDepth) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	if (queueDrawDepth <= 0)
		queueDrawDepth = commandQue.size();

	lineDrawer.StartPath(owner->GetObjDrawMidPos(), cmdColors.start);

	if (owner->selfDCountdown != 0)
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);

	for (auto ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_MOVE: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.move);
			} break;
			case CMD_PATROL: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.patrol);
			} break;
			case CMD_FIGHT: {
				if (ci->GetNumParams() >= 3)
					lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.fight);

			} break;

			case CMD_ATTACK:
			case CMD_MANUALFIRE: {
				if (ci->GetNumParams() == 1) {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

					if (unit != nullptr)
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.attack);

				}

				if (ci->GetNumParams() >= 3) {
					const float x = ci->GetParam(0);
					const float z = ci->GetParam(2);
					const float y = CGround::GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
			} break;

			case CMD_GUARD: {
				const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

				if (unit != nullptr)
					lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.guard);

			} break;

			case CMD_LOAD_ONTO: {
				const CUnit* unit = unitHandler.GetUnitUnsafe(ci->GetParam(0));
				lineDrawer.DrawLineAndIcon(cmdID, unit->pos, cmdColors.load);
			} break;

			case CMD_LOAD_UNITS: {
				if (ci->GetNumParams() == 4) {
					const float3& endPos = ci->GetPos(0);

					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.load);
					lineDrawer.Break(endPos, cmdColors.load);

					glSurfaceCircle(endPos, ci->GetParam(3), { cmdColors.load }, cmdCircleResolution);

					lineDrawer.RestartWithColor(cmdColors.load);
				} else {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(ci->GetParam(0)));

					if (unit != nullptr)
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.load);

				}
			} break;

			case CMD_UNLOAD_UNITS: {
				if (ci->GetNumParams() == 5) {
					const float3& endPos = ci->GetPos(0);

					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.unload);
					lineDrawer.Break(endPos, cmdColors.unload);

					glSurfaceCircle(endPos, ci->GetParam(3), { cmdColors.unload }, cmdCircleResolution);

					lineDrawer.RestartWithColor(cmdColors.unload);
				}
			} break;

			case CMD_UNLOAD_UNIT: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.unload);
			} break;
			case CMD_WAIT: {
				DrawWaitIcon(*ci);
			} break;
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(cmdID);
			} break;

			default: {
				DrawDefaultCommand(*ci, owner);
			} break;
		}
	}

	lineDrawer.FinishPath();
}


void CommandDrawer::DrawWaitIcon(const Command& cmd) const
{
	waitCommandsAI.AddIcon(cmd, lineDrawer.GetLastPos());
}

void CommandDrawer::DrawDefaultCommand(const Command& c, const CUnit* owner) const
{
	// TODO add Lua callin perhaps, for more elaborate needs?
	const CCommandColors::DrawData* dd = cmdColors.GetCustomCmdData(c.GetID());

	if (dd == nullptr)
		return;

	switch (c.GetNumParams()) {
		case  0: { return; } break;
		case  1: {         } break;
		case  2: {         } break;
		default: {
			const float3 endPos = c.GetPos(0) + UpVector * 3.0f;

			if (!dd->showArea || (c.GetNumParams() < 4)) {
				lineDrawer.DrawLineAndIcon(dd->cmdIconID, endPos, dd->color);
			} else {
				lineDrawer.DrawLineAndIcon(dd->cmdIconID, endPos, dd->color);
				lineDrawer.Break(endPos, dd->color);
				glSurfaceCircle(endPos, c.GetParam(3), { dd->color }, cmdCircleResolution);
				lineDrawer.RestartWithColor(dd->color);
			}

			return;
		}
	}

	// allow a second param (ignored here) for custom commands
	const CUnit* unit = GetTrackableUnit(owner, unitHandler.GetUnit(c.GetParam(0)));

	if (unit == nullptr)
		return;

	lineDrawer.DrawLineAndIcon(dd->cmdIconID, unit->GetObjDrawErrorPos(owner->allyteam), dd->color);
}

void CommandDrawer::ClearQueuedBuildingSquaresCache() { biCache.clear(); }

void CommandDrawer::DrawQueuedBuildingSquares(const CBuilderCAI* cai, const SColor& color)
{
	const CCommandQueue& commandQue = cai->commandQue;
	const auto& buildOptions = cai->buildOptions;

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();

	for (const Command& c : commandQue) {
		#if 0
		if (std::find(buildOptions.begin(), buildOptions.end(), c.GetID()) == buildOptions.end())
			continue;
		#endif

		BuildInfo bi;

		if (!bi.Parse(c))
			continue;

		//already drawn by other builder
		BuildInfoHash biHasher; size_t biHash = biHasher(bi);
		auto it = std::find_if(biCache.begin(), biCache.end(), [biHash, &bi](const auto& item) {
			return (biHash == item.first) && (bi == item.second);
		});

		if (it != biCache.end())
			continue;

		biCache.emplace_back(biHash, bi);

		bi.pos = CGameHelper::Pos2BuildPos(bi, false);
		const float xsize = bi.GetXSize() * (SQUARE_SIZE >> 1);
		const float zsize = bi.GetZSize() * (SQUARE_SIZE >> 1);
		const float radius = math::sqrt(xsize * xsize + zsize * zsize);

		if (!camera->InView(bi.pos, radius))
			continue;

		const float h = bi.pos.y;
		const float x1 = bi.pos.x - xsize;
		const float z1 = bi.pos.z - zsize;
		const float x2 = bi.pos.x + xsize;
		const float z2 = bi.pos.z + zsize;

		if (bi.pos.y < 0.0f) {

			static constexpr SColor begColor = { 0.0f, 0.0f, 1.0f, 0.5f };
			static constexpr SColor endColor = { 0.0f, 0.5f, 1.0f, 1.0f };

			// water-plane verts
			rb.AddQuadLines(
				{ float3{x1, 0.0f, z1}, endColor },
				{ float3{x2, 0.0f, z1}, endColor },
				{ float3{x2, 0.0f, z2}, endColor },
				{ float3{x1, 0.0f, z2}, endColor }
			);

			for (const auto& x : { x1, x2 }) {
				for (const auto& z : { z1, z2 }) {
					const auto baseVert = rb.GetBaseVertex();
					rb.AddVertices({
						{float3{x, h, z}, begColor},
						{float3{x, 0, z}, endColor}
					});
					rb.AddIndices({ 0, 1 }, baseVert);
				}
			}
		}

		// above-water verts
		rb.AddQuadLines(
			{ float3{x1, h + 1.0f, z1}, color },
			{ float3{x2, h + 1.0f, z1}, color },
			{ float3{x2, h + 1.0f, z2}, color },
			{ float3{x1, h + 1.0f, z2}, color }
		);
	}
}

