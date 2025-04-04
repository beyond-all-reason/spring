/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LEGACY_ATLAS_ALLOC_H
#define LEGACY_ATLAS_ALLOC_H

#include "IAtlasAllocator.h"

class CLegacyAtlasAlloc : public IAtlasAllocator {
public:
	CLegacyAtlasAlloc()
	{
		atlasSize = {32, 32};
		numLevels = 1;
	}

	bool Allocate() override;
	int GetNumTexLevels() const override;

private:
	bool IncreaseSize();
	static bool CompareTex(const SAtlasEntry* tex1, const SAtlasEntry* tex2);
};

#endif // LEGACY_ATLAS_ALLOC_H
