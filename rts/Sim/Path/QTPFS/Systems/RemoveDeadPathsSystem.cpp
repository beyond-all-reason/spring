/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RemoveDeadPathsSystem.h"

#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Path/QTPFS/Components/RemoveDeadPaths.h"
#include "Sim/Path/QTPFS/NodeLayer.h"
#include "Sim/Path/QTPFS/PathManager.h"
#include "Sim/Path/QTPFS/Registry.h"
#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"
#include "System/Log/ILog.h"
#include "System/Misc/TracyDefs.h"
#include "System/SpringMath.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeProfiler.h"

#include <cassert>
#include <limits>

using namespace SystemGlobals;
using namespace QTPFS;

void RemoveDeadPathsSystem::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto& comp = systemGlobals.CreateSystemComponent<RemoveDeadPathsComponent>();
	systemUtils.OnUpdate().connect<&RemoveDeadPathsSystem::Update>();
}

void RemoveDeadPathsSystem::Update()
{
	SCOPED_TIMER("ECS::RemoveDeadPathsSystem::Update");

	auto& comp = systemGlobals.GetSystemComponent<RemoveDeadPathsComponent>();
	if (gs->frameNum % comp.refreshRate != comp.refreshOffset)
		return;

	auto* pm = dynamic_cast<PathManager*>(pathManager);

	auto view = registry.view<PathDelayedDelete>();
	for (auto pathEntity: view) {
		auto deleteOnFrame = view.get<PathDelayedDelete>(pathEntity).value;
		if (gs->frameNum > deleteOnFrame) {
			pm->DeletePathEntity(pathEntity);
		}
	}
}

void RemoveDeadPathsSystem::Shutdown()
{
	RECOIL_DETAILED_TRACY_ZONE;
	systemUtils.OnUpdate().disconnect<&RemoveDeadPathsSystem::Update>();
}
