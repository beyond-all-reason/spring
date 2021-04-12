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

	virtual bool Init(const CUnitDrawer* ud) { this->ud = ud; return true; };
	virtual void Kill() = 0;

	virtual void EnableCommon(const CUnitDrawer* ud, bool alphaPass) = 0;
	virtual void DisableCommon(const CUnitDrawer* ud, bool alphaPass) = 0;

	virtual void EnableShadow(const CUnitDrawer* ud) = 0;
	virtual void DisableShadow(const CUnitDrawer* ud) = 0;

	virtual void UpdateCurrentShaderSky(const CUnitDrawer* ud, const ISkyLight* skyLight) const {}
	virtual void SetTeamColor(int team, const float2 alpha) const {};
	virtual void SetNanoColor(const float4& color) const {}

	enum ModelShaderProgram {
		MODEL_SHADER_NOSHADOW_STANDARD = 0, ///< model shader (V+F) without self-shadowing
		MODEL_SHADER_SHADOWED_STANDARD = 1, ///< model shader (V+F) with self-shadowing
		MODEL_SHADER_NOSHADOW_DEFERRED = 2, ///< deferred version of MODEL_SHADER_NOSHADOW (GLSL-only)
		MODEL_SHADER_SHADOWED_DEFERRED = 3, ///< deferred version of MODEL_SHADER_SHADOW (GLSL-only)

		MODEL_SHADER_COUNT             = 4,
	};
protected:
	void SetActiveShader(Shader::IProgramObject* shadowShader);
	virtual void SetActiveShader() = 0;
protected:
	const CUnitDrawer* ud;
	std::array<Shader::IProgramObject*, MODEL_SHADER_COUNT> modelShaders;
	Shader::IProgramObject* activeShader = nullptr;
};

struct UnitDrawerStateGLSL: public IUnitDrawerState {
public:
	virtual bool Init(const CUnitDrawer* ud) override;
	virtual void Kill() override;

	virtual void UpdateCurrentShaderSky(const CUnitDrawer* ud, const ISkyLight* skyLight) const override;
	virtual void SetTeamColor(int team, const float2 alpha) const override;
	virtual void SetNanoColor(const float4& color) const override;

	// Inherited via IUnitDrawerState
	virtual void EnableCommon(const CUnitDrawer* ud, bool alphaPass) override;
	virtual void DisableCommon(const CUnitDrawer* ud, bool alphaPass) override;
	virtual void EnableShadow(const CUnitDrawer* ud) override;
	virtual void DisableShadow(const CUnitDrawer* ud) override;

	virtual void SetActiveShader() override { IUnitDrawerState::SetActiveShader(shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL)); };
};

struct UnitDrawerStateGLSL4 : public IUnitDrawerState {
public:
	virtual bool Init(const CUnitDrawer* ud) override;
	virtual void Kill() override;

	virtual void SetTeamColor(int team, const float2 alpha) const override {}; //info exists in the shader
	virtual void SetNanoColor(const float4& color) const override;
public:
	enum class SHADER_DRAWING_MODES_GL4 {
		MODEL_PLAYER = -1,
		LM_PLAYER = 0,
		LM_SHADOW = 1,
		LM_REFLECTION = 2,
	};
	void SetColorMultiplier(float a = 1.0f) { SetColorMultiplier(1.0f, 1.0f, 1.0f, a); };
	void SetColorMultiplier(float r, float g, float b, float a);
	void SetDrawingMode(const SHADER_DRAWING_MODES_GL4 drawMode);
	void SetStaticModelMatrix(const CMatrix44f& mat);
protected:
	// Inherited via IUnitDrawerState
	virtual void EnableCommon(const CUnitDrawer* ud, bool alphaPass) override;
	virtual void DisableCommon(const CUnitDrawer* ud, bool alphaPass) override;
	virtual void EnableShadow(const CUnitDrawer* ud) override;
	virtual void DisableShadow(const CUnitDrawer* ud) override;

	virtual void SetActiveShader() override { IUnitDrawerState::SetActiveShader(shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL_GL4)); };
};

#endif

