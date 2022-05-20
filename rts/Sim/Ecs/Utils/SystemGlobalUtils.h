#ifndef SYSTEM_GLOBAL_UTILS_H__
#define SYSTEM_GLOBAL_UTILS_H__

#include "Sim/Ecs/EcsMain.h"

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
    T& GetSystemComponent() { return EcsMain::registry.get<T>(typeToEntity.at(entt::type_id<T>().index())); }

    template<class T>
    void DestroyComponent() {
        auto typeId = entt::type_id<T>().index();
        if (typeToEntity.contains(typeId)) {
            auto entity = typeToEntity.at(typeId);
            if (EcsMain::registry.valid(entity)) EcsMain::registry.destroy(entity);
        }
    }

    void ClearComponents() { for (auto typeAndEntity : typeToEntity) EcsMain::registry.destroy(typeAndEntity.second); }

private:

    entt::dense_map<ENTT_ID_TYPE, entt::entity> typeToEntity;
};

extern SystemGlobal systemGlobals;

}

#endif