/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <string>
#include <limits>
#include <cstdint>

#include "AtlasedTexture.hpp"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"
#include "System/StringHash.h"
#include "System/SpringMath.h"


class IAtlasAllocator
{
public:
	struct SAtlasEntry
	{
		SAtlasEntry();
		SAtlasEntry(const int2 _size, std::string _name);

		int2 size;
		std::string name;
		AtlasedTexture texCoords;
	};
public:
	IAtlasAllocator();
	virtual ~IAtlasAllocator();

	void SetMaxSize(uint32_t xsize, uint32_t ysize);
public:
	virtual bool Allocate() = 0;
	virtual int GetNumTexLevels() const = 0;
	virtual int GetReqNumTexLevels() const = 0;
	virtual uint32_t GetNumPages() const = 0;
	void SetMaxTexLevel(int maxLevels);
public:
	void AddEntry(const SAtlasEntry& ae);
	void AddEntry(const std::string& name, const int2& size);

	int GetPadding() const;

	void SizeRoundUp();

	spring::unordered_map<std::string, SAtlasEntry>::const_iterator FindEntry(const std::string& name) const;

	const AtlasedTexture& GetEntry(const spring::unordered_map<std::string, SAtlasEntry>::const_iterator& it) const;
	const AtlasedTexture& GetEntry(const std::string& name) const;
	const spring::unordered_map<std::string, SAtlasEntry>& GetEntries() const;

	// pixel center based UV
	AtlasedTexture GetTexCoordsCntr(const spring::unordered_map<std::string, SAtlasEntry>::const_iterator& it) const;

	// pixel edges based UV
	AtlasedTexture GetTexCoordsEdge(const spring::unordered_map<std::string, SAtlasEntry>::const_iterator& it) const;

	AtlasedTexture GetTexCoordsCntr(const std::string& name) const;
	AtlasedTexture GetTexCoordsEdge(const std::string& name) const;

	bool contains(const std::string& name) const;

	//! note: it doesn't clear the atlas! it only clears the entry db!
	void clear();

	int GetMinDim() const;

	const auto& GetMaxSize() const { return maxsize; }
	const auto& GetAtlasSize() const { return atlasSize; }
protected:
	spring::unordered_map<std::string, SAtlasEntry> entries;

	uint2 atlasSize;
	uint2 maxsize = {2048, 2048};
	int numLevels = std::numeric_limits<int>::max();
	int minDim = std::numeric_limits<int>::max();
};
