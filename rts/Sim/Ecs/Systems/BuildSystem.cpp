#include "BuildSystem.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Utils/BuildUtils.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Ecs/Utils/SystemUtils.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"

using namespace SystemGlobals;
using namespace Build;

void BuildSystem::Init() {
    if (modInfo.economySystem == ECONOMY_SYSTEM_ECS) {
        systemGlobals.CreateSystemComponent<BuildSystemComponent>();
        SystemUtils::systemUtils.OnUpdate().connect<&BuildSystem::Update>();
    }
}

void RequestBuildResources() {
    auto combinedGroup = EcsMain::registry.group<ActiveBuild>(entt::get<FlowEconomy::AllocatedUnusedResource>);
    auto group = EcsMain::registry.view<ActiveBuild>();
    auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    for (auto entity : group) {
        if (entitiesLeftToProcess-- == 0) break;

        const auto& buildDetails = group.get<ActiveBuild>(entity);
        const auto buildPower = buildDetails.currentBuildpower;
        const auto buildTarget = buildDetails.buildTarget;

        // currently paused
        if (buildPower == 0.f) {
            EcsMain::registry.remove<FlowEconomy::ResourceUse>(entity);
            LOG_L(L_DEBUG, "BuildSystem::%s: %d -> %d (%d) paused", __func__, (int)entity, (int)buildTarget, (int)buildPower);
            continue;
        }

        const auto& buildCost = EcsMain::registry.get<BuildCost>(buildTarget);
        const auto buildTime = (EcsMain::registry.get<BuildTime>(buildTarget)).value;
        const float buildStep = (buildPower*GAME_SPEED)/buildTime;

        SResourcePack resPull = buildCost * buildStep;
        EcsMain::registry.emplace_or_replace<FlowEconomy::ResourceUse>(entity, resPull);

        LOG_L(L_DEBUG, "BuildSystem::%s: %d -> %d (%f,%f,%f,%f)", __func__, (int)entity, (int)buildTarget, resPull[0], resPull[1], resPull[2], resPull[3]);
    }
}

void BuildTasks() {
    auto group = EcsMain::registry.group<ActiveBuild>(entt::get<FlowEconomy::AllocatedUnusedResource>);
    for (auto entity : group) {
        ReleaseComponentOnExit<FlowEconomy::AllocatedUnusedResource> scopedExit(entity);

        const auto& buildDetails = group.get<ActiveBuild>(entity);
        const auto buildPower = buildDetails.currentBuildpower;
        const auto buildTarget = buildDetails.buildTarget;

        // currently paused
        if (buildPower == 0.f) {
            EcsMain::registry.remove<FlowEconomy::ResourceUse>(entity);
            LOG_L(L_DEBUG, "BuildSystem::%s: %d -> %d (%d) paused", __func__, (int)entity, (int)buildTarget, (int)buildPower);
            continue;
        }

        const auto buildTime = (EcsMain::registry.get<BuildTime>(buildTarget)).value;
        const auto maxHealth = (EcsMain::registry.get<SolidObject::MaxHealth>(buildTarget)).value;
        auto& resAllocated = group.get<FlowEconomy::AllocatedUnusedResource>(entity);
        auto& buildProgress = (EcsMain::registry.get<BuildProgress>(buildTarget)).value;
        auto& health = (EcsMain::registry.get<SolidObject::Health>(buildTarget)).value;
        
        const float buildStep = (buildPower*BUILD_UPDATE_RATE)/buildTime;
        const float proratedBuildStep = buildStep * resAllocated.prorationRate;
        const float nextProgress = buildProgress + proratedBuildStep;
        const float nextHealth = health + maxHealth * proratedBuildStep *.1f; // just to test repair.
        SResourcePack resUsage(resAllocated.res);

        if (nextProgress > 1.f) {
            float allocRatioUsed = ((1.f - buildProgress) / proratedBuildStep);
            resUsage *= allocRatioUsed;
            resAllocated.res -= resUsage;
        }
        else
            resAllocated.res = SResourcePack();

        if (nextProgress >= 1.f) {
            if (nextHealth < maxHealth) {
                const auto repairPower = (EcsMain::registry.get_or_emplace<RepairSpeed>(entity)).value;
                EcsMain::registry.emplace<ActiveRepair>(entity, buildTarget, repairPower);
                EcsMain::registry.erase<ActiveBuild>(entity);
            }
            EcsMain::registry.remove<FlowEconomy::ResourceUse>(entity);
        }

        TryAddToComponent<UnitEconomy::ResourcesCurrentUsage>(entity, resUsage);
        buildProgress = std::min(nextProgress, 1.f);
        health = std::min(nextHealth, maxHealth);

        LOG_L(L_DEBUG, "BuildSystem::%s: %d -> %d (%f%%)", __func__, (int)entity, (int)buildTarget, buildProgress*100.f);
    }
}

void BuildSystem::Update() {
    if ((gs->frameNum % BUILD_UPDATE_RATE) != BUILD_TICK)
       return;

    LOG_L(L_DEBUG, "BuildSystem::%s: %d", __func__, gs->frameNum);

    SCOPED_TIMER("ECS::BuildSystem::Update");

    RequestBuildResources();
    BuildTasks();
}
