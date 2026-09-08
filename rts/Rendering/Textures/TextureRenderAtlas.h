#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>

#include "TextureAtlas.h"
#include "IAtlasAllocator.h"
#include "System/type2.h"
#include "System/float4.h"
#include "System/Color.h"
#include "System/UnorderedMap.hpp"

class CBitmap;
namespace Shader {
	struct IProgramObject;
}

class CTextureRenderAtlas {
public:
	struct UniqueSubTexture {
		uint32_t texID;
		uint32_t stableIdx; // deterministic index for name generation (texID is non-deterministic)
		float4 subTexCoords;
		std::string GetName() const;

		bool operator==(const UniqueSubTexture&) const = default;
	};
public:
	CTextureRenderAtlas(CTextureAtlas::AllocatorType allocType, int atlasSizeX, int atlasSizeY, int maxLevels, uint32_t glInternalType = /*GL_RGBA8*/0x8058, const std::string& atlasName = "");
	~CTextureRenderAtlas();
	CTextureRenderAtlas(const CTextureRenderAtlas&) = delete;
	CTextureRenderAtlas(CTextureRenderAtlas&&) noexcept = default;

	CTextureRenderAtlas& operator=(const CTextureRenderAtlas&) = delete;
	CTextureRenderAtlas& operator=(CTextureRenderAtlas&&) noexcept = default;

	bool TextureExists(const std::string& texName);
	bool TextureExists(const std::string& texName, const std::string& texBackupName);

	bool AddTexFromFile(const std::string& name, const std::string& fileName, const float4& subTexCoords = float4(0.0f, 0.0f, 1.0f, 1.0f));
	bool AddTexFromBitmap(const std::string& name, const CBitmap& bm, const std::string& refFileName, const float4& subTexCoords = float4(0.0f, 0.0f, 1.0f, 1.0f));
	bool AddTex(const std::string& name, int xsize, int ysize, const SColor& color, const std::string& refFileName);

	AtlasedTexture GetTexture(const std::string& texName);
	AtlasedTexture GetTexture(const std::string& texName, const std::string& texBackupName);

	std::vector<std::string> GetAllFileNames() const;
	const auto& GetNameToUniqueSubTexMap() const { return nameToUniqueSubTexStr; }
	const auto& GetUniqueSubTexMap() const { return uniqueSubTextureMap; }

	uint32_t GetTexTarget() const;
	uint32_t GetTexID() const;
	const uint2& GetAtlasSize() const;
	int GetMinDim() const;
	int GetNumTexLevels() const;

	const IAtlasAllocator* GetAllocator() const { return atlasAllocator.get(); }
	const std::string& GetAtlasName() const { return atlasName; }

	bool Finalize() { return CalculateAtlas() && CreateAtlasTexture(); }
	bool CalculateAtlas();
	bool CreateAtlasTexture();

	bool IsValid() const;

	uint32_t DisownTexture();

	bool DumpTexture(const std::string& fileExt = "png") const;
private:


	bool AddTexFromBitmapRaw(const std::string& name, const CBitmap& bm, const float4& subTexCoords, const std::string& refFileName);

	struct FileTexEntry {
		uint32_t texID;
		uint32_t stableIdx;
	};
	spring::unordered_map<std::string, FileTexEntry> filenameToTexID;
	spring::unordered_map<std::string, UniqueSubTexture> uniqueSubTextureMap;
	spring::unordered_map<std::string, std::string> nameToUniqueSubTexStr;

	CTextureAtlas::AllocatorType allocType;
	uint32_t glInternalType;

	std::unique_ptr<GL::TextureBase> atlasTex;
	std::unique_ptr<IAtlasAllocator> atlasAllocator;

	std::string atlasName;
	static inline size_t shaderRef = 0;
	static inline Shader::IProgramObject* shader = nullptr;
	bool atlasFinalized;
	bool atlasRendered;
public:
	static inline AtlasedTexture dummy = AtlasedTexture{};
};