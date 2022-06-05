#include "BuildSystem.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "System/TimeProfiler.h"

BuildSystem buildSystem;

using namespace SystemGlobals;
using namespace Build;

void BuildSystem::Init() {
    systemGlobals.InitSystemComponent<BuildSystemComponent>();
    systemGlobals.SetSystemActiveState<BuildSystemComponent>(true);
}

bool BuildSystem::IsSystemActive() {
    return systemGlobals.IsSystemActive<BuildSystemComponent>();
}

void BuildSystem::AddUnitBuilder(CUnit *unit){
    auto entity = unit->entityReference;
    auto unitDef = unit->unitDef;
    auto buildSpeed = EcsMain::registry.emplace_or_replace<BuildPower>(entity, unitDef->buildSpeed / GAME_SPEED).value;

    LOG("%s: added unit %d (%d) with build speed %f", __func__, unit->id, (int)entity, buildSpeed);
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

    //EcsMain::registry.emplace_or_replace<FlowEconomy::AwaitingEconomyAssignment>(entity);
    EcsMain::registry.emplace_or_replace<ActiveBuild>(entity, targetEntity);

    auto& activeBuild = EcsMain::registry.get<ActiveBuild>(entity);
    activeBuild.buildTarget = targetEntity;

    auto buildPower = EcsMain::registry.get<BuildPower>(entity).value;
    auto buildTime = EcsMain::registry.get<BuildTime>(targetEntity).value;

    activeBuild.currentBuildpower = buildPower;
    LOG("%s: %d: BuildPower = %f", __func__, (int)entity, buildPower);
}

void BuildSystem::AddUnitBeingBuilt(CUnit *unit) {
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

void BuildSystem::RemoveUnitBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    EcsMain::registry.remove<ActiveBuild>(entity);

    LOG("%s", __func__);
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
}

void BuildSystem::UnpauseBuilder(CUnit *unit) {
    auto entity = unit->entityReference;
    auto& activeBuild = EcsMain::registry.get<ActiveBuild>(entity);

    // this may not be currently paused.
    if (activeBuild.currentBuildpower >= 0.f) return;

    // FIXME continue build
    //AddUnitBeingBuilt(entity);
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
    EcsMain::registry.remove<BuildCost>(entity);
    EcsMain::registry.remove<BuildComplete>(entity);
}

void BuildSystem::Update() {
    if (!IsSystemActive())
        return;

    if ((gs->frameNum % BUILD_UPDATE_RATE) != BUILD_TICK)
       return;

    LOG("BuildSystem::%s: %d", __func__, gs->frameNum);

    SCOPED_TIMER("ECS::BuildSystem::Update");

    auto group = EcsMain::registry.group<ActiveBuild>(entt::get<Units::Team, Units::UnitId>);
    for (auto entity : group) {
        auto& buildDetails = group.get<ActiveBuild>(entity);
        auto buildPower = buildDetails.currentBuildpower;
        auto buildTarget = buildDetails.buildTarget;

        // currently paused
        if (buildPower == 0.f) continue;

        auto teamId = (group.get<Units::Team>(entity)).value;
        auto team = teamHandler.Team(teamId);
        auto& buildCost = EcsMain::registry.get<BuildCost>(buildTarget);

        float buildRate = 1.f;
        for (int i=0; i<SResourcePack::MAX_RESOURCES; i++){
            bool foundLowerProrationrate = (buildCost[i] > 0.f) && (team->resProrationRates[i] < buildRate);
            buildRate = foundLowerProrationrate ? team->resProrationRates[i] : buildRate;
        }

        // float buildRate = team->prorationRates[(int)buildDetails.prorationType];
        auto& buildProgress = (EcsMain::registry.get<BuildProgress>(buildTarget)).value;
        auto& health = (EcsMain::registry.get<SolidObject::Health>(buildTarget)).value;
        auto maxHealth = (EcsMain::registry.get<SolidObject::MaxHealth>(buildTarget)).value;
        auto buildTime = (EcsMain::registry.get<BuildTime>(buildTarget)).value;

        float buildStep = (buildPower*BUILD_UPDATE_RATE)/buildTime;
        float proratedBuildStep = buildStep * buildRate;
        float nextProgress = buildProgress + proratedBuildStep;
        float nextHealth = health + maxHealth * proratedBuildStep;

        SResourcePack resPull = buildCost * buildStep;
        SResourcePack resUsage(buildCost);
        resUsage *= (nextProgress >= 1.f) ? (1.f - buildProgress) : proratedBuildStep;

        LOG("BuildSystem::%s: %d -> %d (tid: %d m:%f e:%f)", __func__, (int)entity, (int)buildTarget, teamId, resUsage.metal, resUsage.energy);

        if (team->UseResources(resUsage)) {
            if (nextProgress < 1.f) team->recordFlowEcoPull(resPull);

            buildProgress = std::min(nextProgress, 1.f);
            health = std::min(nextHealth, maxHealth);
            if (buildProgress == 1.f){
                EcsMain::registry.emplace<BuildComplete>(buildTarget);
            }
            LOG("BuildSystem::%s: %d -> %d (%f%%)", __func__, (int)entity, (int)buildTarget, buildProgress*100.f);
        }
        else {
            team->recordFlowEcoPull(resPull);
        }
    }
}
