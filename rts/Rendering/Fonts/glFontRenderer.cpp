#include "glFontRenderer.h"

#include "glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/Shader.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"

#include "System/Misc/TracyDefs.h"


////////////////////////////////////////
//can't be put in VFS due to initialization order
static constexpr const char* vsFont330 = R"(
#version 150 compatibility
#extension GL_ARB_explicit_attrib_location : enable

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 col;

out Data {
	vec4 vCol;
	vec2 vUV;
};

void main() {
	vCol = col;
	vUV  = uv;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0); // TODO: move to UBO
}
)";

static constexpr const char* fsFont330 = R"(
#version 150

uniform sampler2D tex;

in Data{
	vec4 vCol;
	vec2 vUV;
};

out vec4 outColor;

void main() {
	vec2 texSize = vec2(textureSize(tex, 0));

	float alpha = texture(tex, vUV / texSize).x;
	outColor = vec4(vCol.r, vCol.g, vCol.b, vCol.a * alpha);
}
)";

static constexpr const char* fsFontColor330 = R"(
#version 150

uniform sampler2D tex;

in Data{
	vec4 vCol;
	vec2 vUV;
};

out vec4 outColor;

void main() {
	vec2 texSize = vec2(textureSize(tex, 0));

	outColor = texture(tex, vUV / texSize);
	outColor = outColor*vCol;
}
)";


////////////////////////////////////////////

static constexpr const char* vsFont130 = R"(
#version 130

in vec3 pos;
in vec2 uv;
in vec4 col;

out vec4 vCol;
out vec2 vUV;

void main() {
	vCol = col;
	vUV  = uv;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0); // TODO: move to UBO
}
)";

static constexpr const char* fsFont130 = R"(
#version 130

uniform sampler2D tex;

in vec4 vCol;
in vec2 vUV;

void main() {
	vec2 texSize = vec2(textureSize(tex, 0));

	float alpha = texture(tex, vUV / texSize).x;
	gl_FragColor = vec4(vCol.r, vCol.g, vCol.b, vCol.a * alpha);
}
)";
static constexpr const char* fsFontColor130 = R"(
#version 130

uniform sampler2D tex;

in vec4 vCol;
in vec2 vUV;

void main() {
	vec2 texSize = vec2(textureSize(tex, 0));

	float4 col = texture(tex, vUV / texSize);
	gl_FragColor = vCol*col;
}
)";

////////////////////////////////////////////

CglShaderFontRenderer::CglShaderFontRenderer()
{
	RECOIL_DETAILED_TRACY_ZONE;
	primaryBufferTC = TypedRenderBuffer<VA_TYPE_TC>(NUM_TRI_BUFFER_VERTS, NUM_TRI_BUFFER_ELEMS, IStreamBufferConcept::SB_BUFFERSUBDATA);
	outlineBufferTC = TypedRenderBuffer<VA_TYPE_TC>(NUM_TRI_BUFFER_VERTS, NUM_TRI_BUFFER_ELEMS, IStreamBufferConcept::SB_BUFFERSUBDATA);

	++fontShaderRefs;

	if (fontShaderRefs > 1)
		return;

	// can't use shaderHandler here because it invalidates the objects on reload
	// but fonts are expected to be available all the time
	fontShader = std::make_unique<Shader::GLSLProgramObject>("[GL-Font]");
	fontShaderColor = std::make_unique<Shader::GLSLProgramObject>("[GL-Font]");

	LOG("[CglFont::%s] Creating Font shaders: GLEW_ARB_explicit_attrib_location = %s", __func__, globalRendering->supportExplicitAttribLoc ? "true" : "false");
	if (globalRendering->supportExplicitAttribLoc) {
		fontShader->AttachShaderObject(new Shader::GLSLShaderObject(GL_VERTEX_SHADER  , vsFont330));
		fontShader->AttachShaderObject(new Shader::GLSLShaderObject(GL_FRAGMENT_SHADER, fsFont330));
		fontShaderColor->AttachShaderObject(new Shader::GLSLShaderObject(GL_VERTEX_SHADER  , vsFont330));
		fontShaderColor->AttachShaderObject(new Shader::GLSLShaderObject(GL_FRAGMENT_SHADER, fsFontColor330));
	}
	else {
		fontShader->AttachShaderObject(new Shader::GLSLShaderObject(GL_VERTEX_SHADER  , vsFont130));
		fontShader->AttachShaderObject(new Shader::GLSLShaderObject(GL_FRAGMENT_SHADER, fsFont130));
		fontShader->BindAttribLocation("pos", 0);
		fontShader->BindAttribLocation("uv" , 1);
		fontShader->BindAttribLocation("col", 2);
		fontShaderColor->AttachShaderObject(new Shader::GLSLShaderObject(GL_VERTEX_SHADER  , vsFont130));
		fontShaderColor->AttachShaderObject(new Shader::GLSLShaderObject(GL_FRAGMENT_SHADER, fsFontColor130));
		fontShaderColor->BindAttribLocation("pos", 0);
		fontShaderColor->BindAttribLocation("uv" , 1);
		fontShaderColor->BindAttribLocation("col", 2);

	}
	fontShader->Link();
	fontShader->Enable();
	fontShader->SetUniform("tex", 0);
	fontShader->Disable();
	fontShader->Validate();
	assert(fontShader->IsValid());

	fontShaderColor->Link();
	fontShaderColor->Enable();
	fontShaderColor->SetUniform("tex", 0);
	fontShaderColor->Disable();
	fontShaderColor->Validate();
	assert(fontShaderColor->IsValid());
}

CglShaderFontRenderer::~CglShaderFontRenderer()
{
	RECOIL_DETAILED_TRACY_ZONE;
	--fontShaderRefs;
	if (fontShaderRefs > 0)
		return;

	fontShader = nullptr; // fontShader->Release() is called implicitly
	fontShaderColor = nullptr; // fontShader->Release() is called implicitly
}

void CglShaderFontRenderer::AddQuadTrianglesPB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl)
{
	RECOIL_DETAILED_TRACY_ZONE;
	primaryBufferTC.AddQuadTriangles(std::move(tl), std::move(tr), std::move(br), std::move(bl));
}

void CglShaderFontRenderer::AddQuadTrianglesOB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl)
{
	RECOIL_DETAILED_TRACY_ZONE;
	outlineBufferTC.AddQuadTriangles(std::move(tl), std::move(tr), std::move(br), std::move(bl));
}

void CglShaderFontRenderer::DrawTraingleElements()
{
	RECOIL_DETAILED_TRACY_ZONE;
	outlineBufferTC.DrawElements(GL_TRIANGLES);
	primaryBufferTC.DrawElements(GL_TRIANGLES);
}

void CglShaderFontRenderer::HandleTextureUpdate(CFontTexture& fnt, bool onlyUpload)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!onlyUpload)
		fnt.UpdateGlyphAtlasTexture();

	GLint dl = 0;
	glGetIntegerv(GL_LIST_INDEX, &dl);
	if (dl == 0) {
		fnt.UploadGlyphAtlasTextureImpl();
	}
}

void CglShaderFontRenderer::PushGLState(const CglFont& fnt)
{
	RECOIL_DETAILED_TRACY_ZONE;
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST); //just in case
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindTexture(GL_TEXTURE_2D, fnt.GetTexture());

	glGetIntegerv(GL_CURRENT_PROGRAM, &currProgID);

	if (fnt.HasColor()) {
		fontShaderColor->Enable();
	}
	else
		fontShader->Enable();
}

void CglShaderFontRenderer::PopGLState(const CglFont& fnt)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (fnt.HasColor())
		fontShaderColor->Disable();
	else
		fontShader->Disable();

	if (currProgID > 0)
		glUseProgram(currProgID);

	glBindTexture(GL_TEXTURE_2D, 0);

	glPopAttrib();
}

void CglShaderFontRenderer::GetStats(std::array<size_t, 8>& stats) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	stats[0 + 0] = primaryBufferTC.SumElems();
	stats[0 + 1] = primaryBufferTC.SumIndcs();
	stats[0 + 2] = primaryBufferTC.NumSubmits(false);
	stats[0 + 3] = primaryBufferTC.NumSubmits(true);

	stats[4 + 0] = outlineBufferTC.SumElems();
	stats[4 + 1] = outlineBufferTC.SumIndcs();
	stats[4 + 2] = outlineBufferTC.NumSubmits(false);
	stats[4 + 3] = outlineBufferTC.NumSubmits(true);
}

CglNoShaderFontRenderer::CglNoShaderFontRenderer()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (auto& v : verts)
		v.reserve(NUM_TRI_BUFFER_VERTS);
	for (auto& i : indcs)
		i.reserve(NUM_TRI_BUFFER_ELEMS);

	textureSpaceMatrix = glGenLists(1);
	glNewList(textureSpaceMatrix, GL_COMPILE);
	glEndList();
}

CglNoShaderFontRenderer::~CglNoShaderFontRenderer()
{
	RECOIL_DETAILED_TRACY_ZONE;
	glDeleteLists(textureSpaceMatrix, 1);
}

void CglNoShaderFontRenderer::AddQuadTrianglesImpl(bool primary, VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto& v = verts[primary];
	auto& i = indcs[primary];

	const uint16_t baseIndex = static_cast<uint16_t>(v.size());

	v.emplace_back(std::move(tl)); //0
	v.emplace_back(std::move(tr)); //1
	v.emplace_back(std::move(br)); //2
	v.emplace_back(std::move(bl)); //3

	//triangle 1 {tl, tr, bl}
	i.emplace_back(baseIndex + 3);
	i.emplace_back(baseIndex + 0);
	i.emplace_back(baseIndex + 1);

	//triangle 2 {bl, tr, br}
	i.emplace_back(baseIndex + 3);
	i.emplace_back(baseIndex + 1);
	i.emplace_back(baseIndex + 2);
}

void CglNoShaderFontRenderer::AddQuadTrianglesPB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl)
{
	RECOIL_DETAILED_TRACY_ZONE;
	AddQuadTrianglesImpl(true , std::move(tl), std::move(tr), std::move(br), std::move(bl));
}

void CglNoShaderFontRenderer::AddQuadTrianglesOB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl)
{
	RECOIL_DETAILED_TRACY_ZONE;
	AddQuadTrianglesImpl(false, std::move(tl), std::move(tr), std::move(br), std::move(bl));
}

void CglNoShaderFontRenderer::DrawTraingleElements()
{
	RECOIL_DETAILED_TRACY_ZONE;
	static constexpr GLsizei stride = sizeof(VA_TYPE_TC);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	for (size_t idx = 0; idx < 2; ++idx) {
		glVertexPointer(3, GL_FLOAT, stride, &verts[idx].data()->pos);
		glTexCoordPointer(2, GL_FLOAT, stride, &verts[idx].data()->s);
		glColorPointer(4, GL_UNSIGNED_BYTE, stride, &verts[idx].data()->c.r);
		glDrawRangeElements(GL_TRIANGLES, 0, verts[idx].size() - 1, indcs[idx].size(), GL_UNSIGNED_SHORT, indcs[idx].data());
	};

	for (auto& v : verts)
		v.clear();
	for (auto& i : indcs)
		i.clear();
}

void CglNoShaderFontRenderer::HandleTextureUpdate(CFontTexture& fnt, bool onlyUpload)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!onlyUpload)
		fnt.UpdateGlyphAtlasTexture();

	GLint dl = 0;
	glGetIntegerv(GL_LIST_INDEX, &dl);
	if (dl == 0) {
		fnt.UploadGlyphAtlasTextureImpl();

		// update texture space dlist (this affects already compiled dlists too!)
		glNewList(textureSpaceMatrix, GL_COMPILE);
		glScalef(1.0f / fnt.GetTextureWidth(), 1.0f / fnt.GetTextureHeight(), 1.0f);
		glEndList();
	}
}

void CglNoShaderFontRenderer::PushGLState(const CglFont& fnt)
{
	RECOIL_DETAILED_TRACY_ZONE;
	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glCallList(textureSpaceMatrix);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glBindTexture(GL_TEXTURE_2D, fnt.GetTexture());
}

void CglNoShaderFontRenderer::PopGLState(const CglFont& fnt)
{
	RECOIL_DETAILED_TRACY_ZONE;
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_TEXTURE_2D);
	glPopAttrib();
}

void CglNoShaderFontRenderer::GetStats(std::array<size_t, 8>& stats) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	/// placeholder
	std::fill(stats.begin(), stats.end(), 0);
}


std::unique_ptr<CglFontRenderer> CglFontRenderer::CreateInstance()
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	//return std::make_unique<CglNoShaderFontRenderer>();
	if (globalRendering->amdHacks)
		return std::make_unique<CglNoShaderFontRenderer>();

	auto fr = std::make_unique<CglShaderFontRenderer>();
	if (fr->IsValid())
		return fr;

	fr = nullptr;
	return std::make_unique<CglNoShaderFontRenderer>();
#else
	return std::make_unique<CglNullFontRenderer>();
#endif
}

void CglFontRenderer::DeleteInstance(std::unique_ptr<CglFontRenderer>& instance)
{
	RECOIL_DETAILED_TRACY_ZONE;
	instance = nullptr;
}

void CglNullFontRenderer::GetStats(std::array<size_t, 8>& stats) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::fill(stats.begin(), stats.end(), 0u);
}
