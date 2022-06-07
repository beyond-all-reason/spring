#ifndef BUILD_SYSTEM_H__
#define BUILD_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/BuildComponents.h"

class CBuilder;
class CUnit;

class BuildSystem {

public:
    void Init();
    void Update();

    void AddUnitBuilder(CUnit *unit);

    void AddUnitBuildTarget(CUnit *unit, CUnit *target);
    void RemoveUnitBuild(entt::entity entity);
    void RemoveUnitBuilder(CUnit *unit);

    void PauseBuilder(CUnit *unit);
    void UnpauseBuilder(CUnit *unit);

    bool UnitBeingBuilt(entt::entity entity);
    bool UnitBuildComplete(entt::entity entity);

    entt::entity GetUnitBuildTarget(CUnit *unit);
    void SetBuildPower(entt::entity entity, float power);

    bool IsSystemActive();

    const float GetBuildSpeed(entt::entity entity) const { return EcsMain::registry.get<Build::BuildPower>(entity).value; }
    float& GetBuildProgress(entt::entity entity) { return EcsMain::registry.get<Build::BuildProgress>(entity).value; }

    float GetBuildOptionalProgress(entt::entity entity) {
        auto comp = EcsMain::registry.try_get<Build::BuildProgress>(entity);
        return (comp != nullptr) ? comp->value : 1.f; }

    void AddUnitBeingBuilt(CUnit *unit);
};

extern BuildSystem buildSystem;

#endif