/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDRAWER_STATE_H
#define UNITDRAWER_STATE_H

#include <array>

#include "System/type2.h"
#include "Game/Game.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"

struct float4;
class CUnitDrawer;
class CCamera;
struct ISkyLight;

namespace Shader {
	struct IProgramObject;
}

struct IUnitDrawerState {
public:
	static void PushTransform(const CCamera* cam);
	static void PopTransform();
	static float4 GetTeamColor(int team, float alpha);

	IUnitDrawerState() { modelShaders.fill(nullptr); }
	virtual ~IUnitDrawerState() {}

	virtual bool Init(const CUnitDrawer* ud) { return false; }
	virtual void Kill() {}

	virtual bool CanDrawAlpha() const { return true; }
	virtual bool CanDrawDeferred() const { return true; }

	virtual void Enable(const CUnitDrawer* ud, bool alphaPass) { EnableCommon(ud, alphaPass); };
	virtual void Disable(const CUnitDrawer* ud) { DisableCommon(ud); };

	virtual void EnableTextures() const { EnableTexturesCommon(); };
	virtual void DisableTextures() const { DisableTexturesCommon(); };
	virtual void EnableShaders(const CUnitDrawer*) { modelShaders[MODEL_SHADER_ACTIVE]->Enable(); }
	virtual void DisableShaders(const CUnitDrawer*) { modelShaders[MODEL_SHADER_ACTIVE]->Disable(); }

	virtual void UpdateCurrentShaderSky(const CUnitDrawer* ud, const ISkyLight* skyLight) const {}
	virtual void SetTeamColor(int team, const float2 alpha) const = 0;
	virtual void SetNanoColor(const float4& color) const {}

	void SetActiveShader() {
		// shadowed=1 --> shader 1 (deferred=0) or 3 (deferred=1)
		// shadowed=0 --> shader 0 (deferred=0) or 2 (deferred=1)
		const bool shadowed = shadowHandler.ShadowsLoaded();
		const bool deferred = game->GetDrawMode() == CGame::GameDrawMode::gameDeferredDraw;

		modelShaders[MODEL_SHADER_ACTIVE] = modelShaders[shadowed + deferred * 2];
	}

	enum ModelShaderProgram {
		MODEL_SHADER_NOSHADOW_STANDARD = 0, ///< model shader (V+F) without self-shadowing
		MODEL_SHADER_SHADOWED_STANDARD = 1, ///< model shader (V+F) with self-shadowing
		MODEL_SHADER_NOSHADOW_DEFERRED = 2, ///< deferred version of MODEL_SHADER_NOSHADOW (GLSL-only)
		MODEL_SHADER_SHADOWED_DEFERRED = 3, ///< deferred version of MODEL_SHADER_SHADOW (GLSL-only)

		MODEL_SHADER_ACTIVE            = 4, ///< currently active model shader
		MODEL_SHADER_COUNT             = 5,
	};

protected:
	// shared ARB and GLSL state managers
	virtual void EnableCustomFFPState(const CUnitDrawer* ud, bool alphaPass) {};
	virtual void DisableCustomFFPState(const CUnitDrawer* ud) {};

	virtual void EnableCustomShaderState(const CUnitDrawer* ud, bool alphaPass) {};
	virtual void DisableCustomShaderState(const CUnitDrawer* ud) {};

	void EnableCommon(const CUnitDrawer* ud, bool alphaPass);
	void DisableCommon(const CUnitDrawer* ud);
	void EnableTexturesCommon() const;
	void DisableTexturesCommon() const;

protected:
	std::array<Shader::IProgramObject*, MODEL_SHADER_COUNT> modelShaders;
};

struct UnitDrawerStateGLSL: public IUnitDrawerState {
public:
	virtual bool Init(const CUnitDrawer* ud) override;
	virtual void Kill() override;

	virtual void UpdateCurrentShaderSky(const CUnitDrawer* ud, const ISkyLight* skyLight) const override;
	virtual void SetTeamColor(int team, const float2 alpha) const override;
	virtual void SetNanoColor(const float4& color) const override;
protected:
	virtual void EnableCustomFFPState(const CUnitDrawer* ud, bool alphaPass) override;
	virtual void DisableCustomFFPState(const CUnitDrawer* ud) override;

	virtual void EnableCustomShaderState(const CUnitDrawer* ud, bool alphaPass) override;
};

struct UnitDrawerStateGLSL4 : public IUnitDrawerState {
public:
	virtual bool Init(const CUnitDrawer* ud) override;
	virtual void Kill() override;

	virtual void SetTeamColor(int team, const float2 alpha) const override {}; //info exists in the shader
	virtual void SetNanoColor(const float4& color) const override;
protected:
	virtual void EnableCustomFFPState(const CUnitDrawer* ud,bool alphaPass) override;
	virtual void DisableCustomFFPState(const CUnitDrawer* ud) override;
	virtual void EnableCustomShaderState(const CUnitDrawer* ud, bool alphaPass) override;
	virtual void DisableCustomShaderState(const CUnitDrawer* ud) override;
};

#endif

