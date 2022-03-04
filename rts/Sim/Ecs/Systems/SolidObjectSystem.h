#ifndef SOLID_OBJECT_SYSTEM_H__
#define SOLID_OBJECT_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"

class CSolidObject;

class SolidObjectSystem {
public:
    void Init();
    void Update();
    void AddObject(CSolidObject* object);
    void RemoveObject(CSolidObject* object);

    float& ObjectHealth(entt::entity entity) { return EcsMain::registry.get<SolidObject::Health>(entity).value; }
    float& ObjectMaxHealth(entt::entity entity) { return EcsMain::registry.get<SolidObject::MaxHealth>(entity).value; }

private:

};

extern SolidObjectSystem solidObjectSystem;

#endif