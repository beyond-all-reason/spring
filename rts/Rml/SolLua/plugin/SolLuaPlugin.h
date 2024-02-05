// This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

// This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

// RmlSolLua.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "Rml/SolLua/TranslationTable.h"
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/Plugin.h>
#include <RmlUi/Lua/Header.h>

#include "lib/lua/mask_lua_macros.h"
#include <sol2/sol.hpp>
#include "lib/lua/restore_lua_macros.h"

#include <memory>


namespace Rml::SolLua
{

    class SolLuaDocumentElementInstancer;
    class SolLuaEventListenerInstancer;

    class RMLUILUA_API SolLuaPlugin : public Plugin
    {
    public:
        SolLuaPlugin(sol::state_view lua_state);
        SolLuaPlugin(sol::state_view lua_state, const Rml::String& lua_environment_identifier);

        void RemoveLuaItems();
        void AddContextTracking(Context* context);
        void AddDocumentTracking(ElementDocument* document);

        TranslationTable translationTable;
    private:
        int GetEventClasses() override;

        void OnInitialise() override;
        void OnShutdown() override;

        void OnContextDestroy(Context* context) override;
        void OnDocumentUnload(ElementDocument* document) override;

        std::unique_ptr<SolLuaDocumentElementInstancer> document_element_instancer;
        std::unique_ptr<SolLuaEventListenerInstancer> event_listener_instancer;

        sol::state_view m_lua_state;
        Rml::String m_lua_env_identifier;

        std::vector<Context*> luaContexts;
        std::vector<ElementDocument*> luaDocuments;
    };

} // end namespace Rml::SolLua
