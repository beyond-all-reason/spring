#pragma once

#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/EventListener.h>
#include <sol2/sol.hpp>


namespace Rml
{
    class Element;
    class ElementDocument;
}

namespace Rml::SolLua
{
    class SolLuaEventListener : public ::Rml::EventListener
    {
    public:
        SolLuaEventListener(sol::state_view& lua, const Rml::String& code, Rml::Element* element);
        SolLuaEventListener(sol::protected_function func, Rml::Element* element);

        void OnDetach(Rml::Element* element) override;
        void ProcessEvent(Rml::Event& event) override;

    private:
        sol::protected_function m_func;
        Rml::Element* m_element;
    };

} // namespace Rml::SolLua
