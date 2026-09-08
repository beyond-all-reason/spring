/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IconHandler.h"

#include <algorithm>
#include <cassert>
#include <locale>
#include <cctype>
#include <cmath>

#include <fmt/format.h>

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderBuffers.h"
#include "System/Log/ILog.h"
#include "System/UnorderedSet.hpp"
#include "System/UnorderedMap.hpp"
#include "System/StringUtil.h"
#include "Lua/LuaParser.h"
#include "Textures/Bitmap.h"
#include "Textures/TextureRenderAtlas.h"
#include "System/Exceptions.h"

namespace icon {

CIconHandler iconHandler;

/******************************************************************************/
//
//  CIconHandler
//

void CIconHandler::Kill()
{
	defaultIconIdx = INVALID_ICON_INDEX;
	drawOrderIconCount = 0;

	glDeleteTextures(2, atlasTextureIDs.data());
	atlasTextureIDs = { 0 };
	atlasTextureSizes = { int2{0, 0}, int2{0, 0} };

	atlases = { nullptr };
	atlasNeedsUpdate = { false };

	iconsMap.clear();
	iconsData.clear();
}


void CIconHandler::DumpAtlasTextures(const std::string& fileExt) const
{
	if (atlasTextureIDs[0]) {
		for (int level = 0; level < DEFAULT_NUM_OF_TEXTURE_LEVELS; ++level) {
			glSaveTexture(atlasTextureIDs[0], fmt::format("IconsAtlas1-{}.{}", level, fileExt).c_str(), level);
		}
	}
	if (atlasTextureIDs[1]) {
		for (int level = 0; level < DEFAULT_NUM_OF_TEXTURE_LEVELS; ++level) {
			glSaveTexture(atlasTextureIDs[1], fmt::format("IconsAtlas2-{}.{}", level, fileExt).c_str(), level);
		}
	}
}

bool CIconHandler::UpdateAtlasData(size_t atlasIdx)
{
	auto& atlas = atlases[atlasIdx];
	if (atlas)
		return true;

	atlas = std::make_unique<CTextureRenderAtlas>(CTextureAtlas::ATLAS_ALLOC_LEGACY, 0, 0, DEFAULT_NUM_OF_TEXTURE_LEVELS, GL_RGBA8, IntToString(atlasIdx, "IconsAtlas_%i"));

	spring::unordered_set<std::string> invalidIcons;

	// Sort icon names to ensure deterministic ordering across runs
	std::vector<std::string> sortedIconNames;
	sortedIconNames.reserve(iconsMap.size());
	for (const auto& [iconName, _] : iconsMap) {
		sortedIconNames.push_back(iconName);
	}
	std::sort(sortedIconNames.begin(), sortedIconNames.end());

	for (const auto& iconName : sortedIconNames) {
		const auto iconIndex = iconsMap[iconName];
		const auto& iconData = iconsData[iconIndex];

		if (iconData.GetAtlasIndex() != atlasIdx)
			continue;

		if (!atlas->AddTexFromFile(iconName, iconData.GetFileName(), iconData.GetSrcTexCoords())) {
			LOG_L(L_WARNING, "[CIconHandler::%s] Failed to load icon=%s, bitmap=%s", __func__, iconName.c_str(), iconData.GetFileName().c_str());
			invalidIcons.emplace(iconName);
		}
	}

	// everything above was loaded from a single image atlas, no need to run CTextureRenderAtlas machinery
	if (auto allFiles = atlas->GetAllFileNames(); allFiles.size() == 1) {
		const auto& uniqueSubTexMap = atlas->GetUniqueSubTexMap();
		for (const auto& [iconName, uniqSubTexName] : atlas->GetNameToUniqueSubTexMap()) {
			auto it = uniqueSubTexMap.find(uniqSubTexName);
			auto& item = iconsData[*iconsMap.try_get(iconName)];
			item.SetTexCoords(AtlasedTexture{ it->second.subTexCoords, 0 });
		}
	}

	if (auto it = invalidIcons.find("default"); it != invalidIcons.end()) {
		bool res = atlas->AddTexFromFile("default", "bitmaps/defaultradardot.png");
		assert(res);
		invalidIcons.erase(it);
	}

	if (!atlas->CalculateAtlas()) {
		atlas = nullptr;
		return false;
	}

	for (const auto& [iconName, _] : atlas->GetNameToUniqueSubTexMap()) {
		auto& item = iconsData[*iconsMap.try_get(iconName)];
		item.SetTexCoords(atlas->GetTexture(iconName));
	}

	const auto& defTC = iconsData[defaultIconIdx].GetTexCoords();
	for (const auto& iconName : invalidIcons) {
		auto& item = iconsData[*iconsMap.try_get(iconName)];
		item.SetTexCoords(defTC);
	}

	return true;
}

bool CIconHandler::CreateAtlasTexture(size_t atlasIdx)
{
	auto& atlas = atlases[atlasIdx];

	if (!atlas)
		return false;

	if (auto allFiles = atlas->GetAllFileNames(); allFiles.size() == 1) {
		CBitmap bm;

		if (!bm.Load(*allFiles.begin()))
			return false;

		glDeleteTextures(1, &atlasTextureIDs[atlasIdx]);
		atlasTextureIDs[atlasIdx] = 0; // just in case
		atlasTextureIDs[atlasIdx] = bm.CreateMipMapTexture();
		atlasTextureSizes[atlasIdx] = int2(bm.xsize, bm.ysize);

		// CBitmap::CreateTexture defaults to GL_REPEAT, which is not what we want for atlas
		glBindTexture(GL_TEXTURE_2D, atlasTextureIDs[atlasIdx]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		return true;
	}

	if (!atlas->CreateAtlasTexture())
		return false;

	atlasTextureSizes[atlasIdx] = atlas->GetAtlasSize();

	if (atlasTextureIDs[atlasIdx]) {
		glDeleteTextures(1, &atlasTextureIDs[atlasIdx]);
		atlasTextureIDs[atlasIdx] = 0; // just in case
	}

	atlasTextureIDs[atlasIdx] = atlas->DisownTexture();
	atlas = nullptr;

	return true;
}

void CIconHandler::LoadIcons(const std::string& filename)
{
	LuaParser luaParser(filename, SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);

	if (!luaParser.Execute())
		throw content_error(fmt::format("{}: {}", filename, luaParser.GetErrorLog()));

	const LuaTable iconTypes = luaParser.GetRoot();

	std::vector<std::string> iconNames;
	iconTypes.GetKeys(iconNames);

	for (const auto& iconName : iconNames) {
		const auto iconTable = iconTypes.SubTable(iconName);
		AddIcon(0,
			iconName,
			iconTable.GetString("bitmap", ""),
			iconTable.GetFloat("size", 1.0f),
			iconTable.GetFloat("distance", 1.0f),
			iconTable.GetBool("radiusAdjust", false),
			iconTable.GetFloat("u0", 0.0f),
			iconTable.GetFloat("v0", 0.0f),
			iconTable.GetFloat("u1", 1.0f),
			iconTable.GetFloat("v1", 1.0f),
			iconTable.GetFloat("drawOrder", 0.0f)
		);
	}

	atlasNeedsUpdate.set(0);

	// force Update()
	Update();
}

bool CIconHandler::AddIcon(
	uint32_t atlasIndex,
	const std::string& iconName,
	const std::string& texFileName,
	float size,
	float distance,
	bool radAdj,
	float u0, float v0, float u1, float v1,
	float drawOrder
) {
	auto it = iconsMap.find(iconName);
	if (it != iconsMap.end()) {
		FreeIcon(iconName);
	} else {
		it = iconsMap.emplace(iconName, iconsData.size()).first;
		iconsData.emplace_back();
	}

	iconsData[it->second] = IconData{
		iconName,
		texFileName,
		size,
		distance,
		atlasIndex,
		radAdj,
		float4{u0, v0, u1, v1},
		drawOrder
	};

	drawOrderIconCount += (drawOrder != 0.0f);

	// do basic check instead of the full load procedure
	return CFileHandler::FileExists(texFileName, SPRING_VFS_ALL);
}


bool CIconHandler::FreeIcon(const std::string& iconName)
{
	if (iconName == "default")
		return false;

	auto it = iconsMap.find(iconName);

	if (it == iconsMap.end())
		return false;

	if (const auto& ai = iconsData[it->second].GetAtlasIndex(); ai > 0) {
		atlasNeedsUpdate.set(ai);
	}

	drawOrderIconCount -= (iconsData[it->second].GetDrawOrder() != 0.0f);

	iconsData[it->second] = IconData{};
	it->second = defaultIconIdx;

	return true;
}

void CIconHandler::Update()
{
	if (atlasNeedsUpdate.none())
		return;

	auto defIt = iconsMap.find("default");
	if (defIt == iconsMap.end()) {
		defIt = iconsMap.emplace("default", iconsData.size()).first;
		iconsData.emplace_back(
			"default",
			"bitmaps/defaultradardot.png",
			1.0f,
			1.0f,
			0,
			false,
			float4{
				0.0f,
				0.0f,
				1.0f,
				1.0f
			}
		);
	}

	defaultIconIdx = defIt->second;

#ifdef HEADLESS
	// Headless builds don't render icons; skip the atlas pipeline entirely.
	// Clearing the bits makes the atlasNeedsUpdate.none() check above short-circuit
	// all subsequent calls to Update().
	atlasNeedsUpdate.reset();
	return;
#endif

	for (size_t i = 0; i < atlasNeedsUpdate.size(); ++i) {
		if (!atlasNeedsUpdate.test(i))
			continue;

		if (UpdateAtlasData(i) && CreateAtlasTexture(i))
			atlasNeedsUpdate.reset(i);
	}
}

std::pair<bool, spring::unordered_map<std::string, size_t>::const_iterator> CIconHandler::FindIconIdx(const std::string& iconName) const
{
	const auto it = iconsMap.find(iconName);
	return std::make_pair(it != iconsMap.end(), it);
}

const IconData& CIconHandler::GetIconData(const std::string& iconName) const
{
	const auto* ptrIconIdx = iconsMap.try_get(iconName);
	assert(ptrIconIdx != nullptr && (*ptrIconIdx) < iconsData.size());
	return iconsData[*ptrIconIdx];
}

const IconData& CIconHandler::GetIconData(size_t iconIdx) const
{
	assert(iconIdx < iconsData.size());
	return iconsData[iconIdx];
}

size_t CIconHandler::GetIconIdx(const std::string& iconName) const
{
	if (const auto& [found, it] = FindIconIdx(iconName); found)
		return it->second;
	else
		return INVALID_ICON_INDEX;
}

size_t CIconHandler::GetIconIdxOrDefault(const std::string& iconName) const
{
	if (const auto& [found, it] = FindIconIdx(iconName); found)
		return it->second;
	else
		return defaultIconIdx;
}

}