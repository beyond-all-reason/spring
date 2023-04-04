/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include "Map/MapDrawPassTypes.h"

class CSMFGroundDrawer;
struct ISkyLight;
struct LuaMapShaderData;

namespace Shader {
	struct IProgramObject;
}

enum {
	RENDER_STATE_SSP = 0, // standard-shader path (GLSL)
	RENDER_STATE_LUA = 1, // Lua-shader path
	RENDER_STATE_NOP = 2, // NO-OP path
	RENDER_STATE_SEL = 3, // selected path
	RENDER_STATE_CNT = 4,
};


struct ISMFRenderState {
public:
	static ISMFRenderState* GetInstance(bool luaShader, bool noop);
	static void FreeInstance(ISMFRenderState* state) { delete state; }

	virtual ~ISMFRenderState() {}
	virtual bool Init(const CSMFGroundDrawer* smfGroundDrawer) = 0;
	virtual void Kill() = 0;
	virtual void Update(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) = 0;

	virtual bool HasValidShader(const DrawPass::e& drawPass) const = 0;
	virtual bool CanDrawForward() const = 0;
	virtual bool CanDrawDeferred() const = 0;

	virtual void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;
	virtual void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;

	virtual void SetSquareTexGen(const int sqx, const int sqy) const = 0;
	virtual void SetCurrentShader(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;
	virtual void UpdateCurrentShaderSky(const CSMFGroundDrawer* smfGroundDrawer) = 0;
};


struct SMFRenderStateNOOP : public ISMFRenderState {
public:
	~SMFRenderStateNOOP() override = default;
	bool Init(const CSMFGroundDrawer* smfGroundDrawer) override { return false; }
	void Kill() override {}
	void Update(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) override {}

	bool HasValidShader(const DrawPass::e& drawPass) const override { return true; }
	bool CanDrawForward() const override { return false; }
	bool CanDrawDeferred() const override { return false; }

	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override {}
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override {}

	void SetSquareTexGen(const int sqx, const int sqy) const override {}
	void SetCurrentShader(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override {}
	void UpdateCurrentShaderSky(const CSMFGroundDrawer* smfGroundDrawer) override {}
};

struct SMFRenderStateGLSL: public ISMFRenderState {
public:
	explicit SMFRenderStateGLSL(bool lua)
		: useLuaShaders(lua)
		, updateSkyUniforms(true)
	{
		glslShaders.fill(nullptr);
	}

	~SMFRenderStateGLSL() override { glslShaders.fill(nullptr); }

	bool Init(const CSMFGroundDrawer* smfGroundDrawer) override;
	void Kill() override;
	void Update(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) override;

	bool HasValidShader(const DrawPass::e& drawPass) const override;
	bool CanDrawForward() const override { return true; }
	bool CanDrawDeferred() const override { return true; }

	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override;
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override;

	void SetSquareTexGen(const int sqx, const int sqy) const override;
	void SetCurrentShader(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override;
	void UpdateCurrentShaderSky(const CSMFGroundDrawer* smfGroundDrawer) override { updateSkyUniforms = true; }

	enum {
		GLSL_SHADER_FWD_STD = 0,
		GLSL_SHADER_FWD_ADV = 1,
		GLSL_SHADER_DFR_ADV = 2,
		GLSL_SHADER_COUNT   = 3,
	};

private:
	std::array<Shader::IProgramObject*, GLSL_SHADER_COUNT> glslShaders;
	Shader::IProgramObject* currShader = nullptr;

	// if true, shader programs for this state are Lua-defined
	bool useLuaShaders;
	bool updateSkyUniforms;
};