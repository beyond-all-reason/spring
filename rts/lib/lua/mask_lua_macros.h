// lua llimits.h and lstate.h makes some macros that breaks EnTT and sol2
// import mask_lua_macros.h to mask them
// and then import restore_lua_macros.h afterwards
// be sure to keep the restored definitions the same as lua's when upgrading versions

#ifdef cast
#define RESTORE_LUA_MACROS
#undef cast
#undef registry
#endif
