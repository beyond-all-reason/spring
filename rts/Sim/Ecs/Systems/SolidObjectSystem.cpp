#include "SolidObjectSystem.h"

#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Objects/SolidObject.h"

SolidObjectSystem solidObjectSystem;

using namespace SolidObject;

void SolidObjectSystem::Init() {

}

void SolidObjectSystem::Update() {

}

void SolidObjectSystem::AddObject(CSolidObject* object) {
    if (object->entityReference == entt::null) {
        object->entityReference = EcsMain::registry.create();
        auto entity = object->entityReference;

        EcsMain::registry.emplace_or_replace<Health>(entity, 0.f);
        EcsMain::registry.emplace_or_replace<MaxHealth>(entity, 0.f);

        LOG("%s: added solid object %d (%d)", __func__, object->id, (int)entity);
    }
}

void SolidObjectSystem::RemoveObject(CSolidObject* object) {
    if (EcsMain::registry.valid(object->entityReference)){
        EcsMain::registry.destroy(object->entityReference);
    }
    object->entityReference = entt::null;
}