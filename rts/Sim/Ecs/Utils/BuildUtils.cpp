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


void BuildUtils::AddUnitBuilder(CUnit *unit){
    auto entity = unit->entityReference;
    auto unitDef = unit->unitDef;
    auto buildSpeed = EcsMain::registry.emplace_or_replace<BuildPower>(entity, unitDef->buildSpeed / GAME_SPEED).value;
    auto repairSpeed = EcsMain::registry.emplace_or_replace<RepairPower>(entity, unitDef->repairSpeed / GAME_SPEED).value;
    auto maxRepairSpeed = EcsMain::registry.emplace_or_replace<MaxRepairPowerRate>(entity
            , unitDef->maxRepairSpeed / (GAME_SPEED / REPAIR_UPDATE_RATE)).value;

    static_cast<void>(EcsMain::registry.get_or_emplace<UnitEconomy::ResourcesCurrentUsage>(entity));

    LOG_L(L_DEBUG, "%s: added unit %d (%d) with build speed %f, repair speed %f", __func__
            , unit->id, entt::to_entity(entity), buildSpeed, repairSpeed);
}

void BuildUtils::AddUnitBuildTarget(entt::entity entity, entt::entity targetEntity) {
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference %d", __func__, entt::to_entity(entity)); return;
    }
    if (! EcsMain::registry.valid(targetEntity)){
        LOG("%s: invalid target entityId reference %d", __func__, entt::to_entity(targetEntity)); return;
    }
    const auto buildPowerComp = EcsMain::registry.try_get<BuildPower>(entity);
    if (buildPowerComp == nullptr){
        LOG("%s: unit %d has no build capacity", __func__, entt::to_entity(entity));  return;
    }

    auto buildPower = buildPowerComp->value;
    EcsMain::registry.emplace_or_replace<ActiveBuild>(entity, targetEntity, buildPower);

    LOG_L(L_DEBUG, "%s: %d: BuildPower = %f", __func__, entt::to_entity(entity), buildPower);
}

void BuildUtils::AddUnitRepairTarget(entt::entity entity, entt::entity targetEntity) {
    auto buildPower = EcsMain::registry.get<BuildPower>(entity).value;
    EcsMain::registry.emplace_or_replace<ActiveRepair>(entity, targetEntity, buildPower);

    LOG_L(L_DEBUG, "%s: %d: RepairPower = %f", __func__, entt::to_entity(entity), buildPower);
}

void BuildUtils::AddUnitBeingBuilt(CUnit *unit) {
    entt::entity entity = unit->entityReference;
    if (EcsMain::registry.all_of<BuildProgress>(entity))
        return;
    
    EcsMain::registry.emplace<BuildProgress>(entity);
    static_cast<void>(EcsMain::registry.get_or_emplace<BeingBuilt>(entity));
    static_cast<void>(EcsMain::registry.get_or_emplace<BuildTime>(entity, unit->buildTime));
    static_cast<void>(EcsMain::registry.emplace_or_replace<BuildCost>(entity, unit->cost));

    LOG_L(L_DEBUG, "%s: %d: BuildTime = %f", __func__, entt::to_entity(entity), unit->buildTime);
    LOG_L(L_DEBUG, "%s: %d: BuildCostMetal = %f", __func__, entt::to_entity(entity), unit->cost.metal);
    LOG_L(L_DEBUG, "%s: %d: BuildCostEnergy = %f", __func__, entt::to_entity(entity), unit->cost.energy);
}

void BuildUtils::AddUnitBeingRepaired(CUnit *unit) {
    entt::entity entity = unit->entityReference;
    static_cast<void>(EcsMain::registry.get_or_emplace<BuildTime>(entity, unit->buildTime));
    static_cast<void>(EcsMain::registry.get_or_emplace<BuildCost>(entity, unit->cost));
}

void BuildUtils::RemoveUnitBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference %d", __func__, entt::to_entity(entity)); return;
    }

    EcsMain::registry.remove<ActiveBuild>(entity);
    EcsMain::registry.remove<ActiveRepair>(entity);
    EcsMain::registry.remove<FlowEconomy::ResourceUse>(entity);

    LOG_L(L_DEBUG, "%s", __func__);
}

entt::entity BuildUtils::GetUnitBuildTarget(CUnit *unit) {
    entt::entity result = entt::null;

    const auto activeBuild = EcsMain::registry.try_get<ActiveBuild>(unit->entityReference);
    if (activeBuild != nullptr && EcsMain::registry.valid(activeBuild->buildTarget)) {
        result = activeBuild->buildTarget;
    }

    return result;
}

void BuildUtils::PauseBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    {
        auto activeBuild = EcsMain::registry.try_get<ActiveBuild>(entity);
        if (activeBuild != nullptr) {
            activeBuild->currentBuildpower = 0.f;
        }
    }
    {
        auto activeBuild = EcsMain::registry.try_get<ActiveRepair>(entity);
        if (activeBuild != nullptr) {
            activeBuild->currentBuildpower = 0.f;
        }
    }
}

void BuildUtils::UnpauseBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    {
        auto activeBuild = EcsMain::registry.try_get<ActiveBuild>(entity);
        if (activeBuild != nullptr) {
            if (activeBuild->currentBuildpower == 0.f)
                activeBuild->currentBuildpower = EcsMain::registry.get<BuildPower>(entity).value;
        }
    }
    {
        auto activeBuild = EcsMain::registry.try_get<ActiveRepair>(entity);
        if (activeBuild != nullptr) {
            if (activeBuild->currentBuildpower == 0.f)
                activeBuild->currentBuildpower = EcsMain::registry.get<BuildPower>(entity).value;
        }
    }
}

void BuildUtils::SetBuildPower(entt::entity entity, float power) {
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference %d", __func__, entt::to_entity(entity)); return;
    }

    EcsMain::registry.emplace_or_replace<BuildPower>(entity, power);

    LOG_L(L_DEBUG, "%s: BuildPower changed to %f (%d)", __func__, power, entt::to_entity(entity));
}

void BuildUtils::SetRepairPower(entt::entity entity, float power) {
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference %d", __func__, entt::to_entity(entity)); return;
    }

    EcsMain::registry.emplace_or_replace<RepairPower>(entity, power);

    LOG_L(L_DEBUG, "%s: BuildPower changed to %f (%d)", __func__, power, entt::to_entity(entity));
}

void BuildUtils::RemoveUnitBuild(entt::entity entity) {
    EcsMain::registry.remove<BuildTime>(entity);
    EcsMain::registry.remove<BuildCost>(entity);
    EcsMain::registry.remove<BeingBuilt>(entity);
}
