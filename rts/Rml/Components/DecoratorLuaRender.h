// This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

//
// Created by ChrisFloofyKitsune on 2/12/2024.
//

#ifndef SPRING_DECORATORLUARENDER_H
#define SPRING_DECORATORLUARENDER_H

#include <lib/lua/mask_lua_macros.h>
#include "sol2/sol.hpp"
#include <lib/lua/restore_lua_macros.h>

#include <RmlUi/Core.h>

namespace RmlGui
{

class DecoratorLuaRender : public Rml::Decorator
{
public:
	explicit DecoratorLuaRender(std::string render_callback_ident);
	~DecoratorLuaRender() override = default;
	Rml::DecoratorDataHandle GenerateElementData(Rml::Element* element) const override;
	void ReleaseElementData(Rml::DecoratorDataHandle element_data) const override;
	void RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const override;

protected:
	const std::string render_callback_ident;
	bool TryCallback(Rml::Element* element, const std::string& callback, const sol::environment& env, bool try_widget = true) const;
};

class DecoratorLuaRenderInstancer : public Rml::DecoratorInstancer {
public:
	DecoratorLuaRenderInstancer();
	Rml::SharedPtr<Rml::Decorator>
	InstanceDecorator(const Rml::String& name, const Rml::PropertyDictionary& properties,
	                  const Rml::DecoratorInstancerInterface& instancer_interface) override;
protected:
	Rml::PropertyId id_render_callback;
};

}  // namespace RmlGui

#endif  // SPRING_DECORATORLUARENDER_H
