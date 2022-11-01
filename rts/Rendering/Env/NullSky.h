#pragma once

#include "ISky.h"

class CNullSky : public ISky
{
public:
	void Update() override {}
	void Draw() override {}
	void DrawSun() override {}

	void UpdateSunDir() override {}
	void UpdateSkyTexture() override {}
};