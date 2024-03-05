/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TERRAFORM_UTILS_H__
#define TERRAFORM_UTILS_H__

#include "Sim/Ecs/Registry.h"
#include "Sim/Units/Components/UnitComponents.h"
#include "Sim/Units/Unit.h"

void SetTerraformBuildPower(const CUnit* builder, float power) {
    Sim::registry.emplace_or_replace<Unit::TerraformTaskBuildPower>(builder->entityReference, power);
}

void ClearTerraformBuildPower(const CUnit* builder) {
    Sim::registry.remove<Unit::TerraformTaskBuildPower>(builder->entityReference);
}



void AddBuilderToTerraformTask(entt::entity terraformTaskEntity, const CUnit* builderToAdd) {
    assert(Sim::registry.valid(terraformTaskEntity));

    Sim::doubleLinkedListUtils.InsertChain<Unit::TerraformTaskChain>(terraformTaskEntity, builderToAdd->entityReference);
    Sim::registry.emplace_or_replace<Unit::TerraformTaskReference>(builderToAdd->entityReference, terraformTaskEntity);
}

void RemoveBuilderFromTerraformTask(const CUnit* builder) {
    Sim::doubleLinkedListUtils.RemoveChain<Unit::TerraformTaskChain>(builder->entityReference);
    Sim::registry.remove<Unit::TerraformTaskReference>(builder->entityReference);
}



void CreateTerraformRestoreTask(Unit::TerraformBuildTask& newTask, const CUnit* builderToAdd) {
    entt::entity terraformTaskEntity = Sim::registry.create();
    Sim::registry.emplace_or_replace<Unit::TerraformBuildTask>(terraformTaskEntity, newTask);

    AddBuilderToTerraformTask(terraformTaskEntity, builderToAdd);
}

void CreateTerraformFlattenTask(Unit::TerraformBuildTask& newTask, const CUnit* buildee, const CUnit* builderToAdd) {
    entt::entity terraformTaskEntity = Sim::registry.create();

    newTask.buildee = buildee->entityReference;
    Sim::registry.emplace_or_replace<Unit::TerraformTaskReference>(buildee->entityReference, terraformTaskEntity);

    Sim::registry.emplace_or_replace<Unit::TerraformBuildTask>(terraformTaskEntity, newTask);
    AddBuilderToTerraformTask(terraformTaskEntity, builderToAdd);
}



bool IsTerraformDone(const CUnit* builder) {
    return !Sim::registry.all_of<Unit::TerraformTaskChain>(builder->entityReference);
}

bool HasTerraformTask(const CUnit* unit) {
    return Sim::registry.all_of<Unit::TerraformTaskReference>(unit->entityReference);
}

entt::entity GetTerraformTask(const CUnit* unit) {
    return HasTerraformTask(unit)
            ? Sim::registry.get<Unit::TerraformTaskReference>(unit->entityReference).value
            : entt::null;
}

#endif