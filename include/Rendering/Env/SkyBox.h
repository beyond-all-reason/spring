/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKY_BOX_H
#define SKY_BOX_H

#include <string>

#include "ISky.h"
#include "Rendering/GL/VAO.h"
#include "Map/MapTexture.h"

namespace Shader {
	struct IProgramObject;
}

class CSkyBox : public ISky
{
public:
	explicit CSkyBox(uint32_t textureID, uint32_t xsize, uint32_t ysize) { Init(textureID, xsize, ysize, false); }
	explicit CSkyBox(const std::string& texture);
	~CSkyBox() override;
	void Init(uint32_t textureID, uint32_t xsize, uint32_t ysize, bool convertToCM);

	void Update() override {}
	void UpdateSunDir() override {}
	void UpdateSkyTexture() override {}

	void Draw();

	bool IsValid() const override { return valid; }

	std::string GetName() const override { return "SkyBox"; }

	void SetLuaTexture(const MapTextureData& td) override
	{
		skyTex.SetLuaTexture(td);
	}

private:
	VAO skyVAO; //even though VAO has no attached VBOs, it's still needed to perform rendering
	Shader::IProgramObject* shader;
	MapTexture skyTex;
	bool valid = false;
};

#endif // SKY_BOX_H
