/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>

struct CDebugVisibilityDrawer;

struct VisibleQuadData {
	void Update();
	int GetNumQuadsX() const { return numQuadsX; }
	int GetNumQuadsZ() const { return numQuadsZ; }
	const std::vector<bool>& GetQuads() const { return quads; }

private:
	int numQuadsX;
	int numQuadsZ;
	std::vector<bool> quads;

	friend CDebugVisibilityDrawer;
};

inline VisibleQuadData CamVisibleQuads;

class DebugVisibilityDrawer
{
public:
	static inline bool enable = false;
	static void Update();
	static void DrawWorld();
	static void DrawMinimap();

	static CDebugVisibilityDrawer drawer;
};
