/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _PATH_TEXTURE_H
#define _PATH_TEXTURE_H

#include <cstdint>
#include <vector>

#include "ModernInfoTexture.h"
#include "Rendering/GL/FBO.h"
#include "System/Color.h"
#include "System/Misc/SpringTime.h"


struct MoveDef;
struct UnitDef;


class CPathTexture : public CModernInfoTexture
{
public:
	CPathTexture();

public:
	void Update() override;
	bool IsUpdateNeeded() override;

	GLuint GetTexture() override;

	bool ShowMoveDef(const int pathType);
	bool ShowUnitDef(const int udefid);

private:
	const MoveDef* GetSelectedMoveDef();
	const UnitDef* GetCurrentBuildCmdUnitDef();

	void BeginSweep(bool allRows) { updateProcess = 0; restUpdates = 0; fadeAllRows = allRows; }
	void RestartSweep() { BeginSweep(true); }
	int AdvanceSweep(int texelsPerUpdate);

	void UpdateBuildView(const UnitDef* ud, int texStart, int texEnd);
	void UpdateMoveDefView(const MoveDef& md, int texStart, int texEnd);
	bool FadeRows();

private:
	bool isCleared;
//	int updateFrame;
	int updateProcess; // next texel row to compute, texSize.y when the sweep is complete
	int restUpdates;   // updates since the MoveDef sweep completed (see Update)
	unsigned int lastSelectedPathType;
	int forcedPathType;
	int forcedUnitDef;
	spring_time lastUsage;

	// CPU mirror of <texture>; the build view writes into it band by band,
	// the MoveDef view fills <sweepTexels> band by band and fades the rows
	// that changed into it (see Update)
	std::vector<SColor> shownTexels;
	std::vector<SColor> sweepTexels;

	// per texel row: fade steps left until it shows <sweepTexels> (0 = settled)
	std::vector<uint8_t> rowFadeSteps;
	std::vector<uint8_t> rowTouched;
	// first sweep after a view change: every computed row fades in
	bool fadeAllRows;

	// MoveDef view scratch buffers (per map row flags, their horizontal and
	// their full footprint dilation), see UpdateMoveDefView
	std::vector<uint8_t> structSquares;
	std::vector<uint8_t> structSpans;
	std::vector<uint8_t> structFootprints;
	std::vector<uint8_t> waterSquares;
	std::vector<uint8_t> waterSpans;
	std::vector<uint8_t> waterFootprints;
	std::vector<uint8_t> rowFlags;
};

#endif // _PATH_TEXTURE_H
