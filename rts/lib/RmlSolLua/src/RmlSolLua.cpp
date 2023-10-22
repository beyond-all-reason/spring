#include "RmlSolLua.h"

#include <sol2/sol.hpp>
#include <RmlUi/Core.h>

#include "bind/bind.h"


namespace Rml::SolLua
{

    SolLuaPlugin* Initialise(sol::state_view* state)
    {
        SolLuaPlugin* slp;
        if (state != nullptr)
        {
            slp = new SolLuaPlugin(*state);
            ::Rml::RegisterPlugin(slp);
            RegisterLua(state, slp);
        }
        return slp;
    }

    SolLuaPlugin* Initialise(sol::state_view* state, const Rml::String& lua_environment_identifier)
    {
        SolLuaPlugin* slp;
        if (state != nullptr)
        {
            slp = new SolLuaPlugin(*state, lua_environment_identifier);
            ::Rml::RegisterPlugin(slp);
            RegisterLua(state, slp);
        }
        return slp;
    }

    SolLuaPlugin* Initialize(sol::state_view* state)
    {
        return Initialise(state);
    }

    SolLuaPlugin* Initialize(sol::state_view* state, const Rml::String& lua_environment_identifier)
    {
        return Initialise(state, lua_environment_identifier);
    }

    void RegisterLua(sol::state_view* state, SolLuaPlugin* slp)
    {
        bind_color(*state);
        bind_context(*state);
        bind_datamodel(*state);
        bind_element(*state);
        bind_element_derived(*state);
        bind_element_form(*state);
        bind_document(*state);
        bind_event(*state);
        bind_global(*state, &slp->translationTable);
        bind_log(*state);
        bind_vector(*state);
        bind_convert(*state);
    }

} // end namespace Rml::SolLua
