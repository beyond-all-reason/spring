#ifndef BUILD_UTILS_H__
#define BUILD_UTILS_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/BuildComponents.h"

class CBuilder;
class CUnit;

class BuildUtils {

public:
    static void AddUnitBuilder(CUnit *unit);

    static void AddUnitBuildTarget(entt::entity unit, entt::entity target);
    static void AddUnitRepairTarget(entt::entity unit, entt::entity target);

    static void RemoveUnitBuild(entt::entity entity);
    static void RemoveUnitBuilder(CUnit *unit);

    static void PauseBuilder(CUnit *unit);
    static void UnpauseBuilder(CUnit *unit);

    static bool UnitBeingBuilt(entt::entity entity) {
        return (EcsMain::registry.all_of<Build::BeingBuilt>(entity)); }

    static bool UnitBuildComplete(entt::entity entity) {
        return (EcsMain::registry.get<Build::BuildProgress>(entity).value >= 1.f); }

    static entt::entity GetUnitBuildTarget(CUnit *unit);
    static void SetBuildPower(entt::entity entity, float power);
    static void SetRepairPower(entt::entity entity, float power);

    static const float GetBuildSpeed(entt::entity entity) { return EcsMain::registry.get<Build::BuildSpeed>(entity).value; }
    static float& GetBuildProgress(entt::entity entity) {
        return EcsMain::registry.get_or_emplace<Build::BuildProgress>(entity, 1.f).value;
    }

    static void AddUnitBeingBuilt(CUnit *unit);
    static void AddUnitBeingRepaired(CUnit *unit);
};

#endif