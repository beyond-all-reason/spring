#ifndef ECS_MAIN_H__
#define ECS_MAIN_H__

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

#ifdef RESTORE_LUA_MACROS
// from llimits.h
#define cast(t, exp)	((t)(exp))
// from lstate.h
#define registry(L)	(&G(L)->l_registry)
#undef RESTORE_LUA_MACROS
#endif



#endif