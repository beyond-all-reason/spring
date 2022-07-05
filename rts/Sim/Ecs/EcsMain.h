#ifndef ECS_MAIN_H__
#define ECS_MAIN_H__

/* Note that GCC 10.3 is unable to compile the following template (though VC++2019 can)
template<class T>
void TestGroup() {
    auto view = registry.view<T>();
    for (auto entity : view) {
        auto comp = view.get<T>(entity);
    }
}

You get error with GCC 10.3:
error: expected primary-expression before '>' token
      auto comp = view.get<T>(entity);
*/

// lua llimits.h and lstate.h makes some macros that breaks EnTT
#ifdef cast
#define RESTORE_LUA_MACROS
#undef cast
#undef registry
#endif

#include "lib/entt/entt.hpp"

#define GET_VERSION_FROM_ID(x) (x&0xfff00000)
#define GET_ENTITY_FROM_ID(x) (x&0xfffff)

class EcsMain {
public:
    static entt::registry registry;
};

template<typename TF, typename... TR>
void AddComponentsIfNotExist(entt::entity entity) {
    if (!EcsMain::registry.all_of<TF>(entity)){
        EcsMain::registry.emplace<TF>(entity);
    }
    if constexpr (sizeof...(TR) > 0) {
        AddComponentsIfNotExist<TR...>(entity);
    }
}

// (auto defaultValue) requires -fconcepts-ts or c++20
template<class T, class V>
auto& GetOptionalComponent(entt::entity entity, V& defaultValue) {
    auto checkPtr = EcsMain::registry.try_get<T>(entity);
    return checkPtr != nullptr ? *checkPtr : defaultValue;
}

template<typename T>
class ReleaseComponentOnExit {
public:
    ReleaseComponentOnExit(entt::entity scopedEntity): entity(scopedEntity) {}
    ~ReleaseComponentOnExit() { EcsMain::registry.remove<T>(entity); }
private:
    entt::entity entity;
};

template<class T, typename V>
void TryAddToComponent(entt::entity entity, V addition) {
    auto comp = EcsMain::registry.try_get<T>(entity);
    if (comp != nullptr)
        *comp += addition;
}

#ifdef RESTORE_LUA_MACROS
// from llimits.h
#define cast(t, exp)	((t)(exp))
// from lstate.h
#define registry(L)	(&G(L)->l_registry)
#undef RESTORE_LUA_MACROS
#endif



#endif