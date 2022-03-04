#include "BuildSystem.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"

#include "Sim/Misc/TeamHandler.h"

#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"

BuildSystem buildSystem;

using namespace Build;

void returnExcessEco(entt::entity entity, CTeam *team, float excessBuild){
    if (excessBuild > 0.f) {
        SResourcePack resToReturn;
        if (EcsMain::registry.all_of<BuildCostEnergy>(entity)){
            resToReturn.energy = (EcsMain::registry.get<BuildCostEnergy>(entity)).value;
        }
        if (EcsMain::registry.all_of<BuildCostMetal>(entity)) {
            resToReturn.metal = (EcsMain::registry.get<BuildCostMetal>(entity)).value;
        }
        team->resNextIncome += resToReturn * excessBuild;
    }
}

void BuildSystem::AddUnitBuilder(CBuilder *unit){
    auto entity = unit->entityReference;
    EcsMain::registry.emplace_or_replace<BuildPower>(entity, unit->buildSpeed);
}

void BuildSystem::AddUnitBuildTarget(CUnit *unit, CUnit *target) {
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

    if (!EcsMain::registry.all_of<BuildProgress>(targetEntity)){
        EcsMain::registry.emplace_or_replace<BuildProgress>(targetEntity);
        EcsMain::registry.emplace_or_replace<BuildTime>(targetEntity, target->buildTime);
        EcsMain::registry.emplace_or_replace<BuildCostMetal>(targetEntity, target->cost.metal);
        EcsMain::registry.emplace_or_replace<BuildCostEnergy>(targetEntity, target->cost.energy);
    }

    EcsMain::registry.emplace_or_replace<ActiveBuild>(entity, targetEntity);
    InitUnitBuildTarget(entity);
}

// (auto defaultValue) requires -fconcepts-ts or c++20
template<class T>
auto GetOptionalComponent(entt::entity entity, float defaultValue) {
    auto checkPtr = EcsMain::registry.try_get<T>(entity);
    return checkPtr != nullptr ? checkPtr->value : defaultValue;
}

void BuildSystem::InitUnitBuildTarget(entt::entity entity) {
    auto& activeBuild = EcsMain::registry.get<ActiveBuild>(entity);
    auto targetEntity = activeBuild.buildTarget;

    auto buildPower = EcsMain::registry.get<BuildPower>(entity).value;
    auto buildTime = EcsMain::registry.get<BuildTime>(targetEntity).value;
    auto costMetal = GetOptionalComponent<BuildCostMetal>(targetEntity, 0.f);
    auto costEnergy = GetOptionalComponent<BuildCostEnergy>(targetEntity, 0.f);

    float step = buildPower / buildTime;
    float metalUse = costMetal * step;
    float energyUse = costEnergy * step;
    int prorationRate = 0;
    if (metalUse >= 0.f){
        EcsMain::registry.emplace_or_replace<FlowEconomy::MetalProratableUse>(entity, metalUse);
        prorationRate += (int)ProrationRate::PRORATION_ONLY_METAL;
    }
    if (energyUse >= 0.f) {
        EcsMain::registry.emplace_or_replace<FlowEconomy::EnergyProratableUse>(entity, energyUse);
        prorationRate += (int)ProrationRate::PRORATION_ONLY_ENERGY;
    }
    activeBuild.currentBuildpower = buildPower;
    activeBuild.prorationType = (ProrationRate)prorationRate;
}

void BuildSystem::RemovetUnitBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    EcsMain::registry.remove<ActiveBuild>(entity);
    EcsMain::registry.remove<FlowEconomy::MetalProratableUse>(entity);
    EcsMain::registry.remove<FlowEconomy::EnergyProratableUse>(entity);
}

entt::entity BuildSystem::GetUnitBuildTarget(CUnit *unit) {
    entt::entity result = entt::null;

    const auto activeBuild = EcsMain::registry.try_get<ActiveBuild>(unit->entityReference);
    if (activeBuild != nullptr) {
        result = activeBuild->buildTarget;
    }

    return result;
}

void BuildSystem::PauseBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    auto& activeBuild = EcsMain::registry.get<ActiveBuild>(entity);
    activeBuild.currentBuildpower = 0.f;
    activeBuild.prorationType = ProrationRate::PRORATION_NONE;
    EcsMain::registry.remove<FlowEconomy::MetalProratableUse>(entity);
    EcsMain::registry.remove<FlowEconomy::EnergyProratableUse>(entity);
}

void BuildSystem::UnpauseBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    auto& activeBuild = EcsMain::registry.get<ActiveBuild>(entity);

    // this may not be currently paused.
    if (activeBuild.currentBuildpower >= 0.f) return;

    InitUnitBuildTarget(entity);
}

void BuildSystem::UpdateBuildPower(CUnit *unit, float power) {
    auto entity = unit->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    EcsMain::registry.emplace_or_replace<BuildPower>(entity, power);
}

bool BuildSystem::UnitBeingBuilt(entt::entity entity) {
    return EcsMain::registry.all_of<BuildProgress>(entity);
}

bool BuildSystem::UnitBuildComplete(entt::entity entity) {
    return EcsMain::registry.all_of<BuildComplete>(entity);
}

void BuildSystem::RemoveUnitBuild(entt::entity entity) {
    EcsMain::registry.remove<BuildProgress>(entity);
    EcsMain::registry.remove<BuildTime>(entity);
    EcsMain::registry.remove<BuildCostMetal>(entity);
    EcsMain::registry.remove<BuildCostEnergy>(entity);
    EcsMain::registry.remove<BuildComplete>(entity);
}

void BuildSystem::Update() {
    auto group = EcsMain::registry.group<ActiveBuild>(entt::get<Units::Team, Units::UnitId>);

    for (auto entity : group) {
        auto buildDetails = group.get<ActiveBuild>(entity);
        auto buildPower = buildDetails.currentBuildpower;
        auto buildTarget = buildDetails.buildTarget;

        // currently paused
        if (buildPower == 0.f) continue;

        auto teamId = (group.get<Units::Team>(entity)).value;
        auto team = teamHandler.Team(teamId);

        float buildRate = team->prorationRates[(int)buildDetails.prorationType];
        auto& buildProgress = (EcsMain::registry.get<BuildProgress>(buildTarget)).value;
        auto& health = (EcsMain::registry.get<SolidObject::Health>(buildTarget)).value;
        auto maxHealth = (EcsMain::registry.get<SolidObject::MaxHealth>(buildTarget)).value;
        auto buildTime = (EcsMain::registry.get<BuildTime>(buildTarget)).value;

        float buildStep = (buildPower*buildRate)/buildTime;
        float nextProgress = buildProgress + buildStep;
        float nextHealth = health + maxHealth * buildStep;

        returnExcessEco(buildTarget, team, (nextProgress - 1.f));

        buildProgress = std::min(nextProgress, 1.f);
        health = std::min(nextHealth, maxHealth);
        if (buildProgress == 1.f){
            EcsMain::registry.emplace<BuildComplete>(buildTarget);
        }
    }
}
