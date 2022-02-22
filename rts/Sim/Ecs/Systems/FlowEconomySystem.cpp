#include "FlowEconomySystem.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/EcsMain.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeProfiler.h"

FlowEconomySystem flowEconomySystem;

using namespace FlowEconomy;

void FlowEconomySystem::Init() {
    if (modInfo.economySystem == ECONOMY_SYSTEM_ECS) {
        active = true;
        economyMultiplier = 1.f / GAME_SPEED;
        LOG("%s: ECS economy system active (%f)", __func__, economyMultiplier);
    } else {
        active = false;
        LOG("%s: ECS economy system disabled", __func__);
    }
}

void FlowEconomySystem::AddUnitEconomy(CUnit *unit) {
    if (!EcsMain::registry.valid(unit->entityReference)){
        LOG("%s: cannot add generator unit to %d because it hasn't been registered yet.", __func__, unit->id);
    }

    EcsMain::registry.emplace<MetalUnconditionalIncome>(unit->entityReference);
    EcsMain::registry.emplace<MetalUnconditionalUse>(unit->entityReference);
    EcsMain::registry.emplace<MetalProratableIncome>(unit->entityReference);
    EcsMain::registry.emplace<MetalProratableUse>(unit->entityReference);
    EcsMain::registry.emplace<EnergyUnconditionalIncome>(unit->entityReference);
    EcsMain::registry.emplace<EnergyUnconditionalUse>(unit->entityReference);
    EcsMain::registry.emplace<EnergyProratableIncome>(unit->entityReference);
    EcsMain::registry.emplace<EnergyProratableUse>(unit->entityReference);
    EcsMain::registry.emplace<BuildRate>(unit->entityReference);

    LOG("%s: added flow economy to unit %d", __func__, unit->id);
}

template <class T>
float UpdateUnitEconomy(CUnit *unit, float amount) {
    float totalAdjustment = 0.f;

    if(EcsMain::registry.all_of<T>(unit->entityReference)) {
        const auto& ecoComp = EcsMain::registry.get<T>(unit->entityReference);
        totalAdjustment -= ecoComp.value;
        EcsMain::registry.remove<T>(unit->entityReference);
    }

    if (amount > 0.f) {
        EcsMain::registry.emplace<T>(unit->entityReference, amount);
        totalAdjustment += amount;
    }

    return totalAdjustment;
}

template<class TEnergy, class TMetal>
int createResComponents(entt::entity entity, SResourcePack res) {
    int activeCount = 0;

    if (res.energy > 0.f){
        EcsMain::registry.emplace<TEnergy>(entity, res.energy);
        activeCount++;
    }
    if (res.metal > 0.f){
        EcsMain::registry.emplace<TMetal>(entity, res.metal);
        activeCount++;
    }
    return activeCount;
}

template<class TEnergy, class TMetal>
int removeResComponents(entt::entity entity, SResourcePack& adjustment) {
    int activeCount = 0;

    if (EcsMain::registry.all_of<TEnergy>(entity)) {
        const auto& energy = EcsMain::registry.get<TEnergy>(entity);
        adjustment.energy = energy.value;
        activeCount++;
        EcsMain::registry.remove<TEnergy>(entity);
    }
    if (EcsMain::registry.all_of<TMetal>(entity)) {
        const auto& metal = EcsMain::registry.get<TMetal>(entity);
        adjustment.metal = metal.value;
        activeCount++;
        EcsMain::registry.remove<TMetal>(entity);
    }

    return activeCount;
}

void FlowEconomySystem::UpdateUnitProratableResourceUse(CUnit *unit, SResourcePack res) {

    if (! EcsMain::registry.valid(unit->entityReference)){
        LOG("%s: invalid entityId reference for unit id: %d", __func__, unit->id);
        return;
    }

    if (res.energy < 0.f){
        UpdateUnitProratableEnergyCreation(unit, -res.energy);
        res.energy = 0.f;
    }
    if (res.metal < 0.f){
        UpdateUnitProratableMetalCreation(unit, -res.metal);
        res.metal = 0.f;
    }

    // remove resource request
    {
        SResourcePack adjustment;
        int activeCount = removeResComponents<EnergyProratableUse, MetalProratableUse>(unit->entityReference, adjustment);
        bool isDependent = (activeCount == SResourcePack::MAX_RESOURCES);

        if (isDependent){
            teamHandler.Team(unit->team)->predDependentProratableExpense -= adjustment;
        }
        else {
            teamHandler.Team(unit->team)->predIndependentProratableExpense -= adjustment;
        }
    }

    // Add resource request
    {
        int activeCount = createResComponents<EnergyProratableUse, MetalProratableUse>(unit->entityReference, res);
        bool isDependent = (activeCount == SResourcePack::MAX_RESOURCES);

        if (isDependent){
            teamHandler.Team(unit->team)->predDependentProratableExpense += res;
        }
        else {
            teamHandler.Team(unit->team)->predIndependentProratableExpense += res;
        }
    }
}


template<class T>
void UpdateUnitEconomy(entt::entity entity, float amount, float &valueToUpdate){
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__);
        return;
    }

    const auto energy = EcsMain::registry.try_get<T>(entity);
    if (energy != nullptr) {
        float newIncome = valueToUpdate - energy->value;
        valueToUpdate = std::max(0.f, newIncome);

        if (amount <= 0.f){
            EcsMain::registry.remove<T>(entity);
            return;
        }
    }

    if (amount > 0.f){
        EcsMain::registry.emplace_or_replace<T>(entity, amount);
        valueToUpdate += amount;
    }
}

void FlowEconomySystem::UpdateUnitProratableEnergyCreation(CUnit *unit, float amount){
    LOG("%s: %d: income = %f", __func__, gs->frameNum, amount);
    auto team = teamHandler.Team(unit->team);
    UpdateUnitEconomy<EnergyProratableIncome>(unit->entityReference, amount, team->predProratableIncome.energy);
}

void FlowEconomySystem::UpdateUnitProratableMetalCreation(CUnit *unit, float amount){
    LOG("%s: %d: income = %f", __func__, gs->frameNum, amount);
    auto team = teamHandler.Team(unit->team);
    UpdateUnitEconomy<MetalProratableIncome>(unit->entityReference, amount, team->predProratableIncome.metal);
}


void FlowEconomySystem::UpdateUnitFixedEnergyCreation(CUnit *unit, float amount){
    auto team = teamHandler.Team(unit->team);
    UpdateUnitEconomy<EnergyUnconditionalIncome>(unit->entityReference, amount, team->predFixedIncome.energy);
}

void FlowEconomySystem::UpdateUnitFixedEnergyCreation(entt::entity entity, float amount) {
    auto team = teamHandler.Team(EcsMain::registry.get<Units::Team>(entity).value);
    UpdateUnitEconomy<EnergyUnconditionalIncome>(entity, amount, team->predFixedIncome.energy);
}

void FlowEconomySystem::UpdateUnitFixedMetalCreation(CUnit *unit, float amount) {
    auto team = teamHandler.Team(unit->team);
    UpdateUnitEconomy<MetalUnconditionalIncome>(unit->entityReference, amount, team->predFixedIncome.metal);
}


void FlowEconomySystem::Update() {
    if (!active)
        return;
    
    // slow the eco down for debug purposes - to be removed later
    // also, accidentally commited change to stop world drawer timers - should re-enable those leter as well
    if (gs->frameNum % GAME_SPEED)
        return;

    SCOPED_TIMER("ECS::FlowEconomySystem::Update");
    // multi-thread this?
    for (int i=0; i<teamHandler.ActiveTeams(); i++){
       UpdateTeamEconomy(i);
    }
}

void FlowEconomySystem::UpdateTeamEconomy(int teamId){
    // Get available resources for proration
    CTeam* curTeam = teamHandler.Team(teamId);

    SResourcePack storage = curTeam->res;
    SResourcePack income = curTeam->resNextIncome;
    SResourcePack unconditionalUse = curTeam->predFixedExpense;
    SResourcePack independentProratableUse = curTeam->predIndependentProratableExpense;
    SResourcePack dependentProratableUse = curTeam->predDependentProratableExpense;
    SResourcePack fixedIncome = curTeam->predFixedIncome;
    SResourcePack proratableIncome = curTeam->predProratableIncome;

    // derived values
    SResourcePack resForProration = storage + income + (-unconditionalUse);
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
    SResourcePack expense = unconditionalUse + proratableUseTaken;

    // Apply economy updates - take off the cap, then apply storage cap afterwards
    curTeam->ApplyResources(income, expense);
    curTeam->resPull += expense;

    // do after all teams are updated.
    //setEachBuildRate(teamId, minProrationRate, energyProrationRate, metalProrationRate);

    // values to add to the storage next frame. This approach avoids the paradox
    // with variable income impacting variable use.
    // proratable income is dependent on the other resource - so this deliberately looks to wrong way around
    SResourcePack proratedIncomeRates(energyProrationRate, metalProrationRate);
    SResourcePack nextFrameIncome = fixedIncome + proratableIncome * proratedIncomeRates;

    curTeam->resNextIncome = nextFrameIncome;

    if (minProrationRate < 1.f){
        LOG("============================================");
        LOG("%s: %d: income = (%f,%f)", __func__, gs->frameNum, income.energy, income.metal);
        LOG("%s: %d: unconditionalUse = (%f,%f)", __func__, gs->frameNum, unconditionalUse.energy, unconditionalUse.metal);
        LOG("%s: %d: forProration = (%f,%f)", __func__, gs->frameNum, resForProration.energy, resForProration.metal);
        LOG("%s: %d: poratableUse = (%f,%f)", __func__, gs->frameNum, proratableUse.energy, proratableUse.metal);
        LOG("%s: %d: prorationrate = (%f,%f)", __func__, gs->frameNum, energyProrationRate, metalProrationRate);
        LOG("%s: %d: minProrationRate = %f", __func__, gs->frameNum, minProrationRate);
        LOG("%s: %d: resNextIncome.energy = %f", __func__, gs->frameNum, curTeam->resNextIncome.energy);
        LOG("%s: %d: resNextIncome.metal = %f", __func__, gs->frameNum, curTeam->resNextIncome.metal);
    }
}

template <class TRecalc, class TOther>
float recalculateProratableUsage(float higherRate, float lowerRate){
    auto group = EcsMain::registry.group<TRecalc, TOther>();
    float total = 0.f;

    group.each([&total, higherRate, lowerRate](const auto resToCalcUse, const auto otherResUse){
        total += resToCalcUse.value * (otherResUse.value > 0.f) ? lowerRate : higherRate;
    });

    return total;
}

template <class TRecalc, class TOther>
float recalculateProratableUsageForTeam(int teamid, float higherRate, float lowerRate){
    auto group = EcsMain::registry.group<TRecalc, TOther>(entt::get<Units::Team>);
    float total = 0.f;

    group.each([&total, higherRate, lowerRate, teamid](const auto team, const auto resToCalcUse, const auto otherResUse){
        total += resToCalcUse.value
                * (team.value == teamid)
                * (otherResUse.value > 0.f) ? lowerRate : higherRate;
    });

    return total;
}

// template<class T>
// float sumComponent() {
//     auto view = EcsMain::registry.view<T>();
//     float total = 0.f;

//     view.each([&total](const auto &comp){ total += comp.value; });

//     return total;
// }

template<class T>
float sumComponentForTeam(int teamid) {
    auto group = EcsMain::registry.group<T>(entt::get<Units::Team>);
    float total = 0.f;

    for(auto entity: group) {
        auto [comp, team] = group.get(entity);
        total += comp.value * (team.value == teamid);
    }

    return total;
}

void FlowEconomySystem::setEachBuildRate(int teamId, float minProrationRate, float energyProrationRate, float metalPropationRate){
    auto group = EcsMain::registry.group<BuildRate, EnergyProratableUse, MetalProratableUse>(entt::get<Units::Team>);
    float prorationRate[4] = {1.f, energyProrationRate, metalPropationRate, minProrationRate};

    group.each([&prorationRate, teamId](auto buildRate, const auto energyUse, const auto metalUse, const auto team){
        uint32_t chosenRate = (!!energyUse.value) + (!!metalUse.value)*2;
        float buildRates[2];
        buildRates[0] = buildRate.value;
        buildRates[1] = prorationRate[chosenRate];
        buildRate.value = buildRates[(team.value == teamId)];
    });
}
