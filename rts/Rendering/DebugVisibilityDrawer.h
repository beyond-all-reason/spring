/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>

struct CDebugVisibilityDrawer;
struct float3;

struct VisibleQuadData {
	void Init();
	void Update();
	bool isInQuads(const float3& pos);
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
inline VisibleQuadData CamVisibleShadowQuads;

class DebugVisibilityDrawer
{
public:
	static inline bool enable = false;
	static void DrawWorld();
	static void DrawMinimap();
};
