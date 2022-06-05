#ifndef SOLID_OBJECT_SYSTEM_H__
#define SOLID_OBJECT_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"

#include "System/Log/ILog.h"

class CSolidObject;

class SolidObjectUtils {
public:
    static void AddObject(CSolidObject* object);
    static void RemoveObject(CSolidObject* object);

    static float& ObjectHealth(entt::entity entity) { return EcsMain::registry.get<SolidObject::Health>(entity).value; }
    static float& ObjectMaxHealth(entt::entity entity) { return EcsMain::registry.get<SolidObject::MaxHealth>(entity).value; }
};

#endif