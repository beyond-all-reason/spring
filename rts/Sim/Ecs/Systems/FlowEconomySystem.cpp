#include "FlowEconomySystem.h"
#include "EnvResourceSystem.h"
#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeProfiler.h"

#include "Sim/Misc/GlobalSynced.h"


FlowEconomySystem flowEconomySystem;

using namespace FlowEconomy;

void FlowEconomySystem::Init() {
    if (modInfo.economySystem == ECONOMY_SYSTEM_ECS) {
        active = true;
        economyMultiplier = 1.f / (GAME_SPEED / FLOW_ECONOMY_UPDATE_RATE); // sim display refresh rate
        LOG("%s: ECS economy system active (%f)", __func__, economyMultiplier);
    } else {
        active = false;
        LOG("%s: ECS economy system disabled", __func__);
    }
}

void FlowEconomySystem::AddFlowEconomyUnit(CUnit *unit) {

    auto entity = unit->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    if (active)
        return; // eco is only added when needed

    // force add the components needed when the system isn't active to ensure the original code still works.
}

template<class T, typename V>
void TryAddToComponent(entt::entity entity, V addition) {
    auto comp = EcsMain::registry.try_get<T>(entity);
    if (comp != nullptr) {
        *comp += addition;
    }
}

void FlowEconomySystem::ProcessProratableIncome() {
    auto group = EcsMain::registry.group<ResourceAdd, ResourceUse>(entt::get<Units::Team, Units::OwningEntity>);
    for (auto entity : group) {
        auto& resAdd = group.get<ResourceAdd>(entity);
        auto& resUse = group.get<ResourceUse>(entity);
        auto teamId = (group.get<Units::Team>(entity)).value;
        auto team = teamHandler.Team(teamId);
        auto owner = (group.get<Units::OwningEntity>(entity)).value;

        float minProrationRate = 1.f;
        for (int i=0; i<SResourcePack::MAX_RESOURCES; i++){
            LOG("%s: %d %f (%f) -> %f", __func__, i, resUse[i], team->resProrationRates[i], minProrationRate);
            bool foundLowerProrationrate = (resUse[i] > 0.f) && (team->resProrationRates[i] < minProrationRate);
            minProrationRate = foundLowerProrationrate ? team->resProrationRates[i] : minProrationRate;
        }

        SResourcePack resPull = resUse * economyMultiplier;
        SResourcePack proratedResUse = resPull * minProrationRate;
        SResourcePack proratedResAdd = resAdd * minProrationRate * economyMultiplier;

        team->resNext.income += proratedResAdd;
        team->UseResources(proratedResUse);
        team->recordFlowEcoPull(resPull);

        TryAddToComponent<UnitEconomy::ResourcesCurrentMake>(owner, proratedResAdd);
        TryAddToComponent<UnitEconomy::ResourcesCurrentUsage>(owner, proratedResUse);
    }
}

void FlowEconomySystem::ProcessFixedIncome() {
    auto combinedGroup = EcsMain::registry.group<ResourceAdd, ResourceUse>(entt::get<Units::Team, Units::OwningEntity>);
    auto group = EcsMain::registry.group<ResourceAdd>(entt::get<Units::Team, Units::OwningEntity>/*, entt::exclude<ResourceUse>*/);
    auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    for (auto entity : group) {
        if (entitiesLeftToProcess-- == 0) break;
        auto& resAdd = group.get<ResourceAdd>(entity);
        auto teamId = (group.get<Units::Team>(entity)).value;
        auto owner = (group.get<Units::OwningEntity>(entity)).value;

        SResourcePack fixedResAdd = resAdd * economyMultiplier;
        teamHandler.Team(teamId)->resNext.income += fixedResAdd;

        TryAddToComponent<UnitEconomy::ResourcesCurrentMake>(owner, fixedResAdd);
    }
}

void FlowEconomySystem::ProcessExpenses() {
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

        SResourcePack resPull = resUse * economyMultiplier;
        SResourcePack proratedResUse = resPull * minProrationRate;
        team->UseResources(proratedResUse);
        team->recordFlowEcoPull(resPull);

        TryAddToComponent<UnitEconomy::ResourcesCurrentUsage>(owner, proratedResUse);
    }
}

void FlowEconomySystem::Update() {
    if (!active)
        return;

    if ((gs->frameNum % FLOW_ECONOMY_UPDATE_RATE) != FLOW_ECONOMY_TICK)
       return;

    SCOPED_TIMER("ECS::FlowEconomySystem::Update");

    LOG("FlowEconomySystem::%s: %d", __func__, gs->frameNum);

    UpdateAllTeamsEconomy();

    UpdateEconomyPredictions();
}

void FlowEconomySystem::UpdateEconomyPredictions(){
    ProcessExpenses();
    ProcessFixedIncome();
    ProcessProratableIncome();
}

void FlowEconomySystem::UpdateAllTeamsEconomy() {
    for (int i=0; i<teamHandler.ActiveTeams(); i++){
        UpdateTeamEconomy(i);
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
    float supplyDemandRatio = (float)(std::trunc((supply / demand)*truncAccuracy) / truncAccuracy);

    return std::min(1.f, supplyDemandRatio);
}

void FlowEconomySystem::UpdateTeamEconomy(int teamId){
    // Get available resources for proration
    CTeam* curTeam = teamHandler.Team(teamId);

    curTeam->applyExcessToShared();

    // if (teamId == 0) {
    //     LOG("Last snapshot: (%f, %f)", curTeam->resSnapshot.metal, curTeam->resSnapshot.energy);
    //     LOG("New snapshot: (%f, %f)", curTeam->res.metal, curTeam->res.energy);
    //     if (curTeam->res.metal > curTeam->resSnapshot.metal + 0.5f || curTeam->res.energy > curTeam->resSnapshot.energy + 0.5f)
    //         LOG("Upwards Blip Detected!!!");
    // }

    // TODO:
    // singleton components
    // eco draw from building -- switch to dependencies
    // reserve enforcement
    // dynamic reserve
    // SSE SresourcePack?
    // support builders
    // stop/start building
    // check original eco still works
    //  - get alt system up to support
    // rez
    // repair
    // reclaim
    // terraform
    // check all entities are cleared at end of match
    // save/load entities/components

    // reserve
    // resReal = resUse + resFailedUse
    // lastResReserve = resReserve
    // resIdeal = proration == 1.f ? resPull : supply 
    // if resIdeal - resReal < 0.5
    // resReserve = resIdeal
    // else
    // resReserve = (resReal + lastResReserve)/2, min resReal, max resIdeal

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

    // Apply economy updates
    curTeam->flowEcoPull = SResourcePack();
    curTeam->flowEcoFullPull = SResourcePack();
    
    curTeam->AddResources(incomeFromLastFrame);

    curTeam->flowEcoReservedSupply = proratedDemand;

    // do after all teams are updated.
    curTeam->resProrationRates = proratedUseRates;

    if (teamId == 0){
        LOG("============================================");
        LOG("%s: %d: income = (%f,%f)", __func__, gs->frameNum, incomeFromLastFrame.energy, incomeFromLastFrame.metal);
        LOG("%s: %d: forProration = (%f,%f)", __func__, gs->frameNum, supply.energy, supply.metal);
        LOG("%s: %d: poratableUse = (%f,%f)", __func__, gs->frameNum, demand.energy, demand.metal);
        LOG("%s: %d: prorationrate = (%.10f,%.10f)", __func__, gs->frameNum, proratedUseRates[1], proratedUseRates[0]);
        //LOG("%s: %d: minProrationRate = %.10f", __func__, gs->frameNum, minProrationRate);
        //LOG("%s: %d: resNextIncome.energy = %f", __func__, gs->frameNum, curTeam->resNextIncome.energy);
        //LOG("%s: %d: resNextIncome.metal = %f", __func__, gs->frameNum, curTeam->resNextIncome.metal);
    }
}
