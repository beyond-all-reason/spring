#include "RepairReceivedSystem.h"

#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Ecs/Utils/SystemUtils.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"

using namespace Build;

void RepairReceivedSystem::Init() {

    SystemGlobals::systemGlobals.CreateSystemComponent<SystemGlobals::RepairReceivedSystemComponent>();
    auto& systemComp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::RepairReceivedSystemComponent>();

    systemComp.updateFrequency = (modInfo.economySystem == ECONOMY_SYSTEM_ECS)
            ? REPAIR_UPDATE_RATE : SLOW_SYSTEM_UPDATE;

    SystemUtils::systemUtils.OnUpdate().connect<&RepairReceivedSystem::Update>();
}

void RefreshRepairCounter() {
    auto group = EcsMain::registry.view<RepairRecieved>();
    for (auto entity : group) {
        auto& repairReceived = (group.get<RepairRecieved>(entity)).value;
        //LOG_L(L_DEBUG, "RepairReceivedSystem::%s: entity %d had been repaired by %f", __func__, entt::to_entity(entity), repairReceived);
        if (repairReceived > 0.f)
            repairReceived = 0.f;
        else
            EcsMain::registry.erase<RepairRecieved>(entity);
    }
}

void RepairReceivedSystem::Update() {
    auto& systemComp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::RepairReceivedSystemComponent>();

    if ((gs->frameNum % systemComp.updateFrequency) != REPAIR_TICK)
       return;

    LOG_L(L_DEBUG, "RepairReceivedSystem::%s: %d", __func__, gs->frameNum);

    SCOPED_TIMER("ECS::RepairReceivedSystem::Update");

    RefreshRepairCounter();
}
