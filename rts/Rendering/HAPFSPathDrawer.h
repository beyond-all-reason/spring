/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HAPFS_PATHDRAWER_HDR
#define HAPFS_PATHDRAWER_HDR

#include "IPathDrawer.h"

class CPathFinderDef;
struct UnitDef;

namespace HAPFS {

class CPathManager;
class CPathFinder;
class CPathEstimator;

} // namespace HAPFS

struct HAPFSPathDrawer : public IPathDrawer {
public:
	HAPFSPathDrawer();

	void DrawAll() const;
	void DrawInMiniMap();
	void UpdateExtraTexture(int, int, int, int, unsigned char*) const;

	enum BuildSquareStatus {
		NOLOS = 0,
		FREE = 1,
		OBJECTBLOCKED = 2,
		TERRAINBLOCKED = 3,
	};

private:
	void Draw() const;
	void Draw(const CPathFinderDef*) const;
	void Draw(const HAPFS::CPathFinder*) const;
	void Draw(const HAPFS::CPathEstimator*) const;

private:
	HAPFS::CPathManager* pm;
};

#endif
