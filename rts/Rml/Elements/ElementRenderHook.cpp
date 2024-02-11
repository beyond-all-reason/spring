// This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

//
// Created by ChrisFloofyKitsune on 2/9/2024.
//

#include "ElementRenderHook.h"

namespace RmlGui {

ElementRenderHook::ElementRenderHook(const Rml::String& tag): Element(tag)
{
}

bool ElementRenderHook::GetIntrinsicDimensions(Rml::Vector2f& dimensions, float& ratio)
{
	dimensions_scale = Rml::ElementUtilities::GetDensityIndependentPixelRatio(this);

	intrinsic_dimensions.x = GetAttribute<float>("width", -1);
	intrinsic_dimensions.y = GetAttribute<float>("height", -1);

	intrinsic_dimensions *= dimensions_scale;

	dimensions = intrinsic_dimensions;
	ratio = dimensions.x / dimensions.y;
	return true;
}
void ElementRenderHook::OnAttributeChange(const Rml::ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	if (changed_attributes.find("width") != changed_attributes.end() ||
		changed_attributes.find("height") != changed_attributes.end()) {
		DirtyLayout();
	}
}
void ElementRenderHook::OnResize()
{
	actual_dimensions = GetBox().GetSize(Rml::BoxArea::Content);
}

void ElementRenderHook::OnDpRatioChange()
{
	DirtyLayout();
}

void ElementRenderHook::OnRender()
{
	auto pos = GetAbsoluteOffset(Rml::BoxArea::Content).Round();
	Rml::Dictionary eventParams;
	eventParams["x"] = pos.x;
	eventParams["y"] = pos.y;
	eventParams["width"] = actual_dimensions.x;
	eventParams["height"] = actual_dimensions.y;

	DispatchEvent("render", eventParams, true, false);
}
} // RmlGui