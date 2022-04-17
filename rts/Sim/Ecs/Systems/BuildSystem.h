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
    void UpdateBuildPower(CUnit *unit, float power);

    bool IsSystemActive() { return active; }

    float& GetBuildSpeed(entt::entity entity) { return EcsMain::registry.get<Build::BuildPower>(entity).value; }
    float& GetBuildProgress(entt::entity entity) { return EcsMain::registry.get<Build::BuildProgress>(entity).value; }

    float GetBuildOptionalProgress(entt::entity entity) { return GetOptionalComponent<Build::BuildProgress>(entity, 1.f); }

    void AddUnitBeingBuilt(CUnit *unit);

private:
    bool active = true;
};

extern BuildSystem buildSystem;

#endif