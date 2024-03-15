/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This source file is derived from the source code of RmlUi, the HTML/CSS Interface Middleware
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

#ifndef RMLUI_BACKENDS_RENDERER_GL3_RECOIL_H
#define RMLUI_BACKENDS_RENDERER_GL3_RECOIL_H

#include "Rendering/Shaders/Shader.h"

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>

namespace Shader
{
struct IProgramObject;
struct IShaderObject;
};  // namespace Shader

class RenderInterface_GL3_Recoil : public Rml::RenderInterface
{
public:
	RenderInterface_GL3_Recoil();
	~RenderInterface_GL3_Recoil();

	// Returns true if the renderer was successfully constructed.
	explicit operator bool() const
	{
		return std::ranges::all_of(programs, &Shader::IProgramObject::IsValid);
	}

	// The viewport should be updated whenever the window size changes.
	void SetViewport(int viewport_width, int viewport_height);

	// Sets up OpenGL states for taking rendering commands from RmlUi.
	void BeginFrame();
	void EndFrame();

	// Optional, can be used to clear the framebuffer.
	void Clear();

	// -- Inherited from Rml::RenderInterface --

	void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices,
	                    Rml::TextureHandle texture, const Rml::Vector2f& translation) override;

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* vertices, int num_vertices,
	                                            int* indices, int num_indices,
	                                            Rml::TextureHandle texture) override;
	void RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry,
	                            const Rml::Vector2f& translation) override;
	void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(int x, int y, int width, int height) override;

	bool LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions,
	                 const Rml::String& source) override;
	bool GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source,
	                     const Rml::Vector2i& source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

	// Can be passed to RenderGeometry() to enable texture rendering without changing the bound
	// texture. Can used as the output of a TextureCallback to signal that this texture is
	// externally managed.
	//   In that case, you will need to keep a reference to the texture
	//   you want and bind it before rendering the geometry.
	static const Rml::TextureHandle TextureEnableWithoutBinding = Rml::TextureHandle(-1);

private:
	void CreateShaders();

	enum class ProgramId : unsigned char {
		None = 0,
		Texture = 1,
		Color = 2,
		All = (Texture | Color)
	};

	Shader::IProgramObject* programs[2]{};
	void SubmitTransformUniform(ProgramId program_id);

	Rml::Matrix4f transform, projection;
	ProgramId transform_dirty_state = ProgramId::All;
	bool transform_active = false;

	enum class ScissoringState { Disable, Scissor, Stencil };
	ScissoringState scissoring_state = ScissoringState::Disable;

	int viewport_width = 0;
	int viewport_height = 0;
};

#endif
