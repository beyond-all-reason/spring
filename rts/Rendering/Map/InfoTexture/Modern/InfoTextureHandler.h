/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <string>

#include "Rendering/GL/myGL.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"


class CModernInfoTexture;
class CInfoTextureCombiner;


class CInfoTextureHandler : public IInfoTextureHandler
{
public:
	CInfoTextureHandler();
	virtual ~CInfoTextureHandler();

	void Update() override;

public:
	bool IsEnabled() const override;
	bool InMetalMode() const override { return inMetalMode; }

	void DisableCurrentMode() override;
	void SetMode(const std::string& name) override;
	bool HasMode(const std::string& name) const override;
	void ToggleMode(const std::string& name) override;
	const std::string& GetMode() const override;
	const std::vector<std::string> GetModes() const override;

	GLuint GetCurrentInfoTexture() const override;
	int2 GetCurrentInfoTextureSize() const override;

public:
	const CInfoTexture* GetInfoTextureConst(const std::string& name) const override;
	      CInfoTexture* GetInfoTexture     (const std::string& name)       override;
	      CInfoTexture* GetInfoTexture     (const std::string& name, int allyTeam) override;

protected:
	friend class CModernInfoTexture;
	void AddInfoTexture(CModernInfoTexture*);

protected:
	bool returnToLOS = false;
	bool inMetalMode = false;
	bool firstUpdate =  true;

	spring::unordered_map<std::string, CModernInfoTexture*> infoTextures;

	// lazily created per-allyteam variants, keyed by "<name>_<allyTeam>";
	// kept separate from infoTextures so they do not show up as modes, and
	// updated on-demand when requested (at most once per draw-frame)
	// rather than in the per-frame update loop, so they cost nothing
	// while not in use
	struct AllyTeamInfoTexture {
		CModernInfoTexture* tex = nullptr;
		unsigned int lastUpdateFrame = 0;
	};

	void UpdateAllyTeamInfoTexture(CModernInfoTexture* itex) const;

	spring::unordered_map<std::string, AllyTeamInfoTexture> allyTeamInfoTextures;

	// special; always non-NULL at runtime
	CInfoTextureCombiner* infoTex = nullptr;
};

class CNullInfoTextureHandler : public IInfoTextureHandler
{
public:
	virtual ~CNullInfoTextureHandler() {};

	void Update() override {};

public:
	bool IsEnabled() const override { return false; }
	bool InMetalMode() const override { return false; }

	void DisableCurrentMode() override {}
	void SetMode(const std::string& name) override {}
	bool HasMode(const std::string& name) const override { return false; }
	void ToggleMode(const std::string& name) override {}
	const std::string& GetMode() const override { static const std::string modeMock = ""; return modeMock; }
	const std::vector<std::string> GetModes() const override { return std::vector<std::string>(); };

	GLuint GetCurrentInfoTexture() const override { return 0; }
	int2 GetCurrentInfoTextureSize() const override { return int2{ 1, 1 }; }

public:
	const CInfoTexture* GetInfoTextureConst(const std::string& name) const override { return nullptr; }
	      CInfoTexture* GetInfoTexture(const std::string& name)            override { return nullptr; }
	      CInfoTexture* GetInfoTexture(const std::string& name, int allyTeam) override { return nullptr; }
};