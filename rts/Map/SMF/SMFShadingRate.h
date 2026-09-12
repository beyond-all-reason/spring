/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <vector>

#include "System/type2.h"

// coarse (2x2/4x4) shading of distant ground via GL_NV_shading_rate_image, no-op elsewhere;
// a small screen-space rate image is rebuilt per frame from the camera and the terrain heights
class CSMFShadingRate {
public:
	void Kill();

	void SetElmosPerPixel(float f) { elmosPerPixel = f; }
	float GetElmosPerPixel() const { return elmosPerPixel; }

	// returns true iff coarse shading was enabled for this frame
	bool Enable();
	void Disable();

private:
	void UpdateImage();

	unsigned int texID = 0;

	int2 texSize = {0, 0};
	// rate-image texel footprint in framebuffer pixels
	int2 texelSize = {16, 16};

	std::vector<uint8_t> rateData;

	float elmosPerPixel = 0.0f;
};
