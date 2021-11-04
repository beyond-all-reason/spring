#pragma once

#include <cstdint>
#include <unordered_map>

class TexturesSet {
public:
	TexturesSet() = default;
	TexturesSet(const std::initializer_list<std::pair<uint8_t, uint32_t>>& texturesList);
	TexturesSet(const std::initializer_list<uint32_t>& texturesList);

	TexturesSet(const TexturesSet& rhs) { *this = rhs; }
	TexturesSet(TexturesSet&& rhs) noexcept { *this = std::move(rhs); }

	TexturesSet& operator= (const TexturesSet& rhs);
	TexturesSet& operator= (TexturesSet&& rhs) noexcept;

	bool operator==(const TexturesSet& rhs) const { return (GetHash() == rhs.GetHash()); }

	void UpsertTexture(const std::pair <uint8_t, uint32_t>& tex) { UpsertTexture(tex.first, tex.second); }
	void UpsertTexture(uint8_t relBinding, uint32_t textureID);
	bool RemoveTexture(const std::pair <uint8_t, uint32_t>& tex) { return RemoveTexture(tex.first); }
	bool RemoveTexture(uint8_t relBinding);

	std::size_t GetHash() const;
private:
	mutable size_t perfectHash = 0u;
	std::unordered_map<uint8_t, uint32_t> textures; //relBinding, textureID

	static constexpr uint32_t MAX_TEXTURE_ID = 1 << 17; //enough for everyone (c)
};

class TexturesSetHash {
public:
	size_t operator()(const TexturesSet& ts) const { return ts.GetHash(); }
};