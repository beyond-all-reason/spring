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

#include "Sim/Misc/GlobalSynced.h"

#undef REACTIVE_ECONOMY

using namespace FlowEconomy;
using namespace SystemGlobals;

template<class T, typename V>
void TryAddToComponent(entt::entity entity, V addition) {
    auto comp = EcsMain::registry.try_get<T>(entity);
    if (comp != nullptr) {
        *comp += addition;
        LOG("%s: (%f,%f)", __func__, (*comp)[0], (*comp)[1]);
    }
}

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

#ifdef REACTIVE_ECONOMY

void ProcessProratableIncome(FlowEconomySystemComponent& system) {
    auto group = EcsMain::registry.group<ResourceAdd, ResourceUse>(entt::get<Units::Team, Units::OwningEntity>);
    for (auto entity : group) {
        auto& resAdd = group.get<ResourceAdd>(entity);
        auto& resUse = group.get<ResourceUse>(entity);
        auto teamId = (group.get<Units::Team>(entity)).value;
        auto team = teamHandler.Team(teamId);
        auto owner = (group.get<Units::OwningEntity>(entity)).value;

        float minProrationRate = 1.f;
        for (int i=0; i<SResourcePack::MAX_RESOURCES; i++){
            bool foundLowerProrationrate = (resUse[i] > 0.f) && (team->resProrationRates[i] < minProrationRate);
            minProrationRate = foundLowerProrationrate ? team->resProrationRates[i] : minProrationRate;

            LOG("%s: %d %f (%f) -> %f", __func__, i, resUse[i], team->resProrationRates[i], minProrationRate);
        }

        SResourcePack resPull = resUse * system.economyMultiplier;
        SResourcePack proratedResUse = resPull * minProrationRate;
        SResourcePack proratedResAdd = resAdd * minProrationRate * system.economyMultiplier;

        team->resNext.income += proratedResAdd;
        team->UseFlowEcoResources(proratedResUse);
        team->recordFlowEcoPull(resPull, proratedResUse);

        TryAddToComponent<UnitEconomy::ResourcesCurrentMake>(owner, proratedResAdd);
        TryAddToComponent<UnitEconomy::ResourcesCurrentUsage>(owner, proratedResUse);
    }
}

void ProcessFixedIncome(FlowEconomySystemComponent& system) {
    auto combinedGroup = EcsMain::registry.group<ResourceAdd, ResourceUse>(entt::get<Units::Team, Units::OwningEntity>);
    auto group = EcsMain::registry.group<ResourceAdd>(entt::get<Units::Team, Units::OwningEntity>/*, entt::exclude<ResourceUse>*/);
    auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    for (auto entity : group) {
        if (entitiesLeftToProcess-- == 0) break;
        auto& resAdd = group.get<ResourceAdd>(entity);
        auto teamId = (group.get<Units::Team>(entity)).value;
        auto owner = (group.get<Units::OwningEntity>(entity)).value;

        SResourcePack fixedResAdd = resAdd * system.economyMultiplier;
        teamHandler.Team(teamId)->resNext.income += fixedResAdd;

        TryAddToComponent<UnitEconomy::ResourcesCurrentMake>(owner, fixedResAdd);
    }
}

void ProcessExpenses(FlowEconomySystemComponent& system) {
    auto combinedGroup = EcsMain::registry.group<ResourceAdd, ResourceUse>(entt::get<Units::Team, Units::OwningEntity>);
    auto group = EcsMain::registry.group<ResourceUse>(entt::get<Units::Team, Units::OwningEntity>/*, entt::exclude<ResourceAdd>*/);
    auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    for (auto entity : group) {
        if (entitiesLeftToProcess-- == 0) break;
        auto& resUse = group.get<ResourceUse>(entity);
        auto teamId = (group.get<Units::Team>(entity)).value;
        auto team = teamHandler.Team(teamId);
        auto owner = (group.get<Units::OwningEntity>(entity)).value;

        float minProrationRate = 1.f;
        for (int i=0; i<SResourcePack::MAX_RESOURCES; i++){
            bool foundLowerProrationrate = (resUse[i] > 0.f) && (team->resProrationRates[i] < minProrationRate);
            minProrationRate = foundLowerProrationrate ? team->resProrationRates[i] : minProrationRate;
        }

        SResourcePack resPull = resUse * system.economyMultiplier;
        SResourcePack proratedResUse = resPull * minProrationRate;
        team->UseFlowEcoResources(proratedResUse);
        team->recordFlowEcoPull(resPull, proratedResUse);

        TryAddToComponent<UnitEconomy::ResourcesCurrentUsage>(owner, proratedResUse);
    }
}

float getProrationRate(float supplyInUnits, float demandInUnits) {
    if (demandInUnits == 0.f)
        return 1.f;

    double supply(supplyInUnits);
    double demand(demandInUnits);

    constexpr double truncAccuracy = 1000000.;

    // This value is carefully truncated to ensure that the sum of each unit drawing resources
    // never exceeds the available resource supply; otherwise the last unit to draw resources
    // will fail when it should have succeeded: all because it went over the supply by a tiny
    // fraction.
    float supplyDemandRatio = springmath::trunc(supply / demand, truncAccuracy);
    //float supplyDemandRatio = (float)(std::trunc((supply / demand)*truncAccuracy) / truncAccuracy);

    return std::min(1.f, supplyDemandRatio);
}

float getProrationChangeRatio(float currentProrationRate, float previousProrationRate) {
    if (previousProrationRate == 0.f)
        return 1.f;

    double curProration(currentProrationRate);
    double prevProration(previousProrationRate);

    // This is to over estimate the ratio slightly to ensure the reserve is at least as big
    // as the expected pull. It is fine to be slightly over.
    constexpr double ceilAccuracy = 100000.;

    return (float)(std::ceil((curProration / prevProration)*ceilAccuracy) / ceilAccuracy);
}

void UpdateTeamEconomy(int teamId){
    // Get available resources for proration
    CTeam* curTeam = teamHandler.Team(teamId);

    curTeam->applyExcessToShared();

    // if (teamId == 0) {
    //     LOG("Last snapshot: (%f, %f)", curTeam->resSnapshot.metal, curTeam->resSnapshot.energy);
    //     LOG("New snapshot: (%f, %f)", curTeam->res.metal, curTeam->res.energy);
    //     if (curTeam->res.metal > curTeam->resSnapshot.metal + 0.5f || curTeam->res.energy > curTeam->resSnapshot.energy + 0.5f)
    //         LOG("Upwards Blip Detected!!!");
    // }

    curTeam->resSnapshot = curTeam->res;
    curTeam->resCurrent = curTeam->resNext;
    curTeam->resNext = EconomyFlowSnapshot();

    SResourcePack storage = curTeam->res;
    SResourcePack incomeFromLastFrame = curTeam->resCurrent.income;
    SResourcePack demand = curTeam->flowEcoPull;

    // derived values
    SResourcePack supply = storage + incomeFromLastFrame;

    SResourcePack proratedUseRates;
    for (int i=0; i<SResourcePack::MAX_RESOURCES; i++){
        proratedUseRates[i] = getProrationRate(supply[i], demand[i]);
    }

    SResourcePack proratedDemand = demand * proratedUseRates;

    // Calculate eco to reserve for flow eco tasks
    // Stops non-flow eco from using the eco before flow eco tasks can use it
    SResourcePack& previousProratedUseRates = curTeam->resProrationRates;
    SResourcePack newReserved;
    SResourcePack optimalSupply;

    for (int i = 0; i<SResourcePack::MAX_RESOURCES; ++i) {
        // no previous pull, or the pull has increased above our current reservation point, or pull is close to optimal.
        bool resetReserve = (curTeam->flowEcoProratedPull[i] <= 0.f)
                         || (curTeam->flowEcoProratedPull[i] >= curTeam->lastFlowEcoOptimalSupply[i] * 0.9999f)
                         || (curTeam->flowEcoProratedPull[i] >= curTeam->lastFlowEcoReservedSupply[i]);

        float max = std::min(supply[i], demand[i]);
        optimalSupply[i] = max;

        // even when the economy is stable there will be fluctations during proration, we want to avoid
        // these fluctations causing unnecessary jumps back to max reserve.
        float buffer = max*0.0001f;
        float newReserve[2] = { (curTeam->flowEcoProratedPull[i] + curTeam->lastFlowEcoReservedSupply[i]) * 0.5f + buffer
                              , (max) };

        newReserved[i] = std::min(newReserve[resetReserve], max);
    }

    // Apply economy updates
    curTeam->flowEcoPull = SResourcePack();
    curTeam->flowEcoProratedPull = SResourcePack();
    curTeam->AddResources(incomeFromLastFrame);
    curTeam->flowEcoReservedSupply = newReserved;
    curTeam->lastFlowEcoReservedSupply = newReserved;
    curTeam->lastFlowEcoOptimalSupply = optimalSupply;

    // do after all teams are updated.
    curTeam->resProrationRates = proratedUseRates;

    if (teamId == 0){
        LOG("============================================");
        LOG("%s: %d: snapshot = (%f,%f,%f,%f)", __func__, gs->frameNum, curTeam->resSnapshot[0], curTeam->resSnapshot[1], curTeam->resSnapshot[2], curTeam->resSnapshot[3]);
        LOG("%s: %d: resources = (%f,%f,%f,%f)", __func__, gs->frameNum, storage[0], storage[1], storage[2], storage[3]);
        LOG("%s: %d: income = (%f,%f,%f,%f)", __func__, gs->frameNum, incomeFromLastFrame[0], incomeFromLastFrame[1], incomeFromLastFrame[2], incomeFromLastFrame[3]);
        LOG("%s: %d: forProration = (%f,%f,%f,%f)", __func__, gs->frameNum, supply[0], supply[1], supply[2], supply[3]);
        LOG("%s: %d: poratableUse = (%f,%f,%f,%f)", __func__, gs->frameNum, demand[0], demand[1], demand[2], demand[3]);
        LOG("%s: %d: reserved eco = (%f,%f,%f,%f)", __func__, gs->frameNum, newReserved[0], newReserved[1], newReserved[2], newReserved[3]);
        LOG("%s: %d: prorationrate = (%.10f,%.10f,%.10f,%.10f)", __func__, gs->frameNum, proratedUseRates[0], proratedUseRates[1], proratedUseRates[2], proratedUseRates[3]);

        //LOG("%s: %d: minProrationRate = %.10f", __func__, gs->frameNum, minProrationRate);
        //LOG("%s: %d: resNextIncome.energy = %f", __func__, gs->frameNum, curTeam->resNextIncome.energy);
        //LOG("%s: %d: resNextIncome.metal = %f", __func__, gs->frameNum, curTeam->resNextIncome.metal);
    }
}

void UpdateAllTeamsEconomy() {
    for (int i=0; i<teamHandler.ActiveTeams(); i++){
        UpdateTeamEconomy(i);
    }
}

void FlowEconomySystem::Update() {
    if (!systemGlobals.IsSystemActive<FlowEconomySystemComponent>())
        return;

    if ((gs->frameNum % FLOW_ECONOMY_UPDATE_RATE) != FLOW_ECONOMY_TICK)
       return;

    SCOPED_TIMER("ECS::FlowEconomySystem::Update");

    LOG("FlowEconomySystem::%s: %d", __func__, gs->frameNum);

    auto& system = systemGlobals.GetSystemComponent<FlowEconomySystemComponent>();

    UpdateAllTeamsEconomy();

    ProcessExpenses(system);
    ProcessFixedIncome(system);
    ProcessProratableIncome(system);
}

#else // proactive economy

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
    if (!systemGlobals.IsSystemActive<FlowEconomySystemComponent>())
        return;

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

#endif