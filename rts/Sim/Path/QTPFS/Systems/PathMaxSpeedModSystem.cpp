/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathMaxSpeedModSystem.h"

#include <cassert>
#include <limits>

#include "Map/ReadMap.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Path/QTPFS/Components/PathMaxSpeedMod.h"
#include "Sim/Path/QTPFS/NodeLayer.h"
#include "Sim/Path/QTPFS/PathManager.h"
#include "Sim/Path/QTPFS/Registry.h"

#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"
#include "System/SpringMath.h"


using namespace SystemGlobals;
using namespace QTPFS;


void ScanForPathMaxSpeedMod(int frameModulus) {
    auto& comp = systemGlobals.GetSystemComponent<PathMaxSpeedModSystemComponent>();
    auto layersView = registry.view<NodeLayerMaxSpeedSweep>();
    auto pm = dynamic_cast<QTPFS::PathManager*>(pathManager);
    int dataChunk = frameModulus;

    assert(layersView.size() <= MoveDefHandler::MAX_MOVE_DEFS);

    // Initialization
    if (frameModulus <= 0) {
        layersView.each([&layersView, pm](entt::entity entity){
                auto& layer = layersView.get<NodeLayerMaxSpeedSweep>(entity);
                auto& nodeLayer = pm->GetNodeLayer(layer.layerNum);
                layer.updateCurMaxSpeed = (-std::numeric_limits<float>::infinity());
                layer.updateMaxNodes = nodeLayer.GetMaxNodesAlloced();
                layer.updateInProgress = true;
            });
        dataChunk = 0;
    }
    
    // One thread per layer: get maximum speed mod from the nodes walked thus far.
    for_mt(0, layersView.size(), [&layersView, &comp, dataChunk, pm](int idx){
        entt::entity entity = layersView.storage<NodeLayerMaxSpeedSweep>()[idx];
        auto& layer = layersView.get<NodeLayerMaxSpeedSweep>(entity);

        const int idxBeg = ((dataChunk + 0) * layer.updateMaxNodes) / comp.refreshTimeInFrames;
	    const int idxEnd = ((dataChunk + 1) * layer.updateMaxNodes) / comp.refreshTimeInFrames;

        // if (layer.layerNum == 2) {
        //     LOG("Searching %d: %d/%d", dataChunk, layer.updateMaxNodes, comp.refreshTimeInFrames);
        // }

        // TODO: store speed mods in a separate component? Allows for SSE perhaps?
        auto& nodeLayer = pm->GetNodeLayer(layer.layerNum);
        for (int i = idxBeg; i < idxEnd; ++i) {
            auto* curNode = nodeLayer.GetPoolNode(i);
            layer.updateCurMaxSpeed = std::max(layer.updateCurMaxSpeed, curNode->GetSpeedMod() * curNode->IsLeaf());

            // if (layer.layerNum == 2) {
            //     LOG("node: %d max(%f,%f)", i, layer.updateCurMaxSpeed, curNode->GetSpeedMod());
            // }
        }
    });

    // Finished search
    if (dataChunk == comp.refreshTimeInFrames + (-1))
        layersView.each([&comp, &layersView](entt::entity entity){
            auto& layer = layersView.get<NodeLayerMaxSpeedSweep>(entity);
            comp.maxRelSpeedMod[layer.layerNum] = layer.updateCurMaxSpeed;
            layer.updateInProgress = false;

            // if (layer.layerNum == 2) {
            //     LOG("Finished Search - result is %f", comp.maxRelSpeedMod[layer.layerNum]);
            // }
            });
}

void InitLayers() {
    std::vector<entt::entity> layers((size_t)moveDefHandler.GetNumMoveDefs());
    QTPFS::registry.create<decltype(layers)::iterator>(layers.begin(), layers.end());

    int counter = 0;
    std::for_each(layers.begin(), layers.end(), [&counter](entt::entity entity){
        auto& layer = QTPFS::registry.emplace<NodeLayerMaxSpeedSweep>(entity);
        layer.layerNum = counter++;
        layer.updateCurMaxSpeed = 0.f;
        // LOG("%s: added %x:%x entity", __func__, entt::to_integral(entity), entt::to_version(entity));
    });
}

void PathMaxSpeedModSystem::Init()
{
    auto& comp = systemGlobals.CreateSystemComponent<PathMaxSpeedModSystemComponent>();
    auto pm = dynamic_cast<QTPFS::PathManager*>(IPathManager::GetInstance(QTPFS_TYPE));

    InitLayers();

    systemUtils.OnUpdate().connect<&PathMaxSpeedModSystem::Update>();

    // Scan whole map for initial max speed for hCost 
    comp.refreshTimeInFrames = 1;
    ScanForPathMaxSpeedMod(-1);

    comp.refreshTimeInFrames = GAME_SPEED * 30;
    comp.startRefreshOnFrame = NEXT_FRAME_NEVER;
    comp.refeshDelayInFrames = GAME_SPEED;

    comp.state = PathMaxSpeedModSystemComponent::STATE_READY;
}

void PathMaxSpeedModSystem::Update()
{
    SCOPED_TIMER("ECS::PathMaxSpeedModSystem::Update");

    auto& comp = systemGlobals.GetSystemComponent<PathMaxSpeedModSystemComponent>();
    auto nextFrameNum = gs->frameNum + 1;

    switch (comp.state) {
    case (PathMaxSpeedModSystemComponent::STATE_UPDATING):
        ScanForPathMaxSpeedMod(gs->frameNum % comp.refreshTimeInFrames);
        if (nextFrameNum % comp.refreshTimeInFrames == 0)
            comp.state = PathMaxSpeedModSystemComponent::STATE_READY;
        break;
    case (PathMaxSpeedModSystemComponent::STATE_READY):
        if ( (nextFrameNum >= comp.startRefreshOnFrame) && (nextFrameNum % comp.refreshTimeInFrames == 0) ) {
            comp.state = PathMaxSpeedModSystemComponent::STATE_UPDATING;
            comp.startRefreshOnFrame = NEXT_FRAME_NEVER;
        }
        break;
    default:
        assert(false);
    }
}

void PathMaxSpeedModSystem::Shutdown() {
    systemUtils.OnUpdate().disconnect<&PathMaxSpeedModSystem::Update>();

    registry.view<NodeLayerMaxSpeedSweep>()
            .each([](entt::entity entity){ registry.destroy(entity); });
}
