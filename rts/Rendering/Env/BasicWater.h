/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BASIC_WATER_H
#define BASIC_WATER_H

#include "IWater.h"
#include "Rendering/GL/VertexArrayTypes.h"
#include "Rendering/GL/RenderBuffers.h"

class CBasicWater : public IWater
{
public:
	CBasicWater();
	~CBasicWater() override;

	void Draw() override;
	void UpdateWater(CGame* game)  override {}
	int GetID() const override { return WATER_RENDERER_BASIC; }
	const char* GetName() const override { return "basic"; }

	bool CanDrawReflectionPass() const override { return false; }
	bool CanDrawRefractionPass() const override { return false; }
private:
	void GenWaterQuadsRB();
	TypedRenderBuffer<VA_TYPE_T> rb;

	uint32_t textureID;
	uint32_t xsize;
	uint32_t ysize;
};

#endif // BASIC_WATER_H
