#ifndef SYSTEM_GLOBAL_UTILS_H__
#define SYSTEM_GLOBAL_UTILS_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "System/creg/creg.h"

namespace SystemGlobals {

//  undefined reference to `SystemGlobals::SystemGlobal::creg_class'
class SystemGlobal {
public:
    CR_DECLARE(SystemGlobal)

    template<class T>
    void CreateSystemComponent() {
        if (! EcsMain::registry.valid(systemGlobalsEntity))
            systemGlobalsEntity = EcsMain::registry.create();

        EcsMain::registry.emplace_or_replace<T>(systemGlobalsEntity);
    };

    template<class T>
    T& GetSystemComponent() { return EcsMain::registry.get<T>(systemGlobalsEntity); }

    template<class T>
    bool IsSystemActive() { return (nullptr != EcsMain::registry.try_get<T>(systemGlobalsEntity)); }

    void ClearComponents() {
        if (EcsMain::registry.valid(systemGlobalsEntity))
            EcsMain::registry.destroy(systemGlobalsEntity);

        systemGlobalsEntity = entt::null;
    }

private:

    entt::entity systemGlobalsEntity = entt::null;

// public:
//     template<class T>
//     void CreateSystemComponent() {
//         DestroySystemComponent<T>();
        
//         entt::entity entity = EcsMain::registry.create();
//         EcsMain::registry.emplace<T>(entity);

//         typeToEntity.emplace(entt::type_id<T>().index(), entity);
//     };

//     template<class T>
//     void SetSystemActiveState(bool activeState) {
//         EcsMain::registry.emplace_or_replace<SystemActive>(GetSystemEntity<T>(), activeState);
//     };

//     template<class T>
//     T& GetSystemComponent() { return EcsMain::registry.get<T>(GetSystemEntity<T>()); }

//     template<class T>
//     entt::entity GetSystemEntity() {
//         auto typeId = entt::type_id<T>().index();
//         if (typeToEntity.contains(typeId)) {
//             return typeToEntity.at(typeId);
//         }
//         return entt::null;
//     }

//     template<class T>
//     bool IsSystemActive() {
//         auto systemActiveComp = EcsMain::registry.try_get<SystemActive>(GetSystemEntity<T>());
//         if (systemActiveComp != nullptr) {
//             return systemActiveComp->value;
//         }
//         return false;
//     }

//     template<class T>
//     void DestroySystemComponent() {
//         auto typeId = entt::type_id<T>().index();
//         if (typeToEntity.contains(typeId)) {
//             auto entity = typeToEntity.at(typeId);
//             if (EcsMain::registry.valid(entity)) EcsMain::registry.destroy(entity);
//             typeToEntity.erase(typeId);
//         }
//     }

//     void ClearComponents() { for (auto typeAndEntity : typeToEntity) EcsMain::registry.destroy(typeAndEntity.second); }

// private:

//     entt::dense_map<ENTT_ID_TYPE, entt::entity> typeToEntity;
};

extern SystemGlobal systemGlobals;

}

#endif