#include "WindGeneratorHelper.h"
#include "UnitEconomyHelper.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Systems/EnvResourceSystem.h"
#include "Sim/Ecs/Helpers/UnitEconomyHelper.h"
#include "Sim/Ecs/Utils/EconomyTask.h"

#include "Sim/Units/Unit.h"

void WindGeneratorHelper::CreateWindGenerator(CUnit *unit){
    auto entity = unit->entityReference;
    if (!envResourceSystem.IsWindAboutToChange())
        EcsMain::registry.emplace<EnvEconomy::NewWindGenerator>(entity);
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
        LOG("%s: cannot activate generator unit %d because it hasn't been registered yet.", __func__, unit->id);
        return;
    }

    auto unitDefRef = EcsMain::registry.get<Units::UnitDefRef>(entity).value;
    EcsMain::registry.emplace_or_replace<EnvEconomy::WindGeneratorActive>(entity);

    auto economyTaskComp = EcsMain::registry.try_get<EnvEconomy::WindEconomyTaskRef>(entity);
    if (economyTaskComp == nullptr) {
        auto taskEntity = EconomyTaskUtil::CreateUnitEconomyTask(entity);
        EcsMain::registry.emplace<EnvEconomy::WindEconomyTaskRef>(entity, taskEntity);
        EcsMain::registry.emplace<EnvEconomy::WindEnergy>(taskEntity);
        EcsMain::registry.emplace<Units::UnitDefRef>(taskEntity, unitDefRef);
        UnitEconomyHelper::AddFixedEnergyIncome(taskEntity, 0.f);
    }
}

void WindGeneratorHelper::DeactivateGenerator(CUnit* unit){
    auto entity = unit->entityReference;
    if (!EcsMain::registry.valid(entity)){
        LOG("%s: cannot deactivate generator unit %d because it hasn't been registered yet.", __func__, unit->id);
        return;
    }

    EcsMain::registry.remove<EnvEconomy::WindGeneratorActive>(entity);

    auto taskEntityComp = EcsMain::registry.try_get<EnvEconomy::WindEconomyTaskRef>(entity);
    if (taskEntityComp != nullptr) {
        EconomyTaskUtil::DeleteUnitEconomyTask(taskEntityComp->value);
        EcsMain::registry.remove<EnvEconomy::WindEconomyTaskRef>(entity);
    }
}
