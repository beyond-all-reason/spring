#include "SolLuaPlugin.h"

#include "RmlUi/Core/Context.h"
#include "SolLuaInstancer.h"
#include <RmlUi/Core.h>

#include "../bind/bind.h"
#include <algorithm>


namespace Rml::SolLua
{
    SolLuaPlugin::SolLuaPlugin(sol::state_view lua_state)
        : m_lua_state{lua_state}
    {
    }

    SolLuaPlugin::SolLuaPlugin(sol::state_view lua_state, const Rml::String& lua_environment_identifier)
        : m_lua_state{lua_state}, m_lua_env_identifier{lua_environment_identifier}
    {
    }

    int SolLuaPlugin::GetEventClasses()
    {
        return EVT_BASIC | EVT_DOCUMENT;
    }

    void SolLuaPlugin::AddContextTracking(Context* context) {
        luaContexts.emplace_back(context);
    }

    void SolLuaPlugin::OnContextDestroy(Context* context) {
        luaContexts.erase(std::remove(luaContexts.begin(), luaContexts.end(), context), luaContexts.end());
    }

    void SolLuaPlugin::AddDocumentTracking(ElementDocument* document) {
        luaDocuments.emplace_back(document);
    }

    void SolLuaPlugin::OnDocumentUnload(ElementDocument* document) {
        luaDocuments.erase(std::remove(luaDocuments.begin(), luaDocuments.end(), document), luaDocuments.end());
    }

    void SolLuaPlugin::RemoveLuaItems(){
        for(auto d: luaDocuments) {
            d->Close();
        }
        for(auto c: luaContexts) {
            Rml::RemoveContext(c->GetName());
        }
    }

    void SolLuaPlugin::OnInitialise()
    {
        document_element_instancer = std::make_unique<SolLuaDocumentElementInstancer>(m_lua_state, m_lua_env_identifier);
        event_listener_instancer = std::make_unique<SolLuaEventListenerInstancer>(m_lua_state);
        Factory::RegisterElementInstancer("body", document_element_instancer.get());
        Factory::RegisterEventListenerInstancer(event_listener_instancer.get());
    }

    void SolLuaPlugin::OnShutdown()
    {
        // m_lua_state.collect_garbage();
        delete this;
    }
} // end namespace Rml::SolLua
