#ifndef BUILD_UTILS_H__
#define BUILD_UTILS_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/BuildComponents.h"

class CBuilder;
class CUnit;

class BuildUtils {

public:
    static void AddUnitBuilder(CUnit *unit);

    static void AddUnitBuildTarget(CUnit *unit, CUnit *target);
    static void RemoveUnitBuild(entt::entity entity);
    static void RemoveUnitBuilder(CUnit *unit);

    static void PauseBuilder(CUnit *unit);
    static void UnpauseBuilder(CUnit *unit);

    static bool UnitBeingBuilt(entt::entity entity);
    static bool UnitBuildComplete(entt::entity entity);

    static entt::entity GetUnitBuildTarget(CUnit *unit);
    static void SetBuildPower(entt::entity entity, float power);

    static bool IsSystemActive();

    static const float GetBuildSpeed(entt::entity entity) { return EcsMain::registry.get<Build::BuildPower>(entity).value; }
    static float& GetBuildProgress(entt::entity entity) { return EcsMain::registry.get<Build::BuildProgress>(entity).value; }

    static float GetBuildOptionalProgress(entt::entity entity) {
        auto comp = EcsMain::registry.try_get<Build::BuildProgress>(entity);
        return (comp != nullptr) ? comp->value : 1.f; }

    static void AddUnitBeingBuilt(CUnit *unit);
};

#endif