// This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

//
// Created by ChrisFloofyKitsune on 2/12/2024.
//

#include "DecoratorLuaRender.h"

#include "Rml/SolLua/plugin/SolLuaDocument.h"
#include "System/Log/ILog.h"

namespace RmlGui
{

DecoratorLuaRender::DecoratorLuaRender(std::string render_callback_ident)
	: render_callback_ident(std::move(render_callback_ident))
{
}

Rml::DecoratorDataHandle DecoratorLuaRender::GenerateElementData(Rml::Element* element) const
{
	auto doc = dynamic_cast<Rml::SolLua::SolLuaDocument*>(element->GetOwnerDocument());
	if (doc == nullptr)
		return INVALID_DECORATORDATAHANDLE;

	return (Rml::DecoratorDataHandle) new Data{doc->GetLuaEnvironment().lua_state()};
}

void DecoratorLuaRender::ReleaseElementData(Rml::DecoratorDataHandle element_data) const
{
	auto* data = (Data*)element_data;
	delete data;
}
void DecoratorLuaRender::RenderElement(Rml::Element* element,
                                       Rml::DecoratorDataHandle element_data) const
{

	if (render_callback_ident.empty() || render_callback_ident == "none")
		return;

	auto lua_state = sol::state_view(((Data*)element_data)->L);

	if (!TryCallback(element, "widget." + render_callback_ident, lua_state) &&
	    !TryCallback(element, render_callback_ident, lua_state)) {
		LOG_L(L_WARNING, "Could not find a callback called '%s' in widget",
		      render_callback_ident.c_str());
	}
}

bool DecoratorLuaRender::TryCallback(Rml::Element* element, const std::string& callback,
                                     const sol::state_view& lua) const
{
	using maybe_func = sol::optional<sol::protected_function>;
	maybe_func func = lua[render_callback_ident];

	if (func.has_value() && func->valid()) {
		auto pfr = func->call(element);
		if (!pfr.valid()) {
			sol::error err = pfr;
			LOG_L(L_ERROR, "[RmlGui] Error in DecoratorLuaRender callback");
			LOG_L(L_ERROR, err.what());
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