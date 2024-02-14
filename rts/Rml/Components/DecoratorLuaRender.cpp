// This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

//
// Created by ChrisFloofyKitsune on 2/12/2024.
//

#include "DecoratorLuaRender.h"

#include "Lua/LuaOpenGL.h"
#include "Lua/LuaUtils.h"
#include "Rml/SolLua/plugin/SolLuaDocument.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

namespace RmlGui
{

DecoratorLuaRender::DecoratorLuaRender(std::string render_callback_ident)
	: render_callback_ident(std::move(render_callback_ident))
{
}

Rml::DecoratorDataHandle DecoratorLuaRender::GenerateElementData(Rml::Element* element) const
{
	return 0;
}

void DecoratorLuaRender::ReleaseElementData(Rml::DecoratorDataHandle element_data) const
{

}
void DecoratorLuaRender::RenderElement(Rml::Element* element, Rml::DecoratorDataHandle _) const
{
	if (render_callback_ident.empty() || render_callback_ident == "none")
		return;

	auto env = dynamic_cast<Rml::SolLua::SolLuaDocument*>(element->GetOwnerDocument())->GetLuaEnvironment();
	auto L = env.lua_state();

	auto prev_drawing_enabled = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	try {
		if (!TryCallback(element, render_callback_ident, env) &&
			!TryCallback(element, render_callback_ident, env, false)) {
			LOG_L(L_WARNING, "Could not find a callback called '%s' in widget env",
				  render_callback_ident.c_str());
		}
	} catch (content_error& ex) {
		LOG_L(L_FATAL, "[%s]: %s", __func__, ex.what());
	}

	LuaOpenGL::SetDrawingEnabled(L, prev_drawing_enabled);
}

bool DecoratorLuaRender::TryCallback(Rml::Element* element, const std::string& callback,
                                     const sol::environment & env, bool try_widget) const
{
	using opt_pf = sol::optional<sol::protected_function>;
	opt_pf func;
	if (try_widget) {
		func = env.traverse_get<opt_pf>("widget", callback);
	} else {
		func = env.get<opt_pf>(callback);
	}

	if (func.has_value() && func->valid()) {
		auto pfr = func->call(element);
		if (!pfr.valid()) {
			sol::error err = pfr;
			LOG_L(L_ERROR, "[RmlGui] Error in DecoratorLuaRender callback: %s", err.what());
		}
		return true;
	}
	return false;
}

DecoratorLuaRenderInstancer::DecoratorLuaRenderInstancer()
{
	id_render_callback = RegisterProperty("render_callback", "none")
	                         .AddParser("keyword", "none")
	                         .AddParser("string")
	                         .GetId();
	RegisterShorthand("decorator", "render_callback", Rml::ShorthandType::FallThrough);
}

Rml::SharedPtr<Rml::Decorator>
DecoratorLuaRenderInstancer::InstanceDecorator(const Rml::String& name,
                                               const Rml::PropertyDictionary& properties,
                                               const Rml::DecoratorInstancerInterface& _)
{
	auto ident = properties.GetProperty(id_render_callback)->ToString();
	return Rml::MakeShared<DecoratorLuaRender>(std::move(ident));
}
}  // namespace RmlGui