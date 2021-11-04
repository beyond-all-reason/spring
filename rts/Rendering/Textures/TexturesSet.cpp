#include "TexturesSet.h"

#include <cassert>
#include "Rendering/GlobalRendering.h"

TexturesSet::TexturesSet(const std::initializer_list<std::pair<uint8_t, uint32_t>>& texturesList)
{
	for (const auto& [relBinding, textureID] : texturesList) {
		assert(textureID  < MAX_TEXTURE_ID);
		assert(relBinding < CGlobalRendering::MAX_TEXTURE_UNITS);
		auto [it, result] = textures.try_emplace(relBinding, textureID);
		assert(result);
	}
}

TexturesSet::TexturesSet(const std::initializer_list<uint32_t>& texturesList) {
	uint8_t relBinding = 0u;

	for (const auto& textureID : texturesList) {
		assert(textureID < MAX_TEXTURE_ID);
		assert(relBinding < CGlobalRendering::MAX_TEXTURE_UNITS);
		auto [it, result] = textures.try_emplace(relBinding, textureID);
		assert(result);

		++relBinding;
	}
}

TexturesSet& TexturesSet::operator=(const TexturesSet& rhs)
{
	this->textures = rhs.textures;
	this->perfectHash = rhs.perfectHash;

	return *this;
}

TexturesSet& TexturesSet::operator=(TexturesSet&& rhs) noexcept
{
	std::swap(this->textures, rhs.textures);
	this->perfectHash = rhs.perfectHash;

	return *this;
}

void TexturesSet::UpsertTexture(uint8_t relBinding, uint32_t textureID)
{
	assert(textureID  < MAX_TEXTURE_ID);
	assert(relBinding < CGlobalRendering::MAX_TEXTURE_UNITS);
	textures[relBinding] = textureID;
	perfectHash = 0;
}

bool TexturesSet::RemoveTexture(uint8_t relBinding)
{
	assert(relBinding < CGlobalRendering::MAX_TEXTURE_UNITS);
	perfectHash = 0;
	return (textures.erase(relBinding) > 0);
}

std::size_t TexturesSet::GetHash() const
{
	if (textures.empty())
		return 0;

	if (perfectHash == 0) {
		for (const auto& [relBinding, textureID] : textures) {
			perfectHash += relBinding * MAX_TEXTURE_ID + textureID;
		}
	}

	return perfectHash;
}