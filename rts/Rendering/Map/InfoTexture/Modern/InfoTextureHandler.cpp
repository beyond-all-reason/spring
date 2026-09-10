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
#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/StringHash.h"

#include "System/Misc/TracyDefs.h"




CInfoTextureHandler::CInfoTextureHandler()
{
	AddInfoTexture(infoTex = new CInfoTextureCombiner());
	AddInfoTexture(new CLosTexture());
	AddInfoTexture(new CAirLosTexture());
	AddInfoTexture(new CMetalTexture());
	AddInfoTexture(new CMetalExtractionTexture());
	AddInfoTexture(new CRadarTexture());
	AddInfoTexture(new CHeightTexture());
	AddInfoTexture(new CPathTexture());
	// TODO?
	//AddInfoTexture(new CHeatTexture());
	//AddInfoTexture(new CFlowTexture());
	//AddInfoTexture(new CPathCostTexture());
}


CInfoTextureHandler::~CInfoTextureHandler()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (auto& pitex: infoTextures) {
		delete pitex.second;
	}
	for (auto& pitex: allyTeamInfoTextures) {
		delete pitex.second.tex;
	}
	infoTextureHandler = nullptr;
}


void CInfoTextureHandler::AddInfoTexture(CModernInfoTexture* itex)
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


CInfoTexture* CInfoTextureHandler::GetInfoTexture(const std::string& name, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!teamHandler.IsValidAllyTeam(allyTeam))
		return nullptr;

	const std::string key = name + "_" + std::to_string(allyTeam);

	const auto it = allyTeamInfoTextures.find(key);

	if (it != allyTeamInfoTextures.end()) {
		AllyTeamInfoTexture& entry = it->second;

		// updated on-demand instead of in the per-frame update loop,
		// so unrequested textures cost nothing
		if (entry.tex != nullptr && entry.lastUpdateFrame != globalRendering->drawFrame) {
			entry.lastUpdateFrame = globalRendering->drawFrame;
			UpdateAllyTeamInfoTexture(entry.tex);
		}

		return entry.tex;
	}

	// constructing (and initially updating) these binds FBO's and changes
	// the viewport; save and restore both since creation can be triggered
	// by a Lua texture-parse in the middle of a draw pass
	GLint prevFBO = 0;
	GLint prevViewport[4];
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	CModernInfoTexture* itex = nullptr;

	try {
		switch (hashString(name.c_str())) {
			case hashString("los"   ): { itex = new CLosTexture(allyTeam);    } break;
			case hashString("airlos"): { itex = new CAirLosTexture(allyTeam); } break;
			case hashString("radar" ): {
				// the radar texture samples its allyteam's los-texture, create that first
				if (GetInfoTexture("los", allyTeam) != nullptr)
					itex = new CRadarTexture(allyTeam);
			} break;
			default: {
				// per-allyteam variants exist only for the LOS-derived textures
				return nullptr;
			} break;
		}

	} catch (const opengl_error&) {
		LOG_L(L_ERROR, "[CInfoTextureHandler::%s] could not create info-texture \"%s\"", __func__, key.c_str());
		itex = nullptr;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

	if (itex != nullptr)
		UpdateAllyTeamInfoTexture(itex); // avoids one frame of uninitialized texture data

	// also insert nullptr on failure, prevents retrying every request
	allyTeamInfoTextures[key] = { itex, globalRendering->drawFrame };
	return itex;
}


void CInfoTextureHandler::UpdateAllyTeamInfoTexture(CModernInfoTexture* itex) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// updating binds FBO's, changes the viewport and binds textures on
	// the active unit; save and restore the state around it since this
	// can be triggered by a Lua texture-parse mid draw-pass, or nested
	// inside another texture's update (radar samples its los-texture)
	GLint prevFBO = 0;
	GLint prevViewport[4];
	GLint prevActiveTex = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
	glGetIntegerv(GL_VIEWPORT, prevViewport);
	glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTex);

	glActiveTexture(GL_TEXTURE0);
	itex->Update();

	glActiveTexture(prevActiveTex);
	glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
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

const std::vector<std::string> CInfoTextureHandler::GetModes() const
{
	std::vector<string> modes;
	modes.reserve(infoTextures.size());

	for(const auto& [mode, tex]: infoTextures)
		modes.push_back(mode);

	return modes;
}

bool CInfoTextureHandler::HasMode(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return infoTextures.contains(name);
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

	for (auto& [name, tex] : infoTextures) {
		// force first update except for combiner; hides visible uninitialized texmem
		if ((firstUpdate && tex != infoTex) || tex->IsUpdateNeeded())
			tex->Update();
	}

	firstUpdate = false;
}

