/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathSearchSystem.h"

#include "Sim/Path/QTPFS/PathSearch.h"
#include "Sim/Path/QTPFS/Components/PathSearch.h"
#include "Sim/Path/QTPFS/Registry.h"

#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"


using namespace SystemGlobals;
using namespace QTPFS;

void PathSearchSystem::Init()
{
    systemGlobals.CreateSystemComponent<PathSearchSystemComponent>();

    //SystemUtils::systemUtils.OnUpdate().connect<&PathSearchSystem::Update>();
}

void PathSearchSystem::Update()
{
    SCOPED_TIMER("ECS::PathSearchSystem::Update");

    auto& comp = systemGlobals.GetSystemComponent<PathSearchSystemComponent>();
    auto pathSearches = registry.view<PathSearch>();

    for_mt(0, pathSearches.size(), [=, &pathSearches, &comp](int index){
        int currentThread = ThreadPool::GetThreadNum();
        entt::entity entity = pathSearches[index];
        PathSearch& search = pathSearches.get<PathSearch>(entity);
        
        comp.openNodes[currentThread].size();
    });
}
