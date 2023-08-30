/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYSTEM_GLOBAL_UTILS_H__
#define SYSTEM_GLOBAL_UTILS_H__

#include "System/Ecs/EcsMain.h"
//#include "System/Ecs/Components/SystemGlobalComponents.h"
#include "System/creg/creg.h"

namespace SystemGlobals {


class SystemGlobal {
public:
    SystemGlobal(entt::registry& registryReference)
        : registry(registryReference)
    {}

    template<class T>
    T& CreateSystemComponent() {
        if (! registry.valid(systemGlobalsEntity))
            systemGlobalsEntity = registry.create();

        return registry.emplace_or_replace<T>(systemGlobalsEntity);
    };

    template<class T>
    T& GetSystemComponent() { return registry.get<T>(systemGlobalsEntity); }

    template<class T>
    bool IsSystemActive() { return (nullptr != registry.try_get<T>(systemGlobalsEntity)); }

    void ClearComponents() {
        if (registry.valid(systemGlobalsEntity))
            registry.destroy(systemGlobalsEntity);

        systemGlobalsEntity = entt::null;
    }

    template<class Archive>
    void serialize(Archive &ar) { ar(systemGlobalsEntity); }

private:
    entt::entity systemGlobalsEntity = entt::null;
    entt::registry& registry;
};

}

#endif