/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/AABB.hpp"

class CWorldDrawer
{
public:
	void InitPre() const;
	void InitPost() const;
	void Kill();

	void PreUpdate();
	void Update(bool newSimFrame);
	void Draw() const;

	void GenerateIBLTextures() const;
	void ResetMVPMatrices() const;

	// In-map space + all units/feature only
	const auto& GetWorldBounds() const { return worldBounds; }
private:
	void DrawOpaqueObjects() const;
	void DrawAlphaObjects() const;
	void DrawMiscObjects() const;
	void DrawBelowWaterOverlay() const;

private:
	unsigned int numUpdates = 0;
	AABB worldBounds;
};