#ifndef BUILD_SYSTEM_H__
#define BUILD_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"

class CBuilder;
class CUnit;

class BuildSystem {

public:
    void Init();
    void Update();

    void AddUnitBuilder(CBuilder *unit);

    void AddUnitBuildTarget(CUnit *unit, CUnit *target);
    void RemoveUnitBuild(entt::entity entity);
    void RemovetUnitBuilder(CUnit *unit);

    void PauseBuilder(CUnit *unit);
    void UnpauseBuilder(CUnit *unit);

    bool UnitBeingBuilt(entt::entity entity);
    bool UnitBuildComplete(entt::entity entity);

    entt::entity GetUnitBuildTarget(CUnit *unit);
    void UpdateBuildPower(CUnit *unit, float power);

    bool IsSystemActive() { return active; }

private:
    bool active = true;

    void InitUnitBuildTarget(entt::entity);
};

extern BuildSystem buildSystem;

#endif