#include "TextureRenderAtlas.h"

#include <algorithm>
#include <cmath>

#include "LegacyAtlasAlloc.h"
#include "QuadtreeAtlasAlloc.h"
#include "RowAtlasAlloc.h"
#include "MultiPageAtlasAlloc.hpp"

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
#include "System/Log/ILog.h"
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
uniform vec4 srcClamp;

in vec2 vUV;
out vec4 outColor;

void main() {
	outColor = textureLod(tex, clamp(vUV, srcClamp.xy, srcClamp.zw), lod);
}
)";
};

std::string CTextureRenderAtlas::UniqueSubTexture::GetName() const
{
	return fmt::format("{};{},{},{},{}", stableIdx, subTexCoords.x1, subTexCoords.y1, subTexCoords.x2, subTexCoords.y2);
}

CTextureRenderAtlas::CTextureRenderAtlas(
	CTextureAtlas::AllocatorType allocType_,
	int atlasSizeX,
	int atlasSizeY,
	int maxLevels,
	uint32_t glInternalType_,
	const std::string& atlasName_
	)
	: allocType(allocType_)
	, glInternalType(glInternalType_)
	, atlasName(atlasName_)
	, atlasFinalized(false)
	, atlasRendered(false)
{
	RECOIL_DETAILED_TRACY_ZONE;

	using MPLegacyAtlasAlloc = MultiPageAtlasAlloc<CLegacyAtlasAlloc>;
	using MPQuadtreeAtlasAlloc = MultiPageAtlasAlloc<CQuadtreeAtlasAlloc>;
	using MPRowAtlasAlloc = MultiPageAtlasAlloc<CRowAtlasAlloc>;

	static constexpr uint32_t MAX_TEXTURE_PAGES = 16;

	switch (allocType) {
		case CTextureAtlas::ATLAS_ALLOC_LEGACY:      { atlasAllocator = std::make_unique<   CLegacyAtlasAlloc>(                 ); } break;
		case CTextureAtlas::ATLAS_ALLOC_QUADTREE:    { atlasAllocator = std::make_unique< CQuadtreeAtlasAlloc>(                 ); } break;
		case CTextureAtlas::ATLAS_ALLOC_ROW:         { atlasAllocator = std::make_unique<      CRowAtlasAlloc>(                 ); } break;
		case CTextureAtlas::ATLAS_ALLOC_MP_LEGACY:   { atlasAllocator = std::make_unique<  MPLegacyAtlasAlloc>(MAX_TEXTURE_PAGES); } break;
		case CTextureAtlas::ATLAS_ALLOC_MP_QUADTREE: { atlasAllocator = std::make_unique<MPQuadtreeAtlasAlloc>(MAX_TEXTURE_PAGES); } break;
		case CTextureAtlas::ATLAS_ALLOC_MP_ROW:      { atlasAllocator = std::make_unique<     MPRowAtlasAlloc>(MAX_TEXTURE_PAGES); } break;
		default:                                     {                                                              assert(false); } break;
	}

	atlasSizeX = std::min<int>(globalRendering->maxTextureSize, (atlasSizeX > 0) ? atlasSizeX : configHandler->GetInt("MaxTextureAtlasSizeX"));
	atlasSizeY = std::min<int>(globalRendering->maxTextureSize, (atlasSizeY > 0) ? atlasSizeY : configHandler->GetInt("MaxTextureAtlasSizeY"));

	atlasAllocator->SetMaxSize(atlasSizeX, atlasSizeY);
	atlasAllocator->SetMaxTexLevel(maxLevels);

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
		shader->SetUniform("srcClamp", 0.0f, 0.0f, 1.0f, 1.0f);
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

	for (auto& [_, entry] : filenameToTexID) {
		if (entry.texID) {
			glDeleteTextures(1, &entry.texID);
			entry.texID = 0;
		}
	}

	atlasTex = nullptr;
}

bool CTextureRenderAtlas::TextureExists(const std::string& texName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto it = nameToUniqueSubTexStr.find(texName);
	if (it == nameToUniqueSubTexStr.end())
		return false;

	return atlasAllocator->contains(it->second);
}

bool CTextureRenderAtlas::TextureExists(const std::string& texName, const std::string& texBackupName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return TextureExists(texName) || TextureExists(texBackupName);
}

bool CTextureRenderAtlas::AddTexFromFile(const std::string& name, const std::string& fileName, const float4& subTexCoords)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (atlasFinalized)
		return false;

	// doesn't contain the texture already and can't find the file
	if (!filenameToTexID.contains(fileName) && !CFileHandler::FileExists(fileName, SPRING_VFS_ALL))
		return false;

	CBitmap bm;
	if (!bm.Load(fileName))
		return false;

	return AddTexFromBitmapRaw(name, bm, subTexCoords, fileName);
}

bool CTextureRenderAtlas::AddTexFromBitmap(const std::string& name, const CBitmap& bm, const std::string& refFileName, const float4& subTexCoords)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (atlasFinalized)
		return false;

	return AddTexFromBitmapRaw(name, bm, subTexCoords, refFileName);
}


bool CTextureRenderAtlas::AddTexFromBitmapRaw(const std::string& name, const CBitmap& bm, const float4& subTexCoords, const std::string& refFileName)
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto it = filenameToTexID.find(refFileName);
	if (it == filenameToTexID.end()) {
		// Assign stable index at insertion time so all icons sharing a file get the same index
		const uint32_t stableIdx = static_cast<uint32_t>(filenameToTexID.size());
		it = filenameToTexID.emplace(refFileName, FileTexEntry{ bm.CreateMipMapTexture(), stableIdx }).first;
	}

	const auto uniqueSubTex = UniqueSubTexture(
		it->second.texID,
		it->second.stableIdx,
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


bool CTextureRenderAtlas::AddTex(const std::string& name, int xsize, int ysize, const SColor& color, const std::string& refFileName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (atlasFinalized)
		return false;

	if (nameToUniqueSubTexStr.contains(name))
		return false;

	CBitmap bm;
	bm.AllocDummy(color);
	bm = bm.CreateRescaled(xsize, ysize);

	return AddTexFromBitmapRaw(name, bm, float4(0.0f, 0.0f, 1.0f, 1.0f), refFileName);
}

AtlasedTexture CTextureRenderAtlas::GetTexture(const std::string& texName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!atlasFinalized)
		return AtlasedTexture::DefaultAtlasTexture;

	auto it = nameToUniqueSubTexStr.find(texName);
	if (it == nameToUniqueSubTexStr.end())
		return AtlasedTexture::DefaultAtlasTexture;

	return AtlasedTexture(atlasAllocator->GetTexCoordsEdge(it->second));
}

AtlasedTexture CTextureRenderAtlas::GetTexture(const std::string& texName, const std::string& texBackupName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!atlasFinalized)
		return AtlasedTexture::DefaultAtlasTexture;

	auto it = nameToUniqueSubTexStr.find(texName);
	if (it != nameToUniqueSubTexStr.end())
		return AtlasedTexture(atlasAllocator->GetTexCoordsEdge(it->second));

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
	return (atlasAllocator->GetNumPages() > 1) ?
		GL_TEXTURE_2D_ARRAY :
		GL_TEXTURE_2D;
}

uint32_t CTextureRenderAtlas::GetTexID() const
{
	if (!atlasRendered)
		return 0;

	return atlasTex->GetId();
}

int CTextureRenderAtlas::GetMinDim() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return atlasAllocator->GetMinDim();
}

const uint2& CTextureRenderAtlas::GetAtlasSize() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return atlasAllocator->GetAtlasSize();
}

int CTextureRenderAtlas::GetNumTexLevels() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return atlasAllocator->GetNumTexLevels();
}

bool CTextureRenderAtlas::IsValid() const
{
	return atlasFinalized && atlasRendered;
}

uint32_t CTextureRenderAtlas::DisownTexture()
{
	if (!atlasRendered)
		return 0;

	return atlasTex->DisOwn();
}

bool CTextureRenderAtlas::DumpTexture(const std::string& fileExt) const
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (!IsValid()) {
		LOG_L(L_ERROR, "[CTextureRenderAtlas::%s] Can't dump invalid %s atlas", __func__, atlasName.c_str());
		return false;
	}
	const auto numLevels = atlasAllocator->GetNumTexLevels();
	const auto numPages = atlasAllocator->GetNumPages();

	if (numPages > 1) {
		for (uint32_t page = 0; page < numPages; ++page) {
			for (uint32_t level = 0; level < numLevels; ++level) {
				glSaveTextureArray(atlasTex->GetId(), fmt::format("{}_{}_{}.{}", atlasName, page, level, fileExt).c_str(), level, page);
			}
		}
	}
	else {
		for (uint32_t level = 0; level < numLevels; ++level) {
			glSaveTexture(atlasTex->GetId(), fmt::format("{}_{}.{}", atlasName, level, fileExt).c_str(), level);
		}
	}

	return true;
}

bool CTextureRenderAtlas::CalculateAtlas()
{
	if (atlasFinalized)
		return true;

	atlasFinalized = atlasAllocator->Allocate();
	LOG_L(L_INFO, "CTextureRenderAtlas::%s() atlas=%s atlasFinalized=%d", __func__, atlasName.c_str(), atlasFinalized);
	return atlasFinalized;
}

bool CTextureRenderAtlas::CreateAtlasTexture()
{
	if (!atlasFinalized)
		return true;

	if (atlasRendered)
		return true;

	if (!FBO::IsReady())
		return false;

	LOG_L(L_INFO, "CTextureRenderAtlas::%s()[0] atlas=%s FBO::ready=%d", __func__, atlasName.c_str(), FBO::IsReady());

	const auto numLevels = atlasAllocator->GetNumTexLevels();
	const auto numPages = atlasAllocator->GetNumPages();

	const auto& atlasSize = atlasAllocator->GetAtlasSize();

	{
		GL::TextureCreationParams tcp{
			//make function re-entrant
			.texID = atlasTex ? atlasTex->GetId() : 0,
			.reqNumLevels = numLevels,
			.linearMipMapFilter = true,
			.linearTextureFilter = true,
			.wrapMirror = false
		};

		atlasTex = nullptr;

		if (numPages > 1) {
			atlasTex = std::make_unique<GL::Texture2DArray>(atlasSize, numPages, glInternalType, tcp, true);
		}
		else {
			atlasTex = std::make_unique<GL::Texture2D     >(atlasSize, glInternalType, tcp, true);
		}
	}

#ifdef HEADLESS
	// Skip OpenGL rendering in headless mode
	atlasRendered = true;
	return true;
#endif

	{
		using namespace GL::State;
		auto state = GL::SubState(
			DepthTest(GL_FALSE),
			Blending(GL_FALSE),
			DepthMask(GL_FALSE)
		);

		FBO fbo;
		fbo.Init(false);
		fbo.Bind();
		if (numPages > 1)
			fbo.AttachTextureLayer(atlasTex->GetId(), GL_COLOR_ATTACHMENT0, 0, 0);
		else
			fbo.AttachTexture(atlasTex->GetId(), GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0, 0);
		fbo.CheckStatus("TEXTURE-RENDER-ATLAS");

		atlasRendered = (fbo.IsValid() && atlasTex->GetId() > 0);

		if (atlasRendered) {
			auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DT>();

			for (uint32_t page = 0; page < numPages; ++page) {
				for (uint32_t level = 0; level < numLevels; ++level) {
					glViewport(0, 0, std::max(atlasSize.x >> level, 1u), std::max(atlasSize.y >> level, 1u));

					if (numPages > 1)
						fbo.AttachTextureLayer(atlasTex->GetId(), GL_COLOR_ATTACHMENT0, level, page);
					else
						fbo.AttachTexture(atlasTex->GetId(), GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0, level);

					glDrawBuffer(GL_COLOR_ATTACHMENT0);
					glReadBuffer(GL_COLOR_ATTACHMENT0);

					auto shEnToken = shader->EnableScoped();
					// Always sample from LOD 0 of the source texture and let the
					// viewport shrinking handle downsampling. This prevents inter-texel
					// bleed when a single source image contains multiple sub-textures
					// (e.g. icon sprite sheets where glGenerateMipmap averages across
					// icon boundaries).
					shader->SetUniform("lod", 0.0f);

					// Pixel-snapped mip-level rendering:
					// At mip levels > 0, reusing level-0 normalized coords can place quad vertices
					// at fractional pixel positions (e.g. 9.75px at level 2). The diagonal triangle
					// split then rasterizes inconsistently with GL_NEAREST, baking a seam into the
					// atlas mip texture. Fix: compute integer pixel coords per mip level.
					const uint32_t mipW = std::max(atlasSize.x >> level, 1u);
					const uint32_t mipH = std::max(atlasSize.y >> level, 1u);
					const float invMipW = 1.0f / static_cast<float>(mipW);
					const float invMipH = 1.0f / static_cast<float>(mipH);
					const uint32_t scale = 1u << level;
					// At the highest LOD level, don't expand the destination rect into
					// the padding zone. The padding is too thin (sub-mip-pixel) and
					// floor/ceil rounding causes destination rects of neighboring icons
					// to overlap, writing one icon's edge color over another's.
					const int halfPad = (level < numLevels - 1) ? (atlasAllocator->GetPadding() / 2) : 0;

					// draw
					for (auto& [uniqTexName, entry] : atlasAllocator->GetEntries()) {
						if (entry.texCoords.pageNum != page)
							continue;

						const auto& [srcTexID, _stableIdx, srcSubTC] = uniqueSubTextureMap[uniqTexName];

						if (srcTexID == 0)
							continue;

						// Raw inclusive pixel coords from allocator (level-0 space)
						const int px1 = static_cast<int>(entry.texCoords.x1);
						const int py1 = static_cast<int>(entry.texCoords.y1);
						const int px2 = static_cast<int>(entry.texCoords.x2); // inclusive
						const int py2 = static_cast<int>(entry.texCoords.y2); // inclusive

						// Entry size in level-0 pixels (exclusive width = inclusive + 1)
						const float entryW = static_cast<float>(px2 - px1 + 1);
						const float entryH = static_cast<float>(py2 - py1 + 1);

						// Destination rect in mip-level pixels, snapped to integers
						// floor for left/top, ceil for right/bottom ensures full coverage
						const int dstX1 = std::max(static_cast<int>(std::floor(static_cast<float>(px1 + 0 - halfPad) / static_cast<float>(scale))),                      0);
						const int dstY1 = std::max(static_cast<int>(std::floor(static_cast<float>(py1 + 0 - halfPad) / static_cast<float>(scale))),                      0);
						const int dstX2 = std::min(static_cast<int>(std::ceil (static_cast<float>(px2 + 1 + halfPad) / static_cast<float>(scale))), static_cast<int>(mipW));
						const int dstY2 = std::min(static_cast<int>(std::ceil (static_cast<float>(py2 + 1 + halfPad) / static_cast<float>(scale))), static_cast<int>(mipH));

						// Convert integer mip-level pixels to NDC [-1, 1]
						const float ndcX1 = 2.0f * static_cast<float>(dstX1) * invMipW - 1.0f;
						const float ndcY1 = 2.0f * static_cast<float>(dstY1) * invMipH - 1.0f;
						const float ndcX2 = 2.0f * static_cast<float>(dstX2) * invMipW - 1.0f;
						const float ndcY2 = 2.0f * static_cast<float>(dstY2) * invMipH - 1.0f;

						// Derive source UVs from snapped destination rect
						// Maps mip-level pixel boundaries back to source texture coordinates
						const float srcW = srcSubTC.z - srcSubTC.x;
						const float srcH = srcSubTC.w - srcSubTC.y;
						const float srcU1 = srcSubTC.x + (static_cast<float>(dstX1 * static_cast<int>(scale) - px1) / entryW) * srcW;
						const float srcV1 = srcSubTC.y + (static_cast<float>(dstY1 * static_cast<int>(scale) - py1) / entryH) * srcH;
						const float srcU2 = srcSubTC.x + (static_cast<float>(dstX2 * static_cast<int>(scale) - px1) / entryW) * srcW;
						const float srcV2 = srcSubTC.y + (static_cast<float>(dstY2 * static_cast<int>(scale) - py1) / entryH) * srcH;

						auto posTL = VA_TYPE_2DT{ .x = ndcX1, .y = ndcY1, .s = srcU1, .t = srcV1 };
						auto posTR = VA_TYPE_2DT{ .x = ndcX2, .y = ndcY1, .s = srcU2, .t = srcV1 };
						auto posBL = VA_TYPE_2DT{ .x = ndcX1, .y = ndcY2, .s = srcU1, .t = srcV2 };
						auto posBR = VA_TYPE_2DT{ .x = ndcX2, .y = ndcY2, .s = srcU2, .t = srcV2 };

						auto texBind = GL::TexBind(GL_TEXTURE_2D, srcTexID);
						shader->SetUniform("srcClamp", srcSubTC.x, srcSubTC.y, srcSubTC.z, srcSubTC.w);

						// Use GL_LINEAR to avoid GL_NEAREST ambiguity at texel boundaries.
						// At mip levels > 0, for odd-positioned atlas entries, the source UV can land
						// exactly halfway between two source LOD-N texels. GL_NEAREST is undefined there;
						// floating-point UV interpolation differences between the two triangles can
						// then pick different texels, baking a diagonal seam into the atlas mip.
						// GL_LINEAR gives the same result as NEAREST when UVs hit texel centers (level 0)
						// and smooth blending otherwise (levels 1+), eliminating the seam.
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
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
			}
		}

		fbo.DetachAll();
		FBO::Unbind();
		globalRendering->LoadViewport();
	}

	LOG_L(L_INFO, "CTextureRenderAtlas::%s()[1] atlas=%s atlasRendered=%d", __func__, atlasName.c_str(), atlasRendered);

	if (!atlasRendered)
		return false;

	for (auto& [_, entry] : filenameToTexID) {
		if (entry.texID) {
			glDeleteTextures(1, &entry.texID);
			entry.texID = 0;
		}
	}

	return true;
}
