#include "SolLuaDocument.h"

#include <RmlUi/Core/Stream.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Context.h>


namespace Rml::SolLua
{

	sol::protected_function_result ErrorHandler(lua_State* l, sol::protected_function_result pfr)
	{
		if (!pfr.valid())
		{
			sol::error err = pfr;
			Rml::Log::Message(Rml::Log::LT_ERROR, "[Lua] %s", err.what());
			// sol::state_view lua(l);
			// auto handle = lua["__HandleError"];
			// sol::protected_function pf = handle;
			// pf();
		}
		return pfr;
	}

	//-----------------------------------------------------

	SolLuaDocument::SolLuaDocument(sol::state_view state, const Rml::String& tag, const Rml::String& lua_env_identifier)
		: m_state(state), ElementDocument(tag), m_environment(state, sol::create, state.globals()), m_lua_env_identifier(lua_env_identifier)
	{
	}

	void SolLuaDocument::LoadInlineScript(const Rml::String& content, const Rml::String& source_path, int source_line)
	{
		auto* context = GetContext();

		Rml::String buffer{ "--" };
		buffer.append("[");
		buffer.append(context->GetName());
		buffer.append("][");
		buffer.append(GetSourceURL());
		buffer.append("]:");
		buffer.append(Rml::ToString(source_line));
		buffer.append("\n");
		buffer.append(content);

		if (!m_lua_env_identifier.empty())
			m_environment[m_lua_env_identifier] = GetId();

		m_state.safe_script(buffer, m_environment, ErrorHandler);
	}

	void SolLuaDocument::LoadExternalScript(const String& source_path)
	{
		if (!m_lua_env_identifier.empty())
			m_environment[m_lua_env_identifier] = GetId();

		m_state.safe_script_file(source_path, m_environment, ErrorHandler);
	}

	sol::protected_function_result SolLuaDocument::RunLuaScript(const Rml::String& script)
	{
		if (!m_lua_env_identifier.empty())
			m_environment[m_lua_env_identifier] = GetId();

		return m_state.safe_script(script, m_environment, ErrorHandler);
	}

} // namespace Rml::SolLua
