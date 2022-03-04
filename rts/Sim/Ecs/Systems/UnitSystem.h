#ifndef UNIT_SYSTEM_H__
#define UNIT_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Ecs/Components/UnitComponents.h"

class UnitSystem {
public:
    void Init();
    void Update();
    void AddUnit(CUnit* projectile);
    void RemoveUnit(CUnit* projectile);

    float& UnitHealth(entt::entity entity) { return EcsMain::registry.get<SolidObject::Health>(entity).value; }
    float& UnitMaxHealth(entt::entity entity) { return EcsMain::registry.get<SolidObject::MaxHealth>(entity).value; }

private:

};

extern UnitSystem unitSystem;

#endif