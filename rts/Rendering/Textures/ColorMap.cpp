/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <array>
#include <cstring> // memcpy

#include "ColorMap.h"
#include "Bitmap.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/StringUtil.h"
#include "System/SpringMath.h"
#include "System/creg/STL_Map.h"

#include "System/Misc/TracyDefs.h"

CR_BIND(CColorMap, )
CR_REG_METADATA(CColorMap, (
	CR_MEMBER(offt),
	CR_MEMBER(size)
))

template
CColorMap* CColorMap::LoadFromArray<float>(const float* fp, uint32_t num);

template
CColorMap* CColorMap::LoadFromArray<uint8_t>(const uint8_t* fp, uint32_t num);

void CColorMap::InitStatic()
{
	RECOIL_DETAILED_TRACY_ZONE;

	allColorMaps.reserve(512);
	allColorMapValues.reserve(allColorMaps.capacity() * 4);

	vbo = VBO{ GL_SHADER_STORAGE_BUFFER, false, false };
}

void CColorMap::KillStatic()
{
	allColorMaps.clear();
	allColorMapValues.clear();
	namedColorMaps.clear();

	vbo.Release(); vbo = {};
}

VBO& CColorMap::GetSSBO()
{
	if (vbo.GetIdRaw() != 0 && vbo.GetSize() == allColorMapValues.size() * sizeof(float4))
		return vbo;

	auto token = vbo.BindScoped();
	vbo.Resize(allColorMapValues.size() * sizeof(float4), GL_STATIC_READ);

	auto* ptr = vbo.MapBuffer(GL_WRITE_ONLY);
	for (const auto& c : allColorMapValues) {
		const auto f = static_cast<float4>(c);
		std::memcpy(ptr, &f, sizeof(float4));
		ptr += sizeof(float4);
	}
	vbo.UnmapBuffer();

	return vbo;
}

CColorMap* CColorMap::LoadFromBitmapFile(const std::string& fileName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto fn = StringToLower(fileName);
	const auto it = namedColorMaps.find(fn);

	if (it != namedColorMaps.end())
		return &allColorMaps[it->second];

	CBitmap bitmap;

	if (!bitmap.Load(fileName)) {
		bitmap.Alloc(2, 2, 4);
		LOG_L(L_WARNING, "[ColorMap] could not load texture from file \"%s\"", fileName.c_str());
	}

	if (bitmap.compressed || (bitmap.channels != 4) || (bitmap.xsize < 2))
		throw content_error("[ColorMap] unsupported bitmap format in file " + fileName);

	return LoadFromArray(bitmap.GetRawMem(), bitmap.GetMemSize());
}

template<typename T>
CColorMap* CColorMap::LoadFromArray(const T* vals, uint32_t num)
{
	std::array<SColor, 1024> tmpColors;

	if (num % 4 != 0)
		throw content_error("[ColorMap] invalid number of color components (not multiple of 4)");

	if (num > tmpColors.size() * 4) {
		LOG_L(L_WARNING, "[ColorMap] colormap float array is too big %u, truncating to %u", static_cast<uint32_t>(num), static_cast<uint32_t>(4 * tmpColors.size()));
		num = tmpColors.size() * 4;
	}

	num /= 4;

	for (uint32_t i = 0; i < num; ++i) {
		tmpColors[i] = SColor(vals[4 * i + 0], vals[4 * i + 1], vals[4 * i + 2], vals[4 * i + 3]);
	}

#if 0
	const auto valueIt = std::search(allColorMapValues.begin(), allColorMapValues.end(), tmpColors.begin(), tmpColors.begin() + num);
	if (valueIt != allColorMapValues.end()) {
		CColorMap cm{ static_cast<uint32_t>(std::distance(allColorMapValues.begin(), valueIt)), num };
		const auto cmIt = std::find(allColorMaps.begin(), allColorMaps.end(), cm);
		if (cmIt != allColorMaps.end())
			return &(*cmIt);

		return &allColorMaps.emplace_back(std::move(cm));
	}
#else
	const auto searchPred = [num](const auto& cm) {
		return num == cm.size;
	};

	for (auto cmIt = std::find_if(allColorMaps.begin(), allColorMaps.end(), searchPred); cmIt != allColorMaps.end(); cmIt = std::find_if(cmIt + 1, allColorMaps.end(), searchPred)) {
		const auto stValIt = allColorMapValues.begin() + cmIt->GetOffset();
		const auto fiValIt = stValIt + cmIt->GetSize();
		if (std::equal(stValIt, fiValIt, tmpColors.begin()))
			return &(*cmIt);
	}
#endif

	uint32_t offset = allColorMapValues.size();
	allColorMapValues.insert(allColorMapValues.end(), tmpColors.begin(), tmpColors.begin() + num);
	return &allColorMaps.emplace_back(offset, num);
}

CColorMap* CColorMap::LoadFromDefString(const std::string& defString)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::array<float, 4096> tmpFloats;

	uint32_t idx = 0;

	char* pos = const_cast<char*>(defString.c_str());
	char* end = nullptr;

	for (float val; (val = std::strtof(pos, &end), pos != end && idx < tmpFloats.size()); pos = end) {
		tmpFloats[idx++] = val;
	}

	if (idx == 0)
		return (CColorMap::LoadFromBitmapFile("bitmaps\\" + defString));

	if (idx < 8)
		throw content_error("[ColorMap] less than two RGBA colors specified");

	return LoadFromArray(tmpFloats.data(), idx);
}

CColorMap* CColorMap::LoadDummy(const SColor& col)
{
	std::array<SColor, 2> cols = {col};
	return LoadFromArray(reinterpret_cast<uint8_t*>(cols.data()), cols.size() * sizeof(SColor));
}


std::tuple<SColor, SColor, float, float> CColorMap::GetColorsPair(float pos) const
{
	auto [i0, i1] = GetIndices(pos);
	auto& color0 = GetColorAt(i0);
	auto& color1 = GetColorAt(i1);
	auto colEdge0 = GetColorPos(i0);
	auto colEdge1 = GetColorPos(i1);
	return std::make_tuple(color0, color1, colEdge0, colEdge1);
}

void CColorMap::GetColor(unsigned char* color, float pos) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	SColor col = GetColor(pos);
	std::copy(std::begin(col.rgba), std::end(col.rgba), color);
}

SColor CColorMap::GetColor(float pos) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto [i0, i1] = GetIndices(pos);
	const auto fpos = pos * (size - 1);
	const auto ipos = static_cast<uint32_t>(fpos);
	pos = (fpos - ipos);

	return mix(allColorMapValues[i0], allColorMapValues[i1], pos);
}

const SColor& CColorMap::GetColorAt(uint32_t idx) const
{
	idx = std::clamp(idx, offt, offt + size);
	return allColorMapValues[idx];
}

float CColorMap::GetColorPos(uint32_t idx) const
{
	idx = std::clamp(idx, offt, offt + size);
	return static_cast<float>(idx - offt) / (size - 1);
}

std::pair<uint32_t, uint32_t> CColorMap::GetIndices(float pos) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	pos = std::clamp(pos, 0.0f, 0.999f);

	const uint32_t basePos = offt + static_cast<uint32_t>(pos * (size - 1));
	return std::make_pair(
		basePos + 0,
		basePos + 1
	);
}

#ifdef USING_CREG
void CColorMap::SerializeColorMaps(creg::ISerializer* s)
{
	RECOIL_DETAILED_TRACY_ZONE;
	
	const auto Serializer = [s](auto& staticVar) {
		std::unique_ptr<creg::IType> t = creg::DeduceType<std::remove_cvref_t<decltype(staticVar)>>::Get();
		t->Serialize(s, &staticVar);
	};
	Serializer(allColorMaps);
	Serializer(allColorMapValues);
	Serializer(namedColorMaps);
}
#endif
