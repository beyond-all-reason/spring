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
#include "System/Log/ILog.h"
#include "RmlUi/Core/Mesh.h"
#include "RmlUi/Core/Colour.h"
#include "RmlUi/Core/MeshUtilities.h"
#include "RmlUi/Core/Dictionary.h"
#include "RmlUi/Core/Core.h"
#include "RmlUi/Core/SystemInterface.h"
#include "RmlUi/Core/DecorationTypes.h"

// Determines the anti-aliasing quality when creating layers. Enables better-looking visuals, especially when transforms are applied.
static constexpr int NUM_MSAA_SAMPLES = 2;

#define MAX_NUM_STOPS 16
#define BLUR_SIZE 7
#define BLUR_NUM_WEIGHTS ((BLUR_SIZE + 1) / 2)

#define RMLUI_STRINGIFY_IMPL(x) #x
#define RMLUI_STRINGIFY(x) RMLUI_STRINGIFY_IMPL(x)

#define RMLUI_SHADER_HEADER_VERSION "#version 330\n"
#define RMLUI_SHADER_HEADER \
    RMLUI_SHADER_HEADER_VERSION "#define MAX_NUM_STOPS " RMLUI_STRINGIFY(MAX_NUM_STOPS) "\n"

static const char* shader_vert_main = RMLUI_SHADER_HEADER R"(
uniform vec2 _translate;
uniform mat4 _transform;

in vec2 inPosition;
in vec4 inColor0;
in vec2 inTexCoord0;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
	fragTexCoord = inTexCoord0;
	fragColor = inColor0;

	vec2 translatedPos = inPosition + _translate;
	vec4 outPos = _transform * vec4(translatedPos, 0.0, 1.0);

    gl_Position = outPos;
}
)";

static const char* shader_frag_texture = RMLUI_SHADER_HEADER R"(
uniform sampler2D _tex;
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
	vec4 texColor = texture(_tex, fragTexCoord);
	finalColor = fragColor * texColor;
}
)";

static const char* shader_frag_color = RMLUI_SHADER_HEADER R"(
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
	finalColor = fragColor;
}
)";

enum class ShaderGradientFunction
{
	Linear, Radial, Conic, RepeatingLinear, RepeatingRadial, RepeatingConic
}; // Must match shader definitions below.

static const char* shader_frag_gradient = RMLUI_SHADER_HEADER R"(
#define LINEAR 0
#define RADIAL 1
#define CONIC 2
#define REPEATING_LINEAR 3
#define REPEATING_RADIAL 4
#define REPEATING_CONIC 5
#define PI 3.14159265

uniform int _func; // one of the above definitions
uniform vec2 _p;   // linear: starting point,         radial: center,                        conic: center
uniform vec2 _v;   // linear: vector to ending point, radial: 2d curvature (inverse radius), conic: angled unit vector
uniform vec4 _stop_colors[MAX_NUM_STOPS];
uniform float _stop_positions[MAX_NUM_STOPS]; // normalized, 0 -> starting point, 1 -> ending point
uniform int _num_stops;

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

vec4 mix_stop_colors(float t) {
	vec4 color = _stop_colors[0];

	for (int i = 1; i < _num_stops; i++)
		color = mix(color, _stop_colors[i], smoothstep(_stop_positions[i-1], _stop_positions[i], t));

	return color;
}

void main() {
	float t = 0.0;

	if (_func == LINEAR || _func == REPEATING_LINEAR)
	{
		float dist_square = dot(_v, _v);
		vec2 V = fragTexCoord - _p;
		t = dot(_v, V) / dist_square;
	}
	else if (_func == RADIAL || _func == REPEATING_RADIAL)
	{
		vec2 V = fragTexCoord - _p;
		t = length(_v * V);
	}
	else if (_func == CONIC || _func == REPEATING_CONIC)
	{
		mat2 R = mat2(_v.x, -_v.y, _v.y, _v.x);
		vec2 V = R * (fragTexCoord - _p);
		t = 0.5 + atan(-V.x, V.y) / (2.0 * PI);
	}

	if (_func == REPEATING_LINEAR || _func == REPEATING_RADIAL || _func == REPEATING_CONIC)
	{
		float t0 = _stop_positions[0];
		float t1 = _stop_positions[_num_stops - 1];
		t = t0 + mod(t - t0, t1 - t0);
	}

	finalColor = fragColor * mix_stop_colors(t);
}
)";

// "Creation" by Danilo Guanabara, based on: https://www.shadertoy.com/view/XsXXDn
static const char* shader_frag_creation = RMLUI_SHADER_HEADER R"(
uniform float _value;
uniform vec2 _dimensions;

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

void main() {
	float t = _value;
	vec3 c;
	float l;
	for (int i = 0; i < 3; i++) {
		vec2 p = fragTexCoord;
		vec2 uv = p;
		p -= .5;
		p.x *= _dimensions.x / _dimensions.y;
		float z = t + float(i) * .07;
		l = length(p);
		uv += p / l * (sin(z) + 1.) * abs(sin(l * 9. - z - z));
		c[i] = .01 / length(mod(uv, 1.) - .5);
	}
	finalColor = vec4(c / l, fragColor.a);
}
)";

static const char* shader_vert_passthrough = RMLUI_SHADER_HEADER R"(
in vec2 inPosition;
in vec2 inTexCoord0;

out vec2 fragTexCoord;

void main() {
	fragTexCoord = inTexCoord0;
    gl_Position = vec4(inPosition, 0.0, 1.0);
}
)";

static const char* shader_frag_passthrough = RMLUI_SHADER_HEADER R"(
uniform sampler2D _tex;
in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
	finalColor = texture(_tex, fragTexCoord);
}
)";

static const char* shader_frag_color_matrix = RMLUI_SHADER_HEADER R"(
uniform sampler2D _tex;
uniform mat4 _color_matrix;

in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
	// The general case uses a 4x5 color matrix for full rgba transformation, plus a constant term with the last column.
	// However, we only consider the case of rgb transformations. Thus, we could in principle use a 3x4 matrix, but we
	// keep the alpha row for simplicity.
	// In the general case we should do the matrix transformation in non-premultiplied space. However, without alpha
	// transformations, we can do it directly in premultiplied space to avoid the extra division and multiplication
	// steps. In this space, the constant term needs to be multiplied by the alpha value, instead of unity.
	vec4 texColor = texture(_tex, fragTexCoord);
	vec3 transformedColor = vec3(_color_matrix * texColor);
	finalColor = vec4(transformedColor, texColor.a);
}
)";

static const char* shader_frag_blend_mask = RMLUI_SHADER_HEADER R"(
uniform sampler2D _tex;
uniform sampler2D _texMask;

in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
	vec4 texColor = texture(_tex, fragTexCoord);
	float maskAlpha = texture(_texMask, fragTexCoord).a;
	finalColor = texColor * maskAlpha;
}
)";

#define RMLUI_SHADER_BLUR_HEADER \
    RMLUI_SHADER_HEADER "\n#define BLUR_SIZE " RMLUI_STRINGIFY(BLUR_SIZE) "\n#define BLUR_NUM_WEIGHTS " RMLUI_STRINGIFY(BLUR_NUM_WEIGHTS)

static const char* shader_vert_blur = RMLUI_SHADER_BLUR_HEADER R"(
uniform vec2 _texelOffset;

in vec3 inPosition;
in vec2 inTexCoord0;

out vec2 fragTexCoord[BLUR_SIZE];

void main() {
	for(int i = 0; i < BLUR_SIZE; i++)
		fragTexCoord[i] = inTexCoord0 - float(i - BLUR_NUM_WEIGHTS + 1) * _texelOffset;
    gl_Position = vec4(inPosition, 1.0);
}
)";

static const char* shader_frag_blur = RMLUI_SHADER_BLUR_HEADER R"(
uniform sampler2D _tex;
uniform float _weights[BLUR_NUM_WEIGHTS];
uniform vec2 _texCoordMin;
uniform vec2 _texCoordMax;

in vec2 fragTexCoord[BLUR_SIZE];
out vec4 finalColor;

void main() {
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
	for(int i = 0; i < BLUR_SIZE; i++)
	{
		vec2 in_region = step(_texCoordMin, fragTexCoord[i]) * step(fragTexCoord[i], _texCoordMax);
		color += texture(_tex, fragTexCoord[i]) * in_region.x * in_region.y * _weights[abs(i - BLUR_NUM_WEIGHTS + 1)];
	}
	finalColor = color;
}
)";

static const char* shader_frag_drop_shadow = RMLUI_SHADER_HEADER R"(
uniform sampler2D _tex;
uniform vec2 _texCoordMin;
uniform vec2 _texCoordMax;
uniform vec4 _color;

in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
	vec2 in_region = step(_texCoordMin, fragTexCoord) * step(fragTexCoord, _texCoordMax);
	finalColor = texture(_tex, fragTexCoord).a * in_region.x * in_region.y * _color;
}
)";

enum class ProgramId
{
	None = 0,
	Color,
	Texture,
	Gradient,
	Creation,
	Passthrough,
	ColorMatrix,
	BlendMask,
	Blur,
	DropShadow,
	Count,
};
enum class VertShaderId
{
	Main = 0,
	Passthrough,
	Blur,
	Count,
};
enum class FragShaderId
{
	Color = 0,
	Texture,
	Gradient,
	Creation,
	Passthrough,
	ColorMatrix,
	BlendMask,
	Blur,
	DropShadow,
	Count,
};

namespace Uniform
{
#define UniformStr(N, S) const char *const N = S
UniformStr(Translate, "_translate");
UniformStr(Transform, "_transform");
UniformStr(Tex, "_tex");
UniformStr(Color, "_color");
UniformStr(ColorMatrix, "_color_matrix");
UniformStr(TexelOffset, "_texelOffset");
UniformStr(TexCoordMin, "_texCoordMin");
UniformStr(TexCoordMax, "_texCoordMax");
UniformStr(TexMask, "_texMask");
UniformStr(Weights, "_weights");
UniformStr(Func, "_func");
UniformStr(P, "_p");
UniformStr(V, "_v");
UniformStr(StopColors, "_stop_colors");
UniformStr(StopPositions, "_stop_positions");
UniformStr(NumStops, "_num_stops");
UniformStr(Value, "_value");
UniformStr(Dimensions, "_dimensions");
#undef UniformStr
}

namespace Gfx
{

#define VA_ATTR_DEF(T, idx, count, type, member, normalized, name) AttributeDef(idx, count, type, sizeof(T), VA_TYPE_OFFSET(T, member), normalized, name)
struct VA_TYPE_RML_VERTEX
{
	static std::array<AttributeDef, 3> attributeDefs;
};
std::array<AttributeDef, 3> VA_TYPE_RML_VERTEX::attributeDefs = {
	VA_ATTR_DEF(Rml::Vertex, 0, 2, GL_FLOAT, position, false, "inPosition"),
	VA_ATTR_DEF(Rml::Vertex, 1, 4, GL_UNSIGNED_BYTE, colour, true, "inColor0"),
	VA_ATTR_DEF(Rml::Vertex, 2, 2, GL_FLOAT, tex_coord, false, "inTexCoord0")
};
#undef VA_ATTR_DEF

struct VertShaderDefinition
{
	VertShaderId id;
	const char* name_str;
	const char* code_str;
};
struct FragShaderDefinition
{
	FragShaderId id;
	const char* name_str;
	const char* code_str;
};
struct ProgramDefinition
{
	ProgramId id;
	const char* name_str;
	VertShaderId vert_shader;
	FragShaderId frag_shader;
};

// clang-format off
static const VertShaderDefinition vert_shader_definitions[] = {
	{VertShaderId::Main,        "main",        shader_vert_main},
	{VertShaderId::Passthrough, "passthrough", shader_vert_passthrough},
	{VertShaderId::Blur,        "blur",        shader_vert_blur},
};
static const FragShaderDefinition frag_shader_definitions[] = {
	{FragShaderId::Color,       "color",        shader_frag_color},
	{FragShaderId::Texture,     "texture",      shader_frag_texture},
	{FragShaderId::Gradient,    "gradient",     shader_frag_gradient},
	{FragShaderId::Creation,    "creation",     shader_frag_creation},
	{FragShaderId::Passthrough, "passthrough",  shader_frag_passthrough},
	{FragShaderId::ColorMatrix, "color_matrix", shader_frag_color_matrix},
	{FragShaderId::BlendMask,   "blend_mask",   shader_frag_blend_mask},
	{FragShaderId::Blur,        "blur",         shader_frag_blur},
	{FragShaderId::DropShadow,  "drop_shadow",  shader_frag_drop_shadow},
};
static const ProgramDefinition program_definitions[] = {
	{ProgramId::Color,       "color",        VertShaderId::Main,        FragShaderId::Color},
	{ProgramId::Texture,     "texture",      VertShaderId::Main,        FragShaderId::Texture},
	{ProgramId::Gradient,    "gradient",     VertShaderId::Main,        FragShaderId::Gradient},
	{ProgramId::Creation,    "creation",     VertShaderId::Main,        FragShaderId::Creation},
	{ProgramId::Passthrough, "passthrough",  VertShaderId::Passthrough, FragShaderId::Passthrough},
	{ProgramId::ColorMatrix, "color_matrix", VertShaderId::Passthrough, FragShaderId::ColorMatrix},
	{ProgramId::BlendMask,   "blend_mask",   VertShaderId::Passthrough, FragShaderId::BlendMask},
	{ProgramId::Blur,        "blur",         VertShaderId::Blur,        FragShaderId::Blur},
	{ProgramId::DropShadow,  "drop_shadow",  VertShaderId::Passthrough, FragShaderId::DropShadow},
};
// clang-format on

template<typename T, typename Enum>
class EnumArray
{
public:
	const T& operator[](Enum id) const
	{
		RMLUI_ASSERT((size_t) id < (size_t) Enum::Count)
		return ids[size_t(id)];
	}

	T& operator[](Enum id)
	{
		RMLUI_ASSERT((size_t) id < (size_t) Enum::Count)
		return ids[size_t(id)];
	}

	[[nodiscard]] auto begin() const
	{ return ids.begin(); }

	[[nodiscard]] auto end() const
	{ return ids.end(); }

private:
	Rml::Array<T, (size_t) Enum::Count> ids = {};
};

using Programs = EnumArray<Shader::IProgramObject*, ProgramId>;

struct ProgramData
{
	Programs programs;
};

struct CompiledGeometryData
{
	std::unique_ptr<VAO> vao;
	std::unique_ptr<VBO> vbo;
	std::unique_ptr<VBO> ibo;
	GLsizei num_indices = 0;
};

struct FramebufferData
{
	int width, height;
	GLuint framebuffer;
	GLuint color_tex_buffer;
	GLuint color_render_buffer;
	GLuint depth_stencil_buffer;
	bool owns_depth_stencil_buffer;
};

enum class FramebufferAttachment
{
	None, Depth, DepthStencil
};

static void CheckGLError(const char* operation_name)
{
#ifdef RMLUI_DEBUG
	GLenum error_code = glGetError();
	if (error_code != GL_NO_ERROR) {
		static const Rml::Pair<GLenum, const char*> error_names[] = {{GL_INVALID_ENUM,      "GL_INVALID_ENUM"},
																	 {GL_INVALID_VALUE,     "GL_INVALID_VALUE"},
																	 {GL_INVALID_OPERATION, "GL_INVALID_OPERATION"},
																	 {GL_OUT_OF_MEMORY,     "GL_OUT_OF_MEMORY"}};
		const char* error_str = "''";
		for (auto& err: error_names) {
			if (err.first == error_code) {
				error_str = err.second;
				break;
			}
		}
		Rml::Log::Message(Rml::Log::LT_ERROR, "OpenGL error during %s. Error code 0x%x (%s).", operation_name,
						  error_code, error_str);
	}
#endif
	(void) operation_name;
}

static bool CreateFramebuffer(
	FramebufferData& out_fb, int width, int height,
	int samples, FramebufferAttachment attachment,
	GLuint shared_depth_stencil_buffer
)
{
#ifdef RMLUI_PLATFORM_EMSCRIPTEN
	constexpr GLint wrap_mode = GL_CLAMP_TO_EDGE;
#else
	constexpr GLint wrap_mode = GL_CLAMP_TO_BORDER; // GL_REPEAT GL_MIRRORED_REPEAT GL_CLAMP_TO_EDGE
#endif

	constexpr GLenum color_format = GL_RGBA8;   // GL_RGBA8 GL_SRGB8_ALPHA8 GL_RGBA16F
	constexpr GLint min_mag_filter = GL_NEAREST; // GL_NEAREST
	const Rml::Colourf border_color(0.f, 0.f);

	GLuint framebuffer = 0;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	GLuint color_tex_buffer = 0;
	GLuint color_render_buffer = 0;
	if (samples > 0) {
		glGenRenderbuffers(1, &color_render_buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, color_render_buffer);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, color_format, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_render_buffer);
	} else {
		glGenTextures(1, &color_tex_buffer);
		glBindTexture(GL_TEXTURE_2D, color_tex_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, color_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, min_mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);
#ifndef RMLUI_PLATFORM_EMSCRIPTEN
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border_color[0]);
#endif

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex_buffer, 0);
	}

	// Create depth/stencil buffer storage attachment.
	GLuint depth_stencil_buffer = 0;
	if (attachment != FramebufferAttachment::None) {
		if (shared_depth_stencil_buffer) {
			// Share depth/stencil buffer
			depth_stencil_buffer = shared_depth_stencil_buffer;
		} else {
			// Create new depth/stencil buffer
			glGenRenderbuffers(1, &depth_stencil_buffer);
			glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_buffer);

			const GLenum internal_format = (
				attachment == FramebufferAttachment::DepthStencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24
			);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, internal_format, width, height);
		}

		const GLenum attachment_type = (
			attachment == FramebufferAttachment::DepthStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT
		);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment_type, GL_RENDERBUFFER, depth_stencil_buffer);
	}

	const GLuint framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
		Rml::Log::Message(Rml::Log::LT_ERROR, "OpenGL framebuffer could not be generated. Error code %x.",
						  framebuffer_status);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	CheckGLError("CreateFramebuffer");

	out_fb = {};
	out_fb.width = width;
	out_fb.height = height;
	out_fb.framebuffer = framebuffer;
	out_fb.color_tex_buffer = color_tex_buffer;
	out_fb.color_render_buffer = color_render_buffer;
	out_fb.depth_stencil_buffer = depth_stencil_buffer;
	out_fb.owns_depth_stencil_buffer = !shared_depth_stencil_buffer;

	return true;
}

static void DestroyFramebuffer(FramebufferData& fb)
{
	if (fb.framebuffer)
		glDeleteFramebuffers(1, &fb.framebuffer);
	if (fb.color_tex_buffer)
		glDeleteTextures(1, &fb.color_tex_buffer);
	if (fb.color_render_buffer)
		glDeleteRenderbuffers(1, &fb.color_render_buffer);
	if (fb.owns_depth_stencil_buffer && fb.depth_stencil_buffer)
		glDeleteRenderbuffers(1, &fb.depth_stencil_buffer);
	fb = {};
}

static void BindTexture(const FramebufferData& fb)
{
	if (!fb.color_tex_buffer) {
		RMLUI_ERRORMSG(
			"Only framebuffers with color textures can be bound as textures. This framebuffer probably uses multisampling which needs a "
			"blit step first.")
	}

	glBindTexture(GL_TEXTURE_2D, fb.color_tex_buffer);
}

static bool CreateShaders(ProgramData& data)
{
	RMLUI_ASSERT(std::ranges::all_of(data.programs, [](auto* ptr) { return ptr == nullptr; }))

#define sh shaderHandler
	for (const ProgramDefinition& def: program_definitions) {
		auto vert_def = vert_shader_definitions[(size_t) def.vert_shader];
		auto frag_def = frag_shader_definitions[(size_t) def.frag_shader];

		auto program = sh->CreateProgramObject("[Rml RenderInterface]", def.name_str);
		program->AttachShaderObject(sh->CreateShaderObject(vert_def.code_str, "", GL_VERTEX_SHADER));
		program->AttachShaderObject(sh->CreateShaderObject(frag_def.code_str, "", GL_FRAGMENT_SHADER));
		program->BindAttribLocations<VA_TYPE_RML_VERTEX>();
		program->Link();

		if (!program->IsValid()) {
			const char* fmt = "RMLUI Shader '%s' (vert: '%s' frag: '%s') compilation error: %s";
			LOG_L(L_ERROR, fmt, program->GetName().c_str(), vert_def.name_str, frag_def.name_str,
				  program->GetLog().c_str());
			return false;
		}

		data.programs[def.id] = program;
#undef sh
	}

	auto blend_mask_prog = data.programs[ProgramId::BlendMask];
	blend_mask_prog->Enable();
	blend_mask_prog->SetUniform(Uniform::TexMask, 1);
	blend_mask_prog->Disable();

	return true;
}

} // namespace Gfx

RenderInterface_GL3_Recoil::RenderInterface_GL3_Recoil()
{
	auto mut_program_data = Rml::MakeUnique<Gfx::ProgramData>();
	if (Gfx::CreateShaders(*mut_program_data)) {
		program_data = std::move(mut_program_data);
		Rml::Mesh mesh;
		Rml::MeshUtilities::GenerateQuad(mesh, Rml::Vector2f(-1), Rml::Vector2f(2), {});
		fullscreen_quad_geometry = RenderInterface_GL3_Recoil::CompileGeometry(mesh.vertices, mesh.indices);
	}
}

RenderInterface_GL3_Recoil::~RenderInterface_GL3_Recoil()
{
	if (fullscreen_quad_geometry) {
		RenderInterface_GL3_Recoil::ReleaseGeometry(fullscreen_quad_geometry);
		fullscreen_quad_geometry = {};
	}

	if (program_data) {
		shaderHandler->ReleaseProgramObjects("[Rml RenderInterface]");
		program_data.reset();
	}
}

void RenderInterface_GL3_Recoil::SetViewport(int width, int height)
{
	viewport_width = Rml::Math::Max(width, 1);
	viewport_height = Rml::Math::Max(height, 1);
	projection = Rml::Matrix4f::ProjectOrtho(0, (float) viewport_width, (float) viewport_height, 0, -10000, 10000);
}

void RenderInterface_GL3_Recoil::BeginFrame()
{
	RMLUI_ASSERT(viewport_width >= 1 && viewport_height >= 1);

	// Backup GL state.
	glstate_backup.enable_cull_face = glIsEnabled(GL_CULL_FACE);
	glstate_backup.enable_blend = glIsEnabled(GL_BLEND);
	glstate_backup.enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
	glstate_backup.enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
	glstate_backup.enable_depth_test = glIsEnabled(GL_DEPTH_TEST);

	glGetIntegerv(GL_VIEWPORT, glstate_backup.viewport);
	glGetIntegerv(GL_SCISSOR_BOX, glstate_backup.scissor);

	glGetIntegerv(GL_ACTIVE_TEXTURE, &glstate_backup.active_texture);

	glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &glstate_backup.stencil_clear_value);
	glGetFloatv(GL_COLOR_CLEAR_VALUE, glstate_backup.color_clear_value);
	glGetBooleanv(GL_COLOR_WRITEMASK, glstate_backup.color_writemask);

	glGetIntegerv(GL_BLEND_EQUATION_RGB, &glstate_backup.blend_equation_rgb);
	glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &glstate_backup.blend_equation_alpha);
	glGetIntegerv(GL_BLEND_SRC_RGB, &glstate_backup.blend_src_rgb);
	glGetIntegerv(GL_BLEND_DST_RGB, &glstate_backup.blend_dst_rgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &glstate_backup.blend_src_alpha);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &glstate_backup.blend_dst_alpha);

	glGetIntegerv(GL_STENCIL_FUNC, &glstate_backup.stencil_front.func);
	glGetIntegerv(GL_STENCIL_REF, &glstate_backup.stencil_front.ref);
	glGetIntegerv(GL_STENCIL_VALUE_MASK, &glstate_backup.stencil_front.value_mask);
	glGetIntegerv(GL_STENCIL_WRITEMASK, &glstate_backup.stencil_front.writemask);
	glGetIntegerv(GL_STENCIL_FAIL, &glstate_backup.stencil_front.fail);
	glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &glstate_backup.stencil_front.pass_depth_fail);
	glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &glstate_backup.stencil_front.pass_depth_pass);

	glGetIntegerv(GL_STENCIL_BACK_FUNC, &glstate_backup.stencil_back.func);
	glGetIntegerv(GL_STENCIL_BACK_REF, &glstate_backup.stencil_back.ref);
	glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &glstate_backup.stencil_back.value_mask);
	glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &glstate_backup.stencil_back.writemask);
	glGetIntegerv(GL_STENCIL_BACK_FAIL, &glstate_backup.stencil_back.fail);
	glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &glstate_backup.stencil_back.pass_depth_fail);
	glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &glstate_backup.stencil_back.pass_depth_pass);

	// Setup expected GL state.
	glViewport(0, 0, viewport_width, viewport_height);

	glClearStencil(0);
	glClearColor(0, 0, 0, 0);

	glActiveTexture(GL_TEXTURE0);

	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);

	// Set blending function for premultiplied alpha.
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// We do blending in nonlinear sRGB space because that is the common practice and gives results that we are used to.
	glDisable(GL_FRAMEBUFFER_SRGB);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, GLuint(-1));
	glStencilMask(GLuint(-1));
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glDisable(GL_DEPTH_TEST);

	SetTransform(nullptr);

	render_layers.BeginFrame(viewport_width, viewport_height);
	glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetTopLayer().framebuffer);
	glClear(GL_COLOR_BUFFER_BIT);

	UseProgram(ProgramId::None);
	program_transform_dirty.set();
	scissor_state = Rml::Rectanglei::MakeInvalid();

	Gfx::CheckGLError("BeginFrame");
}

void RenderInterface_GL3_Recoil::EndFrame()
{
	const Gfx::FramebufferData& fb_active = render_layers.GetTopLayer();
	const Gfx::FramebufferData& fb_postprocess = render_layers.GetPostprocessPrimary();

	// Resolve MSAA to postprocess framebuffer.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb_active.framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_postprocess.framebuffer);

	glBlitFramebuffer(0, 0, fb_active.width, fb_active.height, 0, 0, fb_postprocess.width, fb_postprocess.height,
					  GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// Draw to backbuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Assuming we have an opaque background, we can just write to it with the premultiplied alpha blend mode and we'll get the correct result.
	// Instead, if we had a transparent destination that didn't use premultiplied alpha, we would need to perform a manual un-premultiplication step.
	glActiveTexture(GL_TEXTURE0);
	Gfx::BindTexture(fb_postprocess);
	UseProgram(ProgramId::Passthrough);
	DrawFullscreenQuad();

	render_layers.EndFrame();

	UseProgram(ProgramId::None);

	// Restore GL state.
	if (glstate_backup.enable_cull_face)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	if (glstate_backup.enable_blend)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	if (glstate_backup.enable_stencil_test)
		glEnable(GL_STENCIL_TEST);
	else
		glDisable(GL_STENCIL_TEST);

	if (glstate_backup.enable_scissor_test)
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);

	if (glstate_backup.enable_depth_test)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	glViewport(glstate_backup.viewport[0], glstate_backup.viewport[1], glstate_backup.viewport[2],
			   glstate_backup.viewport[3]);
	glScissor(glstate_backup.scissor[0], glstate_backup.scissor[1], glstate_backup.scissor[2],
			  glstate_backup.scissor[3]);

	glActiveTexture(glstate_backup.active_texture);

	glClearStencil(glstate_backup.stencil_clear_value);
	glClearColor(glstate_backup.color_clear_value[0], glstate_backup.color_clear_value[1],
				 glstate_backup.color_clear_value[2],
				 glstate_backup.color_clear_value[3]);
	glColorMask(glstate_backup.color_writemask[0], glstate_backup.color_writemask[1], glstate_backup.color_writemask[2],
				glstate_backup.color_writemask[3]);

	glBlendEquationSeparate(glstate_backup.blend_equation_rgb, glstate_backup.blend_equation_alpha);
	glBlendFuncSeparate(glstate_backup.blend_src_rgb, glstate_backup.blend_dst_rgb, glstate_backup.blend_src_alpha,
						glstate_backup.blend_dst_alpha);

	glStencilFuncSeparate(GL_FRONT, glstate_backup.stencil_front.func, glstate_backup.stencil_front.ref,
						  glstate_backup.stencil_front.value_mask);
	glStencilMaskSeparate(GL_FRONT, glstate_backup.stencil_front.writemask);
	glStencilOpSeparate(GL_FRONT, glstate_backup.stencil_front.fail, glstate_backup.stencil_front.pass_depth_fail,
						glstate_backup.stencil_front.pass_depth_pass);

	glStencilFuncSeparate(GL_BACK, glstate_backup.stencil_back.func, glstate_backup.stencil_back.ref,
						  glstate_backup.stencil_back.value_mask);
	glStencilMaskSeparate(GL_BACK, glstate_backup.stencil_back.writemask);
	glStencilOpSeparate(GL_BACK, glstate_backup.stencil_back.fail, glstate_backup.stencil_back.pass_depth_fail,
						glstate_backup.stencil_back.pass_depth_pass);

	Gfx::CheckGLError("EndFrame");
}

void RenderInterface_GL3_Recoil::Clear()
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
}

Rml::CompiledGeometryHandle
RenderInterface_GL3_Recoil::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	auto vao = std::make_unique<VAO>();
	auto vbo = std::make_unique<VBO>(GL_ARRAY_BUFFER);
	auto ibo = std::make_unique<VBO>(GL_ELEMENT_ARRAY_BUFFER);

	vao->Generate();
	vbo->Generate();
	ibo->Generate();

	vao->Bind();

	vbo->Bind();
	vbo->New(vertices.size() * sizeof(Rml::Vertex), GL_STATIC_DRAW, vertices.data());

	for (const AttributeDef& def: Gfx::VA_TYPE_RML_VERTEX::attributeDefs) {
		glEnableVertexAttribArray(def.index);
		glVertexAttribPointer(def.index, def.count, def.type, def.normalize, def.stride, def.data);
	}

	ibo->Bind();
	ibo->New(indices.size() * sizeof(int), GL_STATIC_DRAW, indices.data());

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return (Rml::CompiledGeometryHandle) new Gfx::CompiledGeometryData{
		std::move(vao), std::move(vbo), std::move(ibo), (GLsizei) indices.size()
	};
}

void RenderInterface_GL3_Recoil::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation,
												Rml::TextureHandle texture)
{
	auto* geometry = (Gfx::CompiledGeometryData*) handle;

	if (texture == TexturePostprocess) {
		// Do nothing.
	} else if (texture) {
		UseProgram(ProgramId::Texture);
		SubmitTransformUniform(translation);
		if (texture != TextureEnableWithoutBinding)
			glBindTexture(GL_TEXTURE_2D, (GLuint) texture);
	} else {
		UseProgram(ProgramId::Color);
		glBindTexture(GL_TEXTURE_2D, 0);
		SubmitTransformUniform(translation);
	}

	geometry->vao->Bind();
	glDrawElements(GL_TRIANGLES, geometry->num_indices, GL_UNSIGNED_INT, nullptr);
	geometry->vao->Unbind();
	
	if (texture != TexturePostprocess) {
		UseProgram(ProgramId::None);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	Gfx::CheckGLError("RenderCompiledGeometry");
}

void RenderInterface_GL3_Recoil::ReleaseGeometry(Rml::CompiledGeometryHandle handle)
{
	auto geometry = (Gfx::CompiledGeometryData*) handle;

	geometry->vao->Delete();
	geometry->vbo->Release();
	geometry->ibo->Release();

	delete geometry;
}

/// Flip vertical axis of the rectangle, and move its origin to the vertically opposite side of the viewport.
/// @note Changes coordinate system from RmlUi to OpenGL, or equivalently in reverse.
/// @note The Rectangle::Top and Rectangle::Bottom members will have reverse meaning in the returned rectangle.
static Rml::Rectanglei VerticallyFlipped(Rml::Rectanglei rect, int viewport_height)
{
	RMLUI_ASSERT(rect.Valid())
	Rml::Rectanglei flipped_rect = rect;
	flipped_rect.p0.y = viewport_height - rect.p1.y;
	flipped_rect.p1.y = viewport_height - rect.p0.y;
	return flipped_rect;
}

void RenderInterface_GL3_Recoil::SetScissor(Rml::Rectanglei region, bool vertically_flip)
{
	if (region.Valid() != scissor_state.Valid()) {
		if (region.Valid())
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
	}

	if (region.Valid() && vertically_flip)
		region = VerticallyFlipped(region, viewport_height);

	if (region.Valid() && region != scissor_state) {
		// Some render APIs don't like offscreen positions (WebGL in particular), so clamp them to the viewport.
		const int x = Rml::Math::Clamp(region.Left(), 0, viewport_width);
		const int y = Rml::Math::Clamp(viewport_height - region.Bottom(), 0, viewport_height);

		glScissor(x, y, region.Width(), region.Height());
	}

	Gfx::CheckGLError("SetScissorRegion");
	scissor_state = region;
}

void RenderInterface_GL3_Recoil::EnableScissorRegion(bool enable)
{
	// Assume enable is immediately followed by a SetScissorRegion() call, and ignore it here.
	if (!enable)
		SetScissor(Rml::Rectanglei::MakeInvalid(), false);
}

void RenderInterface_GL3_Recoil::SetScissorRegion(Rml::Rectanglei region)
{
	SetScissor(region);
}

void RenderInterface_GL3_Recoil::EnableClipMask(bool enable)
{
	if (enable)
		glEnable(GL_STENCIL_TEST);
	else
		glDisable(GL_STENCIL_TEST);
}

void
RenderInterface_GL3_Recoil::RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry,
											 Rml::Vector2f translation)
{
	RMLUI_ASSERT(glIsEnabled(GL_STENCIL_TEST))
	using Rml::ClipMaskOperation;

	const bool clear_stencil = (operation == ClipMaskOperation::Set || operation == ClipMaskOperation::SetInverse);
	if (clear_stencil) {
		// @performance Increment the reference value instead of clearing each time.
		glClear(GL_STENCIL_BUFFER_BIT);
	}

	GLint stencil_test_value = 0;
	glGetIntegerv(GL_STENCIL_REF, &stencil_test_value);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glStencilFunc(GL_ALWAYS, GLint(1), GLuint(-1));

	switch (operation) {
		case ClipMaskOperation::Set: {
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			stencil_test_value = 1;
		}
			break;
		case ClipMaskOperation::SetInverse: {
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			stencil_test_value = 0;
		}
			break;
		case ClipMaskOperation::Intersect: {
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
			stencil_test_value += 1;
		}
			break;
	}

	RenderGeometry(geometry, translation, {});

	// Restore state
	// @performance Cache state so we don't toggle it unnecessarily.
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, stencil_test_value, GLuint(-1));
}

Rml::TextureHandle RenderInterface_GL3_Recoil::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	CBitmap bmp;
	if (!bmp.Load(source)) {
		return false;
	}
	texture_dimensions.x = bmp.xsize;
	texture_dimensions.y = bmp.ysize;
	return bmp.CreateTexture();
}

Rml::TextureHandle
RenderInterface_GL3_Recoil::GenerateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions)
{
	GLuint texture_id = 0;
	glGenTextures(1, &texture_id);
	if (texture_id == 0) {
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to generate texture.");
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, source_dimensions.x, source_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				 source_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);

	return (Rml::TextureHandle) texture_id;
}

void RenderInterface_GL3_Recoil::DrawFullscreenQuad()
{
	RenderGeometry(fullscreen_quad_geometry, {}, RenderInterface_GL3_Recoil::TexturePostprocess);
}

void RenderInterface_GL3_Recoil::DrawFullscreenQuad(Rml::Vector2f uv_offset, Rml::Vector2f uv_scaling)
{
	Rml::Mesh mesh;
	Rml::MeshUtilities::GenerateQuad(mesh, Rml::Vector2f(-1), Rml::Vector2f(2), {});
	if (uv_offset != Rml::Vector2f() || uv_scaling != Rml::Vector2f(1.f)) {
		for (Rml::Vertex& vertex: mesh.vertices)
			vertex.tex_coord = (vertex.tex_coord * uv_scaling) + uv_offset;
	}
	const Rml::CompiledGeometryHandle geometry = CompileGeometry(mesh.vertices, mesh.indices);
	RenderGeometry(geometry, {}, RenderInterface_GL3_Recoil::TexturePostprocess);
	ReleaseGeometry(geometry);
}

static Rml::Colourf ConvertToColorf(Rml::ColourbPremultiplied c0)
{
	Rml::Colourf result;
	for (int i = 0; i < 4; i++)
		result[i] = (1.f / 255.f) * float(c0[i]);
	return result;
}

static void SigmaToParameters(const float desired_sigma, int& out_pass_level, float& out_sigma)
{
	constexpr int max_num_passes = 10;
	static_assert(max_num_passes < 31);
	constexpr float max_single_pass_sigma = 3.0f;
	out_pass_level = Rml::Math::Clamp(Rml::Math::Log2(int(desired_sigma * (2.f / max_single_pass_sigma))), 0,
									  max_num_passes);
	out_sigma = Rml::Math::Clamp(desired_sigma / float(1 << out_pass_level), 0.0f, max_single_pass_sigma);
}

static void
SetTexCoordLimits(Shader::IProgramObject* program, Rml::Rectanglei rectangle_flipped, Rml::Vector2i framebuffer_size)
{
	// Offset by half-texel values so that texture lookups are clamped to fragment centers, thereby avoiding color
	// bleeding from neighboring texels due to bilinear interpolation.
	const Rml::Vector2f min =
		(Rml::Vector2f(rectangle_flipped.p0) + Rml::Vector2f(0.5f)) / Rml::Vector2f(framebuffer_size);
	const Rml::Vector2f max =
		(Rml::Vector2f(rectangle_flipped.p1) - Rml::Vector2f(0.5f)) / Rml::Vector2f(framebuffer_size);

	program->SetUniform(Uniform::TexCoordMin, min.x, min.y);
	program->SetUniform(Uniform::TexCoordMax, max.x, max.y);
}

static void SetBlurWeights(Shader::IProgramObject* program, float sigma)
{
	constexpr int num_weights = BLUR_NUM_WEIGHTS;
	float weights[num_weights];
	float normalization = 0.0f;
	for (int i = 0; i < num_weights; i++) {
		if (Rml::Math::Absolute(sigma) < 0.1f)
			weights[i] = float(i == 0);
		else
			weights[i] = Rml::Math::Exp(-float(i * i) / (2.0f * sigma * sigma)) /
						 (Rml::Math::SquareRoot(2.f * Rml::Math::RMLUI_PI) * sigma);

		normalization += (i == 0 ? 1.f : 2.0f) * weights[i];
	}

	for (float& weight: weights)
		weight /= normalization;

	program->SetUniform1v(Uniform::Weights, num_weights, weights);
}

void RenderInterface_GL3_Recoil::RenderBlur(float sigma, const Gfx::FramebufferData& source_destination,
											const Gfx::FramebufferData& temp,
											const Rml::Rectanglei window_flipped)
{
	RMLUI_ASSERT(&source_destination != &temp && source_destination.width == temp.width &&
				 source_destination.height == temp.height)
	RMLUI_ASSERT(window_flipped.Valid())

	int pass_level = 0;
	SigmaToParameters(sigma, pass_level, sigma);

	const Rml::Rectanglei original_scissor = scissor_state;

	// Begin by downscaling so that the blur pass can be done at a reduced resolution for large sigma.
	Rml::Rectanglei scissor = window_flipped;

	UseProgram(ProgramId::Passthrough);
	SetScissor(scissor, true);

	// Downscale by iterative half-scaling with bilinear filtering, to reduce aliasing.
	glViewport(0, 0, source_destination.width / 2, source_destination.height / 2);

	// Scale UVs if we have even dimensions, such that texture fetches align perfectly between texels, thereby producing a 50% blend of
	// neighboring texels.
	const Rml::Vector2f uv_scaling = {
		(source_destination.width % 2 == 1) ? (1.f - 1.f / float(source_destination.width)) : 1.f,
		(source_destination.height % 2 == 1) ? (1.f - 1.f / float(source_destination.height)) : 1.f};

	for (int i = 0; i < pass_level; i++) {
		scissor.p0 = (scissor.p0 + Rml::Vector2i(1)) / 2;
		scissor.p1 = Rml::Math::Max(scissor.p1 / 2, scissor.p0);
		const bool from_source = (i % 2 == 0);
		Gfx::BindTexture(from_source ? source_destination : temp);
		glBindFramebuffer(GL_FRAMEBUFFER, (from_source ? temp : source_destination).framebuffer);
		SetScissor(scissor, true);

		DrawFullscreenQuad({}, uv_scaling);
	}

	glViewport(0, 0, source_destination.width, source_destination.height);

	// Ensure texture data end up in the temp buffer. Depending on the last downscaling, we might need to move it from the source_destination buffer.
	const bool transfer_to_temp_buffer = (pass_level % 2 == 0);
	if (transfer_to_temp_buffer) {
		Gfx::BindTexture(source_destination);
		glBindFramebuffer(GL_FRAMEBUFFER, temp.framebuffer);
		DrawFullscreenQuad();
	}

	// Set up uniforms.
	auto blur_prog = UseProgram(ProgramId::Blur);
	SetBlurWeights(blur_prog, sigma);
	SetTexCoordLimits(blur_prog, scissor, {source_destination.width, source_destination.height});

	auto SetTexelOffset = [&blur_prog](Rml::Vector2f blur_direction, int texture_dimension) {
		const Rml::Vector2f texel_offset = blur_direction * (1.0f / float(texture_dimension));
		blur_prog->SetUniform(Uniform::TexelOffset, texel_offset.x, texel_offset.y);
	};

	// Blur render pass - vertical.
	Gfx::BindTexture(temp);
	glBindFramebuffer(GL_FRAMEBUFFER, source_destination.framebuffer);

	SetTexelOffset({0.f, 1.f}, temp.height);
	DrawFullscreenQuad();

	// Blur render pass - horizontal.
	Gfx::BindTexture(source_destination);
	glBindFramebuffer(GL_FRAMEBUFFER, temp.framebuffer);

	// Add a 1px transparent border around the blur region by first clearing with a padded scissor. This helps prevent
	// artifacts when upscaling the blur result in the later step. On Intel and AMD, we have observed that during
	// blitting with linear filtering, pixels outside the 'src' region can be blended into the output. On the other
	// hand, it looks like Nvidia clamps the pixels to the source edge, which is what we really want. Regardless, we
	// work around the issue with this extra step.
	SetScissor(scissor.Extend(1), true);
	glClear(GL_COLOR_BUFFER_BIT);
	SetScissor(scissor, true);

	SetTexelOffset({1.f, 0.f}, source_destination.width);
	DrawFullscreenQuad();

	// Blit the blurred image to the scissor region with upscaling.
	SetScissor(window_flipped, true);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, temp.framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, source_destination.framebuffer);

	const Rml::Vector2i src_min = scissor.p0;
	const Rml::Vector2i src_max = scissor.p1;
	const Rml::Vector2i dst_min = window_flipped.p0;
	const Rml::Vector2i dst_max = window_flipped.p1;
	glBlitFramebuffer(src_min.x, src_min.y, src_max.x, src_max.y, dst_min.x, dst_min.y, dst_max.x, dst_max.y,
					  GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// The above upscale blit might be jittery at low resolutions (large pass levels). This is especially noticeable when moving an element with
	// backdrop blur around or when trying to click/hover an element within a blurred region since it may be rendered at an offset. For more stable
	// and accurate rendering we next upscale the blur image by an exact power-of-two. However, this may not fill the edges completely so we need to
	// do the above first. Note that this strategy may sometimes result in visible seams. Alternatively, we could try to enlarge the window to the
	// next power-of-two size and then downsample and blur that.
	const Rml::Vector2i target_min = src_min * (1 << pass_level);
	const Rml::Vector2i target_max = src_max * (1 << pass_level);
	if (target_min != dst_min || target_max != dst_max) {
		glBlitFramebuffer(src_min.x, src_min.y, src_max.x, src_max.y, target_min.x, target_min.y, target_max.x,
						  target_max.y, GL_COLOR_BUFFER_BIT,
						  GL_LINEAR);
	}

	// Restore render state.
	SetScissor(original_scissor);

	Gfx::CheckGLError("Blur");
}

void RenderInterface_GL3_Recoil::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	glDeleteTextures(1, (GLuint*) &texture_handle);
}

void RenderInterface_GL3_Recoil::SetTransform(const Rml::Matrix4f* new_transform)
{
	transform = (new_transform ? (projection * (*new_transform)) : projection);
	program_transform_dirty.set();
}

enum class FilterType
{
	Invalid = 0, Passthrough, Blur, DropShadow, ColorMatrix, MaskImage
};
struct CompiledFilter
{
	FilterType type;

	// Passthrough
	float blend_factor;

	// Blur
	float sigma;

	// Drop shadow
	Rml::Vector2f offset;
	Rml::ColourbPremultiplied color;

	// ColorMatrix
	Rml::Matrix4f color_matrix;
};

Rml::CompiledFilterHandle
RenderInterface_GL3_Recoil::CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters)
{
	CompiledFilter filter = {};

	if (name == "opacity") {
		filter.type = FilterType::Passthrough;
		filter.blend_factor = Rml::Get(parameters, "value", 1.0f);
	} else if (name == "blur") {
		filter.type = FilterType::Blur;
		filter.sigma = Rml::Get(parameters, "sigma", 1.0f);
	} else if (name == "drop-shadow") {
		filter.type = FilterType::DropShadow;
		filter.sigma = Rml::Get(parameters, "sigma", 0.f);
		filter.color = Rml::Get(parameters, "color", Rml::Colourb()).ToPremultiplied();
		filter.offset = Rml::Get(parameters, "offset", Rml::Vector2f(0.f));
	} else if (name == "brightness") {
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		filter.color_matrix = Rml::Matrix4f::Diag(value, value, value, 1.f);
	} else if (name == "contrast") {
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float grayness = 0.5f - 0.5f * value;
		filter.color_matrix = Rml::Matrix4f::Diag(value, value, value, 1.f);
		filter.color_matrix.SetColumn(3, Rml::Vector4f(grayness, grayness, grayness, 1.f));
	} else if (name == "invert") {
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Math::Clamp(Rml::Get(parameters, "value", 1.0f), 0.f, 1.f);
		const float inverted = 1.f - 2.f * value;
		filter.color_matrix = Rml::Matrix4f::Diag(inverted, inverted, inverted, 1.f);
		filter.color_matrix.SetColumn(3, Rml::Vector4f(value, value, value, 1.f));
	} else if (name == "grayscale") {
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float rev_value = 1.f - value;
		const Rml::Vector3f gray = value * Rml::Vector3f(0.2126f, 0.7152f, 0.0722f);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{gray.x + rev_value, gray.y, gray.z, 0.f},
			{gray.x, gray.y + rev_value, gray.z, 0.f},
			{gray.x, gray.y, gray.z + rev_value, 0.f},
			{0.f, 0.f, 0.f, 1.f}
		);
		// clang-format on
	} else if (name == "sepia") {
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float rev_value = 1.f - value;
		const Rml::Vector3f r_mix = value * Rml::Vector3f(0.393f, 0.769f, 0.189f);
		const Rml::Vector3f g_mix = value * Rml::Vector3f(0.349f, 0.686f, 0.168f);
		const Rml::Vector3f b_mix = value * Rml::Vector3f(0.272f, 0.534f, 0.131f);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{r_mix.x + rev_value, r_mix.y, r_mix.z, 0.f},
			{g_mix.x, g_mix.y + rev_value, g_mix.z, 0.f},
			{b_mix.x, b_mix.y, b_mix.z + rev_value, 0.f},
			{0.f, 0.f, 0.f, 1.f}
		);
		// clang-format on
	} else if (name == "hue-rotate") {
		// Hue-rotation and saturation values based on: https://www.w3.org/TR/filter-effects-1/#attr-valuedef-type-huerotate
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float s = Rml::Math::Sin(value);
		const float c = Rml::Math::Cos(value);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{0.213f + 0.787f * c - 0.213f * s, 0.715f - 0.715f * c - 0.715f * s, 0.072f - 0.072f * c + 0.928f * s, 0.f},
			{0.213f - 0.213f * c + 0.143f * s, 0.715f + 0.285f * c + 0.140f * s, 0.072f - 0.072f * c - 0.283f * s, 0.f},
			{0.213f - 0.213f * c - 0.787f * s, 0.715f - 0.715f * c + 0.715f * s, 0.072f + 0.928f * c + 0.072f * s, 0.f},
			{0.f, 0.f, 0.f, 1.f}
		);
		// clang-format on
	} else if (name == "saturate") {
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{0.213f + 0.787f * value, 0.715f - 0.715f * value, 0.072f - 0.072f * value, 0.f},
			{0.213f - 0.213f * value, 0.715f + 0.285f * value, 0.072f - 0.072f * value, 0.f},
			{0.213f - 0.213f * value, 0.715f - 0.715f * value, 0.072f + 0.928f * value, 0.f},
			{0.f, 0.f, 0.f, 1.f}
		);
		// clang-format on
	}

	if (filter.type != FilterType::Invalid)
		return reinterpret_cast<Rml::CompiledFilterHandle>(new CompiledFilter(std::move(filter)));

	Rml::Log::Message(Rml::Log::LT_WARNING, "Unsupported filter type '%s'.", name.c_str());
	return {};
}

void RenderInterface_GL3_Recoil::ReleaseFilter(Rml::CompiledFilterHandle filter)
{
	delete reinterpret_cast<CompiledFilter*>(filter);
}

enum class CompiledShaderType
{
	Invalid = 0, Gradient, Creation
};
struct CompiledShader
{
	CompiledShaderType type;

	// Gradient
	ShaderGradientFunction gradient_function;
	Rml::Vector2f p;
	Rml::Vector2f v;
	Rml::Vector<float> stop_positions;
	Rml::Vector<Rml::Colourf> stop_colors;

	// Shader
	Rml::Vector2f dimensions;
};

Rml::CompiledShaderHandle
RenderInterface_GL3_Recoil::CompileShader(const Rml::String& name, const Rml::Dictionary& parameters)
{
	auto ApplyColorStopList = [](CompiledShader& shader, const Rml::Dictionary& shader_parameters) {
		auto it = shader_parameters.find("color_stop_list");
		RMLUI_ASSERT(it != shader_parameters.end() && it->second.GetType() == Rml::Variant::COLORSTOPLIST)
		const auto& color_stop_list = it->second.GetReference<Rml::ColorStopList>();
		const int num_stops = Rml::Math::Min((int) color_stop_list.size(), MAX_NUM_STOPS);

		shader.stop_positions.resize(num_stops);
		shader.stop_colors.resize(num_stops);
		for (int i = 0; i < num_stops; i++) {
			const Rml::ColorStop& stop = color_stop_list[i];
			RMLUI_ASSERT(stop.position.unit == Rml::Unit::NUMBER)
			shader.stop_positions[i] = stop.position.number;
			shader.stop_colors[i] = ConvertToColorf(stop.color);
		}
	};

	CompiledShader shader = {};

	if (name == "linear-gradient") {
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (
			repeating ? ShaderGradientFunction::RepeatingLinear : ShaderGradientFunction::Linear
		);
		shader.p = Rml::Get(parameters, "p0", Rml::Vector2f(0.f));
		shader.v = Rml::Get(parameters, "p1", Rml::Vector2f(0.f)) - shader.p;
		ApplyColorStopList(shader, parameters);
	} else if (name == "radial-gradient") {
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (
			repeating ? ShaderGradientFunction::RepeatingRadial : ShaderGradientFunction::Radial
		);
		shader.p = Rml::Get(parameters, "center", Rml::Vector2f(0.f));
		shader.v = Rml::Vector2f(1.f) / Rml::Get(parameters, "radius", Rml::Vector2f(1.f));
		ApplyColorStopList(shader, parameters);
	} else if (name == "conic-gradient") {
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (repeating ? ShaderGradientFunction::RepeatingConic : ShaderGradientFunction::Conic);
		shader.p = Rml::Get(parameters, "center", Rml::Vector2f(0.f));
		const float angle = Rml::Get(parameters, "angle", 0.f);
		shader.v = {Rml::Math::Cos(angle), Rml::Math::Sin(angle)};
		ApplyColorStopList(shader, parameters);
	} else if (name == "shader") {
		const Rml::String value = Rml::Get(parameters, "value", Rml::String());
		if (value == "creation") {
			shader.type = CompiledShaderType::Creation;
			shader.dimensions = Rml::Get(parameters, "dimensions", Rml::Vector2f(0.f));
		}
	}

	if (shader.type != CompiledShaderType::Invalid)
		return reinterpret_cast<Rml::CompiledShaderHandle>(new CompiledShader(std::move(shader)));

	Rml::Log::Message(Rml::Log::LT_WARNING, "Unsupported shader type '%s'.", name.c_str());
	return {};
}

void RenderInterface_GL3_Recoil::RenderShader(Rml::CompiledShaderHandle shader_handle,
											  Rml::CompiledGeometryHandle geometry_handle,
											  Rml::Vector2f translation, Rml::TextureHandle /*texture*/)
{
	RMLUI_ASSERT(shader_handle && geometry_handle)
	const CompiledShader& shader = *reinterpret_cast<CompiledShader*>(shader_handle);
	const CompiledShaderType type = shader.type;
	const auto* geometry = (Gfx::CompiledGeometryData*) geometry_handle;

	switch (type) {
		case CompiledShaderType::Gradient: {
			RMLUI_ASSERT(shader.stop_positions.size() == shader.stop_colors.size())
			const int num_stops = (int) shader.stop_positions.size();

			auto gradient_prog = UseProgram(ProgramId::Gradient);
			gradient_prog->SetUniform(Uniform::Func, static_cast<int>(shader.gradient_function));
			gradient_prog->SetUniform(Uniform::P, shader.p.x, shader.p.y);
			gradient_prog->SetUniform(Uniform::V, shader.v.x, shader.v.y);
			gradient_prog->SetUniform(Uniform::NumStops, num_stops);
			gradient_prog->SetUniform1v(Uniform::StopPositions, num_stops, shader.stop_positions.data());
			gradient_prog->SetUniform4v(Uniform::StopColors, num_stops, (float*) &shader.stop_colors[0]);

			SubmitTransformUniform(translation);
			geometry->vao->Bind();
			glDrawElements(GL_TRIANGLES, geometry->num_indices, GL_UNSIGNED_INT, nullptr);
			geometry->vao->Unbind();
		}
			break;
		case CompiledShaderType::Creation: {
			const double time = Rml::GetSystemInterface()->GetElapsedTime();

			auto creation_prog = UseProgram(ProgramId::Creation);
			creation_prog->SetUniform(Uniform::Value, (float) time);
			creation_prog->SetUniform(Uniform::Dimensions, shader.dimensions.x, shader.dimensions.y);

			SubmitTransformUniform(translation);
			geometry->vao->Bind();
			glDrawElements(GL_TRIANGLES, geometry->num_indices, GL_UNSIGNED_INT, nullptr);
			geometry->vao->Unbind();
		}
			break;
		case CompiledShaderType::Invalid: {
			Rml::Log::Message(Rml::Log::LT_WARNING, "Unhandled render shader %d.", (int) type);
		}
			break;
	}

	Gfx::CheckGLError("RenderShader");
}

void RenderInterface_GL3_Recoil::ReleaseShader(Rml::CompiledShaderHandle shader_handle)
{
	delete reinterpret_cast<CompiledShader*>(shader_handle);
}

void RenderInterface_GL3_Recoil::BlitLayerToPostprocessPrimary(Rml::LayerHandle layer_handle)
{
	const Gfx::FramebufferData& source = render_layers.GetLayer(layer_handle);
	const Gfx::FramebufferData& destination = render_layers.GetPostprocessPrimary();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, source.framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.framebuffer);

	// Blit and resolve MSAA. Any active scissor state will restrict the size of the blit region.
	glBlitFramebuffer(0, 0, source.width, source.height, 0, 0, destination.width, destination.height,
					  GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void RenderInterface_GL3_Recoil::RenderFilters(Rml::Span<const Rml::CompiledFilterHandle> filter_handles)
{
	for (const Rml::CompiledFilterHandle filter_handle: filter_handles) {
		const CompiledFilter& filter = *reinterpret_cast<const CompiledFilter*>(filter_handle);
		const FilterType type = filter.type;

		switch (type) {
			case FilterType::Passthrough: {
				UseProgram(ProgramId::Passthrough);
				glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);
				glBlendColor(0.0f, 0.0f, 0.0f, filter.blend_factor);

				const Gfx::FramebufferData& source = render_layers.GetPostprocessPrimary();
				const Gfx::FramebufferData& destination = render_layers.GetPostprocessSecondary();
				Gfx::BindTexture(source);
				glBindFramebuffer(GL_FRAMEBUFFER, destination.framebuffer);

				DrawFullscreenQuad();

				render_layers.SwapPostprocessPrimarySecondary();
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			}
				break;
			case FilterType::Blur: {
				glDisable(GL_BLEND);

				const Gfx::FramebufferData& source_destination = render_layers.GetPostprocessPrimary();
				const Gfx::FramebufferData& temp = render_layers.GetPostprocessSecondary();

				const Rml::Rectanglei window_flipped = VerticallyFlipped(scissor_state, viewport_height);
				RenderBlur(filter.sigma, source_destination, temp, window_flipped);

				glEnable(GL_BLEND);
			}
				break;
			case FilterType::DropShadow: {
				auto drop_shadow_prog = UseProgram(ProgramId::DropShadow);
				glDisable(GL_BLEND);

				Rml::Colourf color = ConvertToColorf(filter.color);
				drop_shadow_prog->SetUniform4v(Uniform::Color, &color[0]);

				const Gfx::FramebufferData& primary = render_layers.GetPostprocessPrimary();
				const Gfx::FramebufferData& secondary = render_layers.GetPostprocessSecondary();
				Gfx::BindTexture(primary);
				glBindFramebuffer(GL_FRAMEBUFFER, secondary.framebuffer);

				const Rml::Rectanglei window_flipped = VerticallyFlipped(scissor_state, viewport_height);
				SetTexCoordLimits(drop_shadow_prog, window_flipped, {primary.width, primary.height});

				const Rml::Vector2f uv_offset =
					filter.offset / Rml::Vector2f(-(float) viewport_width, (float) viewport_height);
				DrawFullscreenQuad(uv_offset);

				if (filter.sigma >= 0.5f) {
					const Gfx::FramebufferData& tertiary = render_layers.GetPostprocessTertiary();
					RenderBlur(filter.sigma, secondary, tertiary, window_flipped);
				}

				UseProgram(ProgramId::Passthrough);
				BindTexture(primary);
				glEnable(GL_BLEND);
				DrawFullscreenQuad();

				render_layers.SwapPostprocessPrimarySecondary();
			}
				break;
			case FilterType::ColorMatrix: {
				auto color_matrix_prog = UseProgram(ProgramId::ColorMatrix);
				glDisable(GL_BLEND);

				constexpr bool transpose = std::is_same<decltype(filter.color_matrix), Rml::RowMajorMatrix4f>::value;
				color_matrix_prog->SetUniformMatrix4x4(Uniform::ColorMatrix, transpose, filter.color_matrix.data());

				const Gfx::FramebufferData& source = render_layers.GetPostprocessPrimary();
				const Gfx::FramebufferData& destination = render_layers.GetPostprocessSecondary();
				Gfx::BindTexture(source);
				glBindFramebuffer(GL_FRAMEBUFFER, destination.framebuffer);

				DrawFullscreenQuad();

				render_layers.SwapPostprocessPrimarySecondary();
				glEnable(GL_BLEND);
			}
				break;
			case FilterType::MaskImage: {
				UseProgram(ProgramId::BlendMask);
				glDisable(GL_BLEND);

				const Gfx::FramebufferData& source = render_layers.GetPostprocessPrimary();
				const Gfx::FramebufferData& blend_mask = render_layers.GetBlendMask();
				const Gfx::FramebufferData& destination = render_layers.GetPostprocessSecondary();

				Gfx::BindTexture(source);
				glActiveTexture(GL_TEXTURE1);
				Gfx::BindTexture(blend_mask);
				glActiveTexture(GL_TEXTURE0);

				glBindFramebuffer(GL_FRAMEBUFFER, destination.framebuffer);

				DrawFullscreenQuad();

				render_layers.SwapPostprocessPrimarySecondary();
				glEnable(GL_BLEND);
			}
				break;
			case FilterType::Invalid: {
				Rml::Log::Message(Rml::Log::LT_WARNING, "Unhandled render filter %d.", (int) type);
			}
				break;
		}
	}

	Gfx::CheckGLError("RenderFilter");
}

Rml::LayerHandle RenderInterface_GL3_Recoil::PushLayer()
{
	const Rml::LayerHandle layer_handle = render_layers.PushLayer();

	glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetLayer(layer_handle).framebuffer);
	glClear(GL_COLOR_BUFFER_BIT);

	return layer_handle;
}

void RenderInterface_GL3_Recoil::CompositeLayers(Rml::LayerHandle source_handle, Rml::LayerHandle destination_handle,
												 Rml::BlendMode blend_mode,
												 Rml::Span<const Rml::CompiledFilterHandle> filters)
{
	using Rml::BlendMode;

	// Blit source layer to postprocessing buffer. Do this regardless of whether we actually have any filters to be
	// applied, because we need to resolve the multi-sampled framebuffer in any case.
	// @performance If we have BlendMode::Replace and no filters or mask then we can just blit directly to the destination.
	BlitLayerToPostprocessPrimary(source_handle);

	// Render the filters, the PostprocessPrimary framebuffer is used for both input and output.
	RenderFilters(filters);

	// Render to the destination layer.
	glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetLayer(destination_handle).framebuffer);
	Gfx::BindTexture(render_layers.GetPostprocessPrimary());

	UseProgram(ProgramId::Passthrough);

	if (blend_mode == BlendMode::Replace)
		glDisable(GL_BLEND);

	DrawFullscreenQuad();

	if (blend_mode == BlendMode::Replace)
		glEnable(GL_BLEND);

	if (destination_handle != render_layers.GetTopLayerHandle())
		glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetTopLayer().framebuffer);

	Gfx::CheckGLError("CompositeLayers");
}

void RenderInterface_GL3_Recoil::PopLayer()
{
	render_layers.PopLayer();
	glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetTopLayer().framebuffer);
}

Rml::TextureHandle RenderInterface_GL3_Recoil::SaveLayerAsTexture()
{
	RMLUI_ASSERT(scissor_state.Valid());
	const Rml::Rectanglei bounds = scissor_state;

	Rml::TextureHandle render_texture = GenerateTexture({}, bounds.Size());
	if (!render_texture)
		return {};

	BlitLayerToPostprocessPrimary(render_layers.GetTopLayerHandle());

	EnableScissorRegion(false);

	const Gfx::FramebufferData& source = render_layers.GetPostprocessPrimary();
	const Gfx::FramebufferData& destination = render_layers.GetPostprocessSecondary();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, source.framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.framebuffer);
	
	// Flip the image vertically, as that convention is used for textures, and move to origin.
	glBlitFramebuffer(                                  //
		bounds.Left(), source.height - bounds.Bottom(), // src0
		bounds.Right(), source.height - bounds.Top(),   // src1
		0, bounds.Height(),                             // dst0
		bounds.Width(), 0,                              // dst1
		GL_COLOR_BUFFER_BIT, GL_NEAREST                 //
	);

	glBindTexture(GL_TEXTURE_2D, (GLuint) render_texture);

	const Gfx::FramebufferData& texture_source = destination;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, texture_source.framebuffer);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bounds.Width(), bounds.Height());

	SetScissor(bounds);
	glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetTopLayer().framebuffer);
	Gfx::CheckGLError("SaveLayerAsTexture");

	return render_texture;
}

Rml::CompiledFilterHandle RenderInterface_GL3_Recoil::SaveLayerAsMaskImage()
{
	BlitLayerToPostprocessPrimary(render_layers.GetTopLayerHandle());

	const Gfx::FramebufferData& source = render_layers.GetPostprocessPrimary();
	const Gfx::FramebufferData& destination = render_layers.GetBlendMask();

	glBindFramebuffer(GL_FRAMEBUFFER, destination.framebuffer);
	BindTexture(source);
	UseProgram(ProgramId::Passthrough);
	glDisable(GL_BLEND);

	DrawFullscreenQuad();

	glEnable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetTopLayer().framebuffer);
	Gfx::CheckGLError("SaveLayerAsMaskImage");

	CompiledFilter filter = {};
	filter.type = FilterType::MaskImage;
	return reinterpret_cast<Rml::CompiledFilterHandle>(new CompiledFilter(std::move(filter)));
}

Shader::IProgramObject* RenderInterface_GL3_Recoil::UseProgram(ProgramId program_id)
{
	RMLUI_ASSERT(program_data)
	if (active_program_id != program_id) {
		if (active_program_id != ProgramId::None)
			program_data->programs[active_program_id]->Disable();
		if (program_id != ProgramId::None)
			program_data->programs[program_id]->Enable();
		active_program_id = program_id;
	}

	return program_data->programs[active_program_id];
}

int RenderInterface_GL3_Recoil::GetUniformLocation(const char* name) const
{
	return glGetUniformLocation(program_data->programs[active_program_id]->GetObjID(), name);
}

void RenderInterface_GL3_Recoil::SubmitTransformUniform(Rml::Vector2f translation)
{
	static_assert((size_t) ProgramId::Count < MaxNumPrograms, "Maximum number of programs exceeded.");
	const auto program_index = (size_t) active_program_id;
	auto program = program_data->programs[active_program_id];

	if (program_transform_dirty.test(program_index)) {
		program->SetUniformMatrix4x4(Uniform::Transform, false, transform.data());
		program_transform_dirty.set(program_index, false);
	}

	program->SetUniform(Uniform::Translate, translation.x, translation.y);

	Gfx::CheckGLError("SubmitTransformUniform");
}

RenderInterface_GL3_Recoil::operator bool() const
{
	bool result = true;
	for (auto i = 1; i < (int) ProgramId::Count; i++) {
		auto prog = program_data->programs[(ProgramId) i];
		result = result && prog != nullptr && prog->IsValid();
	}
	return result;
}

RenderInterface_GL3_Recoil::RenderLayerStack::RenderLayerStack()
{
	fb_postprocess.resize(4);
}

RenderInterface_GL3_Recoil::RenderLayerStack::~RenderLayerStack()
{
	DestroyFramebuffers();
}

Rml::LayerHandle RenderInterface_GL3_Recoil::RenderLayerStack::PushLayer()
{
	RMLUI_ASSERT(layers_size <= (int) fb_layers.size())

	if (layers_size == (int) fb_layers.size()) {
		// All framebuffers should share a single stencil buffer.
		GLuint shared_depth_stencil = (fb_layers.empty() ? 0 : fb_layers.front().depth_stencil_buffer);

		fb_layers.push_back(Gfx::FramebufferData{});
		Gfx::CreateFramebuffer(fb_layers.back(), width, height, NUM_MSAA_SAMPLES,
							   Gfx::FramebufferAttachment::DepthStencil, shared_depth_stencil);
	}

	layers_size += 1;
	return GetTopLayerHandle();
}

void RenderInterface_GL3_Recoil::RenderLayerStack::PopLayer()
{
	RMLUI_ASSERT(layers_size > 0)
	layers_size -= 1;
}

const Gfx::FramebufferData& RenderInterface_GL3_Recoil::RenderLayerStack::GetLayer(Rml::LayerHandle layer) const
{
	RMLUI_ASSERT((size_t) layer < (size_t) layers_size)
	return fb_layers[layer];
}

const Gfx::FramebufferData& RenderInterface_GL3_Recoil::RenderLayerStack::GetTopLayer() const
{
	return GetLayer(GetTopLayerHandle());
}

Rml::LayerHandle RenderInterface_GL3_Recoil::RenderLayerStack::GetTopLayerHandle() const
{
	RMLUI_ASSERT(layers_size > 0)
	return static_cast<Rml::LayerHandle>(layers_size - 1);
}

void RenderInterface_GL3_Recoil::RenderLayerStack::SwapPostprocessPrimarySecondary()
{
	std::swap(fb_postprocess[0], fb_postprocess[1]);
}

void RenderInterface_GL3_Recoil::RenderLayerStack::BeginFrame(int new_width, int new_height)
{
	RMLUI_ASSERT(layers_size == 0)

	if (new_width != width || new_height != height) {
		width = new_width;
		height = new_height;

		DestroyFramebuffers();
	}

	PushLayer();
}

void RenderInterface_GL3_Recoil::RenderLayerStack::EndFrame()
{
	RMLUI_ASSERT(layers_size == 1)
	PopLayer();
}

void RenderInterface_GL3_Recoil::RenderLayerStack::DestroyFramebuffers()
{
	RMLUI_ASSERTMSG(layers_size == 0,
					"Do not call this during frame rendering, that is, between BeginFrame() and EndFrame().")

	for (Gfx::FramebufferData& fb: fb_layers)
		Gfx::DestroyFramebuffer(fb);

	fb_layers.clear();

	for (Gfx::FramebufferData& fb: fb_postprocess)
		Gfx::DestroyFramebuffer(fb);
}

const Gfx::FramebufferData& RenderInterface_GL3_Recoil::RenderLayerStack::EnsureFramebufferPostprocess(int index)
{
	RMLUI_ASSERT(index < (int) fb_postprocess.size())
	Gfx::FramebufferData& fb = fb_postprocess[index];
	if (!fb.framebuffer)
		Gfx::CreateFramebuffer(fb, width, height, 0, Gfx::FramebufferAttachment::None, 0);
	return fb;
}
