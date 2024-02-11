/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//
// Created by ChrisFloofyKitsune on 2/9/2024.
//

#ifndef ELEMENTRENDEREVENTEMITTER_H
#define ELEMENTRENDEREVENTEMITTER_H

#include <RmlUi/Core.h>
#include <RmlUi/Core/Element.h>

namespace RmlGui
{

class ElementRenderHook : public Rml::Element
{
public:
	explicit ElementRenderHook(const Rml::String&);

	bool GetIntrinsicDimensions(Rml::Vector2f& dimensions, float& ratio) override;
	void OnAttributeChange(const Rml::ElementAttributes& changed_attributes) override;
	void OnResize() override;
	void OnDpRatioChange() override;

	void OnRender() override;
private:
	Rml::Vector2f intrinsic_dimensions = {-1, -1};
	Rml::Vector2f actual_dimensions = { -1, -1};
	float dimensions_scale = 1.0f;
};

}  // namespace RmlGui

#endif  // ELEMENTRENDEREVENTEMITTER_H
