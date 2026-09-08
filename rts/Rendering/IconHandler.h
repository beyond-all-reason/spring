/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <vector>
#include <string>
#include <bitset>

#include "System/float3.h"
#include "System/UnorderedMap.hpp"
#include "System/ScopedResource.h"
#include "Rendering/GL/RenderBuffersFwd.h"
#include "Rendering/Textures/TextureAtlas.h"

struct UnitDef;
class CTextureRenderAtlas;

namespace icon {
	static constexpr size_t INVALID_ICON_INDEX = size_t(-1);

	class IconData {
	public:
		explicit IconData()
			: name {}
			, fileName{}
			, size{ 0.0f }
			, distance{ 1.0f }
			, distSqr{ 1.0f }
			, drawOrder{ 0.0f }
			, radiusAdjust{ false }
			, srcTexCoords{}
			, texCoords{}
		{}
		explicit IconData(const std::string& name_, const std::string& fileName_, float size_, float distance_, uint32_t atlasIndex_, bool radiusAdjust_, const float4& srcTexCoords_, float drawOrder_ = 0.0f)
			: name{ name_ }
			, fileName{ fileName_ }
			, size{ size_ }
			, distance{ distance_ }
			, distSqr{ distance_ * distance_ }
			, drawOrder{ drawOrder_ }
			, radiusAdjust{ radiusAdjust_ }
			, srcTexCoords{ srcTexCoords_ }
			, texCoords{ 0.0f, 0.0f, 1.0f, 1.0f, atlasIndex_ }
		{}

		const auto& GetName() const { return name; }
		const auto& GetSize() const { return size; }
		const auto& GetDistance() const { return distance; }
		const auto& GetDistanceSq() const { return distSqr; }
		const auto& GetDrawOrder() const { return drawOrder; }
		const auto& GetRadiusAdjust() const { return radiusAdjust; }
		const auto& GetSrcTexCoords() const { return srcTexCoords; }
		const auto& GetTexCoords() const { return texCoords; }
		const auto& GetAtlasIndex() const { return texCoords.pageNum; }
		const auto& GetFileName() const { return fileName; }

		static constexpr auto GetRadiusScale() { return 30.0f; }

		void SetTexCoords(AtlasedTexture&& tc) { texCoords = std::move(tc); }
		void SetTexCoords(const AtlasedTexture& tc) { texCoords = tc; }
	private:
		std::string name;
		std::string fileName;
		float size;
		float distance;
		float distSqr;
		float drawOrder;

		bool radiusAdjust;
		float4 srcTexCoords;
		AtlasedTexture texCoords;
	};

	class CIconHandler {
		public:
			CIconHandler() = default;

			// unique, no copy, no move
			CIconHandler(const CIconHandler&) = delete;
			CIconHandler& operator=(const CIconHandler&) = delete;
			CIconHandler(CIconHandler&&) = delete;
			CIconHandler& operator=(CIconHandler&&) = delete;

			void Init() { LoadIcons("gamedata/icontypes.lua"); }
			void Kill();

			bool AddIcon(
				uint32_t atlasIndex,
				const std::string& iconName,
				const std::string& texFileName,
				float size,
				float distance,
				bool radAdj,
				float u0, float v0, float u1, float v1,
				float drawOrder = 0.0f
			);

			bool FreeIcon(const std::string& iconName);

			void Update();

			const auto& GetIconsMap() const { return iconsMap; }
			const auto& GetIconsData() const { return iconsData; }
			const IconData& GetIconData(const std::string& iconName) const;
			const IconData& GetIconData(size_t iconIdx) const;
			size_t GetIconIdx(const std::string& iconName) const;
			size_t GetIconIdxOrDefault(const std::string& iconName) const;
			std::pair<bool, spring::unordered_map<std::string, size_t>::const_iterator> FindIconIdx(const std::string& iconName) const;
			size_t GetDefaultIconIdx() const { return defaultIconIdx; }
			/// true when any icon defines a nonzero drawOrder; sorting is skipped entirely otherwise
			bool HasDrawOrders() const { return (drawOrderIconCount > 0); }

			const auto& GetAtlasTextureIDs() const { return atlasTextureIDs; }
			auto GetAtlasTextureID(size_t i) const { return atlasTextureIDs[i]; }
			const auto& GetAtlasSize(size_t i) const { return atlasTextureSizes[i]; }

			void DumpAtlasTextures(const std::string& fileExt = "png") const;
		private:
			bool UpdateAtlasData(size_t atlasIdx);
			bool CreateAtlasTexture(size_t atlasIdx);
			void LoadIcons(const std::string& filename);
		private:
			static constexpr int DEFAULT_NUM_OF_TEXTURE_LEVELS = 4;

			spring::unordered_map<std::string, size_t> iconsMap;
			std::vector<IconData> iconsData;

			size_t defaultIconIdx = INVALID_ICON_INDEX;
			size_t drawOrderIconCount = 0;

			std::array<uint32_t, 2> atlasTextureIDs = {};
			std::array<int2, 2> atlasTextureSizes = {};
			std::array<std::unique_ptr<CTextureRenderAtlas>, 2> atlases;
			std::bitset<2> atlasNeedsUpdate = { false };
	};

	extern CIconHandler iconHandler;
}