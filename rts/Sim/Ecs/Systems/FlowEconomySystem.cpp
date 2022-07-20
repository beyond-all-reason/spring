#include "FlowEconomySystem.h"

#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitDef.h"
#include "System/SpringMath.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"

#include "Sim/Misc/GlobalSynced.h"

using namespace FlowEconomy;
using namespace SystemGlobals;

void ReturnUnusedEco(entt::registry &registry, entt::entity entity) {
    //LOG("%s: %d returning unused eco", __func__, (int)entity);

    auto resUnused = EcsMain::registry.get<AllocatedUnusedResource>(entity).res;
    if (resUnused == SResourcePack()) return;

    //LOG("%s: %d (%f,%f,%f,%f)", __func__, (int)entity, resUnused[0], resUnused[1], resUnused[2], resUnused[3]);

    auto teamComp = EcsMain::registry.try_get<Units::Team>(entity);
    if (teamComp == nullptr) return;

    //LOG("%s: %d on team %d", __func__, (int)entity, teamComp->value);

    auto ownerComp = EcsMain::registry.try_get<Units::OwningEntity>(entity);
    if (ownerComp != nullptr) entity = ownerComp->value;

    //LOG("%s: %d after owner check", __func__, (int)entity);

    auto team = teamHandler.Team(teamComp->value);
    team->UnuseResources(resUnused);

    TryAddToComponent<UnitEconomy::ResourcesCurrentUsage>(entity, -resUnused);

    //LOG("%s: %d eco returned", __func__, (int)entity);
}

void FlowEconomySystem::Init() {
    if (modInfo.economySystem == ECONOMY_SYSTEM_ECS) {
        systemGlobals.CreateSystemComponent<FlowEconomySystemComponent>();

        auto& flowEconomyComp = systemGlobals.GetSystemComponent<FlowEconomySystemComponent>();
        flowEconomyComp.economyMultiplier = 1.f / (GAME_SPEED / FLOW_ECONOMY_UPDATE_RATE); // sim display refresh rate

        EcsMain::registry.on_destroy<AllocatedUnusedResource>().connect<&ReturnUnusedEco>();

        LOG("%s: ECS economy system active (%f)", __func__, flowEconomyComp.economyMultiplier);
    } else {
        LOG("%s: ECS economy system disabled", __func__);
    }
}

void ProcessProratableIncome(const FlowEconomySystemComponent& system) {
    auto group = EcsMain::registry.group<ResourceAdd, ResourceUse>(entt::get<Units::Team, Units::OwningEntity, IsPassiveEconomyTask, AllocatedUnusedResource>);
    for (auto entity : group) {
        const auto& resAdd = group.get<ResourceAdd>(entity);
        auto teamId = (group.get<Units::Team>(entity)).value;
        auto owner = (group.get<Units::OwningEntity>(entity)).value;
        auto& allocatedResources = group.get<AllocatedUnusedResource>(entity);
        auto team = teamHandler.Team(teamId);

        SResourcePack proratedResAdd = resAdd * allocatedResources.prorationRate * system.economyMultiplier;
        team->resNext.add += proratedResAdd;

        TryAddToComponent<UnitEconomy::ResourcesCurrentMake>(owner, proratedResAdd);
        TryAddToComponent<UnitEconomy::ResourcesCurrentUsage>(owner, allocatedResources.res);

        LOG("%s: %d (%f,%f,%f,%f)", __func__, (int)entity, allocatedResources.res[0], allocatedResources.res[1], allocatedResources.res[2], allocatedResources.res[3]);

        allocatedResources.res = SResourcePack();
    }
}

void ProcessFixedIncome(const FlowEconomySystemComponent& system) {
    auto combinedGroup = EcsMain::registry.group<ResourceAdd, ResourceUse>(entt::get<Units::Team, Units::OwningEntity, IsPassiveEconomyTask, AllocatedUnusedResource>);
    auto group = EcsMain::registry.group<ResourceAdd>(entt::get<Units::Team, Units::OwningEntity, IsPassiveEconomyTask>);
    auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    for (auto entity : group) {
        if (entitiesLeftToProcess-- == 0) break;
        const auto& resAdd = group.get<ResourceAdd>(entity);
        auto teamId = (group.get<Units::Team>(entity)).value;
        auto owner = (group.get<Units::OwningEntity>(entity)).value;

        SResourcePack fixedResAdd = resAdd * system.economyMultiplier;
        teamHandler.Team(teamId)->resNext.add += fixedResAdd;

        TryAddToComponent<UnitEconomy::ResourcesCurrentMake>(owner, fixedResAdd);
    }
}

void ProcessExpenses(const FlowEconomySystemComponent& system) {
    auto combinedGroup = EcsMain::registry.group<ResourceAdd, ResourceUse>(entt::get<Units::Team, Units::OwningEntity, IsPassiveEconomyTask, AllocatedUnusedResource>);
    auto group = EcsMain::registry.group<ResourceUse>(entt::get<Units::Team, Units::OwningEntity, IsPassiveEconomyTask, AllocatedUnusedResource>);
    auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    for (auto entity : group) {
        if (entitiesLeftToProcess-- == 0) break;

        auto owner = (group.get<Units::OwningEntity>(entity)).value;
        auto& allocatedResources = group.get<AllocatedUnusedResource>(entity);

        TryAddToComponent<UnitEconomy::ResourcesCurrentUsage>(owner, allocatedResources.res);

        LOG("%s: %d (%f,%f,%f,%f)", __func__, (int)entity, allocatedResources.res[0], allocatedResources.res[1], allocatedResources.res[2], allocatedResources.res[3]);
        allocatedResources.res = SResourcePack();
    }
}

float MinProrationRateUsed(const SResourcePack& resUse, const SResourcePack& rates) {
    float minProrationRate = 1.f;
    for (int i=0; i<SResourcePack::MAX_RESOURCES; ++i){
        if (resUse[i] > 0.f)
            minProrationRate = std::min(rates[i], minProrationRate);
    }
    return minProrationRate;
}

void GetFullEconomyDraw(const FlowEconomySystemComponent& system) {
    auto group = EcsMain::registry.group<ResourceUse>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto& resUse = group.get<ResourceUse>(entity);
        auto teamId = (group.get<Units::Team>(entity)).value;

        teamHandler.Team(teamId)->resNext.use += resUse * system.economyMultiplier;
    }
}

void AllocateResources(const FlowEconomySystemComponent& system) {
    auto group = EcsMain::registry.group<ResourceUse>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto& resUse = group.get<ResourceUse>(entity);
        auto teamId = (group.get<Units::Team>(entity)).value;
        CTeam* curTeam = teamHandler.Team(teamId);

        float prorationRate = (curTeam->resProrationOn)
                                    ? MinProrationRateUsed(resUse, curTeam->resProrationRates)
                                    : 1.f;
        SResourcePack resAllocated = resUse * prorationRate * system.economyMultiplier;
        EcsMain::registry.emplace_or_replace<AllocatedUnusedResource>(entity, resAllocated, prorationRate);

        curTeam->resCurrent.use += resAllocated;

        LOG("%s: %d (%f,%f,%f,%f)", __func__, (int)entity, resAllocated[0], resAllocated[1], resAllocated[2], resAllocated[3]);
    }
}

void UpdateTeamEconomy(int teamId){
    SResourcePack maxProrationrate(1.f);

    // Get available resources for proration
    CTeam* curTeam = teamHandler.Team(teamId);
    curTeam->resSnapshot = curTeam->res;
    curTeam->resCurrent = curTeam->resNext;
    curTeam->resNext = SResourceFlow();

    SResourcePack storage = curTeam->res;
    SResourcePack incomeFromLastFrame = curTeam->resCurrent.add;
    SResourcePack supply = storage + incomeFromLastFrame;
    SResourcePack demand = curTeam->resCurrent.use;
    SResourcePack proratedUseRates = SResourcePack::min(supply / demand, maxProrationrate);

    curTeam->resDemand = demand;
    curTeam->resPull += demand;
    curTeam->resCurrent.use = SResourcePack();
    curTeam->resProrationRates = proratedUseRates;
    curTeam->resProrationOn = !(proratedUseRates == maxProrationrate);

    if (teamId == 0){
        LOG("============================================");
        LOG("%s: %d: snapshot = (%f,%f,%f,%f)", __func__, gs->frameNum, curTeam->resSnapshot[0], curTeam->resSnapshot[1], curTeam->resSnapshot[2], curTeam->resSnapshot[3]);
        LOG("%s: %d: resources = (%f,%f,%f,%f)", __func__, gs->frameNum, storage[0], storage[1], storage[2], storage[3]);
        LOG("%s: %d: income = (%f,%f,%f,%f)", __func__, gs->frameNum, incomeFromLastFrame[0], incomeFromLastFrame[1], incomeFromLastFrame[2], incomeFromLastFrame[3]);
        LOG("%s: %d: forProration = (%f,%f,%f,%f)", __func__, gs->frameNum, supply[0], supply[1], supply[2], supply[3]);
        LOG("%s: %d: poratableUse = (%f,%f,%f,%f)", __func__, gs->frameNum, demand[0], demand[1], demand[2], demand[3]);
        LOG("%s: %d: prorationrate = (%.10f,%.10f,%.10f,%.10f)", __func__, gs->frameNum, proratedUseRates[0], proratedUseRates[1], proratedUseRates[2], proratedUseRates[3]);

        //LOG("%s: %d: minProrationRate = %.10f", __func__, gs->frameNum, minProrationRate);
        //LOG("%s: %d: resNextIncome.energy = %f", __func__, gs->frameNum, curTeam->resNextIncome.energy);
        //LOG("%s: %d: resNextIncome.metal = %f", __func__, gs->frameNum, curTeam->resNextIncome.metal);
    }
}

void UpdateAllTeamsEconomy() {
    for (int i=0; i<teamHandler.ActiveTeams(); ++i)
        UpdateTeamEconomy(i);
}

void ApplyAllTeamsEconomy() {
    for (int i=0; i<teamHandler.ActiveTeams(); ++i){
        CTeam* curTeam = teamHandler.Team(i);

        SResourcePack storage = curTeam->res;
        SResourcePack incomeFromLastFrame = curTeam->resCurrent.add;
        SResourcePack supply = storage + incomeFromLastFrame;
        SResourcePack demand = curTeam->resDemand;
        SResourcePack maxUse = SResourcePack::min(supply, demand);

        curTeam->resCurrent.use = SResourcePack::min(curTeam->resCurrent.use, maxUse);
        curTeam->ApplyResourceFlow(curTeam->resCurrent);
    }
}

void FlowEconomySystem::Update() {
    LOG("FlowEconomySystem::%s: Check active %d", __func__, gs->frameNum);

    if (!systemGlobals.IsSystemActive<FlowEconomySystemComponent>())
        return;

    LOG("FlowEconomySystem::%s: Check tick %d", __func__, gs->frameNum);

    if ((gs->frameNum % FLOW_ECONOMY_UPDATE_RATE) != FLOW_ECONOMY_TICK)
       return;

    SCOPED_TIMER("ECS::FlowEconomySystem::Update");

    LOG("FlowEconomySystem::%s: %d", __func__, gs->frameNum);

    auto& system = systemGlobals.GetSystemComponent<FlowEconomySystemComponent>();

    GetFullEconomyDraw(system);
    UpdateAllTeamsEconomy();
    AllocateResources(system);
    ApplyAllTeamsEconomy();

    // Unit Economy Tasks -- probably should be moved to another system
    ProcessExpenses(system);
    ProcessFixedIncome(system);
    ProcessProratableIncome(system);
}
