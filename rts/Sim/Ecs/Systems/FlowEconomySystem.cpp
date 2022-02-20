#include "FlowEconomySystem.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/EcsMain.h"

#include "Sim/Misc/TeamHandler.h"
#include "System/TimeProfiler.h"
#include "Sim/Units/Unit.h"
#include "System/Threading/ThreadPool.h"

FlowEconomySystem flowEconomySystem;

using namespace FlowEconomy;

void FlowEconomySystem::Init() {

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

void FlowEconomySystem::Update() {
    SCOPED_TIMER("ECS::FlowEconomySystem::Update");
    // multi-thread this?
    for (int i=0; i<teamHandler.ActiveTeams(); i++){
       UpdateTeamEconomy(i);
    }
}

void FlowEconomySystem::UpdateTeamEconomy(int teamId){
    // Get available resources for proration

    bool useMt = (teamHandler.Team(teamId)->GetNumUnits() > 100);

    float energyForProration;
    float metalForProration;
    float poratableEnergyUse;
    float poratableMetalUse;

    if (!useMt) {
     energyForProration
        = teamHandler.Team(teamId)->resStorage.energy
        + teamHandler.Team(teamId)->resNextIncome.energy
        + (-getUnconditionalEnergyUse(teamId));
     metalForProration
        = teamHandler.Team(teamId)->resStorage.metal
        + teamHandler.Team(teamId)->resNextIncome.metal
        + (-getUnconditionalMetalUse(teamId));

     poratableEnergyUse = getPoratableEnergyUse(teamId);
     poratableMetalUse = getPoratableMetalUse(teamId);
    }
    else
    {
    for_mt(0, 4, [this, teamId, &energyForProration, &metalForProration, &poratableEnergyUse, &poratableMetalUse](int jobId){
        if (jobId == 0)
            energyForProration
                = teamHandler.Team(teamId)->resStorage.energy
                + teamHandler.Team(teamId)->resNextIncome.energy
                + (-getUnconditionalEnergyUse(teamId));
        else if (jobId == 1)
            metalForProration
                = teamHandler.Team(teamId)->resStorage.metal
                + teamHandler.Team(teamId)->resNextIncome.metal
                + (-getUnconditionalMetalUse(teamId));
        else if (jobId == 2)
            poratableEnergyUse = getPoratableEnergyUse(teamId);
        else if (jobId == 3)
            poratableMetalUse = getPoratableMetalUse(teamId);
    });
    }

    float energyProrationrate = (poratableEnergyUse != 0.f) ? std::min(1.f, energyForProration / poratableEnergyUse) : 1.f;
    float metalPropationrate = (poratableMetalUse != 0.f) ? std::min(1.f, metalForProration / poratableMetalUse) : 1.f;

    // if one resource needs heavier proration (lower rate) than the other,
    // then the lighter strained resource may have more resources to offer.
    if (energyProrationrate > metalPropationrate){
        energyProrationrate = getRecalculatedEnergyUse(teamId, energyProrationrate, metalPropationrate);
    }
    else if (metalPropationrate > energyProrationrate){
        metalPropationrate = getRecalculatedMetalUse(teamId, energyProrationrate, metalPropationrate);
    }

    float minPropationRate = std::max(0.f, std::min(energyProrationrate, metalPropationrate));


    if (!useMt){
    setEachBuildRate(teamId, minPropationRate, energyProrationrate, metalPropationrate);

    // values to add to the storage next frame. This approach avoids the paradox
    // with variable income impacting variable use.
    teamHandler.Team(teamId)->resNextIncome.energy = getTotalEnergyIncome(teamId, metalPropationrate);
    teamHandler.Team(teamId)->resNextIncome.metal = getTotalMetalIncome(teamId, energyProrationrate);
    }
    else {
    for_mt(0, 3, [this, teamId, minPropationRate, energyProrationrate, metalPropationrate](int jobId){
        if (jobId == 0)
            setEachBuildRate(teamId, minPropationRate, energyProrationrate, metalPropationrate);
        else if (jobId == 1)
            teamHandler.Team(teamId)->resNextIncome.energy = getTotalEnergyIncome(teamId, metalPropationrate);
        else if (jobId == 2)
            teamHandler.Team(teamId)->resNextIncome.metal = getTotalMetalIncome(teamId, energyProrationrate);
    });
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

template<class T>
float sumComponent() {
    auto view = EcsMain::registry.view<T>();
    float total = 0.f;

    view.each([&total](const auto &comp){ total += comp.value; });

    return total;
}

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


// template<class T>
// void sumComponentForEachTeam(std::vector<float> &totals) {
//     auto group = EcsMain::registry.group<T>(entt::get<Units::Team>);

//     for (auto &total: totals) total = 0.f;

//     for(auto entity: group) {
//         auto [comp, team] = group.get<T, Units::Team>(entity);
//         totals[team.value] += comp.value;
//     }
// }

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

float FlowEconomySystem::getRecalculatedEnergyUse(int teamId, float energyRate, float metalRate){
    return recalculateProratableUsageForTeam<EnergyProratableUse, MetalProratableUse>(teamId, energyRate, metalRate);
}

float FlowEconomySystem::getRecalculatedMetalUse(int teamId, float energyRate, float metalRate){
    return recalculateProratableUsageForTeam<MetalProratableUse, EnergyProratableUse>(teamId, metalRate, energyRate);
}

float FlowEconomySystem::getUnconditionalEnergyUse(int teamId){
    return sumComponentForTeam<EnergyUnconditionalUse>(teamId);
}

float FlowEconomySystem::getUnconditionalMetalUse(int teamId){
    return sumComponentForTeam<MetalUnconditionalUse>(teamId);
}

float FlowEconomySystem::getPoratableEnergyUse(int teamId){
    return sumComponentForTeam<EnergyProratableUse>(teamId);
}

float FlowEconomySystem::getPoratableMetalUse(int teamId){
    return sumComponentForTeam<MetalProratableUse>(teamId);
}

float FlowEconomySystem::getTotalEnergyIncome(int teamId, float prorationRate) {
    return sumComponentForTeam<EnergyUnconditionalIncome>(teamId)
        + sumComponentForTeam<EnergyProratableIncome>(teamId)*prorationRate;
}

float FlowEconomySystem::getTotalMetalIncome(int teamId, float prorationRate) {
    return sumComponentForTeam<MetalUnconditionalIncome>(teamId)
        + sumComponentForTeam<MetalProratableIncome>(teamId)*prorationRate;
}
