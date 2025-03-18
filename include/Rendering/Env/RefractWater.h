/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef REFRACTED_WATER_H
#define REFRACTED_WATER_H

#include "AdvWater.h"

class CRefractWater : public CAdvWater 
{
public:
	~CRefractWater() override { FreeResources(); }
	void InitResources(bool loadShader) override;
	void FreeResources() override;

	void LoadGfx();

	void Draw() override;
	WATER_RENDERER GetID() const override { return WATER_RENDERER_REFL_REFR; }
protected:
	void SetupWaterDepthTex();

	unsigned int target = 0;
	/// the screen is copied into this texture and used for water rendering
	GLuint subSurfaceTex = 0;
};

#endif // REFRACTED_WATER_H
