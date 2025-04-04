#pragma once

#include "ISky.h"

#include "Rendering/GL/FBO.h"
#include "Rendering/GL/VAO.h"

#include <array>

namespace Shader {
struct IProgramObject;
}

class CModernSky : public ISky {
public:
	CModernSky();
	~CModernSky() override;

	void Update() override {};
	void Draw() override;

	void UpdateSunDir() override {}

	void UpdateSkyTexture() override {}

	bool IsValid() const override { return valid; }

	std::string GetName() const override { return "ModernSky"; }

private:
	VAO vao;
	std::array<Shader::IProgramObject*, 2> skyShaders = {nullptr};
	bool valid = false;
};
