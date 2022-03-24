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

class EcsMain {
public:
    static entt::registry registry;
};

// (auto defaultValue) requires -fconcepts-ts or c++20
template<class T>
auto GetOptionalComponent(entt::entity entity, float defaultValue) {
    auto checkPtr = EcsMain::registry.try_get<T>(entity);
    return checkPtr != nullptr ? checkPtr->value : defaultValue;
}

#ifdef RESTORE_LUA_MACROS
// from llimits.h
#define cast(t, exp)	((t)(exp))
// from lstate.h
#define registry(L)	(&G(L)->l_registry)
#undef RESTORE_LUA_MACROS
#endif



#endif