#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>

#include "TextureAtlas.h"
#include "IAtlasAllocator.h"
#include "System/type2.h"
#include "System/Color.h"
#include "System/UnorderedMap.hpp"

class CBitmap;
namespace Shader {
	struct IProgramObject;
}

class CTextureRenderAtlas {
public:
	CTextureRenderAtlas(CTextureAtlas::AllocatorType allocType, int atlasSizeX, int atlasSizeY, uint32_t glInternalType = /*GL_RGBA8*/0x8058, const std::string& atlasName = "");
	~CTextureRenderAtlas();
	CTextureRenderAtlas(const CTextureRenderAtlas&) = delete;
	CTextureRenderAtlas(CTextureRenderAtlas&&) noexcept = default;

	CTextureRenderAtlas& operator=(const CTextureRenderAtlas&) = delete;
	CTextureRenderAtlas& operator=(CTextureRenderAtlas&&) noexcept = default;

	bool TextureExists(const std::string& texName);
	bool TextureExists(const std::string& texName, const std::string& texBackupName);

	bool AddTexFromFile(const std::string& name, const std::string& file);
	bool AddTexFromBitmap(const std::string& name, const CBitmap& bm);
	bool AddTex(const std::string& name, int xsize, int ysize, const SColor& color);

	AtlasedTexture GetTexture(const std::string& texName);
	AtlasedTexture GetTexture(const std::string& texName, const std::string& texBackupName);

	uint32_t GetTexTarget() const;
	uint32_t GetTexID() const { return texID; }
	int GetMinDim() const;
	int GetNumTexLevels() const;
	void SetMaxTexLevel(int maxLevels);

	const IAtlasAllocator* GetAllocator() const { return atlasAllocator.get(); }
	const std::string& GetAtlasName() const { return atlasName; }

	bool Finalize();
	bool IsValid() const { return finalized && texID != 0; }

	bool DumpTexture() const;
private:
	bool AddTexFromBitmapRaw(const std::string& name, const CBitmap& bm);

	spring::unordered_map<std::string, uint32_t> nameToTexID;
	int atlasSizeX;
	int atlasSizeY;
	CTextureAtlas::AllocatorType allocType;
	uint32_t glInternalType;
	uint32_t texID;
	std::unique_ptr<IAtlasAllocator> atlasAllocator;
	std::string atlasName;
	static inline size_t shaderRef = 0;
	static inline Shader::IProgramObject* shader = nullptr;
	bool finalized;
public:
	static inline AtlasedTexture dummy = AtlasedTexture{};
};