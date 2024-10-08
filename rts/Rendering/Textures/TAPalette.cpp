/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TAPalette.h"
#include "System/FileSystem/FileHandler.h"

#include "System/Misc/TracyDefs.h"

void CTAPalette::Init(CFileHandler& paletteFile)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!paletteFile.FileExists())
		return;

	for (SColor& color: colors) {
		paletteFile.Read(&color.i, 4);
		// ignore alpha
		color.a = 255;
	}
}

