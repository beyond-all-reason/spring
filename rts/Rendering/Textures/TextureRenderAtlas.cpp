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

#include "System/Misc/TracyDefs.h"

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
	RECOIL_DETAILED_TRACY_ZONE;
	switch (allocType) {
		case CTextureAtlas::ATLAS_ALLOC_LEGACY:   { atlasAllocator = std::make_unique<  CLegacyAtlasAlloc>(); } break;
		case CTextureAtlas::ATLAS_ALLOC_QUADTREE: { atlasAllocator = std::make_unique<CQuadtreeAtlasAlloc>(); } break;
		case CTextureAtlas::ATLAS_ALLOC_ROW:      { atlasAllocator = std::make_unique<     CRowAtlasAlloc>(); } break;
		default:                                  {                                            assert(false); } break;
	}

	atlasSizeX = std::min(globalRendering->maxTextureSize, (atlasSizeX > 0) ? atlasSizeX : configHandler->GetInt("MaxTextureAtlasSizeX"));
	atlasSizeY = std::min(globalRendering->maxTextureSize, (atlasSizeY > 0) ? atlasSizeY : configHandler->GetInt("MaxTextureAtlasSizeY"));

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
	RECOIL_DETAILED_TRACY_ZONE;
	shaderRef--;

	if (shaderRef == 0)
		shaderHandler->ReleaseProgramObjects("[TextureRenderAtlas]");

	for (auto& [_, tID] : filenameToTexID) {
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
	RECOIL_DETAILED_TRACY_ZONE;
	return finalized && nameToUniqueSubTexStr.contains(texName);
}

bool CTextureRenderAtlas::TextureExists(const std::string& texName, const std::string& texBackupName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return finalized && (nameToUniqueSubTexStr.contains(texName) || nameToUniqueSubTexStr.contains(texBackupName));
}

bool CTextureRenderAtlas::AddTexFromFile(const std::string& name, const std::string& fileName, const float4& subTexCoords)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (finalized)
		return false;

	if (!filenameToTexID.contains(fileName) && !CFileHandler::FileExists(fileName, SPRING_VFS_ALL))
		return false;

	CBitmap bm;
	if (!bm.Load(fileName))
		return false;

	return AddTexFromBitmapRaw(name, fileName, bm, subTexCoords);
}

bool CTextureRenderAtlas::AddTexFromBitmap(const std::string& name, const std::string& refFileName, const CBitmap& bm, const float4& subTexCoords)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (finalized)
		return false;

	return AddTexFromBitmapRaw(name, refFileName, bm, subTexCoords);
}


bool CTextureRenderAtlas::AddTexFromBitmapRaw(const std::string& name, const std::string& refFileName, const CBitmap& bm, const float4& subTexCoords)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (!filenameToTexID.contains(refFileName)) {
		filenameToTexID.emplace(refFileName, bm.CreateMipMapTexture());
	}

	const auto uniqueSubTex = UniqueSubTexture(
		filenameToTexID[refFileName],
		subTexCoords
	);
	const auto uniqueSubTexStr = uniqueSubTex.GetName();

	if (!atlasAllocator->contains(uniqueSubTexStr)) {
		int2 subTexSize = {
			static_cast<int>(bm.xsize * (subTexCoords.z - subTexCoords.x)),
			static_cast<int>(bm.ysize * (subTexCoords.w - subTexCoords.y))
		};
		atlasAllocator->AddEntry(uniqueSubTexStr, subTexSize);
		uniqueSubTextureMap[uniqueSubTexStr] = uniqueSubTex;
	}

	return nameToUniqueSubTexStr.emplace(name, uniqueSubTexStr).second;
}


bool CTextureRenderAtlas::AddTex(const std::string& name, const std::string& refFileName, int xsize, int ysize, const SColor& color)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (finalized)
		return false;

	if (nameToUniqueSubTexStr.contains(name))
		return false;

	CBitmap bm;
	bm.AllocDummy(color);
	bm = bm.CreateRescaled(xsize, ysize);

	return AddTexFromBitmapRaw(name, refFileName, bm, float4(0.0f, 0.0f, 1.0f, 1.0f));
}

AtlasedTexture CTextureRenderAtlas::GetTexture(const std::string& texName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!finalized)
		return AtlasedTexture::DefaultAtlasTexture;

	auto it = nameToUniqueSubTexStr.find(texName);
	if (it == nameToUniqueSubTexStr.end())
		return AtlasedTexture::DefaultAtlasTexture;

	return AtlasedTexture(atlasAllocator->GetTexCoords(it->second));
}

AtlasedTexture CTextureRenderAtlas::GetTexture(const std::string& texName, const std::string& texBackupName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!finalized)
		return AtlasedTexture::DefaultAtlasTexture;

	auto it = nameToUniqueSubTexStr.find(texName);
	if (it != nameToUniqueSubTexStr.end())
		return AtlasedTexture(atlasAllocator->GetTexCoords(it->second));

	if (texBackupName.empty())
		return AtlasedTexture::DefaultAtlasTexture;

	return GetTexture(texBackupName);
}

std::vector<std::string> CTextureRenderAtlas::GetAllFileNames() const
{
	std::vector<std::string> fileNames;
	fileNames.reserve(filenameToTexID.size());
	for (const auto& [name, _] : filenameToTexID) {
		fileNames.emplace_back(name);
	}

	return fileNames;
}

uint32_t CTextureRenderAtlas::GetTexTarget() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return GL_TEXTURE_2D;
}

int CTextureRenderAtlas::GetMinDim() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return atlasAllocator->GetMinDim();
}

int CTextureRenderAtlas::GetNumTexLevels() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return atlasAllocator->GetNumTexLevels();
}

void CTextureRenderAtlas::SetMaxTexLevel(int maxLevels)
{
	RECOIL_DETAILED_TRACY_ZONE;
	atlasAllocator->SetMaxTexLevel(maxLevels);
}

bool CTextureRenderAtlas::Finalize()
{
	RECOIL_DETAILED_TRACY_ZONE;
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

		RecoilTexStorage2D(GL_TEXTURE_2D, levels, glInternalType, as.x, as.y);
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
			for (auto& [uniqTexName, entry] : atlasAllocator->GetEntries()) {
				const auto atlasedTexCoords = atlasAllocator->GetTexCoords(uniqTexName);
				const auto& [srcTexID, srcSubTC] = uniqueSubTextureMap[uniqTexName];

				if (srcTexID == 0)
					continue;

				VA_TYPE_2DT posTL = { .x = Norm2SNorm(atlasedTexCoords.x1), .y = Norm2SNorm(atlasedTexCoords.y1), .s = srcSubTC.x, .t = srcSubTC.y };
				VA_TYPE_2DT posTR = { .x = Norm2SNorm(atlasedTexCoords.x2), .y = Norm2SNorm(atlasedTexCoords.y1), .s = srcSubTC.z, .t = srcSubTC.y };
				VA_TYPE_2DT posBL = { .x = Norm2SNorm(atlasedTexCoords.x1), .y = Norm2SNorm(atlasedTexCoords.y2), .s = srcSubTC.x, .t = srcSubTC.w };
				VA_TYPE_2DT posBR = { .x = Norm2SNorm(atlasedTexCoords.x2), .y = Norm2SNorm(atlasedTexCoords.y2), .s = srcSubTC.z, .t = srcSubTC.w };

				auto texBind = GL::TexBind(GL_TEXTURE_2D, srcTexID);

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

	for (auto& [_, texID] : filenameToTexID) {
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
	RECOIL_DETAILED_TRACY_ZONE;
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

std::string CTextureRenderAtlas::UniqueSubTexture::GetName() const
{
	return fmt::format("{};{},{},{},{}", texID, subTexCoords.x1, subTexCoords.y1, subTexCoords.x2, subTexCoords.y2);
}
