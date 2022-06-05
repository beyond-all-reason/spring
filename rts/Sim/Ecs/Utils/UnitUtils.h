#ifndef UNIT_UTILS_H__
#define UNIT_UTILS_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Ecs/Components/UnitComponents.h"

class UnitUtils {
public:
    static void AddUnit(CUnit* projectile);
    static void RemoveUnit(CUnit* projectile);

    static float& UnitHealth(entt::entity entity) { return EcsMain::registry.get<SolidObject::Health>(entity).value; }
    static float& UnitMaxHealth(entt::entity entity) { return EcsMain::registry.get<SolidObject::MaxHealth>(entity).value; }
};

#endif