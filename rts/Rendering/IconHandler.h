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

class UnitDef;

namespace icon {
	static constexpr size_t INVALID_ICON_INDEX = size_t(-1);

	class IconData {
	public:
		explicit IconData()
			: fileName{}
			, size{ 0.0f }
			, distance{ 1.0f }
			, distSqr{ 1.0f }
			, atlasIndex{ 0 }
			, radiusAdjust{ false }
			, srcTexCoords{}
			, texCoords{}
		{}
		explicit IconData(const std::string& fileName_, float size_, float distance_, size_t atlasIndex_, bool radiusAdjust_, const float4& srcTexCoords_)
			: fileName{ fileName_ }
			, size{ size_ }
			, distance{ distance_ }
			, distSqr{ distance_ * distance_ }
			, atlasIndex{ atlasIndex_ }
			, radiusAdjust{ radiusAdjust_ }
			, srcTexCoords{ srcTexCoords_ }
			, texCoords{}
		{}

		const auto& GetSize() const { return size; }
		const auto& GetDistance() const { return distance; }
		const auto& GetDistanceSq() const { return distSqr; }
		const auto& GetAtlasIndex() const { return atlasIndex; }
		const auto& GetRadiusAdjust() const { return radiusAdjust; }
		const auto& GetSrcTexCoords() const { return srcTexCoords; }
		const auto& GetTexCoords() const { return texCoords; }
		const auto& GetFileName() const { return fileName; }

		static constexpr auto GetRadiusScale() { return 30.0f; }

		void SetTexCoords(AtlasedTexture&& tc) { texCoords = std::move(tc); }
		void SetTexCoords(const AtlasedTexture& tc) { texCoords = tc; }
	private:
		std::string fileName;
		float size;
		float distance;
		float distSqr;

		size_t atlasIndex;
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
				size_t atlasIndex,
				const std::string& iconName,
				const std::string& texFileName,
				float size,
				float distance,
				bool radAdj,
				float u0, float v0, float u1, float v1
			);

			bool FreeIcon(const std::string& iconName);

			void Update();

			const auto& GetIconData(const std::string& iconName) const { return iconsData[*iconsMap.try_get(iconName)]; }
			const auto& GetIconData(size_t iconIdx) const { return iconsData[iconIdx]; }
			size_t GetIconIdx(const std::string& iconName) const;
			std::pair<bool, spring::unordered_map<std::string, size_t>::const_iterator> FindIconIdx(const std::string& iconName) const;
			size_t GetDefaultIconIdx() const { return defaultIconIdx; }

			const auto& GetAtlasTextureIDs() const { return atlasTextureIDs; }
		private:
			void LoadIcons(const std::string& filename);
		private:
			static constexpr int DEFAULT_NUM_OF_TEXTURE_LEVELS = 4;

			spring::unordered_map<std::string, size_t> iconsMap;
			std::vector<IconData> iconsData;

			size_t defaultIconIdx = INVALID_ICON_INDEX;

			std::array<uint32_t, 2> atlasTextureIDs = {};
			std::bitset<2> atlasNeedsUpdate = { false };
	};

	extern CIconHandler iconHandler;
}