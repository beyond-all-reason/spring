/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IATLAS_ALLOC_H
#define IATLAS_ALLOC_H

#include <string>
#include <limits>

#include "System/float4.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"
#include "System/StringHash.h"
#include "System/SpringMath.h"


class IAtlasAllocator
{
public:
	struct SAtlasEntry
	{
		SAtlasEntry() = default;
		SAtlasEntry(const int2 _size, std::string _name)
			: size(_size)
			, name(std::move(_name))
		{}

		int2 size;
		std::string name;
		float4 texCoords;
	};
public:
	IAtlasAllocator() = default;
	virtual ~IAtlasAllocator() {}

	void SetMaxSize(int xsize, int ysize) { maxsize = int2(xsize, ysize); }
	void SetNonPowerOfTwo(bool nonPowerOfTwo) { npot = nonPowerOfTwo; }

public:
	virtual bool Allocate() = 0;
	virtual int GetNumTexLevels() const = 0;
	virtual int GetReqNumTexLevels() const = 0;
	void SetMaxTexLevel(int maxLevels) { numLevels = maxLevels; };
public:
	void AddEntry(const std::string& name, int2 size)
	{
		minDim = argmin(minDim, size.x, size.y);
		entries[name] = SAtlasEntry(size, name);
	}

	const auto& GetEntry(const std::string& name) { return entries[name].texCoords; }
	const auto& GetEntries() const { return entries; }

	float4 GetTexCoords(const std::string& name)
	{
		float4 uv(entries[name].texCoords);
		uv.x1 /= atlasSize.x;
		uv.y1 /= atlasSize.y;
		uv.x2 /= atlasSize.x;
		uv.y2 /= atlasSize.y;

		// adjust texture coordinates by half a texel (opengl uses centeroids)
		uv.x1 += 0.5f / atlasSize.x;
		uv.y1 += 0.5f / atlasSize.y;
		uv.x2 += 0.5f / atlasSize.x;
		uv.y2 += 0.5f / atlasSize.y;

		return uv;
	}

	bool contains(const std::string& name) const { return entries.contains(name); }

	//! note: it doesn't clear the atlas! it only clears the entry db!
	void clear()
	{
		minDim = std::numeric_limits<int>::max();
		entries.clear();
	}

	int GetMinDim() const { return minDim < std::numeric_limits<int>::max() ? minDim : 1; }

	int2 GetMaxSize() const { return maxsize; }
	int2 GetAtlasSize() const { return atlasSize; }

protected:
	spring::unordered_map<std::string, SAtlasEntry> entries;

	int2 atlasSize;
	int2 maxsize = {2048, 2048};
	int numLevels = std::numeric_limits<int>::max();
	int minDim = std::numeric_limits<int>::max();

	bool npot = false;
};

#endif // IATLAS_ALLOC_H
