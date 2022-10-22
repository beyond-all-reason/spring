#pragma once

#include "ISky.h"
#include "Rendering/GL/VAO.h"
#include "Rendering/GL/FBO.h"

namespace Shader {
	struct IProgramObject;
}

class CModernSky : public ISky
{
public:
	CModernSky();
	~CModernSky() override;

	void Update() override {};
	void Draw() override;
	void DrawSun() override { /*no separate draw is possible here*/ };

	void UpdateSunDir() override {}
	void UpdateSkyTexture() override {}
private:

private:
	VAO vao;
	Shader::IProgramObject* skyShader  = nullptr;
};
