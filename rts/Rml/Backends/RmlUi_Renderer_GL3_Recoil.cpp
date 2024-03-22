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
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>
#include <string.h>

#if defined(RMLUI_PLATFORM_WIN32) && !defined(__MINGW32__)
// function call missing argument list
#pragma warning(disable : 4551)
// unreferenced local function has been removed
#pragma warning(disable : 4505)
#endif

#include "Rendering/GL/VAO.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/myGL.h"

#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Matrix44f.h"

static const std::string rml_shader_header = "#version 130\n";

static const std::string shader_main_vertex = R"(
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

static const std::string shader_main_fragment_texture = R"(
uniform sampler2D _tex;
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
	vec4 texColor = texture(_tex, fragTexCoord);
	finalColor = fragColor * texColor;
}
)";
static const std::string shader_main_fragment_color = R"(
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
	finalColor = fragColor;
}
)";

namespace
{
namespace ProgramUniform
{
const char* const Translate = "_translate";
const char* const Transform = "_transform";
const char* const Tex = "_tex";
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
	AttributeDef(0, 2, GL_FLOAT, sizeof(Rml::Vertex), (const void*)offsetof(Rml::Vertex, position),
                 GL_FALSE, "pos"),
	AttributeDef(1, 4, GL_UNSIGNED_BYTE, sizeof(Rml::Vertex),
                 (const void*)offsetof(Rml::Vertex, colour), GL_TRUE, "col"),
	AttributeDef(2, 2, GL_FLOAT, sizeof(Rml::Vertex), (const void*)offsetof(Rml::Vertex, tex_coord),
                 GL_FALSE, "uv")};
}  // namespace

RenderInterface_GL3_Recoil::RenderInterface_GL3_Recoil()
{
	CreateShaders();
}

RenderInterface_GL3_Recoil::~RenderInterface_GL3_Recoil()
{
	shaderHandler->ReleaseProgramObjects("[Rml RenderInterface]");
}

void RenderInterface_GL3_Recoil::CreateShaders()
{
#define sh shaderHandler
	static const std::string prog_handles[2] = {"rml_tex", "rml_color"};

	static const std::string* frag_code[2] = {&shader_main_fragment_texture,
	                                          &shader_main_fragment_color};

	for (int i = 0; i < 2; i++) {
		Shader::IProgramObject* po =
			sh->CreateProgramObject("[Rml RenderInterface]", prog_handles[i]);
		po->AttachShaderObject(
			sh->CreateShaderObject(shader_main_vertex, rml_shader_header, GL_VERTEX_SHADER));
		po->AttachShaderObject(
			sh->CreateShaderObject(*frag_code[i], rml_shader_header, GL_FRAGMENT_SHADER));
		po->BindAttribLocations<CompiledGeometryData>();
		po->Link();

		po->Enable();
		po->SetUniform(ProgramUniform::Translate, 0, 0);
		po->SetUniformMatrix4x4(ProgramUniform::Transform, false, CMatrix44f::Identity().m);
		po->SetUniform(ProgramUniform::Tex, 0);
		po->Disable();
		po->Validate();

		programs[i] = po;
	}
#undef sh
}

void RenderInterface_GL3_Recoil::SetViewport(int width, int height)
{
	viewport_width = width;
	viewport_height = height;
}

void RenderInterface_GL3_Recoil::BeginFrame()
{
	RMLUI_ASSERT(viewport_width >= 0 && viewport_height >= 0);

	glPushAttrib(GL_VIEWPORT_BIT | GL_STENCIL_BUFFER_BIT | GL_SCISSOR_BIT | GL_POLYGON_BIT |
	             GL_COLOR_BUFFER_BIT);

	// Setup expected GL state.
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

	projection = Rml::Matrix4f::ProjectOrtho(0, (float)viewport_width, (float)viewport_height, 0,
	                                         -10000, 10000);
	SetTransform(nullptr);
}

void RenderInterface_GL3_Recoil::EndFrame()
{
	// Restore GL state.
	glPopAttrib();
}

void RenderInterface_GL3_Recoil::Clear()
{
	glClearStencil(0);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void RenderInterface_GL3_Recoil::RenderGeometry(Rml::Vertex* vertices, int num_vertices,
                                                int* indices, int num_indices,
                                                const Rml::TextureHandle texture,
                                                const Rml::Vector2f& translation)
{
	Rml::CompiledGeometryHandle geometry =
		CompileGeometry(vertices, num_vertices, indices, num_indices, texture);

	if (geometry) {
		RenderCompiledGeometry(geometry, translation);
		ReleaseCompiledGeometry(geometry);
	}
}

Rml::CompiledGeometryHandle
RenderInterface_GL3_Recoil::CompileGeometry(Rml::Vertex* vertices, int num_vertices, int* indices,
                                            int num_indices, Rml::TextureHandle texture)
{
	constexpr GLenum draw_usage = GL_STATIC_DRAW;

	auto vao = std::make_unique<VAO>();
	auto vbo = std::make_unique<VBO>(GL_ARRAY_BUFFER);
	auto ibo = std::make_unique<VBO>(GL_ELEMENT_ARRAY_BUFFER);

	vao->Generate();
	vbo->Generate();
	ibo->Generate();

	vao->Bind();

	vbo->Bind();
	vbo->New(num_vertices * sizeof(Rml::Vertex), GL_STATIC_DRAW, vertices);

	for (const AttributeDef& def : CompiledGeometryData::attributeDefs) {
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

void RenderInterface_GL3_Recoil::RenderCompiledGeometry(Rml::CompiledGeometryHandle handle,
                                                        const Rml::Vector2f& translation)
{
	auto* geometry = (CompiledGeometryData*)handle;

	Shader::IProgramObject* program = nullptr;

	if (geometry->texture) {
		program = programs[(size_t)ProgramId::Texture - 1];
		program->Enable();
		if (geometry->texture != TextureEnableWithoutBinding) {
			glBindTexture(GL_TEXTURE_2D, (GLuint)geometry->texture);
		}
		SubmitTransformUniform(ProgramId::Texture);
	} else {
		program = programs[(size_t)ProgramId::Color - 1];
		program->Enable();
		glBindTexture(GL_TEXTURE_2D, 0);
		SubmitTransformUniform(ProgramId::Color);
	}

	program->SetUniform(ProgramUniform::Translate, translation.x, translation.y);

	geometry->vao->Bind();
	glDrawElements(GL_TRIANGLES, geometry->num_indices, GL_UNSIGNED_INT, (const GLvoid*)0);
	geometry->vao->Unbind();

	program->Disable();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderInterface_GL3_Recoil::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle handle)
{
	auto geometry = (CompiledGeometryData*)handle;

	geometry->vao->Delete();
	geometry->vbo->Release();
	geometry->ibo->Release();

	delete geometry;
}

void RenderInterface_GL3_Recoil::EnableScissorRegion(bool enable)
{
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

void RenderInterface_GL3_Recoil::SetScissorRegion(int x, int y, int width, int height)
{
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

bool RenderInterface_GL3_Recoil::LoadTexture(Rml::TextureHandle& texture_handle,
                                             Rml::Vector2i& texture_dimensions,
                                             const Rml::String& source)
{
	CBitmap bmp;
	if (!bmp.Load(source)) {
		return false;
	}
	texture_dimensions.x = bmp.xsize;
	texture_dimensions.y = bmp.ysize;
	texture_handle = bmp.CreateTexture();
	return texture_handle != 0;
}

bool RenderInterface_GL3_Recoil::GenerateTexture(Rml::TextureHandle& texture_handle,
                                                 const Rml::byte* source,
                                                 const Rml::Vector2i& source_dimensions)
{
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

	texture_handle = (Rml::TextureHandle)texture_id;

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

void RenderInterface_GL3_Recoil::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	// Something was using a texture loaded/managed outside of Rml. Do nothing.
	if (texture_handle == TextureEnableWithoutBinding)
		return;

	glDeleteTextures(1, (GLuint*)&texture_handle);
}

void RenderInterface_GL3_Recoil::SetTransform(const Rml::Matrix4f* new_transform)
{
	transform_active = (new_transform != nullptr);
	transform = projection * (new_transform ? *new_transform : Rml::Matrix4f::Identity());
	transform_dirty_state = ProgramId::All;
}

void RenderInterface_GL3_Recoil::SubmitTransformUniform(ProgramId program_id)
{
	if ((int)program_id & (int)transform_dirty_state && program_id != ProgramId::All) {
		programs[(int)program_id - 1]->SetUniformMatrix4x4(ProgramUniform::Transform, false,
		                                                   transform.data());
		transform_dirty_state = ProgramId((int)transform_dirty_state & ~(int)program_id);
	}
}
