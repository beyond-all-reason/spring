#include "FlowEconomySystem.h"
#include "EnvResourceSystem.h"
#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
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
}

void FlowEconomySystem::UpdateFixedMetalIncome(){
    auto group = EcsMain::registry.group<MetalFixedIncome>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto income = (group.get<MetalFixedIncome>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.fixedIncome.metal += income;
        LOG("%s: [%d] metal %f", __func__, teamId, income);
    }
}

void FlowEconomySystem::UpdateFixedEnergyIncome(){
    auto group = EcsMain::registry.group<EnergyFixedIncome>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto income = (group.get<EnergyFixedIncome>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.fixedIncome.energy += income;
        LOG("%s: [%d] energy %f", __func__, teamId, income);
    }
}

void FlowEconomySystem::UpdateProratableMetalIncome(){
    // double check this is the correct way to utilise the natural behaviour of groups
    auto group = EcsMain::registry.group<MetalProratableIncome>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto income = (group.get<MetalProratableIncome>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.proratableIncome.metal += income;
        LOG("%s: [%d] metal %f", __func__, teamId, income);
    }
}

void FlowEconomySystem::UpdateProratableEnergyIncome(){
    auto group = EcsMain::registry.group<EnergyProratableIncome>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto income = (group.get<EnergyProratableIncome>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.proratableIncome.energy += income;
        LOG("%s: [%d] energy %f", __func__, teamId, income);
    }
}

// void FlowEconomySystem::UpdateProratableMetalExpense(){
//     auto combinedGroup = EcsMain::registry.group<MetalProratableUse, EnergyProratableUse>(entt::get<Units::Team>);
//     auto group = EcsMain::registry.group<MetalProratableUse>(entt::get<Units::Team>);
//     auto entitiesLeftToProcess = group.size() - combinedGroup.size();
//     for (auto entity : group) {
//         if (entitiesLeftToProcess-- == 0) break;
//         auto income = (group.get<MetalProratableUse>(entity)).value;
//         auto teamId = (group.get<Units::Team>(entity)).value;
//         teamHandler.Team(teamId)->resNext.proratableExpense.metal += income;
//         LOG("%s: [%d] metal %f", __func__, teamId, income);
//     }
// }

// void FlowEconomySystem::UpdateProratableEnergyExpense(){
//     auto combinedGroup = EcsMain::registry.group<MetalProratableUse, EnergyProratableUse>(entt::get<Units::Team>);
//     auto group = EcsMain::registry.group<EnergyProratableUse>(entt::get<Units::Team>);
//     auto entitiesLeftToProcess = group.size() - combinedGroup.size();
//     for (auto entity : group) {
//         if (entitiesLeftToProcess-- == 0) break;
//         auto income = (group.get<EnergyProratableUse>(entity)).value;
//         auto teamId = (group.get<Units::Team>(entity)).value;
//         teamHandler.Team(teamId)->resNext.proratableExpense.energy += income;
//         LOG("%s: [%d] energy %f", __func__, teamId, income);
//     }
// }

// void FlowEconomySystem::UpdateProratableCombinedExpense(){
//     auto group = EcsMain::registry.group<MetalProratableUse, EnergyProratableUse>(entt::get<Units::Team>);
//     for (auto entity : group) {
//         SResourcePack income;
//         income.energy = (group.get<EnergyProratableUse>(entity)).value;
//         income.metal = (group.get<MetalProratableUse>(entity)).value;
//         auto teamId = (group.get<Units::Team>(entity)).value;
//         teamHandler.Team(teamId)->resNext.dependentProratableExpense += income;
//         LOG("%s: [%d] energy %f", __func__, teamId, income.energy);
//         LOG("%s: [%d] metal %f", __func__, teamId, income.metal);
//     }
// }

void FlowEconomySystem::Update() {
    if (!active)
        return;

    if ((gs->frameNum % FLOW_ECONOMY_UPDATE_RATE) != FLOW_ECONOMY_TICK)
       return;

    SCOPED_TIMER("ECS::FlowEconomySystem::Update");

    LOG("FlowEconomySystem::%s: %d", __func__, gs->frameNum);

    UpdateEconomyPredictions();
    UpdateAllTeamsEconomy();
    InformWaitingEntitiesEconomyIsAssigned();
}

void FlowEconomySystem::UpdateEconomyPredictions(){
    UpdateFixedEnergyIncome();
    UpdateFixedMetalIncome();

    UpdateProratableEnergyIncome();
    UpdateProratableMetalIncome();
    
    // Add counter for resource usage
    // UpdateFixedMetalExpense();
    // UpdateFixedEnergyExpense();

    // UpdateProratableMetalExpense();
    // UpdateProratableEnergyExpense();
    // UpdateProratableCombinedExpense();
}

void FlowEconomySystem::UpdateAllTeamsEconomy() {
    for (int i=0; i<teamHandler.ActiveTeams(); i++){
        UpdateTeamEconomy(i);
    }
}

void FlowEconomySystem::InformWaitingEntitiesEconomyIsAssigned() {
    EcsMain::registry.view<AwaitingEconomyAssignment>().each([](auto entity){EcsMain::registry.remove<AwaitingEconomyAssignment>(entity);});
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

    curTeam->resCurrent = curTeam->resNext;
    curTeam->resNext = EconomyFlowSnapshot();

    SResourcePack storage = curTeam->res;
    SResourcePack incomeFromLastFrame = curTeam->resNextIncome;
    SResourcePack demand = curTeam->flowEcoPull + curTeam->resCurrent.proratableExpense;

    SResourcePack fixedIncome = curTeam->resCurrent.fixedIncome;
    SResourcePack proratableIncome = curTeam->resCurrent.proratableIncome;

    // derived values
    SResourcePack supply = storage + incomeFromLastFrame;

    float energyProrationRate = getProrationRate(supply.energy, demand.energy);
    float metalProrationRate = getProrationRate(supply.metal, demand.metal);
    float minProrationRate = std::min(energyProrationRate, metalProrationRate);

    SResourcePack proratedUseRates(metalProrationRate, energyProrationRate);
    SResourcePack proratableUseTaken;// = demand * proratedUseRates;

    // Apply economy updates
    curTeam->flowEcoPull = SResourcePack();
    curTeam->flowEcoFullPull = SResourcePack();
    
    curTeam->AddResources(incomeFromLastFrame);
    curTeam->resPull += proratableUseTaken*economyMultiplier;

    curTeam->flowEcoReservedSupply = proratableUseTaken * economyMultiplier;

    // do after all teams are updated.
    curTeam->prorationRates[(int)Build::ProrationRate::PRORATION_NONE] = 1.f;
    curTeam->prorationRates[(int)Build::ProrationRate::PRORATION_ONLY_METAL] = metalProrationRate;
    curTeam->prorationRates[(int)Build::ProrationRate::PRORATION_ONLY_ENERGY] = energyProrationRate;
    curTeam->prorationRates[(int)Build::ProrationRate::PRORATION_ALL] = minProrationRate;

    // values to add to the storage next frame. This approach avoids the paradox
    // with variable income impacting variable use.
    // proratable income is dependent on the other resource - so this deliberately looks to wrong way around
    SResourcePack proratedIncomeRates(energyProrationRate, metalProrationRate);
    SResourcePack nextFrameIncome = fixedIncome + proratableIncome * proratedIncomeRates;

    curTeam->resNextIncome = nextFrameIncome * economyMultiplier;

    if (teamId == 0){
        LOG("============================================");
        LOG("%s: %d: income = (%f,%f)", __func__, gs->frameNum, incomeFromLastFrame.energy, incomeFromLastFrame.metal);
        LOG("%s: %d: forProration = (%f,%f)", __func__, gs->frameNum, supply.energy, supply.metal);
        LOG("%s: %d: poratableUse = (%f,%f)", __func__, gs->frameNum, demand.energy, demand.metal);
        LOG("%s: %d: prorationrate = (%.10f,%.10f)", __func__, gs->frameNum, energyProrationRate, metalProrationRate);
        LOG("%s: %d: minProrationRate = %.10f", __func__, gs->frameNum, minProrationRate);
        LOG("%s: %d: resNextIncome.energy = %f", __func__, gs->frameNum, curTeam->resNextIncome.energy);
        LOG("%s: %d: resNextIncome.metal = %f", __func__, gs->frameNum, curTeam->resNextIncome.metal);
    }
}
