/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TextureAtlas.h"

#include "Bitmap.h"
#include "LegacyAtlasAlloc.h"
#include "QuadtreeAtlasAlloc.h"
#include "RowAtlasAlloc.h"
#include "MultiPageAtlasAlloc.hpp"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/UnorderedSet.hpp"

#include <cstring>
#include <fmt/format.h>

#include "System/Misc/TracyDefs.h"

CONFIG(int, MaxTextureAtlasSizeX).defaultValue(4096).minimumValue(512).maximumValue(32768).description("The max X size of the projectile and Lua texture atlasses");
CONFIG(int, MaxTextureAtlasSizeY).defaultValue(4096).minimumValue(512).maximumValue(32768).description("The max Y size of the projectile and Lua texture atlasses");

CTextureAtlas::CTextureAtlas(uint32_t allocType_, uint32_t atlasSizeX_, uint32_t atlasSizeY_, const std::string& name_, bool reloadable_)
	: allocType{ allocType_ }
	, atlasSizeX{ atlasSizeX_ }
	, atlasSizeY{ atlasSizeY_ }
	, reloadable{ reloadable_ }
	, name{ name_ }
{

	textures.reserve(256);
	memTextures.reserve(128);
	ReinitAllocator();
}

CTextureAtlas::~CTextureAtlas()
{
	RECOIL_DETAILED_TRACY_ZONE;

	memTextures.clear();
	files.clear();

	spring::SafeDelete(atlasAllocator);
}

void CTextureAtlas::ReinitAllocator()
{
	RECOIL_DETAILED_TRACY_ZONE;
	spring::SafeDelete(atlasAllocator);

	using MPLegacyAtlasAlloc   = MultiPageAtlasAlloc<CLegacyAtlasAlloc>;
	using MPQuadtreeAtlasAlloc = MultiPageAtlasAlloc<CQuadtreeAtlasAlloc>;
	using MPRowAtlasAlloc      = MultiPageAtlasAlloc<CRowAtlasAlloc>;

	static constexpr uint32_t MAX_TEXTURE_PAGES = 16;

	switch (allocType) {
		case ATLAS_ALLOC_LEGACY      : { atlasAllocator = new    CLegacyAtlasAlloc(                 ); } break;
		case ATLAS_ALLOC_QUADTREE    : { atlasAllocator = new  CQuadtreeAtlasAlloc(                 ); } break;
		case ATLAS_ALLOC_ROW         : { atlasAllocator = new       CRowAtlasAlloc(                 ); } break;
		case ATLAS_ALLOC_MP_LEGACY   : { atlasAllocator = new   MPLegacyAtlasAlloc(MAX_TEXTURE_PAGES); } break;
		case ATLAS_ALLOC_MP_QUADTREE : { atlasAllocator = new MPQuadtreeAtlasAlloc(MAX_TEXTURE_PAGES); } break;
		case ATLAS_ALLOC_MP_ROW      : { atlasAllocator = new      MPRowAtlasAlloc(MAX_TEXTURE_PAGES); } break;
		default:                       {                               assert(false); } break;
	}

	// NB: maxTextureSize can be as large as 32768, resulting in a 4GB atlas
	atlasSizeX = std::min(static_cast<uint32_t>(globalRendering->maxTextureSize), (atlasSizeX > 0) ? atlasSizeX : static_cast<uint32_t>(configHandler->GetInt("MaxTextureAtlasSizeX")));
	atlasSizeY = std::min(static_cast<uint32_t>(globalRendering->maxTextureSize), (atlasSizeY > 0) ? atlasSizeY : static_cast<uint32_t>(configHandler->GetInt("MaxTextureAtlasSizeY")));

	atlasAllocator->SetMaxSize(atlasSizeX, atlasSizeY);
}

size_t CTextureAtlas::AddTex(std::string texName, int xsize, int ysize, TextureType texType)
{
	RECOIL_DETAILED_TRACY_ZONE;
	MemTex& tex = memTextures.emplace_back();

	tex.xsize = xsize;
	tex.ysize = ysize;
	tex.texType = texType;
	tex.mem.resize((xsize * ysize * GetBPP(texType)) / 8, 0);

	StringToLowerInPlace(texName);
	tex.names.emplace_back(std::move(texName));

	atlasAllocator->AddEntry(tex.names.back(), int2(xsize, ysize));

	return (memTextures.size() - 1);
}

size_t CTextureAtlas::AddTexFromMem(std::string texName, int xsize, int ysize, TextureType texType, const void* data)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const size_t texIdx = AddTex(std::move(texName), xsize, ysize, texType);

	MemTex& tex = memTextures[texIdx];

	std::memcpy(tex.mem.data(), data, tex.mem.size());
	return texIdx;
}

size_t CTextureAtlas::AddTexFromFile(std::string texName, const std::string& file)
{
	RECOIL_DETAILED_TRACY_ZONE;
	StringToLowerInPlace(texName);

	// if the file is already loaded, use that instead
	const std::string& lcFile = StringToLower(file);

	if (const auto it = files.find(lcFile); it != files.end()) {
		memTextures[it->second].names.emplace_back(texName);
		return (it->second);
	}

	CBitmap bitmap;
	if (!bitmap.Load(file)) {
		bitmap.Alloc(2, 2, 4);
		LOG_L(L_WARNING, "[TexAtlas::%s] could not load texture from file \"%s\"", __func__, file.c_str());
	}

	// only support RGBA for now
	if (bitmap.channels != 4 || bitmap.compressed)
		throw content_error("Unsupported bitmap format in file " + file);

	return (files[lcFile] = AddTexFromMem(std::move(texName), bitmap.xsize, bitmap.ysize, RGBA32, bitmap.GetRawMem()));
}


bool CTextureAtlas::Finalize()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (initialized && !reloadable)
		return true;

	const bool success = atlasAllocator->Allocate() && (initialized = CreateTexture());

	if (!reloadable) {
		memTextures.clear();
		files.clear();
	}

	return success;
}

uint32_t CTextureAtlas::GetTexTarget() const
{
	return (atlasAllocator->GetNumPages() > 1) ?
		GL_TEXTURE_2D_ARRAY :
		GL_TEXTURE_2D;
}

uint32_t CTextureAtlas::GetNumPages() const
{
	return atlasAllocator->GetNumPages();
}

int CTextureAtlas::GetNumTexLevels() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return atlasAllocator->GetNumTexLevels();
}

void CTextureAtlas::SetMaxTexLevel(int maxLevels)
{
	RECOIL_DETAILED_TRACY_ZONE;
	atlasAllocator->SetMaxTexLevel(maxLevels);
}

bool CTextureAtlas::CreateTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int2 atlasSize = atlasAllocator->GetAtlasSize();
	const int numLevels = atlasAllocator->GetNumTexLevels();
	const auto numPages = atlasAllocator->GetNumPages();

	// ATI drivers like to *crash* in glTexImage if x=0 or y=0
	if (atlasSize.x <= 0 || atlasSize.y <= 0 || numPages == 0) {
		LOG_L(L_ERROR, "[TextureAtlas::%s] bad allocation for atlas \"%s\" (size=<%d,%d,%u>)", __func__, name.c_str(), atlasSize.x, atlasSize.y, numPages);
		return false;
	}

	// make spacing between textures black transparent to avoid ugly lines with linear filtering
	std::vector<std::vector<uint8_t>> atlasPages(
		numPages,
		std::vector<uint8_t>(atlasSize.x * atlasSize.y * 4, 0)
	);

	for (const MemTex& memTex: memTextures) {
		auto it = atlasAllocator->FindEntry(memTex.names.front());
		const auto& pixCoords = atlasAllocator->GetEntry(it);

		if (pixCoords.pageNum > numPages)
			continue;

		auto texCoords = atlasAllocator->GetTexCoordsCntr(it);
		const int xpos = static_cast<int>(pixCoords.x);
		const int ypos = static_cast<int>(pixCoords.y);

		for (const auto& name: memTex.names) {
			textures[name] = texCoords;
		}

		auto& atlasPage = atlasPages[pixCoords.pageNum];

		for (int y = 0; y < memTex.ysize; ++y) {
			int* dst = ((int*) atlasPage.data()) + xpos + (ypos + y) * atlasSize.x;
			int* src = ((int*)memTex.mem.data()) +        (       y) * memTex.xsize;

			memcpy(dst, src, memTex.xsize * 4);
		}
	}

	if (debug) {
		for (auto atIt = atlasPages.begin(); atIt != atlasPages.end(); ++atIt) {
			CBitmap tex(atIt->data(), atlasSize.x, atlasSize.y);
			tex.Save(fmt::format("{}-{}-{}x{}.png", name, std::distance(atlasPages.begin(), atIt), atlasSize.x, atlasSize.y), true);
		}
	}

	GL::TextureCreationParams tcp {
		//make function re-entrant
		.texID = atlasTex ? atlasTex->GetId() : 0,
		.reqNumLevels = numLevels,
		.linearMipMapFilter = true,
		.linearTextureFilter = true,
		.wrapMirror = false
	};

	if (numPages > 1) {
		atlasTex = std::make_unique<GL::Texture2DArray>(atlasSize, numPages, GL_RGBA8, tcp, true);
		auto binding = atlasTex->ScopedBind();
		const auto* atlasTexTyped = static_cast<GL::Texture2DArray*>(atlasTex.get());
		for (uint32_t pageNum = 0; pageNum < numPages; ++pageNum) {
			atlasTexTyped->UploadImage(atlasPages[pageNum].data(), pageNum);
		}
		atlasTexTyped->ProduceMipmaps();
	}
	else {
		atlasTex = std::make_unique<GL::Texture2D     >(atlasSize, GL_RGBA8, tcp, true);
		auto binding = atlasTex->ScopedBind();
		const auto* atlasTexTyped = static_cast<GL::Texture2D*     >(atlasTex.get());
		atlasTexTyped->UploadImage(atlasPages.front().data());
		atlasTexTyped->ProduceMipmaps();
	}

	return atlasTex && (atlasTex->GetId() > 0);
}


void CTextureAtlas::BindTexture()
{
	if (!initialized)
		return;

	atlasTex->Bind();
}

void CTextureAtlas::UnbindTexture()
{
	if (!initialized)
		return;

	atlasTex->Unbind();
}

bool CTextureAtlas::TextureExists(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (textures.find(StringToLower(name)) != textures.end());
}

const spring::unordered_map<std::string, IAtlasAllocator::SAtlasEntry>& CTextureAtlas::GetTextures() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return atlasAllocator->GetEntries();
}

void CTextureAtlas::ReloadTextures()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!reloadable) {
		LOG_L(L_ERROR, "[CTextureAtlas::%s] Attempting to reload non-reloadable texture atlas name=\"%s\"", __func__, name.c_str());
		return;
	}

	ReinitAllocator();

	spring::unordered_set<size_t> nonFileEntries;
	for (size_t i = 0; i < memTextures.size(); ++i) {
		nonFileEntries.emplace(i);
	}

	for (const auto& [filename, idx] : files) {
		assert(idx < memTextures.size());
		nonFileEntries.erase(idx);
		auto& memTex = memTextures[idx];

		CBitmap bitmap;
		if (!bitmap.Load(filename)) {
			LOG_L(L_WARNING, "[TexAtlas::%s] could not reload texture from file \"%s\"", __func__, filename.c_str());
			bitmap.Alloc(2, 2, 4);
			bitmap.Fill(SColor(1.0f, 0.0f, 0.0f, 1.0f));
		}

		memTex.xsize = bitmap.xsize;
		memTex.ysize = bitmap.ysize;
		memTex.texType = RGBA32;
		memTex.mem.resize(bitmap.GetMemSize(), 0);
		std::memcpy(memTex.mem.data(), bitmap.GetRawMem(), memTex.mem.size());

		for (const auto& texName : memTex.names) {
			atlasAllocator->AddEntry(texName, int2(memTex.xsize, memTex.ysize));
		}
	}

	for (auto idx : nonFileEntries) {
		const auto& memTex = memTextures[idx];
		for (const auto& texName : memTex.names) {
			atlasAllocator->AddEntry(texName, int2(memTex.xsize, memTex.ysize));
		}
	}

	Finalize();
}

void CTextureAtlas::DumpTexture(const char* newFileName, const std::string& fileExt) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!initialized)
		return;

	std::string filename = newFileName ? newFileName : name.c_str();

	const auto numPages = atlasAllocator->GetNumPages();

	if (numPages > 1) {
		for (uint32_t page = 0; page < numPages; ++page) {
			glSaveTextureArray(atlasTex->GetId(), fmt::format("{}_{}.{}", filename, page, fileExt).c_str(), page);
		}
	}
	else {
		filename += "." + fileExt;
		glSaveTexture(atlasTex->GetId(), filename.c_str());
	}
}


AtlasedTexture& CTextureAtlas::GetTexture(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (TextureExists(name))
		return textures[StringToLower(name)];

	return const_cast<AtlasedTexture&>(AtlasedTexture::DefaultAtlasTexture);
}


AtlasedTexture& CTextureAtlas::GetTextureWithBackup(const std::string& name, const std::string& backupName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (TextureExists(name))
		return textures[StringToLower(name)];

	if (TextureExists(backupName))
		return textures[StringToLower(backupName)];

	return const_cast<AtlasedTexture&>(AtlasedTexture::DefaultAtlasTexture);
}

std::string CTextureAtlas::GetTextureName(AtlasedTexture* tex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (texToName.empty()) {
		for (auto& kv : textures)
			texToName[&kv.second] = kv.first;
	}
	const auto it = texToName.find(tex);
	return (it != texToName.end()) ? it->second : "";
}

int2 CTextureAtlas::GetSize() const {
	RECOIL_DETAILED_TRACY_ZONE;
	return (atlasAllocator->GetAtlasSize());
}

