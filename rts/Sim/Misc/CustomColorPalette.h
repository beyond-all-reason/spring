/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <array>
#include <cstdint>

#include "System/float4.h"

static constexpr int MAX_PALETTE_COLORS = 2048;
static constexpr int CUSTOM_COLOR_PALETTE_BASE = 256; // first custom color index (power of 2 aligned)
static constexpr int MAX_CUSTOM_COLORS  = MAX_PALETTE_COLORS - CUSTOM_COLOR_PALETTE_BASE; // 1792

class CCustomColorPalette {
public:
	void Init();
	void Kill();

	void SetColor(uint16_t index, float r, float g, float b);
	float4 GetColor(uint16_t index) const;
	static bool IsValidIndex(uint16_t index) { return index < MAX_CUSTOM_COLORS; }

	static uint16_t EncodePaletteIndex(uint16_t customIndex) {
		return CUSTOM_COLOR_PALETTE_BASE + customIndex;
	}

	static uint16_t DecodePaletteIndex(uint16_t paletteIndex) {
		return paletteIndex - CUSTOM_COLOR_PALETTE_BASE;
	}

	static bool IsCustomPaletteIndex(uint16_t paletteIndex) {
		return paletteIndex >= CUSTOM_COLOR_PALETTE_BASE;
	}

	static CCustomColorPalette& GetInstance() {
		static CCustomColorPalette instance;
		return instance;
	}

private:
	std::array<float4, MAX_CUSTOM_COLORS> colors{};
	bool initialized = false;
};

extern CCustomColorPalette customColorPalette;
