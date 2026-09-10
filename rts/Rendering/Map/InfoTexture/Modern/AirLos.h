/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AIRLOS_TEXTURE_H
#define _AIRLOS_TEXTURE_H

#include "ModernInfoTexture.h"
#include "Rendering/GL/FBO.h"


namespace Shader {
	struct IProgramObject;
}


class CAirLosTexture : public CModernInfoTexture
{
public:
	// allyTeam < 0 tracks gu->myAllyTeam, otherwise the given allyteam
	CAirLosTexture(int allyTeam = -1);
	~CAirLosTexture();

public:
	void Update() override;
	bool IsUpdateNeeded() override { return true; }
private:
	GL::Texture2D uploadTex;
	const int allyTeam;
};

#endif // _AIRLOS_TEXTURE_H
