#pragma once

#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/EventListenerInstancer.h>

#include <sol2/sol.hpp>


namespace Rml::SolLua
{

	class SolLuaDocumentElementInstancer : public ::Rml::ElementInstancer
	{
	public:
		SolLuaDocumentElementInstancer(sol::state_view state, const Rml::String& lua_env_identifier) : m_state{ state }, m_lua_env_identifier{ lua_env_identifier } {}
		ElementPtr InstanceElement(Element* parent, const String& tag, const XMLAttributes& attributes) override;
		void ReleaseElement(Element* element) override;
	protected:
		sol::state_view m_state;
		Rml::String m_lua_env_identifier;
	};

	class SolLuaEventListenerInstancer : public ::Rml::EventListenerInstancer
	{
	public:
		SolLuaEventListenerInstancer(sol::state_view state) : m_state{ state } {}
		EventListener* InstanceEventListener(const String& value, Element* element) override;
	protected:
		sol::state_view m_state;
	};

} // end namespace Rml::SolLua
