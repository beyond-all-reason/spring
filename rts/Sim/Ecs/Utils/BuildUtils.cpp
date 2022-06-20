#include "buildUtils.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Ecs/Utils/UnitEconomyUtils.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "System/TimeProfiler.h"

using namespace SystemGlobals;
using namespace Build;

bool BuildUtils::IsSystemActive() {
    return systemGlobals.IsSystemActive<BuildSystemComponent>();
}

void BuildUtils::AddUnitBuilder(CUnit *unit){
    auto entity = unit->entityReference;
    auto unitDef = unit->unitDef;
    auto buildSpeed = EcsMain::registry.emplace_or_replace<BuildPower>(entity, unitDef->buildSpeed / GAME_SPEED).value;

    AddComponentsIfNotExist<UnitEconomy::ResourcesCurrentUsage>(entity);

    LOG("%s: added unit %d (%d) with build speed %f", __func__, unit->id, (int)entity, buildSpeed);
}

void BuildUtils::AddUnitBuildTarget(CUnit *unit, CUnit *target) {
    auto entity = unit->entityReference;
    auto targetEntity = target->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }
    if (! EcsMain::registry.valid(targetEntity)){
        LOG("%s: invalid target entityId reference", __func__); return;
    }
    const auto buildPowerComp = EcsMain::registry.try_get<BuildPower>(entity);
    if (buildPowerComp == nullptr){
        LOG("%s: unit %d has no build capacity", __func__, unit->id);  return;
    }

    auto& activeBuild = EcsMain::registry.emplace_or_replace<ActiveBuild>(entity, targetEntity);
    activeBuild.buildTarget = targetEntity;

    auto buildPower = EcsMain::registry.get<BuildPower>(entity).value;
    auto buildTime = EcsMain::registry.get<BuildTime>(targetEntity).value;

    activeBuild.currentBuildpower = buildPower;
    LOG("%s: %d: BuildPower = %f", __func__, (int)entity, buildPower);
}

void BuildUtils::AddUnitBeingBuilt(CUnit *unit) {
    entt::entity entity = unit->entityReference;
    if (EcsMain::registry.all_of<BuildProgress>(entity))
        return;
        
    EcsMain::registry.emplace_or_replace<BuildProgress>(entity);
    EcsMain::registry.emplace_or_replace<BuildTime>(entity, unit->buildTime);
    LOG("%s: %d: BuildTime = %f", __func__, (int)entity, unit->buildTime);

    SResourcePack zeroResources;
    bool hasBuildCost = !(unit->cost <= zeroResources);
    if (hasBuildCost) {
        EcsMain::registry.emplace_or_replace<BuildCost>(entity, unit->cost);
        LOG("%s: %d: BuildCostMetal = %f", __func__, (int)entity, unit->cost.metal);
        LOG("%s: %d: BuildCostEnergy = %f", __func__, (int)entity, unit->cost.energy);
    }
}

void BuildUtils::RemoveUnitBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    EcsMain::registry.remove<ActiveBuild>(entity);

    LOG("%s", __func__);
}

entt::entity BuildUtils::GetUnitBuildTarget(CUnit *unit) {
    entt::entity result = entt::null;

    const auto activeBuild = EcsMain::registry.try_get<ActiveBuild>(unit->entityReference);
    if (activeBuild != nullptr) {
        result = activeBuild->buildTarget;
    }

    return result;
}

void BuildUtils::PauseBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    auto& activeBuild = EcsMain::registry.get<ActiveBuild>(entity);
    activeBuild.currentBuildpower = 0.f;
}

void BuildUtils::UnpauseBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    auto& activeBuild = EcsMain::registry.get<ActiveBuild>(entity);

    // this may not be currently paused.
    if (activeBuild.currentBuildpower > 0.f) return;

    activeBuild.currentBuildpower = EcsMain::registry.get<BuildPower>(entity).value;
}

void BuildUtils::SetBuildPower(entt::entity entity, float power) {
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    LOG("%s: BuildPower changed to %f (%d)", __func__, power, (int)entity);

    EcsMain::registry.emplace_or_replace<BuildPower>(entity, power);
}

bool BuildUtils::UnitBeingBuilt(entt::entity entity) {
    return EcsMain::registry.all_of<BuildProgress>(entity);
}

bool BuildUtils::UnitBuildComplete(entt::entity entity) {
    return EcsMain::registry.all_of<BuildComplete>(entity);
}

void BuildUtils::RemoveUnitBuild(entt::entity entity) {
    EcsMain::registry.remove<BuildProgress>(entity);
    EcsMain::registry.remove<BuildTime>(entity);
    EcsMain::registry.remove<BuildCost>(entity);
    EcsMain::registry.remove<BuildComplete>(entity);
}
