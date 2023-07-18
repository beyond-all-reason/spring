/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>

struct QuadData {
	int numQuadsX;
	int numQuadsZ;
	std::vector<bool> visibleQuads;
};

struct CDebugVisibilityDrawer;

class DebugVisibilityDrawer
{
public:
	static inline bool enable = false;
	static void Update();
	static void DrawWorld();
	static void DrawMinimap();

	static CDebugVisibilityDrawer drawer;
	static QuadData quads;
};
