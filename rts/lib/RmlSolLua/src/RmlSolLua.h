#pragma once

#include <RmlUi/Core.h>
#include "plugin/SolLuaPlugin.h"

#ifdef RMLUILUA_API
    #undef RMLUILUA_API
#endif

#if !defined RMLUI_STATIC_LIB
    #ifdef RMLUI_PLATFORM_WIN32
        #if defined RmlLua_EXPORTS
            #define RMLUILUA_API __declspec(dllexport)
        #else
            #define RMLUILUA_API __declspec(dllimport)
        #endif
    #else
        #define RMLUILUA_API __attribute__((visibility("default")))
    #endif
#else
    #define RMLUILUA_API
#endif


namespace sol
{
    class state_view;
}

namespace Rml::SolLua
{
    /// <summary>
    /// Initializes RmlSolLua using the supplied Lua state.
    /// </summary>
    /// <param name="state">The Lua state to initialize into.</param>
    RMLUILUA_API SolLuaPlugin* Initialise(sol::state_view* state);

    /// <summary>
    /// Initializes RmlSolLua using the supplied Lua state.
    /// Sets the Lua variable specified by lua_environment_identifier to the document's id when running Lua code.
    /// </summary>
    /// <param name="state">The Lua state to initialize into.</param>
    /// <param name="lua_environment_identifier">The Lua variable name that is set to the document's id.</param>
    RMLUILUA_API SolLuaPlugin* Initialise(sol::state_view* state, const Rml::String& lua_environment_identifier);

    /// <summary>
    /// Initializes RmlSolLua using the supplied Lua state.
    /// </summary>
    /// <param name="state">The Lua state to initialize into.</param>
    RMLUILUA_API SolLuaPlugin* Initialize(sol::state_view* state);

    /// <summary>
    /// Initializes RmlSolLua using the supplied Lua state.
    /// Sets the Lua variable specified by lua_environment_identifier to the document's id when running Lua code.
    /// </summary>
    /// <param name="state">The Lua state to initialize into.</param>
    /// <param name="lua_environment_identifier">The Lua variable name that is set to the document's id.</param>
    RMLUILUA_API SolLuaPlugin* Initialize(sol::state_view* state, const Rml::String& lua_environment_identifier);

    /// <summary>
    /// Registers RmlSolLua into the specified Lua state.
    /// </summary>
    /// <param name="state">The Lua state to register into.</param>
    RMLUILUA_API void RegisterLua(sol::state_view* state, SolLuaPlugin* slp);

} // end namespace Rml::SolLua
