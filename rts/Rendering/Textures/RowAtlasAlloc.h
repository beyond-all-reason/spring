/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ROW_ATLAS_ALLOC_H
#define ROW_ATLAS_ALLOC_H

#include "IAtlasAllocator.h"

#include <vector>

class CRowAtlasAlloc : public IAtlasAllocator {
public:
	CRowAtlasAlloc()
	{
		atlasSize = {256, 256};
		numLevels = 1;
	}

	bool Allocate() override;
	int GetNumTexLevels() const override;

private:
	struct Row {
		Row(int _ypos, int _height)
		    : position(_ypos)
		    , height(_height)
		    , width(0) {};

		int position;
		int height;
		int width;
	};

private:
	int nextRowPos = 0;
	std::vector<Row> imageRows;

private:
	void EstimateNeededSize();
	Row* AddRow(int glyphWidth, int glyphHeight);
	Row* FindRow(int glyphWidth, int glyphHeight);
	static bool CompareTex(const SAtlasEntry* tex1, const SAtlasEntry* tex2);
};

#endif // ROW_ATLAS_ALLOC_H
