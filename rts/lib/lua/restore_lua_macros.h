// import after mask_lua_macros.h

#ifdef RESTORE_LUA_MACROS
// from llimits.h
#define cast(t, exp)	((t)(exp))
// from lstate.h
#define registry(L)	(&G(L)->l_registry)
#undef RESTORE_LUA_MACROS
#endif