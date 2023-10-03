#pragma once

#include <RmlUi/Core/ElementDocument.h>

#include <sol2/sol.hpp>


namespace Rml::SolLua
{
	/// <summary>
	/// Lua error handler.
	/// </summary>
	/// <param name="">(Unused) Lua state.</param>
	/// <param name="pfr">The result that holds our error.</param>
	/// <returns>The error result.</returns>
	sol::protected_function_result ErrorHandler(lua_State*, sol::protected_function_result pfr);


	class SolLuaDocument : public ::Rml::ElementDocument
	{
	public:
		/// <summary>
		/// Construct the SolLuaDocument.
		/// </summary>
		/// <param name="state">The Lua state to register into.</param>
		/// <param name="tag">The document tag (body).</param>
		SolLuaDocument(sol::state_view state, const Rml::String& tag, const Rml::String& lua_env_identifier);

		/// <summary>
		/// Loads an inline script.
		/// </summary>
		/// <param name="context">The UI context.</param>
		/// <param name="source_path">Source path.</param>
		/// <param name="source_line">Source line.</param>
		void LoadInlineScript(const Rml::String& content, const Rml::String& source_path, int source_line) override;

		/// <summary>
		/// Loads a script from a file.
		/// </summary>
		/// <param name="source_path">The file to load.</param>
		void LoadExternalScript(const Rml::String& source_path) override;

		/// <summary>
		/// Runs a piece of Lua script within the environment with our error handler.
		/// </summary>
		/// <param name="script">The script to run.</param>
		/// <returns>The result of the script.</returns>
		sol::protected_function_result RunLuaScript(const Rml::String& script);

		/// <summary>
		/// Gets the Lua environment attached to this document.
		/// </summary>
		/// <returns>A reference to the Lua environment.</returns>
		sol::environment& GetLuaEnvironment() { return m_environment; }

		/// <summary>
		/// Gets the Lua environment identifier attached to this document.
		/// </summary>
		/// <returns>A const reference to the Lua environment identifier.</returns>
		const Rml::String& GetLuaEnvironmentIdentifier() const { return m_lua_env_identifier; }

	protected:
		sol::state_view m_state;
		sol::environment m_environment;
		Rml::String m_lua_env_identifier;
	};

} // namespace Rml::SolLua
