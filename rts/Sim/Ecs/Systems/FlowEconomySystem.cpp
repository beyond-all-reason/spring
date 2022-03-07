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
        economyMultiplier = 1.f / GAME_SPEED; // sim display refresh rate
        LOG("%s: ECS economy system active (%f)", __func__, economyMultiplier);
    } else {
        active = false;
        LOG("%s: ECS economy system disabled", __func__);
    }
}

void FlowEconomySystem::AddFlowEconomyUnit(CUnit *unit) {
    if (!active)
        return;

    auto entity = unit->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }
}

template<class T>
void UpdateUnitEconomy(entt::entity entity, float amount){
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    if (amount <= 0.f){
        EcsMain::registry.remove<T>(entity);
    }
    else {
        EcsMain::registry.emplace_or_replace<T>(entity, amount);
    }
}

void FlowEconomySystem::UpdateUnitProratableEnergyIncome(CUnit *unit, float amount){
    UpdateUnitEconomy<EnergyProratableIncome>(unit->entityReference, amount);
}

void FlowEconomySystem::UpdateUnitProratableMetalIncome(CUnit *unit, float amount){
    UpdateUnitEconomy<MetalProratableIncome>(unit->entityReference, amount);
}

void FlowEconomySystem::UpdateUnitFixedEnergyIncome(CUnit *unit, float amount){
    UpdateUnitEconomy<EnergyFixedIncome>(unit->entityReference, amount);
}

void FlowEconomySystem::UpdateUnitFixedEnergyIncome(entt::entity entity, float amount) {
    UpdateUnitEconomy<EnergyFixedIncome>(entity, amount);
}

void FlowEconomySystem::UpdateUnitFixedMetalIncome(CUnit *unit, float amount) {
    UpdateUnitEconomy<MetalFixedIncome>(unit->entityReference, amount);
}

void FlowEconomySystem::UpdateUnitProratableEnergyUse(CUnit *unit, float amount){
    UpdateUnitEconomy<EnergyProratableUse>(unit->entityReference, amount);
}

void FlowEconomySystem::UpdateUnitProratableMetalUse(CUnit *unit, float amount){
    UpdateUnitEconomy<MetalProratableUse>(unit->entityReference, amount);
}

void FlowEconomySystem::UpdateFixedMetalIncome(){
    auto group = EcsMain::registry.group<MetalFixedIncome>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto income = (group.get<MetalFixedIncome>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.fixedIncome.metal += income;
    }
}

void FlowEconomySystem::UpdateFixedEnergyIncome(){
    auto group = EcsMain::registry.group<EnergyFixedIncome>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto income = (group.get<EnergyFixedIncome>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.fixedIncome.energy += income;
    }
}

void FlowEconomySystem::UpdateProratableMetalIncome(){
    // double check this is the correct way to utilise the natural behaviour of groups
    auto group = EcsMain::registry.group<MetalProratableIncome>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto income = (group.get<MetalProratableIncome>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.proratableIncome.metal += income;
    }
}

void FlowEconomySystem::UpdateProratableEnergyIncome(){
    auto group = EcsMain::registry.group<EnergyProratableIncome>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto income = (group.get<EnergyProratableIncome>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.proratableIncome.energy += income;
    }
}

void FlowEconomySystem::UpdateFixedMetalExpense(){
    auto group = EcsMain::registry.group<MetalFixedUse>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto income = (group.get<MetalFixedUse>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.fixedExpense.metal += income;
    }
}

void FlowEconomySystem::UpdateFixedEnergyExpense(){
    auto group = EcsMain::registry.group<EnergyFixedUse>(entt::get<Units::Team>);
    for (auto entity : group) {
        auto income = (group.get<EnergyFixedUse>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.fixedExpense.energy += income;
    }
}

void FlowEconomySystem::UpdateProratableMetalExpense(){
    auto combinedGroup = EcsMain::registry.group<MetalProratableUse, EnergyProratableUse>(entt::get<Units::Team>);
    auto group = EcsMain::registry.group<MetalProratableUse>(entt::get<Units::Team>);
    auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    for (auto entity : group) {
        if (entitiesLeftToProcess-- == 0) break;
        auto income = (group.get<MetalProratableUse>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.independentProratableExpense.metal += income;
    }
}

void FlowEconomySystem::UpdateProratableEnergyExpense(){
    auto combinedGroup = EcsMain::registry.group<MetalProratableUse, EnergyProratableUse>(entt::get<Units::Team>);
    auto group = EcsMain::registry.group<EnergyProratableUse>(entt::get<Units::Team>);
    auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    for (auto entity : group) {
        if (entitiesLeftToProcess-- == 0) break;
        auto income = (group.get<EnergyProratableUse>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.independentProratableExpense.energy += income;
    }
}

void FlowEconomySystem::UpdateProratableCombinedExpense(){
    auto group = EcsMain::registry.group<MetalProratableUse, EnergyProratableUse>(entt::get<Units::Team>);
    for (auto entity : group) {
        SResourcePack income;
        income.energy = (group.get<EnergyProratableUse>(entity)).value;
        income.metal = (group.get<MetalProratableUse>(entity)).value;
        auto teamId = (group.get<Units::Team>(entity)).value;
        teamHandler.Team(teamId)->resNext.dependentProratableExpense += income;
    }
}

bool FlowEconomySystem::RegisterOneOffExpense(CUnit* unit, float amount) {
    // TODO
    return false;
}

void FlowEconomySystem::Update() {
    if (!active)
        return;

    SCOPED_TIMER("ECS::FlowEconomySystem::Update");

    SlowUpdate();
}

void FlowEconomySystem::SlowUpdate() {
    if ((gs->frameNum % FLOW_ECONOMY_UPDATE_RATE) != FLOW_ECONOMY_TICK)
       return;

    UpdateFixedEnergyIncome();
    UpdateFixedMetalIncome();

    UpdateProratableEnergyIncome();
    UpdateProratableMetalIncome();
    
    // Add counter for resource usage
    UpdateFixedMetalExpense();
    UpdateFixedEnergyExpense();

    UpdateProratableMetalExpense();
    UpdateProratableEnergyExpense();
    UpdateProratableCombinedExpense();

    for (int i=0; i<teamHandler.ActiveTeams(); i++){
        UpdateTeamEconomy(i);
    }
}

void FlowEconomySystem::UpdateTeamEconomy(int teamId){
    // Get available resources for proration
    CTeam* curTeam = teamHandler.Team(teamId);

    if ((gs->frameNum % ECONOMY_SLOWUPDATE_RATE) == FLOW_ECONOMY_TICK){
        curTeam->resCurrent = curTeam->resNext;
        curTeam->resNext = EconomyFlowSnapshot();
    }

    SResourcePack storage = curTeam->res;
    SResourcePack incomeFromLastFrame = curTeam->resNextIncome;

    SResourcePack fixedUse = curTeam->resCurrent.fixedExpense;
    SResourcePack independentProratableUse = curTeam->resCurrent.independentProratableExpense;
    SResourcePack dependentProratableUse = curTeam->resCurrent.independentProratableExpense;

    SResourcePack fixedIncome = curTeam->resCurrent.fixedIncome;
    SResourcePack proratableIncome = curTeam->resCurrent.proratableIncome;

    // derived values
    SResourcePack resForProration = storage + incomeFromLastFrame + (-fixedUse);
    SResourcePack proratableUse = independentProratableUse + dependentProratableUse;

    float energyProrationRate = (proratableUse.energy != 0.f) ? std::min(1.f, resForProration.energy / proratableUse.energy) : 1.f;
    float metalProrationRate = (proratableUse.metal != 0.f) ? std::min(1.f, resForProration.metal / proratableUse.metal) : 1.f;
    float minProrationRate = std::max(0.f, std::min(energyProrationRate, metalProrationRate));

    // if one resource needs heavier proration (lower rate) than the other,
    // then the lighter strained resource may have more resources to offer.
    if (energyProrationRate > metalProrationRate){
        float adjustedEnergyForProration = resForProration.energy + (-dependentProratableUse.energy) * minProrationRate;
        energyProrationRate = (adjustedEnergyForProration != 0.f) ? std::min(1.f, adjustedEnergyForProration / independentProratableUse.energy) : 1.f;

        // LOG("%s: %d: updated adjusted E:%f, rate: %f", __func__, gs->frameNum
        //         , adjustedEnergyForProration, energyProrationRate);
    }
    else if (metalProrationRate > energyProrationRate){
        float adjustedMetalForProration = resForProration.metal + (-dependentProratableUse.metal) * minProrationRate;
        metalProrationRate = (adjustedMetalForProration != 0.f) ? std::min(1.f, adjustedMetalForProration / independentProratableUse.metal) : 1.f;

        // LOG("%s: %d: updated adjusted M:%f, rate: %f", __func__, gs->frameNum
        //         , adjustedMetalForProration, metalProrationRate);
    }

    SResourcePack independentProratedUseRates(metalProrationRate, energyProrationRate);
    SResourcePack proratableUseTaken = independentProratableUse * independentProratedUseRates
                                    + dependentProratableUse * minProrationRate;
    SResourcePack expense = (fixedUse + proratableUseTaken)*economyMultiplier;

    // Apply economy updates - take off the cap, then apply storage cap afterwards
    curTeam->ApplyResources(incomeFromLastFrame*economyMultiplier, expense);
    curTeam->resPull += expense;

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

    curTeam->resNextIncome = nextFrameIncome;

    if (minProrationRate < 1.f){
        LOG("============================================");
        LOG("%s: %d: income = (%f,%f)", __func__, gs->frameNum, incomeFromLastFrame.energy, incomeFromLastFrame.metal);
        LOG("%s: %d: unconditionalUse = (%f,%f)", __func__, gs->frameNum, fixedUse.energy, fixedUse.metal);
        LOG("%s: %d: forProration = (%f,%f)", __func__, gs->frameNum, resForProration.energy, resForProration.metal);
        LOG("%s: %d: poratableUse = (%f,%f)", __func__, gs->frameNum, proratableUse.energy, proratableUse.metal);
        LOG("%s: %d: prorationrate = (%f,%f)", __func__, gs->frameNum, energyProrationRate, metalProrationRate);
        LOG("%s: %d: minProrationRate = %f", __func__, gs->frameNum, minProrationRate);
        LOG("%s: %d: resNextIncome.energy = %f", __func__, gs->frameNum, curTeam->resNextIncome.energy);
        LOG("%s: %d: resNextIncome.metal = %f", __func__, gs->frameNum, curTeam->resNextIncome.metal);
    }
}
