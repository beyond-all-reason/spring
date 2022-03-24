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
    auto entity = object->entityReference;
    if (entity == entt::null) {
        LOG("%s: error, object %d has no entity id (%d)", __func__, object->id, (int)entity);
    }

    EcsMain::registry.emplace_or_replace<Health>(entity, 0.f);
    EcsMain::registry.emplace_or_replace<MaxHealth>(entity, 0.f);

    LOG("%s: added solid object %d (%d)", __func__, object->id, (int)entity);
}

void SolidObjectSystem::RemoveObject(CSolidObject* object) {
    if (EcsMain::registry.valid(object->entityReference)){
        EcsMain::registry.destroy(object->entityReference);
    }
    object->entityReference = entt::null;
}