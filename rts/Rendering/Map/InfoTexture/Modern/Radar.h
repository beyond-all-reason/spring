/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _RADAR_TEXTURE_H
#define _RADAR_TEXTURE_H


#include "ModernInfoTexture.h"
#include "Rendering/GL/FBO.h"


namespace Shader {
	struct IProgramObject;
}


class CRadarTexture : public CModernInfoTexture
{
public:
	// allyTeam < 0 tracks gu->myAllyTeam, otherwise the given allyteam
	CRadarTexture(int allyTeam = -1);
	~CRadarTexture() override;

public:
	void Update() override;
	bool IsUpdateNeeded() override { return true; }
private:
	GL::Texture2D uploadTexRadar;
	GL::Texture2D uploadTexJammer;
	const int allyTeam;
};

#endif // _RADAR_TEXTURE_H
