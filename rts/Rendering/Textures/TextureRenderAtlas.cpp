#include "TextureRenderAtlas.h"

#include <algorithm>

#include "LegacyAtlasAlloc.h"
#include "QuadtreeAtlasAlloc.h"
#include "RowAtlasAlloc.h"

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/TexBind.h"
#include "Rendering/GL/SubState.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/StringUtil.h"
#include "fmt/format.h"

#include <tracy/Tracy.hpp>

namespace {
static constexpr const char* vsTRA = R"(
#version 130

in vec2 pos;
in vec2 uv;

out vec2 vUV;

void main() {
	vUV  = uv;
	gl_Position = vec4(pos, 0.0, 1.0);
}
)";

static constexpr const char* fsTRA = R"(
#version 130

uniform sampler2D tex;
uniform float lod;

in vec2 vUV;
out vec4 outColor;

void main() {
	outColor = textureLod(tex, vUV, lod);
}
)";
};

CTextureRenderAtlas::CTextureRenderAtlas(
	CTextureAtlas::AllocatorType allocType_,
	int atlasSizeX_,
	int atlasSizeY_,
	uint32_t glInternalType_,
	const std::string& atlasName_
	)
	: atlasSizeX(atlasSizeX_)
	, atlasSizeY(atlasSizeY_)
	, allocType(allocType_)
	, glInternalType(glInternalType_)
	, texID(0)
	, atlasName(atlasName_)
	, finalized(false)
{
	//ZoneScoped;
	switch (allocType) {
		case CTextureAtlas::ATLAS_ALLOC_LEGACY:   { atlasAllocator = std::make_unique<  CLegacyAtlasAlloc>(); } break;
		case CTextureAtlas::ATLAS_ALLOC_QUADTREE: { atlasAllocator = std::make_unique<CQuadtreeAtlasAlloc>(); } break;
		case CTextureAtlas::ATLAS_ALLOC_ROW:      { atlasAllocator = std::make_unique<     CRowAtlasAlloc>(); } break;
		default:                                  {                                            assert(false); } break;
	}

	atlasSizeX = std::min(globalRendering->maxTextureSize, (atlasSizeX > 0) ? atlasSizeX : configHandler->GetInt("MaxTextureAtlasSizeX"));
	atlasSizeY = std::min(globalRendering->maxTextureSize, (atlasSizeY > 0) ? atlasSizeY : configHandler->GetInt("MaxTextureAtlasSizeY"));

	atlasAllocator->SetNonPowerOfTwo(globalRendering->supportNonPowerOfTwoTex);
	atlasAllocator->SetMaxSize(atlasSizeX, atlasSizeY);

	if (shaderRef == 0) {
		shader = shaderHandler->CreateProgramObject("[TextureRenderAtlas]", "TextureRenderAtlas");
		shader->AttachShaderObject(shaderHandler->CreateShaderObject(vsTRA, "", GL_VERTEX_SHADER));
		shader->AttachShaderObject(shaderHandler->CreateShaderObject(fsTRA, "", GL_FRAGMENT_SHADER));
		shader->BindAttribLocation("pos", 0);
		shader->BindAttribLocation("uv", 1);
		shader->Link();

		shader->Enable();
		shader->SetUniform("tex", 0);
		shader->SetUniform("lod", 0.0f);
		shader->Disable();
		shader->Validate();
	}

	shaderRef++;
}

CTextureRenderAtlas::~CTextureRenderAtlas()
{
	//ZoneScoped;
	shaderRef--;

	if (shaderRef == 0)
		shaderHandler->ReleaseProgramObjects("[TextureRenderAtlas]");

	for (auto& [_, tID] : nameToTexID) {
		if (tID) {
			glDeleteTextures(1, &tID);
			tID = 0;
		}
	}

	if (texID) {
		glDeleteTextures(1, &texID);
		texID = 0;
	}
}

bool CTextureRenderAtlas::TextureExists(const std::string& texName)
{
	//ZoneScoped;
	return finalized && nameToTexID.contains(texName);
}

bool CTextureRenderAtlas::TextureExists(const std::string& texName, const std::string& texBackupName)
{
	//ZoneScoped;
	return finalized && (nameToTexID.contains(texName) || nameToTexID.contains(texBackupName));
}

bool CTextureRenderAtlas::AddTexFromFile(const std::string& name, const std::string& file)
{
	//ZoneScoped;
	if (finalized)
		return false;

	if (nameToTexID.contains(name))
		return false;

	if (!CFileHandler::FileExists(name, SPRING_VFS_ALL))
		return false;

	CBitmap bm;
	if (!bm.Load(file))
		return false;

	return AddTexFromBitmapRaw(name, bm);
}

bool CTextureRenderAtlas::AddTexFromBitmap(const std::string& name, const CBitmap& bm)
{
	//ZoneScoped;
	if (finalized)
		return false;

	if (nameToTexID.contains(name))
		return false;

	return AddTexFromBitmapRaw(name, bm);
}


bool CTextureRenderAtlas::AddTexFromBitmapRaw(const std::string& name, const CBitmap& bm)
{
	//ZoneScoped;
	atlasAllocator->AddEntry(name, int2{ bm.xsize, bm.ysize });
	nameToTexID[name] = bm.CreateMipMapTexture();

	return true;
}


bool CTextureRenderAtlas::AddTex(const std::string& name, int xsize, int ysize, const SColor& color)
{
	//ZoneScoped;
	if (finalized)
		return false;

	if (nameToTexID.contains(name))
		return false;

	CBitmap bm;
	bm.AllocDummy(color);
	bm = bm.CreateRescaled(xsize, ysize);

	return AddTexFromBitmapRaw(name, bm);
}

AtlasedTexture CTextureRenderAtlas::GetTexture(const std::string& texName)
{
	//ZoneScoped;
	if (!finalized)
		return AtlasedTexture::DefaultAtlasTexture;

	if (!nameToTexID.contains(texName))
		return AtlasedTexture::DefaultAtlasTexture;

	return AtlasedTexture(atlasAllocator->GetTexCoords(texName));
}

AtlasedTexture CTextureRenderAtlas::GetTexture(const std::string& texName, const std::string& texBackupName)
{
	//ZoneScoped;
	if (!finalized)
		return AtlasedTexture::DefaultAtlasTexture;

	if (nameToTexID.contains(texName))
		return AtlasedTexture(atlasAllocator->GetTexCoords(texName));

	if (texBackupName.empty())
		return AtlasedTexture::DefaultAtlasTexture;

	return GetTexture(texBackupName);
}

uint32_t CTextureRenderAtlas::GetTexTarget() const
{
	//ZoneScoped;
	return GL_TEXTURE_2D;
}

int CTextureRenderAtlas::GetMinDim() const
{
	//ZoneScoped;
	return atlasAllocator->GetMinDim();
}

int CTextureRenderAtlas::GetNumTexLevels() const
{
	//ZoneScoped;
	return atlasAllocator->GetNumTexLevels();
}

void CTextureRenderAtlas::SetMaxTexLevel(int maxLevels)
{
	//ZoneScoped;
	atlasAllocator->SetMaxTexLevel(maxLevels);
}

bool CTextureRenderAtlas::Finalize()
{
	//ZoneScoped;
	if (finalized)
		return false;

	if (!FBO::IsSupported())
		return false;

	if (!atlasAllocator->Allocate())
		return false;

	int levels = atlasAllocator->GetNumTexLevels();

	const auto as = atlasAllocator->GetAtlasSize();
	{
		glGenTextures(1, &texID);
		auto texBind = GL::TexBind(GL_TEXTURE_2D, texID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (levels > 1) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glSpringTexStorage2D(GL_TEXTURE_2D, levels, glInternalType, as.x, as.y);
	}
	{
		using namespace GL::State;
		auto state = GL::SubState(
			DepthTest(GL_FALSE),
			Blending(GL_FALSE),
			DepthMask(GL_FALSE)
		);

		auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DT>();

		FBO fbo;
		fbo.Init(false);
		fbo.Bind();
		fbo.AttachTexture(texID, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0, 0);
		fbo.CheckStatus("TEXTURE-RENDER-ATLAS");
		finalized = fbo.IsValid();

		const auto Norm2SNorm = [](float value) { return (value * 2.0f - 1.0f); };

		for (uint32_t level = 0; finalized && (level < levels); ++level) {
			glViewport(0, 0, std::max(as.x >> level, 1), std::max(as.y >> level, 1));

			fbo.AttachTexture(texID, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0, level);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glReadBuffer(GL_COLOR_ATTACHMENT0);

			auto shEnToken = shader->EnableScoped();
			shader->SetUniform("lod", static_cast<float>(level));
			// draw
			for (auto& [name, entry] : atlasAllocator->GetEntries()) {
				const auto tc = atlasAllocator->GetTexCoords(name);

				VA_TYPE_2DT posTL = { .x = Norm2SNorm(tc.x1), .y = Norm2SNorm(tc.y1), .s = 0.0f, .t = 0.0f };
				VA_TYPE_2DT posTR = { .x = Norm2SNorm(tc.x2), .y = Norm2SNorm(tc.y1), .s = 1.0f, .t = 0.0f };
				VA_TYPE_2DT posBL = { .x = Norm2SNorm(tc.x1), .y = Norm2SNorm(tc.y2), .s = 0.0f, .t = 1.0f };
				VA_TYPE_2DT posBR = { .x = Norm2SNorm(tc.x2), .y = Norm2SNorm(tc.y2), .s = 1.0f, .t = 1.0f };

				const auto texID = nameToTexID[name];
				if (texID == 0)
					continue;

				auto texBind = GL::TexBind(GL_TEXTURE_2D, texID);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				rb.AddQuadTriangles(
					std::move(posTL),
					std::move(posTR),
					std::move(posBR),
					std::move(posBL)
				);

				rb.DrawElements(GL_TRIANGLES);
			}
		}

		fbo.DetachAll();
		FBO::Unbind();
		globalRendering->LoadViewport();
	}

	if (!finalized)
		return false;

	for (auto& [_, texID] : nameToTexID) {
		if (texID) {
			glDeleteTextures(1, &texID);
			texID = 0;
		}
	}

	//DumpTexture();

	return true;
}

bool CTextureRenderAtlas::DumpTexture() const
{
	//ZoneScoped;
	if (!finalized)
		return false;

	if (texID == 0)
		return false;

	int levels = atlasAllocator->GetNumTexLevels();

	for (uint32_t level = 0; level < levels; ++level) {
		glSaveTexture(texID, fmt::format("{}_{}.png", atlasName, level).c_str(), level);
	}

	return true;
}
