#include "RepairSystem.h"

#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Utils/SystemUtils.h"
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

        auto& resAllocated = group.get<FlowEconomy::AllocatedUnusedResource>(entity);
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

		auto& health = (EcsMain::registry.get<SolidObject::Health>(repairTarget)).value;
		const auto maxHealth = (EcsMain::registry.get<SolidObject::MaxHealth>(repairTarget)).value;
        const auto buildTime = (EcsMain::registry.get<BuildTime>(repairTarget)).value;
        
        const auto maxRepairRate = (EcsMain::registry.get_or_emplace<MaxRepairSpeed>(entity)).value;
        auto& repairRecieved = (EcsMain::registry.get_or_emplace<RepairRecieved>(repairTarget)).value;
        SResourcePack resUsage(resAllocated.res);

        const float proratedPower = (repairPower*REPAIR_UPDATE_RATE) * resAllocated.prorationRate;
        const float finishPower = (1.f - (health / maxHealth)) * buildTime;
        const float availablePower = maxRepairRate - repairRecieved;

        LOG_L(L_DEBUG, "RepairSystem::%s: %d -> %d (%f : %f : %f)", __func__
                , entt::to_entity(entity), entt::to_entity(repairTarget), proratedPower, finishPower, availablePower);

        const float power = std::max(0.f, std::min(proratedPower, std::min(finishPower, availablePower)));
        if (power < proratedPower) {
            resUsage *= (power / proratedPower);
            resAllocated.res -= resUsage;
        }
        else
            resAllocated.res = SResourcePack();

        const float step = power / buildTime;
        const float proratedHealthStep = step * maxHealth;
        const float nextHealth = health + proratedHealthStep;
        if (nextHealth >= maxHealth) {
            EcsMain::registry.remove<FlowEconomy::ResourceUse>(entity);
            EcsMain::registry.erase<ActiveRepair>(entity);
        }

        health = std::min(nextHealth, maxHealth);
        repairRecieved += power;
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
