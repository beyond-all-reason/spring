#include "RepairSystem.h"

#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Utils/SystemUtils.h"
#include "Sim/Ecs/Utils/UnitUtils.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "System/Log/DefaultFilter.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"

using namespace Build;

//LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_ECS)

void RepairSystem::Init() {
    if (modInfo.economySystem == ECONOMY_SYSTEM_ECS) {
        SystemUtils::systemUtils.OnUpdate().connect<&RepairSystem::Update>();
        log_filter_section_setMinLevel(LOG_LEVEL_DEBUG, LOG_SECTION_CURRENT);
    }
}

void RequestRepairResources() {
    auto combinedGroup = EcsMain::registry.group<ActiveRepair>(entt::get<FlowEconomy::AllocatedUnusedResource>);
    auto group = EcsMain::registry.view<ActiveRepair>();
    auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    for (auto entity : group) {
        if (entitiesLeftToProcess-- == 0) break;

        const auto& repairDetails = group.get<ActiveRepair>(entity);
        const auto repairPower = repairDetails.currentBuildpower;
        const auto repairTarget = repairDetails.buildTarget;

        // currently paused
        if (repairPower == 0.f) {
            EcsMain::registry.remove<FlowEconomy::ResourceUse>(entity);
            LOG_L(L_DEBUG, "RepairSystem::%s: %d -> %d (%f) paused", __func__
                    , entt::to_entity(entity), entt::to_entity(repairTarget), repairPower);
            continue;
        }

        const auto& buildCost = EcsMain::registry.get<BuildCost>(repairTarget);
        const auto buildTime = (EcsMain::registry.get<BuildTime>(repairTarget)).value;
        const float repairStep = (repairPower*GAME_SPEED)/buildTime;

        SResourcePack resPull = buildCost * repairStep * SResourcePack(0.f, modInfo.repairEnergyCostFactor);
        EcsMain::registry.emplace_or_replace<FlowEconomy::ResourceUse>(entity, resPull);

        LOG_L(L_DEBUG, "RepairSystem::%s: %d -> %d (%f,%f,%f,%f)", __func__
                , entt::to_entity(entity), entt::to_entity(repairTarget), resPull[0], resPull[1], resPull[2], resPull[3]);
    }
}

void RepairTasks() {
    auto group = EcsMain::registry.group<ActiveRepair>(entt::get<FlowEconomy::AllocatedUnusedResource>);
    for (auto entity : group) {
        ReleaseComponentOnExit<FlowEconomy::AllocatedUnusedResource> scopedExit(entity);

        const auto& repairDetails = group.get<ActiveRepair>(entity);
        const auto repairPower = repairDetails.currentBuildpower;
        const auto repairTarget = repairDetails.buildTarget;

        // currently paused
        if (repairPower == 0.f) {
            EcsMain::registry.remove<FlowEconomy::ResourceUse>(entity);
            LOG_L(L_DEBUG, "RepairSystem::%s: %d -> %d (%f) paused", __func__
                    , entt::to_entity(entity), entt::to_entity(repairTarget), repairPower);
            continue;
        }

		auto& health = UnitUtils::UnitHealth(repairTarget);
		const auto maxHealth = UnitUtils::UnitMaxHealth(repairTarget);
        const auto buildTime = (EcsMain::registry.get<BuildTime>(repairTarget)).value;
        auto& resAllocated = group.get<FlowEconomy::AllocatedUnusedResource>(entity);

        const float step = (repairPower*REPAIR_UPDATE_RATE)/buildTime;
        const float proratedHealthStep = step * resAllocated.prorationRate * maxHealth *.1f; // just to test repair.;
        const float nextHealth = health + proratedHealthStep;
        SResourcePack resUsage(resAllocated.res);

        if (nextHealth > maxHealth) {
            float allocRatioUsed = ((maxHealth - health) / proratedHealthStep);
            resUsage *= allocRatioUsed;
            resAllocated.res -= resUsage;
        }
        else
            resAllocated.res = SResourcePack();

        if (nextHealth >= maxHealth) {
            EcsMain::registry.remove<FlowEconomy::ResourceUse>(entity);
            EcsMain::registry.erase<ActiveRepair>(entity);
        }
        health = std::min(nextHealth, maxHealth);
        TryAddToComponent<UnitEconomy::ResourcesCurrentUsage>(entity, resUsage);

        LOG_L(L_DEBUG, "RepairSystem::%s: %d -> %d (%f/%f)", __func__
                , entt::to_entity(entity), entt::to_entity(repairTarget), health, maxHealth);
    }
}

void RepairSystem::Update() {
    if ((gs->frameNum % REPAIR_UPDATE_RATE) != REPAIR_TICK)
       return;

    LOG_L(L_DEBUG, "RepairSystem::%s: %d", __func__, gs->frameNum);

    SCOPED_TIMER("ECS::RepairSystem::Update");

    RequestRepairResources();
    RepairTasks();
}
