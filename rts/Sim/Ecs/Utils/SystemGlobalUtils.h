#ifndef SYSTEM_GLOBAL_UTILS_H__
#define SYSTEM_GLOBAL_UTILS_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"

namespace SystemGlobals {

class SystemGlobal {
public:
    template<class T>
    void InitSystemComponent() {
        DestroyComponent<T>();
        
        entt::entity entity = EcsMain::registry.create();
        EcsMain::registry.emplace<T>(entity);

        typeToEntity.emplace(entt::type_id<T>().index(), entity);
    };

    template<class T>
    void SetSystemActiveState(bool activeState) {
        EcsMain::registry.emplace_or_replace<SystemActive>(GetSystemEntity<T>(), activeState);
    };

    template<class T>
    T& GetSystemComponent() { return EcsMain::registry.get<T>(GetSystemEntity<T>()); }

    template<class T>
    entt::entity GetSystemEntity() { return typeToEntity.at(entt::type_id<T>().index()); }

    template<class T>
    bool IsSystemComponentActive() {
        auto systemActiveComp = EcsMain::registry.try_get<SystemActive>(GetSystemEntity<T>());
        if (systemActiveComp != nullptr) {
            return systemActiveComp->value;
        }
        return false;
    }

    template<class T>
    void DestroyComponent() {
        auto typeId = entt::type_id<T>().index();
        if (typeToEntity.contains(typeId)) {
            auto entity = typeToEntity.at(typeId);
            if (EcsMain::registry.valid(entity)) EcsMain::registry.destroy(entity);
            typeToEntity.erase(typeId);
        }
    }

    void ClearComponents() { for (auto typeAndEntity : typeToEntity) EcsMain::registry.destroy(typeAndEntity.second); }

private:

    entt::dense_map<ENTT_ID_TYPE, entt::entity> typeToEntity;
};

extern SystemGlobal systemGlobals;

}

#endif