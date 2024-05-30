/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//
// Created by ChrisFloofyKitsune on 1/27/2024.
//

/*
 * This source file is based on the ElementLuaTexture.cpp file of RmlUi, the HTML/CSS Interface
 * Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "ElementLuaTexture.h"

#include "Lua/LuaOpenGLUtils.h"
#include "Rendering/GL/myGL.h"
#include "Rml/Backends/RmlUi_Backend.h"
#include "Rml/Backends/RmlUi_Renderer_GL3_Recoil.h"

#include "RmlUi/Core/ComputedValues.h"
#include "RmlUi/Core/ElementUtilities.h"
#include "RmlUi/Core/MeshUtilities.h"
#include "RmlUi/Core/PropertyIdSet.h"

namespace RmlGui
{
ElementLuaTexture::ElementLuaTexture(const Rml::String& tag) :
	Element(tag),
	dimensions(-1, -1), rect_source(RectSource::None), luaTexture()
{
	dimensions_scale = 1.0f;
	geometry_dirty = false;
	texture_dirty = true;
}

ElementLuaTexture::~ElementLuaTexture() {
	if (geometry_handle != (Rml::CompiledGeometryHandle) nullptr) {
		GetRenderInterface()->ReleaseGeometry(geometry_handle);
	}
};

Rml::Vector2i ElementLuaTexture::GetTextureDimensions()
{
	auto [x, y, _] = luaTexture.GetSize();
	return {x, y};
}

bool ElementLuaTexture::GetIntrinsicDimensions(Rml::Vector2f& _dimensions, float& _ratio)
{
	// Check if we need to reload the texture.
	if (texture_dirty)
		LoadTexture();

	auto texDimensions = GetTextureDimensions();
	
	// Calculate the x dimension.
	if (HasAttribute("width"))
		dimensions.x = GetAttribute<float>("width", -1);
	else if (rect_source == RectSource::None)
		dimensions.x = (float) texDimensions.x;
	else
		dimensions.x = rect.Width();

	// Calculate the y dimension.
	if (HasAttribute("height"))
		dimensions.y = GetAttribute<float>("height", -1);
	else if (rect_source == RectSource::None)
		dimensions.y = (float) texDimensions.y;
	else
		dimensions.y = rect.Height();

	dimensions *= dimensions_scale;

	// Return the calculated dimensions. If this changes the size of the element, it will result in
	// a call to 'onresize' below which will regenerate the geometry.
	_dimensions = dimensions;
	_ratio = dimensions.x / dimensions.y;

	return true;
}

void ElementLuaTexture::OnRender()
{
	// Regenerate the geometry if required (this will be set if 'rect' changes but does not result
	// in a resize).
	if (geometry_dirty)
		GenerateGeometry();
	
	glBindTexture(GL_TEXTURE_2D, luaTexture.GetTextureID());
	GetRenderInterface()->RenderGeometry(
		geometry_handle,
		GetAbsoluteOffset(Rml::BoxArea::Content).Round(),
		RenderInterface_GL3_Recoil::TextureEnableWithoutBinding
	);
}

void ElementLuaTexture::OnAttributeChange(const Rml::ElementAttributes& changed_attributes)
{
	// Call through to the base element's OnAttributeChange().
	Element::OnAttributeChange(changed_attributes);

	bool dirty_layout = false;

	// Check for a changed 'src' attribute. If this changes, the old texture handle is released,
	// forcing a reload when the layout is regenerated.
	if (changed_attributes.find("src") != changed_attributes.end()) {
		texture_dirty = true;
		dirty_layout = true;
	}

	// Check for a changed 'width' attribute. If this changes, a layout is forced which will
	// recalculate the dimensions.
	if (changed_attributes.find("width") != changed_attributes.end() ||
	    changed_attributes.find("height") != changed_attributes.end()) {
		dirty_layout = true;
	}

	// Check for a change to the 'rect' attribute. If this changes, the coordinates are
	// recomputed and a layout forced. If a sprite is set to source, then that will override any
	// attribute.
	if (changed_attributes.find("rect") != changed_attributes.end()) {
		UpdateRect();

		// Rectangle has changed; this will most likely result in a size change, so we need to force
		// a layout.
		dirty_layout = true;
	}

	if (dirty_layout)
		DirtyLayout();
}

static Rml::PropertyIdSet checked_properties = []() -> Rml::PropertyIdSet {
	Rml::PropertyIdSet set;

	set.Insert(Rml::PropertyId::ImageColor);
	set.Insert(Rml::PropertyId::Opacity);

	set.Insert(Rml::PropertyId::BorderTopLeftRadius);
	set.Insert(Rml::PropertyId::BorderTopRightRadius);
	set.Insert(Rml::PropertyId::BorderBottomRightRadius);
	set.Insert(Rml::PropertyId::BorderBottomLeftRadius);

	set.Insert(Rml::PropertyId::BorderTopWidth);
	set.Insert(Rml::PropertyId::BorderLeftWidth);
	set.Insert(Rml::PropertyId::BorderRightWidth);
	set.Insert(Rml::PropertyId::BorderBottomWidth);
	return set;
}();

void ElementLuaTexture::OnPropertyChange(const Rml::PropertyIdSet& changed_properties)
{
	Element::OnPropertyChange(changed_properties);

	if (!(changed_properties & checked_properties).Empty()) {
		GenerateGeometry();
	}
}

void ElementLuaTexture::OnChildAdd(Element* child)
{
	// Load the texture once we have attached to the document so that it can immediately be found
	// during the call to `Rml::GetTextureSourceList`. The texture won't actually be loaded from the
	// backend before it is shown. However, only do this if we have an active context so that the
	// dp-ratio can be retrieved. If there is no context now the texture loading will be deferred
	// until the next layout update.
	if (child == this && texture_dirty && GetContext()) {
		LoadTexture();
	}
}

void ElementLuaTexture::OnResize()
{
	GenerateGeometry();
}

void ElementLuaTexture::OnDpRatioChange()
{
	texture_dirty = true;
	DirtyLayout();
}

void ElementLuaTexture::GenerateGeometry()
{
	auto render_interface = GetRenderInterface();
	// Release the old geometry before making the new one.
	if (geometry_handle != (Rml::CompiledGeometryHandle) nullptr) {
		render_interface->ReleaseGeometry(geometry_handle);
		geometry_handle = (Rml::CompiledGeometryHandle) nullptr;
	}
	Rml::Mesh mesh;

	// Generate the texture coordinates.
	Rml::Vector2f tex_coords[2];
	if (rect_source != RectSource::None) {
		const auto texture_dimensions =
			Rml::Vector2f(Rml::Math::Max(GetTextureDimensions(), Rml::Vector2i(1)));
		tex_coords[0] = rect.TopLeft() / texture_dimensions;
		tex_coords[1] = rect.BottomRight() / texture_dimensions;
	} else {
		tex_coords[0] = Rml::Vector2f(0, 0);
		tex_coords[1] = Rml::Vector2f(1, 1);
	}

	const Rml::ComputedValues& computed = GetComputedValues();

	float opacity = computed.opacity();
	Rml::ColourbPremultiplied quad_colour = computed.image_color().ToPremultiplied(opacity);

	Rml::Vector2f quad_size = GetBox().GetSize(Rml::BoxArea::Content).Round();

	if (
		computed.border_top_left_radius() > 0 ||
		computed.border_top_right_radius() > 0 ||
		computed.border_bottom_left_radius() > 0 ||
		computed.border_bottom_right_radius() > 0
	) {
		Rml::Vector4f radii{
			computed.border_top_left_radius(),
			computed.border_top_right_radius(),
			computed.border_bottom_left_radius(),
			computed.border_bottom_right_radius(),
		};

		const Rml::ColourbPremultiplied clear_colors[4] = {{0, 0},
														   {0, 0},
														   {0, 0},
														   {0, 0}};
		Rml::MeshUtilities::GenerateBackgroundBorder(mesh, GetBox(), Rml::Vector2f(), radii, quad_colour, clear_colors);

		// GenerateBackgroundBorder does *not* set UV coords, so we must do that ourselves.
		Rml::Vector<Rml::Vertex>& vertices = mesh.vertices;

		// Map the vertex positions to tex_coord positions
		std::ranges::for_each(vertices, [&quad_size, &tex_coords](Rml::Vertex& v) {
			float tx = v. 	position.x / quad_size.x;
			float ty = v.position.y / quad_size.y;

			v.tex_coord.x = Rml::Math::Lerp(tx, tex_coords[0].x, tex_coords[1].x);
			v.tex_coord.y = Rml::Math::Lerp(ty, tex_coords[0].y, tex_coords[1].y);
		});

	} else {
		Rml::MeshUtilities::GenerateQuad(
			mesh, Rml::Vector2f(0, 0), quad_size,
			quad_colour, tex_coords[0], tex_coords[1]
		);
	}
	
	geometry_handle = render_interface->CompileGeometry(mesh.vertices, mesh.indices);
	geometry_dirty = false;
}

bool ElementLuaTexture::LoadTexture()
{
	texture_dirty = false;
	geometry_dirty = true;
	dimensions_scale = Rml::ElementUtilities::GetDensityIndependentPixelRatio(this);

	const auto source_name = GetAttribute<Rml::String>("src", "");
	if (source_name.empty()) {
		rect_source = RectSource::None;
	}

	return LuaOpenGLUtils::ParseTextureImage(RmlGui::GetLuaState(), luaTexture, source_name);
}

void ElementLuaTexture::UpdateRect()
{
	bool valid_rect = false;

	if (const auto rect_string = GetAttribute<Rml::String>("rect", ""); !rect_string.empty()) {
		Rml::StringList coords_list;
		Rml::StringUtilities::ExpandString(coords_list, rect_string, ' ');

		if (coords_list.size() != 4) {
			Rml::Log::Message(Rml::Log::LT_WARNING,
			                  "Element '%s' has an invalid 'rect' attribute; rect requires 4 "
			                  "space-separated values, found %zu.",
			                  GetAddress().c_str(), coords_list.size());
		} else {
			const Rml::Vector2f position = {Rml::FromString(coords_list[0], 0.f),
			                                Rml::FromString(coords_list[1], 0.f)};
			const Rml::Vector2f size = {Rml::FromString(coords_list[2], 0.f),
			                            Rml::FromString(coords_list[3], 0.f)};
			rect = Rml::Rectanglef::FromPositionSize(position, size);

			// We have new, valid coordinates; force the geometry to be regenerated.
			valid_rect = true;
			rect_source = RectSource::Attribute;
		}
	}

	if (!valid_rect) {
		rect = {};
		rect_source = RectSource::None;
	}

	geometry_dirty = true;
}

}  // namespace RmlGui
