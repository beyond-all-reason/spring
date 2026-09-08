/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IAtlasAllocator.h"

#include <algorithm>
#include <limits>

IAtlasAllocator::SAtlasEntry::SAtlasEntry() = default;

IAtlasAllocator::SAtlasEntry::SAtlasEntry(const int2 _size, std::string _name)
	: size(_size)
	, name(std::move(_name))
	, texCoords()
{}

IAtlasAllocator::IAtlasAllocator() = default;

IAtlasAllocator::~IAtlasAllocator() {}

void IAtlasAllocator::SetMaxSize(uint32_t xsize, uint32_t ysize)
{
	maxsize = uint2(xsize, ysize);
}

void IAtlasAllocator::SetMaxTexLevel(int maxLevels)
{
	numLevels = maxLevels;
}

void IAtlasAllocator::AddEntry(const SAtlasEntry& ae)
{
	AddEntry(ae.name, ae.size);
}

void IAtlasAllocator::AddEntry(const std::string& name, const int2& size)
{
	minDim = argmin(minDim, size.x, size.y);
	entries[name] = SAtlasEntry(size, name);
}

int IAtlasAllocator::GetPadding() const
{
	return static_cast<int>(1u << std::max(GetNumTexLevels() - 1, 0));
}

void IAtlasAllocator::SizeRoundUp()
{
    // guarantees that the resulting texture size can be divided in half at least [GetNumTexLevels() - 1] times
    auto alignment = static_cast<size_t>(GetPadding());

	atlasSize.x = std::min(AlignUp(atlasSize.x, alignment), maxsize.x);
	atlasSize.y = std::min(AlignUp(atlasSize.y, alignment), maxsize.y);
}

spring::unordered_map<std::string, IAtlasAllocator::SAtlasEntry>::const_iterator IAtlasAllocator::FindEntry(const std::string& name) const
{
	return entries.find(name);
}

const AtlasedTexture& IAtlasAllocator::GetEntry(const spring::unordered_map<std::string, SAtlasEntry>::const_iterator& it) const
{
	if (it == entries.end())
		return AtlasedTexture::DefaultAtlasTexture;

	return it->second.texCoords;
}

const AtlasedTexture& IAtlasAllocator::GetEntry(const std::string& name) const
{
	return GetEntry(FindEntry(name));
}

const spring::unordered_map<std::string, IAtlasAllocator::SAtlasEntry>& IAtlasAllocator::GetEntries() const
{
	return entries;
}

// pixel center based UV
AtlasedTexture IAtlasAllocator::GetTexCoordsCntr(const spring::unordered_map<std::string, SAtlasEntry>::const_iterator& it) const
{
	if (it == entries.end())
		return AtlasedTexture::DefaultAtlasTexture;

	const AtlasedTexture& a = it->second.texCoords; // all coords are inclusive
	AtlasedTexture uv = a;

	const float invW = 1.0f / atlasSize.x;
	const float invH = 1.0f / atlasSize.y;

	// Convert inclusive texel indices -> normalized coordinates of texel centers.
	uv.x1 = (a.x1 + 0.5f) * invW;
	uv.y1 = (a.y1 + 0.5f) * invH;
	uv.x2 = (a.x2 + 0.5f) * invW;
	uv.y2 = (a.y2 + 0.5f) * invH;

	return uv;
}

// pixel edges based UV
AtlasedTexture IAtlasAllocator::GetTexCoordsEdge(const spring::unordered_map<std::string, SAtlasEntry>::const_iterator& it) const
{
	if (it == entries.end())
		return AtlasedTexture::DefaultAtlasTexture;

	const AtlasedTexture& a = it->second.texCoords; // all coords are inclusive
	AtlasedTexture uv = a;

	const float invW = 1.0f / atlasSize.x;
	const float invH = 1.0f / atlasSize.y;

	// Edges in texel space are [x1, x2+1] and [y1, y2+1] because x2/y2 are inclusive.
	uv.x1 = (a.x1       ) * invW;
	uv.y1 = (a.y1       ) * invH;
	uv.x2 = (a.x2 + 1.0f) * invW;
	uv.y2 = (a.y2 + 1.0f) * invH;

	return uv;
}

AtlasedTexture IAtlasAllocator::GetTexCoordsCntr(const std::string& name) const
{
	return GetTexCoordsCntr(FindEntry(name));
}

AtlasedTexture IAtlasAllocator::GetTexCoordsEdge(const std::string& name) const
{
	return GetTexCoordsEdge(FindEntry(name));
}

bool IAtlasAllocator::contains(const std::string& name) const
{
	return entries.contains(name);
}

//! note: it doesn't clear the atlas! it only clears the entry db!
void IAtlasAllocator::clear()
{
	minDim = std::numeric_limits<int>::max();
	entries.clear();
}

int IAtlasAllocator::GetMinDim() const
{
	return minDim < std::numeric_limits<int>::max() ? minDim : 1;
}