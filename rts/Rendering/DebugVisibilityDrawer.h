/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

struct CDebugVisibilityDrawer;

class DebugVisibilityDrawer
{
public:
	static inline bool enable = false;
	static void DrawWorld();
	static void DrawMinimap();

	static CDebugVisibilityDrawer drawer;
};