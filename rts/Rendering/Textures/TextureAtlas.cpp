/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TextureAtlas.h"

#include "Bitmap.h"
#include "LegacyAtlasAlloc.h"
#include "QuadtreeAtlasAlloc.h"
#include "RowAtlasAlloc.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/PBO.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/UnorderedSet.hpp"

#include <cstring>

#include <tracy/Tracy.hpp>

CONFIG(int, MaxTextureAtlasSizeX).defaultValue(4096).minimumValue(512).maximumValue(32768).description("The max X size of the projectile and Lua texture atlasses");
CONFIG(int, MaxTextureAtlasSizeY).defaultValue(4096).minimumValue(512).maximumValue(32768).description("The max Y size of the projectile and Lua texture atlasses");

CR_BIND(AtlasedTexture, )
CR_REG_METADATA(AtlasedTexture, (CR_IGNORED(x), CR_IGNORED(y), CR_IGNORED(z), CR_IGNORED(w)))


const AtlasedTexture& AtlasedTexture::DefaultAtlasTexture = AtlasedTexture{};
CTextureAtlas::CTextureAtlas(uint32_t allocType_, int32_t atlasSizeX_, int32_t atlasSizeY_, const std::string& name_, bool reloadable_)
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
	//ZoneScoped;
	if (freeTexture) {
		glDeleteTextures(1, &atlasTexID);
		atlasTexID = 0u;
	}

	memTextures.clear();
	files.clear();

	spring::SafeDelete(atlasAllocator);
}

void CTextureAtlas::ReinitAllocator()
{
	//ZoneScoped;
	spring::SafeDelete(atlasAllocator);

	switch (allocType) {
		case ATLAS_ALLOC_LEGACY:   { atlasAllocator = new   CLegacyAtlasAlloc(); } break;
		case ATLAS_ALLOC_QUADTREE: { atlasAllocator = new CQuadtreeAtlasAlloc(); } break;
		case ATLAS_ALLOC_ROW:      { atlasAllocator = new      CRowAtlasAlloc(); } break;
		default:                   {                              assert(false); } break;
	}

	// NB: maxTextureSize can be as large as 32768, resulting in a 4GB atlas
	atlasSizeX = std::min(globalRendering->maxTextureSize, (atlasSizeX > 0) ? atlasSizeX : configHandler->GetInt("MaxTextureAtlasSizeX"));
	atlasSizeY = std::min(globalRendering->maxTextureSize, (atlasSizeY > 0) ? atlasSizeY : configHandler->GetInt("MaxTextureAtlasSizeY"));

	atlasAllocator->SetNonPowerOfTwo(globalRendering->supportNonPowerOfTwoTex);
	atlasAllocator->SetMaxSize(atlasSizeX, atlasSizeY);
}

size_t CTextureAtlas::AddTex(std::string texName, int xsize, int ysize, TextureType texType)
{
	//ZoneScoped;
	memTextures.emplace_back();
	MemTex& tex = memTextures.back();

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
	//ZoneScoped;
	const size_t texIdx = AddTex(std::move(texName), xsize, ysize, texType);

	MemTex& tex = memTextures[texIdx];

	std::memcpy(tex.mem.data(), data, tex.mem.size());
	return texIdx;
}

size_t CTextureAtlas::AddTexFromFile(std::string texName, const std::string& file)
{
	//ZoneScoped;
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

	// only suport RGBA for now
	if (bitmap.channels != 4 || bitmap.compressed)
		throw content_error("Unsupported bitmap format in file " + file);

	return (files[lcFile] = AddTexFromMem(std::move(texName), bitmap.xsize, bitmap.ysize, RGBA32, bitmap.GetRawMem()));
}


bool CTextureAtlas::Finalize()
{
	//ZoneScoped;
	if (initialized && !reloadable)
		return true;

	const bool success = atlasAllocator->Allocate() && (initialized = CreateTexture());

	if (!reloadable) {
		memTextures.clear();
		files.clear();
	}

	return success;
}

const uint32_t CTextureAtlas::GetTexTarget() const
{
	//ZoneScoped;
	return GL_TEXTURE_2D; // just constant for now
}

int CTextureAtlas::GetNumTexLevels() const
{
	//ZoneScoped;
	return atlasAllocator->GetNumTexLevels();
}

void CTextureAtlas::SetMaxTexLevel(int maxLevels)
{
	//ZoneScoped;
	atlasAllocator->SetMaxTexLevel(maxLevels);
}

bool CTextureAtlas::CreateTexture()
{
	//ZoneScoped;
	const int2 atlasSize = atlasAllocator->GetAtlasSize();
	const int numLevels = atlasAllocator->GetNumTexLevels();

	// ATI drivers like to *crash* in glTexImage if x=0 or y=0
	if (atlasSize.x <= 0 || atlasSize.y <= 0) {
		LOG_L(L_ERROR, "[TextureAtlas::%s] bad allocation for atlas \"%s\" (size=<%d,%d>)", __func__, name.c_str(), atlasSize.x, atlasSize.y);
		return false;
	}

	PBO pbo;
	pbo.Bind();
	pbo.New(atlasSize.x * atlasSize.y * 4);

	unsigned char* data = reinterpret_cast<unsigned char*>(pbo.MapBuffer(GL_WRITE_ONLY));

	if (data != nullptr) {
		// make spacing between textures black transparent to avoid ugly lines with linear filtering
		std::memset(data, 0, atlasSize.x * atlasSize.y * 4);

		for (const MemTex& memTex: memTextures) {
			const float4 texCoords = atlasAllocator->GetTexCoords(memTex.names[0]);
			const float4 absCoords = atlasAllocator->GetEntry(memTex.names[0]);

			const int xpos = absCoords.x;
			const int ypos = absCoords.y;

			AtlasedTexture tex(texCoords);

			for (const auto& name: memTex.names) {
				textures[name] = std::move(tex); //make sure textures[name] gets only its guts replaced, so all pointers remain valid
			}

			for (int y = 0; y < memTex.ysize; ++y) {
				int* dst = ((int*)           data  ) + xpos + (ypos + y) * atlasSize.x;
				int* src = ((int*)memTex.mem.data()) +        (       y) * memTex.xsize;

				memcpy(dst, src, memTex.xsize * 4);
			}
		}

		if (debug) {
			CBitmap tex(data, atlasSize.x, atlasSize.y);
			tex.Save(name + "-" + IntToString(atlasSize.x) + "x" + IntToString(atlasSize.y) + ".png", true);
		}
	} else {
		LOG_L(L_ERROR, "[TextureAtlas::%s] failed to map PBO for atlas \"%s\" (size=<%d,%d>)", __func__, name.c_str(), atlasSize.x, atlasSize.y);
	}

	pbo.UnmapBuffer();

	if (atlasTexID == 0u) //make function re=entrant
		glGenTextures(1, &atlasTexID);

	glBindTexture(GL_TEXTURE_2D, atlasTexID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (numLevels > 1) ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  numLevels - 1);
	if (numLevels > 1) {
		glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, atlasSize.x, atlasSize.y, GL_RGBA, GL_UNSIGNED_BYTE, pbo.GetPtr()); //FIXME disable texcompression, PBO
	} else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasSize.x, atlasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pbo.GetPtr());
	}

	pbo.Invalidate();
	pbo.Unbind();
	pbo.Release();

	return (data != nullptr);
}


void CTextureAtlas::BindTexture()
{
	//ZoneScoped;
	glBindTexture(GL_TEXTURE_2D, atlasTexID);
}

bool CTextureAtlas::TextureExists(const std::string& name)
{
	//ZoneScoped;
	return (textures.find(StringToLower(name)) != textures.end());
}

const spring::unordered_map<std::string, IAtlasAllocator::SAtlasEntry>& CTextureAtlas::GetTextures() const
{
	//ZoneScoped;
	return atlasAllocator->GetEntries();
}

void CTextureAtlas::ReloadTextures()
{
	//ZoneScoped;
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

void CTextureAtlas::DumpTexture(const char* newFileName) const
{
	//ZoneScoped;
	if (!initialized)
		return;

	std::string filename = newFileName ? newFileName : name.c_str();
	filename += ".png";

	glSaveTexture(atlasTexID, filename.c_str());
}


AtlasedTexture& CTextureAtlas::GetTexture(const std::string& name)
{
	//ZoneScoped;
	if (TextureExists(name))
		return textures[StringToLower(name)];

	return const_cast<AtlasedTexture&>(AtlasedTexture::DefaultAtlasTexture);
}


AtlasedTexture& CTextureAtlas::GetTextureWithBackup(const std::string& name, const std::string& backupName)
{
	//ZoneScoped;
	if (TextureExists(name))
		return textures[StringToLower(name)];

	if (TextureExists(backupName))
		return textures[StringToLower(backupName)];

	return const_cast<AtlasedTexture&>(AtlasedTexture::DefaultAtlasTexture);
}

std::string CTextureAtlas::GetTextureName(AtlasedTexture* tex)
{
	//ZoneScoped;
	if (texToName.empty()) {
		for (auto& kv : textures)
			texToName[&kv.second] = kv.first;
	}
	const auto it = texToName.find(tex);
	return (it != texToName.end()) ? it->second : "";
}

int2 CTextureAtlas::GetSize() const {
	//ZoneScoped;
	return (atlasAllocator->GetAtlasSize());
}

