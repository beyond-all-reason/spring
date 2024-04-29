/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InfoTextureHandler.h"
#include "AirLos.h"
#include "Combiner.h"
#include "Height.h"
#include "Los.h"
#include "Metal.h"
#include "MetalExtraction.h"
#include "Path.h"
#include "Radar.h"

#include "System/Misc/TracyDefs.h"




CInfoTextureHandler::CInfoTextureHandler()
{
	if (infoTextureHandler == nullptr)
		infoTextureHandler = this;

	AddInfoTexture(infoTex = new CInfoTextureCombiner());
	AddInfoTexture(new CLosTexture());
	AddInfoTexture(new CAirLosTexture());
	AddInfoTexture(new CMetalTexture());
	AddInfoTexture(new CMetalExtractionTexture());
	AddInfoTexture(new CRadarTexture());
	AddInfoTexture(new CHeightTexture());
	AddInfoTexture(new CPathTexture());

	// avoid calling this here, it introduces dependencies
	// on engine components that have not been created yet
	// (HeightMapTexture, GuiHandler, ...)
	// Update();
}


CInfoTextureHandler::~CInfoTextureHandler()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (auto& pitex: infoTextures) {
		delete pitex.second;
	}
	infoTextureHandler = nullptr;
}


void CInfoTextureHandler::AddInfoTexture(CPboInfoTexture* itex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	infoTextures[itex->GetName()] = itex;
}


const CInfoTexture* CInfoTextureHandler::GetInfoTextureConst(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	static const CDummyInfoTexture dummy;

	const auto it = infoTextures.find(name);

	if (it != infoTextures.end())
		return it->second;

	return &dummy;
}

CInfoTexture* CInfoTextureHandler::GetInfoTexture(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (const_cast<CInfoTexture*>(GetInfoTextureConst(name)));
}


bool CInfoTextureHandler::IsEnabled() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (infoTex->IsEnabled());
}


void CInfoTextureHandler::DisableCurrentMode()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (returnToLOS && (GetMode() != "los")) {
		// return to LOS-mode if it was active before
		SetMode("los");
	} else {
		// otherwise disable overlay entirely
		SetMode("");
	}
}


void CInfoTextureHandler::SetMode(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	returnToLOS &= (name !=      ""); // NOLINT(readability-container-size-empty)
	returnToLOS |= (name ==   "los");
	inMetalMode  = (name == "metal");

	infoTex->SwitchMode(name);
}


void CInfoTextureHandler::ToggleMode(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (infoTex->GetMode() == name)
		return (DisableCurrentMode());

	SetMode(name);
}


const std::string& CInfoTextureHandler::GetMode() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (infoTex->GetMode());
}

GLuint CInfoTextureHandler::GetCurrentInfoTexture() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (infoTex->GetTexture());
}

int2 CInfoTextureHandler::GetCurrentInfoTextureSize() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (infoTex->GetTexSize());
}


void CInfoTextureHandler::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	for (auto& p: infoTextures) {
		CPboInfoTexture* tex = p.second;

		// force first update except for combiner; hides visible uninitialized texmem
		if ((firstUpdate && tex != infoTex) || tex->IsUpdateNeeded())
			tex->Update();
	}

	firstUpdate = false;
}

