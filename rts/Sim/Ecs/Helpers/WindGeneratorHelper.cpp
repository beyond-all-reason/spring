#include "WindGeneratorHelper.h"
#include "UnitEconomyHelper.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Ecs/Systems/EnvResourceSystem.h"
#include "Sim/Ecs/Helpers/UnitEconomyHelper.h"

#include "Sim/Units/Unit.h"

void WindGeneratorHelper::CreateWindGenerator(CUnit *unit){
    auto entity = unit->entityReference;
    if (!envResourceSystem.IsWindAboutToChange())
        EcsMain::registry.emplace<EnvEconomy::NewWindGenerator>(entity);
    EcsMain::registry.emplace<EnvEconomy::WindEnergy>(entity);
    EcsMain::registry.emplace<EnvEconomy::WindGenerator>(entity);
}

void WindGeneratorHelper::RemoveWindGenerator(CUnit *unit){
    auto entity = unit->entityReference;
    DeactivateGenerator(unit);
    EcsMain::registry.remove<EnvEconomy::NewWindGenerator>(entity);
    EcsMain::registry.remove<EnvEconomy::WindGenerator>(entity);
}

void WindGeneratorHelper::ActivateGenerator(CUnit* unit){
    auto entity = unit->entityReference;
    if (!EcsMain::registry.valid(entity)){
        LOG("%s: cannot add generator unit to %d because it hasn't been registered yet.", __func__, unit->id);
        return;
    }

    EcsMain::registry.emplace_or_replace<EnvEconomy::WindEnergy>(entity);
    EcsMain::registry.emplace_or_replace<EnvEconomy::WindGeneratorActive>(entity);
    UnitEconomyHelper::UpdateUnitFixedEnergyIncome(unit, 1.f);
    EcsMain::registry.get<FlowEconomy::EnergyFixedIncome>(entity).value = 0.f;
}

void WindGeneratorHelper::DeactivateGenerator(CUnit* unit){
    auto entity = unit->entityReference;
    if (!EcsMain::registry.valid(entity)){
        LOG("%s: cannot add generator unit to %d because it hasn't been registered yet.", __func__, unit->id);
        return;
    }

    auto unitIncome = EcsMain::registry.get<FlowEconomy::EnergyFixedIncome>(entity).value;
    auto windIncome = EcsMain::registry.get<EnvEconomy::WindEnergy>(entity).value;

    UnitEconomyHelper::UpdateUnitFixedEnergyIncome(unit, unitIncome - windIncome);
    EcsMain::registry.remove<EnvEconomy::WindEnergy>(entity);
    EcsMain::registry.remove<EnvEconomy::WindGeneratorActive>(entity);
}
