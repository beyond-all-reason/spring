#include "TextureCollection.h"

#include <algorithm>
#include <iterator>

#include "Rendering/GL/myGL.h"

#include "System/Misc/TracyDefs.h"

CTextureCollection::~CTextureCollection()
{
	RECOIL_DETAILED_TRACY_ZONE;
	glDeleteTextures(textureIDs.size(), textureIDs.data());
}

bool CTextureCollection::TextureExists(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = std::ranges::find(textureNames, name);
	return it != textureNames.end();
}

bool CTextureCollection::TextureExists(const std::string& name, const std::string& backupName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (TextureExists(name))
		return true;

	if (TextureExists(backupName))
		return true;

	return false;
}

uint32_t CTextureCollection::GetTextureID(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = std::ranges::find(textureNames, name);
	if (it == textureNames.end())
		return 0;

	return textureIDs[std::distance(textureNames.begin(), it)];
}

uint32_t CTextureCollection::GetTextureID(const std::string& name, const std::string& backupName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (auto texID = GetTextureID(name); texID != 0)
		return texID;

	if (backupName.empty())
		return 0;

	return GetTextureID(backupName);
}

size_t CTextureCollection::GetTexturePos(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = std::ranges::find(textureNames, name);
	if (it == textureNames.end())
		return INVALID_TEXTURE_POS;

	return std::distance(textureNames.begin(), it);
}

size_t CTextureCollection::GetTexturePos(const std::string& name, const std::string& backupName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (auto pos = GetTexturePos(name); pos != INVALID_TEXTURE_POS)
		return pos;

	if (backupName.empty())
		return INVALID_TEXTURE_POS;

	return GetTexturePos(backupName);
}

size_t CTextureCollection::AddTexFromFile(const std::string& name, const std::string& filename)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CBitmap bitmap;
	if (!bitmap.Load(filename))
		return INVALID_TEXTURE_POS;

	return AddTexFromBitmap(name, filename, bitmap);
}

size_t CTextureCollection::AddTexFromBitmap(const std::string& name, const std::string& filename, const CBitmap& bitmap)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto texID = bitmap.CreateMipMapTexture();
	if (texID == 0)
		return INVALID_TEXTURE_POS;

	textureIDs.emplace_back(texID);
	textureNames.emplace_back(name);
	texturePaths.emplace_back(filename);

	return textureIDs.size() - 1;
}

size_t CTextureCollection::AddTexBlank(std::string name, int xsize, int ysize, const SColor& c)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CBitmap bitmap;
	bitmap.AllocDummy(c);
	bitmap = bitmap.CreateRescaled(xsize, ysize);

	const auto texID = bitmap.CreateTexture();
	textureIDs.emplace_back(texID);
	textureNames.emplace_back(name);
	texturePaths.emplace_back("");

	return textureIDs.size() - 1;
}

bool CTextureCollection::DeleteTex(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = std::ranges::find(textureNames, name);
	if (it == textureNames.end())
		return false;

	const size_t pos = std::distance(textureNames.begin(), it);
	if (pos == textureNames.size() - 1) {
		textureNames.pop_back();
		texturePaths.pop_back();
		glDeleteTextures(1, &textureIDs.back());
		textureIDs.pop_back();
		return true;
	}

	textureNames[pos] = textureNames.back(); textureNames.pop_back();
	texturePaths[pos] = texturePaths.back(); texturePaths.pop_back();

	glDeleteTextures(1, &textureIDs[pos]);
	textureIDs[pos] = textureIDs.back(); textureIDs.pop_back();
	return true;
}

void CTextureCollection::Reload()
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(textureIDs.size() == textureNames.size() && textureNames.size() == texturePaths.size());
	for (size_t i = 0; i < textureIDs.size(); ++i) {
		if (texturePaths[i].empty())
			continue; //skip fallback textures

		CBitmap bitmap;
		if (!bitmap.Load(texturePaths[i]))
			continue; //skip missing texture files

		const auto texID = bitmap.CreateMipMapTexture(0.0f, 0.0f, 0, textureIDs[i]);
		if (texID != textureIDs[i]) {
			assert(false); //logic error, should never reach that point
			glDeleteTextures(1, &textureIDs[i]);
			textureIDs[i] = texID;
		}
	}
}
