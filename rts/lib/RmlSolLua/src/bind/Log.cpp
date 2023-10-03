#include "bind.h"


namespace Rml::SolLua
{

	void bind_log(sol::state_view& lua)
	{
		lua.new_enum<Rml::Log::Type>("RmlLogType",
			{
				{ "always", Rml::Log::LT_ALWAYS },
				{ "error", Rml::Log::LT_ERROR },
				{ "warning", Rml::Log::LT_WARNING },
				{ "info", Rml::Log::LT_INFO },
				{ "debug", Rml::Log::LT_DEBUG }
			}
		);

		// Log.logtype.always
		// Log.Message(Log.logtype.error, "This is an error.")
		auto log = lua.create_named_table("Log");
		log["logtype"] = lua["RmlLogType"];
		log.set_function("Message", [](Rml::Log::Type type, const std::string& message) { Log::Message(type, "%s", message.c_str()); });

		// print("Text")
		lua.set_function("print", [](const std::string& message) { Log::Message(Rml::Log::LT_INFO, "%s", message.c_str()); });
	}

} // end namespace Rml::SolLua
