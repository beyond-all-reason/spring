/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDrawerState.hpp"
#include "UnitDrawer.h"
#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Env/SkyLight.h"
#include "Rendering/GL/GeometryBuffer.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"



void IUnitDrawerState::PushTransform(const CCamera* cam) {
	// set model-drawing transform; view is combined with projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMultMatrixf(cam->GetViewMatrix());
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

void IUnitDrawerState::PopTransform() {
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}



float4 IUnitDrawerState::GetTeamColor(int team, float alpha) {
	assert(teamHandler.IsValidTeam(team));

	const   CTeam* t = teamHandler.Team(team);
	const uint8_t* c = t->color;

	return (float4(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, alpha));
}

void IUnitDrawerState::EnableCommon(const CUnitDrawer* ud, bool alphaPass) {
	glPolygonMode(GL_FRONT_AND_BACK, ud->GetPolygonMode());

	glEnable(GL_TEXTURE_2D);

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	if (alphaPass) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
	}

	EnableCustomFFPState(ud, alphaPass);

	PushTransform(camera);
	EnableTexturesCommon();

	SetActiveShader();
	assert(modelShaders[MODEL_SHADER_ACTIVE] != nullptr);
	modelShaders[MODEL_SHADER_ACTIVE]->Enable();
	EnableCustomShaderState(ud, alphaPass);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void IUnitDrawerState::DisableCommon(const CUnitDrawer* ud) {
	assert(modelShaders[MODEL_SHADER_ACTIVE] != nullptr);

	DisableCustomShaderState(ud);
	modelShaders[MODEL_SHADER_ACTIVE]->Disable();

	DisableCustomFFPState(ud);

	glPopAttrib();

	DisableTexturesCommon();
	PopTransform();
}


void IUnitDrawerState::EnableTexturesCommon() const {
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	if (game->GetDrawMode() == CGame::GameDrawMode::gameShadowDraw) //shadow pass
		return;

	if (shadowHandler.ShadowsLoaded())
		shadowHandler.SetupShadowTexSampler(GL_TEXTURE2, true);

	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler.GetEnvReflectionTextureID());

	glActiveTexture(GL_TEXTURE4);
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler.GetSpecularTextureID());

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
}

void IUnitDrawerState::DisableTexturesCommon() const {
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);

	if (game->GetDrawMode() == CGame::GameDrawMode::gameShadowDraw) //shadow pass
		return;

	if (shadowHandler.ShadowsLoaded())
		shadowHandler.ResetShadowTexSampler(GL_TEXTURE2, true);

	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	glActiveTexture(GL_TEXTURE4);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
}

bool UnitDrawerStateGLSL::Init(const CUnitDrawer* ud) {
	#define sh shaderHandler

	const GL::LightHandler* lightHandler = ud->GetLightHandler();
	const std::string shaderNames[MODEL_SHADER_COUNT - 1] = {
		"ModelShaderGLSL-NoShadowStandard",
		"ModelShaderGLSL-ShadowedStandard",
		"ModelShaderGLSL-NoShadowDeferred",
		"ModelShaderGLSL-ShadowedDeferred",
	};
	const std::string extraDefs =
		("#define BASE_DYNAMIC_MODEL_LIGHT " + IntToString(lightHandler->GetBaseLight()) + "\n") +
		("#define MAX_DYNAMIC_MODEL_LIGHTS " + IntToString(lightHandler->GetMaxLights()) + "\n");

	for (unsigned int n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n] = sh->CreateProgramObject("[UnitDrawer]", shaderNames[n]);
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelVertProg.glsl", extraDefs, GL_VERTEX_SHADER));
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelFragProg.glsl", extraDefs, GL_FRAGMENT_SHADER));

		modelShaders[n]->SetFlag("USE_SHADOWS", int((n & 1) == 1));
		modelShaders[n]->SetFlag("DEFERRED_MODE", int(n >= MODEL_SHADER_NOSHADOW_DEFERRED));
		modelShaders[n]->SetFlag("GBUFFER_NORMTEX_IDX", GL::GeometryBuffer::ATTACHMENT_NORMTEX);
		modelShaders[n]->SetFlag("GBUFFER_DIFFTEX_IDX", GL::GeometryBuffer::ATTACHMENT_DIFFTEX);
		modelShaders[n]->SetFlag("GBUFFER_SPECTEX_IDX", GL::GeometryBuffer::ATTACHMENT_SPECTEX);
		modelShaders[n]->SetFlag("GBUFFER_EMITTEX_IDX", GL::GeometryBuffer::ATTACHMENT_EMITTEX);
		modelShaders[n]->SetFlag("GBUFFER_MISCTEX_IDX", GL::GeometryBuffer::ATTACHMENT_MISCTEX);
		modelShaders[n]->SetFlag("GBUFFER_ZVALTEX_IDX", GL::GeometryBuffer::ATTACHMENT_ZVALTEX);

		modelShaders[n]->Link();
		modelShaders[n]->SetUniformLocation("diffuseTex");        // idx  0 (t1: diffuse + team-color)
		modelShaders[n]->SetUniformLocation("shadingTex");        // idx  1 (t2: spec/refl + self-illum)
		modelShaders[n]->SetUniformLocation("shadowTex");         // idx  2
		modelShaders[n]->SetUniformLocation("reflectTex");        // idx  3 (cube)
		modelShaders[n]->SetUniformLocation("specularTex");       // idx  4 (cube)
		modelShaders[n]->SetUniformLocation("sunDir");            // idx  5
		modelShaders[n]->SetUniformLocation("cameraPos");         // idx  6
		modelShaders[n]->SetUniformLocation("cameraMat");         // idx  7
		modelShaders[n]->SetUniformLocation("cameraMatInv");      // idx  8
		modelShaders[n]->SetUniformLocation("teamColor");         // idx  9
		modelShaders[n]->SetUniformLocation("nanoColor");         // idx 10
		modelShaders[n]->SetUniformLocation("sunAmbient");        // idx 11
		modelShaders[n]->SetUniformLocation("sunDiffuse");        // idx 12
		modelShaders[n]->SetUniformLocation("shadowDensity");     // idx 13
		modelShaders[n]->SetUniformLocation("shadowMatrix");      // idx 14
		modelShaders[n]->SetUniformLocation("shadowParams");      // idx 15
		// modelShaders[n]->SetUniformLocation("alphaPass");         // idx 16

		modelShaders[n]->Enable();
		modelShaders[n]->SetUniform1i(0, 0); // diffuseTex  (idx 0, texunit 0)
		modelShaders[n]->SetUniform1i(1, 1); // shadingTex  (idx 1, texunit 1)
		modelShaders[n]->SetUniform1i(2, 2); // shadowTex   (idx 2, texunit 2)
		modelShaders[n]->SetUniform1i(3, 3); // reflectTex  (idx 3, texunit 3)
		modelShaders[n]->SetUniform1i(4, 4); // specularTex (idx 4, texunit 4)
		modelShaders[n]->SetUniform3fv(5, &sky->GetLight()->GetLightDir().x);
		modelShaders[n]->SetUniform3fv(6, &camera->GetPos()[0]);
		modelShaders[n]->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
		modelShaders[n]->SetUniformMatrix4fv(8, false, camera->GetViewMatrixInverse());
		modelShaders[n]->SetUniform4f(9, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->SetUniform4f(10, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->SetUniform3fv(11, &sunLighting->modelAmbientColor[0]);
		modelShaders[n]->SetUniform3fv(12, &sunLighting->modelDiffuseColor[0]);
		modelShaders[n]->SetUniform1f(13, sunLighting->modelShadowDensity);
		modelShaders[n]->SetUniformMatrix4fv(14, false, shadowHandler.GetShadowMatrixRaw());
		modelShaders[n]->SetUniform4fv(15, &(shadowHandler.GetShadowParams().x));
		// modelShaders[n]->SetUniform1f(16, 0.0f); // alphaPass
		modelShaders[n]->Disable();
		modelShaders[n]->Validate();
	}

	// make the active shader non-NULL
	SetActiveShader();

	return true;

#undef sh
}

void UnitDrawerStateGLSL::Kill() {
	modelShaders.fill(nullptr);
	shaderHandler->ReleaseProgramObjects("[UnitDrawer]");
}

void UnitDrawerStateGLSL::EnableCustomShaderState(const CUnitDrawer* ud, bool alphaPass) {
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform3fv(6, &camera->GetPos()[0]);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformMatrix4fv(8, false, camera->GetViewMatrixInverse());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformMatrix4fv(14, false, shadowHandler.GetShadowMatrixRaw());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(15, &(shadowHandler.GetShadowParams().x));

	const_cast<GL::LightHandler*>(ud->GetLightHandler())->Update(modelShaders[MODEL_SHADER_ACTIVE]);
}

void UnitDrawerStateGLSL::EnableCustomFFPState(const CUnitDrawer* ud, bool alphaPass)
{
	glEnable(GL_ALPHA_TEST);

	if (alphaPass)
		glAlphaFunc(GL_GREATER, 0.1f);
	else
		glAlphaFunc(GL_GREATER, 0.5f);
}

void UnitDrawerStateGLSL::DisableCustomFFPState(const CUnitDrawer* ud)
{
	glDisable(GL_ALPHA_TEST);
}

void UnitDrawerStateGLSL::UpdateCurrentShaderSky(const CUnitDrawer* ud, const ISkyLight* skyLight) const {
	// note: the NOSHADOW shaders do not care about shadow-density
	for (unsigned int n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n]->Enable();
		modelShaders[n]->SetUniform3fv(5, &skyLight->GetLightDir().x);
		modelShaders[n]->SetUniform3fv(11, &sunLighting->modelAmbientColor[0]);
		modelShaders[n]->SetUniform3fv(12, &sunLighting->modelDiffuseColor[0]);
		modelShaders[n]->SetUniform1f(13, sunLighting->modelShadowDensity);
		modelShaders[n]->Disable();
	}
}


void UnitDrawerStateGLSL::SetTeamColor(int team, const float2 alpha) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());

	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(9, std::move(GetTeamColor(team, alpha.x)));
	// modelShaders[MODEL_SHADER_ACTIVE]->SetUniform1f(16, alpha.y);
}

void UnitDrawerStateGLSL::SetNanoColor(const float4& color) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());

	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(10, color);
}

bool UnitDrawerStateGLSL4::Init(const CUnitDrawer* ud)
{
#define sh shaderHandler

	const std::string shaderNames[MODEL_SHADER_COUNT - 1] = {
		"ModelShaderGLSL4-NoShadowStandard",
		"ModelShaderGLSL4-ShadowedStandard",
		"ModelShaderGLSL4-NoShadowDeferred",
		"ModelShaderGLSL4-ShadowedDeferred",
	};

	for (unsigned int n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n] = sh->CreateProgramObject("[UnitDrawer-GL4]", shaderNames[n]);
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelVertProgGL4.glsl", "", GL_VERTEX_SHADER));
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelFragProgGL4.glsl", "", GL_FRAGMENT_SHADER));

		modelShaders[n]->SetFlag("USE_SHADOWS", int((n & 1) == 1));
		modelShaders[n]->SetFlag("DEFERRED_MODE", int(n >= MODEL_SHADER_NOSHADOW_DEFERRED));
		modelShaders[n]->SetFlag("GBUFFER_NORMTEX_IDX", GL::GeometryBuffer::ATTACHMENT_NORMTEX);
		modelShaders[n]->SetFlag("GBUFFER_DIFFTEX_IDX", GL::GeometryBuffer::ATTACHMENT_DIFFTEX);
		modelShaders[n]->SetFlag("GBUFFER_SPECTEX_IDX", GL::GeometryBuffer::ATTACHMENT_SPECTEX);
		modelShaders[n]->SetFlag("GBUFFER_EMITTEX_IDX", GL::GeometryBuffer::ATTACHMENT_EMITTEX);
		modelShaders[n]->SetFlag("GBUFFER_MISCTEX_IDX", GL::GeometryBuffer::ATTACHMENT_MISCTEX);
		modelShaders[n]->SetFlag("GBUFFER_ZVALTEX_IDX", GL::GeometryBuffer::ATTACHMENT_ZVALTEX);

		modelShaders[n]->Link();

		modelShaders[n]->Enable();
		modelShaders[n]->SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f); // alphaCtrl, default, always pass
		modelShaders[n]->Disable();

		modelShaders[n]->Validate();
	}

	// make the active shader non-NULL
	SetActiveShader();

	return true;

#undef sh
}

void UnitDrawerStateGLSL4::Kill() {
	modelShaders.fill(nullptr);
	shaderHandler->ReleaseProgramObjects("[UnitDrawer-GL4]");
}

void UnitDrawerStateGLSL4::SetNanoColor(const float4& color) const
{
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform("nanoColor", color.x, color.y, color.z, color.w);
}

void UnitDrawerStateGLSL4::EnableCustomFFPState(const CUnitDrawer* ud, bool alphaPass)
{
	S3DModelVAO::GetInstance().Bind();
}

void UnitDrawerStateGLSL4::DisableCustomFFPState(const CUnitDrawer* ud)
{
	S3DModelVAO::GetInstance().Unbind();
}

void UnitDrawerStateGLSL4::EnableCustomShaderState(const CUnitDrawer* ud, bool alphaPass)
{
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());

	int camMode = 0;
	switch (game->GetDrawMode()) {
		case CGame::GameDrawMode::gameShadowDraw: { camMode = 1; }
		case CGame::GameDrawMode::gameReflectionDraw: { camMode = 2; }
		default: { camMode = 0; } break;
	}

	LOG("[UnitDrawerStateGLSL4::EnableCustomShaderState] camMode = %d", camMode);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform("cameraMode", camMode);

	if (alphaPass)
		modelShaders[MODEL_SHADER_ACTIVE]->SetUniform("alphaCtrl", 0.1f, 1.0f, 0.0f, 0.0f); // test > 0.1
	else
		modelShaders[MODEL_SHADER_ACTIVE]->SetUniform("alphaCtrl", 0.5f, 1.0f, 0.0f, 0.0f); // test > 0.5
}

void UnitDrawerStateGLSL4::DisableCustomShaderState(const CUnitDrawer* ud)
{
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());

	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform("cameraMode", 0);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f); //default, always pass
}
