/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This source file is derived from the source code of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2024 The RmlUi Team, and contributors
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

#include "System/TemplateUtils.hpp"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GL/SubState.h"

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>
#include <bitset>

enum class ProgramId;
enum class UniformId;

namespace Gfx {
	struct ProgramData;
	struct FramebufferData;
}

namespace Shader
{
	struct IShaderObject;
	struct IProgramObject;
}

struct StateGL {
	static auto GetSubState(int w, int h) {
		using namespace GL::State;
		return GL::SubState(
			Viewport(0, 0, w, h),
			ClearStencil(0),
			ClearColor(0.0f, 0.0f, 0.0f, 0.0f),
			Culling(false),
			ScissorTest(false),
			Scissor(0, 0, w, h),
			Blending(true),
			BlendEquation(GL_FUNC_ADD),
			BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA),
			FrameBufferSRBG(false),
			StencilTest(true),
			StencilFunc(GL_ALWAYS, 1, GLuint(-1)),
			StencilMask(GLuint(-1)),
			StencilOp(GL_KEEP, GL_KEEP, GL_KEEP)
		);
	}
};
//using StateGLReturnType = std::invoke_result_t<decltype(StateGL::GetSubState), int, int>;
using StateGLReturnType = spring::return_type_t<decltype(&StateGL::GetSubState)>;

class RenderInterface_GL3_Recoil : public Rml::RenderInterface
{
public:
	RenderInterface_GL3_Recoil();
	~RenderInterface_GL3_Recoil() override;

	// Returns true if the renderer was successfully constructed.
	explicit operator bool() const;

	// The viewport should be updated whenever the window size changes.
	void SetViewport(int viewport_width, int viewport_height);

	// Sets up OpenGL states for taking rendering commands from RmlUi.
	void BeginFrame();
	void EndFrame();

	// Optional, can be used to clear the framebuffer.
	void Clear();

	// -- Inherited from Rml::RenderInterface --

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle handle) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	void EnableClipMask(bool enable) override;
	void RenderToClipMask(Rml::ClipMaskOperation mask_operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

	Rml::LayerHandle PushLayer() override;
	void CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode,
						 Rml::Span<const Rml::CompiledFilterHandle> filters) override;
	void PopLayer() override;

	Rml::TextureHandle SaveLayerAsTexture() override;

	Rml::CompiledFilterHandle SaveLayerAsMaskImage() override;

	Rml::CompiledFilterHandle CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters) override;
	void ReleaseFilter(Rml::CompiledFilterHandle filter) override;

	Rml::CompiledShaderHandle CompileShader(const Rml::String& name, const Rml::Dictionary& parameters) override;
	void RenderShader(Rml::CompiledShaderHandle shader_handle, Rml::CompiledGeometryHandle geometry_handle, Rml::Vector2f translation,
					  Rml::TextureHandle texture) override;
	void ReleaseShader(Rml::CompiledShaderHandle effect_handle) override;

	// Can be passed to RenderGeometry() to enable texture rendering without changing the bound texture.
	static constexpr Rml::TextureHandle TextureEnableWithoutBinding = Rml::TextureHandle(-1);
	// Can be passed to RenderGeometry() to leave the bound texture and used program unchanged.
	static constexpr Rml::TextureHandle TexturePostprocess = Rml::TextureHandle(-2);

private:
	Shader::IProgramObject* UseProgram(ProgramId program_id);
	int GetUniformLocation(const char* name) const;
	void SubmitTransformUniform(Rml::Vector2f translation);

	void BlitLayerToPostprocessPrimary(Rml::LayerHandle layer_handle);
	void RenderFilters(Rml::Span<const Rml::CompiledFilterHandle> filter_handles);

	void SetScissor(Rml::Rectanglei region, bool vertically_flip = false);

	void DrawFullscreenQuad();
	void DrawFullscreenQuad(Rml::Vector2f uv_offset, Rml::Vector2f uv_scaling = Rml::Vector2f(1.f));

	void RenderBlur(float sigma, const Gfx::FramebufferData& source_destination, const Gfx::FramebufferData& temp, Rml::Rectanglei window_flipped);

	static constexpr size_t MaxNumPrograms = 32;
	std::bitset<MaxNumPrograms> program_transform_dirty;

	Rml::Matrix4f transform;
	Rml::Matrix4f projection;

	ProgramId active_program_id = {};
	Rml::Rectanglei scissor_state;

	int viewport_width = 0;
	int viewport_height = 0;

	Rml::CompiledGeometryHandle fullscreen_quad_geometry = {};

	Rml::UniquePtr<const Gfx::ProgramData> program_data;

	/*
	    Manages render targets, including the layer stack and postprocessing framebuffers.

	    Layers can be pushed and popped, creating new framebuffers as needed. Typically, geometry is rendered to the top
	    layer. The layer framebuffers may have MSAA enabled.

	    Postprocessing framebuffers are separate from the layers, and are commonly used to apply texture-wide effects
	    such as filters. They are used both as input and output during rendering, and do not use MSAA.
	*/
	class RenderLayerStack {
	public:
		RenderLayerStack();
		~RenderLayerStack();

		// Push a new layer. All references to previously retrieved layers are invalidated.
		Rml::LayerHandle PushLayer();

		// Pop the top layer. All references to previously retrieved layers are invalidated.
		void PopLayer();

		[[nodiscard]] const Gfx::FramebufferData& GetLayer(Rml::LayerHandle layer) const;
		[[nodiscard]] const Gfx::FramebufferData& GetTopLayer() const;
		[[nodiscard]] Rml::LayerHandle GetTopLayerHandle() const;

		const Gfx::FramebufferData& GetPostprocessPrimary() { return EnsureFramebufferPostprocess(0); }
		const Gfx::FramebufferData& GetPostprocessSecondary() { return EnsureFramebufferPostprocess(1); }
		const Gfx::FramebufferData& GetPostprocessTertiary() { return EnsureFramebufferPostprocess(2); }
		const Gfx::FramebufferData& GetBlendMask() { return EnsureFramebufferPostprocess(3); }

		void SwapPostprocessPrimarySecondary();

		void BeginFrame(int new_width, int new_height);
		void EndFrame();

	private:
		void DestroyFramebuffers();
		const Gfx::FramebufferData& EnsureFramebufferPostprocess(int index);

		int width = 0, height = 0;

		// The number of active layers is manually tracked since we re-use the framebuffers stored in the fb_layers stack.
		int layers_size = 0;

		Rml::Vector<Gfx::FramebufferData> fb_layers;
		Rml::Vector<Gfx::FramebufferData> fb_postprocess;
	};

	RenderLayerStack render_layers;

	std::unique_ptr<StateGLReturnType> glSubState;
};

#endif
