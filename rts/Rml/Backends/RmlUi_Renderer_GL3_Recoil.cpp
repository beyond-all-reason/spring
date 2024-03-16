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

#include "RmlUi_Renderer_GL3_Recoil.h"
#include <RmlUi/Core/Log.h>

#include "Rendering/GL/VAO.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/myGL.h"

#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Matrix44f.h"

static const std::string rml_shader_header = "#version 130\n";

//language=glsl
static const std::string shader_vertex = R"(
uniform vec2 _translate;
uniform mat4 _transform;

in vec2 pos;
in vec4 col;
in vec2 uv;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
	fragTexCoord = uv;
	fragColor = col;

	vec2 translatedPos = pos + _translate.xy;
	vec4 outPos = _transform * vec4(translatedPos, 0, 1);

    gl_Position = outPos;
}
)";

//language=glsl
static const std::string shader_fragment = R"(
uniform bool _useTexture;
uniform sampler2D _texture;

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
	if (_useTexture) {
		finalColor = fragColor * texture(_texture, fragTexCoord);
	} else {
		finalColor = fragColor;
	}
}
)";

namespace {
	namespace ProgramUniform {
		const char *const Translate = "_translate";
		const char *const Transform = "_transform";
		const char *const Texture = "_tex";
		const char *const UseTexture = "_useTexture";
	}  // namespace ProgramUniform

	struct CompiledGeometryData {
		Rml::TextureHandle texture;
		std::unique_ptr<VAO> vao;
		std::unique_ptr<VBO> vbo;
		std::unique_ptr<VBO> ibo;
		int num_indices;

		static std::array<AttributeDef, 3> attributeDefs;
	};

	std::array<AttributeDef, 3> CompiledGeometryData::attributeDefs = {
			AttributeDef(0, 2, GL_FLOAT, sizeof(Rml::Vertex), (const void *) offsetof(Rml::Vertex, position),
						 GL_FALSE, "pos"),
			AttributeDef(1, 4, GL_UNSIGNED_BYTE, sizeof(Rml::Vertex),
						 (const void *) offsetof(Rml::Vertex, colour), GL_TRUE, "col"),
			AttributeDef(2, 2, GL_FLOAT, sizeof(Rml::Vertex), (const void *) offsetof(Rml::Vertex, tex_coord),
						 GL_FALSE, "uv")};
}  // namespace

RenderInterface_GL3_Recoil::RenderInterface_GL3_Recoil() {
	CreateShaders();
}

RenderInterface_GL3_Recoil::~RenderInterface_GL3_Recoil() {
	shaderHandler->ReleaseProgramObjects("[Rml RenderInterface]");
}

void RenderInterface_GL3_Recoil::CreateShaders() {
#define sh shaderHandler
	shader_program = sh->CreateProgramObject("[Rml RenderInterface]", "rml_shader");
	shader_program->AttachShaderObject(sh->CreateShaderObject(shader_vertex, rml_shader_header, GL_VERTEX_SHADER));
	shader_program->AttachShaderObject(sh->CreateShaderObject(shader_fragment, rml_shader_header, GL_FRAGMENT_SHADER));
	shader_program->BindAttribLocations<CompiledGeometryData>();
	shader_program->Link();

	shader_program->Enable();
	shader_program->SetUniform(ProgramUniform::Translate, 0, 0);
	shader_program->SetUniformMatrix4x4(ProgramUniform::Transform, false, CMatrix44f::Identity().m);
	shader_program->SetUniform(ProgramUniform::Texture, 0);
	shader_program->SetUniform(ProgramUniform::UseTexture, false);
	shader_program->Disable();
	shader_program->Validate();
#undef sh
}

void RenderInterface_GL3_Recoil::SetViewport(int width, int height) {
	viewport_width = width;
	viewport_height = height;
	projection = Rml::Matrix4f::ProjectOrtho(0, (float) viewport_width, (float) viewport_height, 0,
											 -10000, 10000);
}

void RenderInterface_GL3_Recoil::BeginFrame() {
	RMLUI_ASSERT(viewport_width >= 0 && viewport_height >= 0);

	// Setup expected GL state.
	glPushAttrib(GL_VIEWPORT_BIT | GL_STENCIL_BUFFER_BIT | GL_SCISSOR_BIT | GL_POLYGON_BIT |
				 GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, viewport_width, viewport_height);

	glClearStencil(0);
	glClearColor(0, 0, 0, 1);

	glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, GLuint(-1));
	glStencilMask(GLuint(-1));
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	SetTransform(nullptr);
}

void RenderInterface_GL3_Recoil::EndFrame() {
	// Restore GL state.
	glPopAttrib();
}

void RenderInterface_GL3_Recoil::Clear() {
	glClearStencil(0);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

/// Called by RmlUi when it wants to render geometry that the application does not wish to optimise. Note that
/// RmlUi renders everything as triangles.
/// @param[in] vertices The geometry's vertex data.
/// @param[in] num_vertices The number of vertices passed to the function.
/// @param[in] indices The geometry's index data.
/// @param[in] num_indices The number of indices passed to the function. This will always be a multiple of three.
/// @param[in] texture The texture to be applied to the geometry. This may be nullptr, in which case the geometry is untextured.
/// @param[in] translation The translation to apply to the geometry.
void RenderInterface_GL3_Recoil::RenderGeometry(Rml::Vertex *vertices, int num_vertices,
												int *indices, int num_indices,
												const Rml::TextureHandle texture,
												const Rml::Vector2f &translation) {
	Rml::CompiledGeometryHandle geometry =
			CompileGeometry(vertices, num_vertices, indices, num_indices, texture);

	if (geometry) {
		RenderCompiledGeometry(geometry, translation);
		ReleaseCompiledGeometry(geometry);
	}
}

/// Called by RmlUi when it wants to compile geometry it believes will be static for the forseeable future.
/// If supported, this should return a handle to an optimised, application-specific version of the data. If
/// not, do not override the function or return zero; the simpler RenderGeometry() will be called instead.
/// @param[in] vertices The geometry's vertex data.
/// @param[in] num_vertices The number of vertices passed to the function.
/// @param[in] indices The geometry's index data.
/// @param[in] num_indices The number of indices passed to the function. This will always be a multiple of three.
/// @param[in] texture The texture to be applied to the geometry. This may be nullptr, in which case the geometry is untextured.
/// @return The application-specific compiled geometry. Compiled geometry will be stored and rendered using RenderCompiledGeometry() in future
/// calls, and released with ReleaseCompiledGeometry() when it is no longer needed.
Rml::CompiledGeometryHandle
RenderInterface_GL3_Recoil::CompileGeometry(Rml::Vertex *vertices, int num_vertices, int *indices,
											int num_indices, Rml::TextureHandle texture) {
	auto vao = std::make_unique<VAO>();
	auto vbo = std::make_unique<VBO>(GL_ARRAY_BUFFER);
	auto ibo = std::make_unique<VBO>(GL_ELEMENT_ARRAY_BUFFER);

	vao->Generate();
	vbo->Generate();
	ibo->Generate();

	vao->Bind();

	vbo->Bind();
	vbo->New(num_vertices * sizeof(Rml::Vertex), GL_STATIC_DRAW, vertices);

	for (const AttributeDef &def: CompiledGeometryData::attributeDefs) {
		glEnableVertexAttribArray(def.index);
		glVertexAttribPointer(def.index, def.count, def.type, def.normalize, def.stride, def.data);
	}

	ibo->Bind();
	ibo->New(sizeof(int) * num_indices, GL_STATIC_DRAW, indices);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return (Rml::CompiledGeometryHandle) new CompiledGeometryData{
			texture, std::move(vao), std::move(vbo), std::move(ibo), num_indices};
}

/// Called by RmlUi when it wants to release application-compiled geometry.
/// @param[in] geometry The application-specific compiled geometry to release.
void RenderInterface_GL3_Recoil::RenderCompiledGeometry(Rml::CompiledGeometryHandle handle,
														const Rml::Vector2f &translation) {
	auto *geometry = (CompiledGeometryData *) handle;
	shader_program->Enable();

	if (geometry->texture) {
		shader_program->SetUniform(ProgramUniform::UseTexture, true);
		if (geometry->texture != TextureEnableWithoutBinding) {
			glBindTexture(GL_TEXTURE_2D, (GLuint) geometry->texture);
		}
	} else {
		shader_program->SetUniform(ProgramUniform::UseTexture, false);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	shader_program->SetUniform(ProgramUniform::Translate, translation.x, translation.y);
	if (transform_dirty) {
		shader_program->SetUniformMatrix4x4(ProgramUniform::Transform, false, transform.data());
		transform_dirty = false;
	}

	geometry->vao->Bind();
	glDrawElements(GL_TRIANGLES, geometry->num_indices, GL_UNSIGNED_INT, nullptr);
	geometry->vao->Unbind();

	shader_program->Disable();
	glBindTexture(GL_TEXTURE_2D, 0);
}

/// Called by RmlUi when it wants to release application-compiled geometry.
/// @param[in] geometry The application-specific compiled geometry to release.
void RenderInterface_GL3_Recoil::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle handle) {
	auto geometry = (CompiledGeometryData *) handle;

	geometry->vao->Delete();
	geometry->vbo->Release();
	geometry->ibo->Release();

	delete geometry;
}

/// Called by RmlUi when it wants to enable or disable scissoring to clip content.
/// @param[in] enable True if scissoring is to enabled, false if it is to be disabled.
void RenderInterface_GL3_Recoil::EnableScissorRegion(bool enable) {
	ScissoringState new_state = ScissoringState::Disable;

	if (enable)
		new_state = (transform_active ? ScissoringState::Stencil : ScissoringState::Scissor);

	if (new_state != scissoring_state) {
		// Disable old
		if (scissoring_state == ScissoringState::Scissor)
			glDisable(GL_SCISSOR_TEST);
		else if (scissoring_state == ScissoringState::Stencil)
			glStencilFunc(GL_ALWAYS, 1, GLuint(-1));

		// Enable new
		if (new_state == ScissoringState::Scissor)
			glEnable(GL_SCISSOR_TEST);
		else if (new_state == ScissoringState::Stencil)
			glStencilFunc(GL_EQUAL, 1, GLuint(-1));

		scissoring_state = new_state;
	}
}

/// Called by RmlUi when it wants to change the scissor region.
/// @param[in] x The left-most pixel to be rendered. All pixels to the left of this should be clipped.
/// @param[in] y The top-most pixel to be rendered. All pixels to the top of this should be clipped.
/// @param[in] width The width of the scissored region. All pixels to the right of (x + width) should be clipped.
/// @param[in] height The height of the scissored region. All pixels to below (y + height) should be clipped.
void RenderInterface_GL3_Recoil::SetScissorRegion(int x, int y, int width, int height) {
	if (transform_active) {
		const float left = float(x);
		const float right = float(x + width);
		const float top = float(y);
		const float bottom = float(y + height);

		Rml::Vertex vertices[4];
		vertices[0].position = {left, top};
		vertices[1].position = {right, top};
		vertices[2].position = {right, bottom};
		vertices[3].position = {left, bottom};

		int indices[6] = {0, 2, 1, 0, 3, 2};

		glClear(GL_STENCIL_BUFFER_BIT);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glStencilFunc(GL_ALWAYS, 1, GLuint(-1));
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		RenderGeometry(vertices, 4, indices, 6, 0, Rml::Vector2f(0, 0));

		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_EQUAL, 1, GLuint(-1));
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	} else {
		glScissor(x, viewport_height - (y + height), width, height);
	}
}

/// Called by RmlUi when a texture is required by the library.
/// @param[out] texture_handle The handle to write the texture handle for the loaded texture to.
/// @param[out] texture_dimensions The variable to write the dimensions of the loaded texture.
/// @param[in] source The application-defined image source, joined with the path of the referencing document.
/// @return True if the load attempt succeeded and the handle and dimensions are valid, false if not.
bool RenderInterface_GL3_Recoil::LoadTexture(Rml::TextureHandle &texture_handle,
											 Rml::Vector2i &texture_dimensions,
											 const Rml::String &source) {
	CBitmap bmp;
	if (!bmp.Load(source)) {
		return false;
	}
	texture_dimensions.x = bmp.xsize;
	texture_dimensions.y = bmp.ysize;
	texture_handle = bmp.CreateTexture();
	return texture_handle != 0;
}

/// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
/// @param[out] texture_handle The handle to write the texture handle for the generated texture to.
/// @param[in] source The raw 8-bit texture data. Each pixel is made up of four 8-bit values, indicating red, green, blue and alpha in that order.
/// @param[in] source_dimensions The dimensions, in pixels, of the source data.
/// @return True if the texture generation succeeded and the handle is valid, false if not.
bool RenderInterface_GL3_Recoil::GenerateTexture(Rml::TextureHandle &texture_handle,
												 const Rml::byte *source,
												 const Rml::Vector2i &source_dimensions) {
	GLuint texture_id = 0;
	glGenTextures(1, &texture_id);
	if (texture_id == 0) {
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to generate texture.");
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, texture_id);

	GLint internal_format = GL_RGBA8;
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, source_dimensions.x, source_dimensions.y, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, source);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	texture_handle = (Rml::TextureHandle) texture_id;

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

/// Called by RmlUi when a loaded texture is no longer required.
/// @param texture The texture handle to release.
void RenderInterface_GL3_Recoil::ReleaseTexture(Rml::TextureHandle texture_handle) {
	// Something was using a texture loaded/managed outside Rml. Do nothing.
	if (texture_handle == TextureEnableWithoutBinding)
		return;

	glDeleteTextures(1, (GLuint *) &texture_handle);
}

/// Called by RmlUi when it wants the renderer to use a new transform matrix.
/// This will only be called if 'transform' properties are encountered. If no transform applies to the current element, nullptr
/// is submitted. Then it expects the renderer to use an identity matrix or otherwise omit the multiplication with the transform.
/// @param[in] transform The new transform to apply, or nullptr if no transform applies to the current element.
void RenderInterface_GL3_Recoil::SetTransform(const Rml::Matrix4f *new_transform) {
	transform_active = (new_transform != nullptr);
	transform = projection * (new_transform ? *new_transform : Rml::Matrix4f::Identity());
	transform_dirty = true;
}
