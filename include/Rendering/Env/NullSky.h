#pragma once

#include "ISky.h"

class CNullSky : public ISky
{
public:
	void Update() override {}
	void Draw() override {}

	void UpdateSunDir() override {}
	void UpdateSkyTexture() override {}

	bool IsValid() const override { return true; }

	std::string GetName() const override { return "NullSky"; }
};