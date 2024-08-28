/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COLOR_MAP_H
#define COLOR_MAP_H

#include <string>
#include <vector>
#include <tuple>

#include "System/Color.h"
#include "System/UnorderedMap.hpp"
#include "Rendering/GL/VBO.h"

/**
 * Simple class to interpolate between 32bit RGBA colors
 * Do not delete an instance of this class created by any Load function,
 * they are deleted automaticly.
 */
class CColorMap
{
public:
	CR_DECLARE_STRUCT(CColorMap)
	CColorMap() = default;
	/// offset & size constructor
	CColorMap(uint32_t offt_, uint32_t size_)
		: offt(offt_)
		, size(size_)
	{}
	CColorMap(const CColorMap&) = delete;
	CColorMap(CColorMap&& cm) noexcept { *this = std::move(cm); }

	CColorMap& operator = (const CColorMap&) = delete;
	CColorMap& operator = (CColorMap&&) noexcept = default;

	constexpr bool operator == (const CColorMap& o) { return offt == o.offt && size == o.size; }
public:
	/**
	 * @param color buffer with room for 4 bytes
	 * @param pos value between 0.0f and 1.0f, returns pointer to color
	 */
	std::tuple<SColor, SColor, float, float> GetColorsPair(float pos = 0.0f) const;
	void GetColor(unsigned char* color, float pos) const;
	SColor GetColor(float pos) const;
	const SColor& GetColorAt(uint32_t index) const;
	std::pair<uint32_t, uint32_t> GetIndices(float pos) const;
	float GetColorPos(uint32_t idx) const;

	auto GetOffset() const { return offt; }
	auto GetSize() const { return size; }
public:
	static void InitStatic();
	static void KillStatic();

	static VBO& GetSSBO();

	static const auto* GetFlatColorData() { return reinterpret_cast<const uint8_t*>(allColorMapValues.data()); }
	static const auto  GetFlatColorSize() { return allColorMapValues.size() * sizeof(SColor); }

	static void SerializeColorMaps(creg::ISerializer* s);

	/// Load colormap from a bitmap
	static CColorMap* LoadFromBitmapFile(const std::string& fileName);

	/// Load from floats or uint8
	template<typename T>
	static CColorMap* LoadFromArray(const T* fp, uint32_t num);

	/**
	 * Load from a string containing a number of float values or filename.
	 * example: "1.0 0.5 1.0 ... "
	 */
	static CColorMap* LoadFromDefString(const std::string& defString);

	static CColorMap* LoadDummy(const SColor& col);
private:
	uint32_t offt;
	uint32_t size;

	inline static std::vector<CColorMap> allColorMaps;
	inline static std::vector<SColor> allColorMapValues;
	inline static spring::unordered_map<std::string, uint32_t> namedColorMaps;

	inline static VBO vbo;
};

#endif // COLOR_MAP_H
