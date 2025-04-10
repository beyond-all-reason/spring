/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ADV_WATER_H
#define ADV_WATER_H

#include "IWater.h"

#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"

class CAdvWater : public IWater {
public:
	~CAdvWater() override { FreeResources(); }

	void InitResources(bool loadShader) override;
	void FreeResources() override;

	WATER_RENDERER GetID() const override { return WATER_RENDERER_REFLECTIVE; }

	void Draw() override;
	void Draw(bool useBlending);
	void UpdateWater(const CGame* game) override;

	bool CanDrawReflectionPass() const override { return true; }

	bool CanDrawRefractionPass() const override { return false; }

protected:
	FBO reflectFBO;
	FBO bumpFBO;

	GLuint reflectTexture;
	GLuint bumpTexture;
	GLuint rawBumpTexture[4];
	float3 waterSurfaceColor;

	unsigned int waterFP;
};

#endif // ADV_WATER_H
