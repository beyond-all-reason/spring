/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//
// Created by ChrisFloofyKitsune on 1/27/2024.
//

/*
 * This source file is based on the ElementImage.h file of RmlUi, the HTML/CSS Interface Middleware
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

#ifndef ELEMENTLUATEXTURE_H
#define ELEMENTLUATEXTURE_H
#include <unordered_set>

#include "RmlUi/Core/Element.h"
#include "RmlUi/Core/Geometry.h"
#include "RmlUi/Core/Texture.h"
#include "Lua/LuaOpenGLUtils.h"

namespace RmlGui
{
	/// The 'LuaTexture' element can render textures created/used in the Lua environment.
	///
	/// The 'src' attribute is used to specify what texture to load
	/// See LuaOpenGLUtils::ParseTextureImage on how to format this special string
	///
	/// See Rml::ElementImage for use of width/height/rect params
	/// https://mikke89.github.io/RmlUiDoc/pages/rml/images.html
	///
	/// @author ChrisFloofyKitsune
	class ElementLuaTexture : public Rml::Element
	{
	public:
		RMLUI_RTTI_DefineWithParent(ElementLuaTexture, Element)

		/// Constructs a new ElementLuaTexture. This should not be called directly; use the Factory instead.
		/// @param[in] tag The tag the element was declared as in RML.
		ElementLuaTexture(const Rml::String& tag);
		virtual ~ElementLuaTexture();

		/// Returns the element's inherent size.
		bool GetIntrinsicDimensions(Rml::Vector2f& dimensions, float& ratio) override;

	protected:
		/// Renders the image.
		void OnRender() override;

		/// Regenerates the element's geometry.
		void OnResize() override;

		/// Our intrinsic dimensions may change with the dp-ratio.
		void OnDpRatioChange() override;

		/// Checks for changes to the image's source or dimensions.
		/// @param[in] changed_attributes A list of attributes changed on the element.
		void OnAttributeChange(const Rml::ElementAttributes& changed_attributes) override;

		/// Called when properties on the element are changed.
		/// @param[in] changed_properties The properties changed on the element.
		void OnPropertyChange(const Rml::PropertyIdSet& changed_properties) override;

		/// Detect when we have been added to the document.
		void OnChildAdd(Element* child) override;

	private:
		// Generates the element's geometry.
		void GenerateGeometry();

		// Loads the element's texture, as specified by the 'src' attribute.
		bool LoadTexture();

		// Loads the rect value from the element's attribute
		void UpdateRect();

		// Handle to the externally provided texture to be used
		LuaMatTexture luaTexture;

		// True if we need to refetch the texture's source from the element's attributes.
		bool texture_dirty;

		// A factor which scales the intrinsic dimensions based on the dp-ratio and image scale.
		float dimensions_scale;

		// The element's computed intrinsic dimensions. If either of these values are set to -1, then
		// that dimension has not been computed yet.
		Rml::Vector2f dimensions;

		// The rectangle extracted from the 'rect' attribute. The rect_source will be None if
		// these have not been specified or are invalid.
		Rml::Rectanglef rect;
		enum class RectSource { None, Attribute } rect_source;

		// The geometry used to render this element.
		Rml::CompiledGeometryHandle geometry_handle = (Rml::CompiledGeometryHandle) nullptr;
		bool geometry_dirty;

		Rml::Vector2i GetTextureDimensions();
	};
} // namespace RmlGui

#endif //ELEMENTLUATEXTURE_H
