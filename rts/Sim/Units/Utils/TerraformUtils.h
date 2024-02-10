#ifndef TERRAFORM_UTILS_H__
#define TERRAFORM_UTILS_H__

#include "Sim/Ecs/Registry.h"
#include "Sim/Units/Components/UnitComponents.h"

class CBuilder;

void AddBuilderToTerraformTask(entt::entity terraformTaskEntity, CBuilder* builderToAdd) {
    auto* terraformTask = Sim::registry.try_get<Unit::TerraformBuildTask>(terraformTaskEntity);
    assert (terraformTask != nullptr);
}

void CreateTerraformTask(CBuilder* builderToAdd) {
    entt::entity terraformTaskEntity = Sim::registry.create();

    Unit::TerraformBuildTask newTask;

    Sim::registry.emplace<Unit::TerraformBuildTask>(terraformTaskEntity, newTask);
}

#endif