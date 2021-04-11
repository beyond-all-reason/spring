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

void IUnitDrawerState::SetActiveShader(Shader::IProgramObject* shadowShader)
{
	// shadowed=1 --> shader 1 (deferred=0) or 3 (deferred=1)
	// shadowed=0 --> shader 0 (deferred=0) or 2 (deferred=1)
	if (shadowHandler.InShadowPass()) {
		activeShader = shadowShader;
		assert(activeShader != nullptr);
		return;
	}

	const bool shadowed = shadowHandler.ShadowsLoaded();
	const bool deferred = game->GetDrawMode() == CGame::GameDrawMode::gameDeferredDraw;

	activeShader = modelShaders[shadowed + deferred * 2];

	assert(activeShader != nullptr);
}

bool UnitDrawerStateGLSL::Init(const CUnitDrawer* ud) {
	#define sh shaderHandler

	const GL::LightHandler* lightHandler = ud->GetLightHandler();
	const std::string shaderNames[MODEL_SHADER_COUNT] = {
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
	assert(activeShader->IsBound());

	activeShader->SetUniform4fv(9, std::move(GetTeamColor(team, alpha.x)));
	// activeShader->SetUniform1f(16, alpha.y);
}

void UnitDrawerStateGLSL::SetNanoColor(const float4& color) const {
	assert(activeShader->IsBound());

	activeShader->SetUniform4fv(10, color);
}

void UnitDrawerStateGLSL::EnableCommon(const CUnitDrawer* ud, bool alphaPass)
{
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);

	glPolygonMode(GL_FRONT_AND_BACK, ud->GetPolygonMode());

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	if (shadowHandler.ShadowsLoaded())
		shadowHandler.SetupShadowTexSampler(GL_TEXTURE2, true);

	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler.GetEnvReflectionTextureID());

	glActiveTexture(GL_TEXTURE4);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler.GetSpecularTextureID());

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	if (alphaPass) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		glAlphaFunc(GL_GREATER, 0.1f);
	}
	else {
		glDisable(GL_BLEND);
		glAlphaFunc(GL_GREATER, 0.5f);
	}

	PushTransform(camera);

	SetActiveShader();
	activeShader->Enable();
	activeShader->SetUniform3fv(6, &camera->GetPos()[0]);
	activeShader->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
	activeShader->SetUniformMatrix4fv(8, false, camera->GetViewMatrixInverse());
	activeShader->SetUniformMatrix4fv(14, false, shadowHandler.GetShadowMatrixRaw());
	activeShader->SetUniform4fv(15, &(shadowHandler.GetShadowParams().x));

	const_cast<GL::LightHandler*>(ud->GetLightHandler())->Update(activeShader);
}

void UnitDrawerStateGLSL::DisableCommon(const CUnitDrawer* ud, bool alphaPass)
{
	assert(activeShader->IsBound());
	activeShader->Disable();

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);

	if (shadowHandler.ShadowsLoaded())
		shadowHandler.ResetShadowTexSampler(GL_TEXTURE2, true);

	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	glActiveTexture(GL_TEXTURE4);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);

	glDisable(GL_ALPHA_TEST);
	glPopAttrib();

	PopTransform();
}

void UnitDrawerStateGLSL::EnableShadow(const CUnitDrawer* ud)
{
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);

	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_ALPHA_TEST);

	glPolygonMode(GL_FRONT_AND_BACK, ud->GetPolygonMode());

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glColor3f(1.0f, 1.0f, 1.0f);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	SetActiveShader();
	activeShader->Enable();
}

void UnitDrawerStateGLSL::DisableShadow(const CUnitDrawer* ud)
{
	activeShader->Disable();

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);

	glPopAttrib();
}

bool UnitDrawerStateGLSL4::Init(const CUnitDrawer* ud)
{
#define sh shaderHandler

	const std::string shaderNames[MODEL_SHADER_COUNT] = {
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
	assert(activeShader->IsBound());
	activeShader->SetUniform("nanoColor", color.x, color.y, color.z, color.w);
}

void UnitDrawerStateGLSL4::EnableCommon(const CUnitDrawer* ud, bool alphaPass)
{
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);

	glPolygonMode(GL_FRONT_AND_BACK, ud->GetPolygonMode());

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);


	if (shadowHandler.ShadowsLoaded())
		shadowHandler.SetupShadowTexSampler(GL_TEXTURE2, true);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler.GetEnvReflectionTextureID());

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler.GetSpecularTextureID());

	glActiveTexture(GL_TEXTURE0);

	if (alphaPass) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
	}
	else {
		glDisable(GL_BLEND);
	}

	S3DModelVAO::GetInstance().Bind();

	const int camMode = (game->GetDrawMode() == CGame::GameDrawMode::gameReflectionDraw) ? 2 : 0;

	SetActiveShader();
	activeShader->Enable();
	activeShader->SetUniform("cameraMode", camMode);

	if (alphaPass)
		activeShader->SetUniform("alphaCtrl", 0.1f, 1.0f, 0.0f, 0.0f); // test > 0.1
	else
		activeShader->SetUniform("alphaCtrl", 0.5f, 1.0f, 0.0f, 0.0f); // test > 0.5
}

void UnitDrawerStateGLSL4::DisableCommon(const CUnitDrawer* ud, bool alphaPass)
{
	assert(activeShader->IsBound());

	activeShader->SetUniform("cameraMode", 0);
	activeShader->SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f); //default, always pass
	activeShader->Disable();

	S3DModelVAO::GetInstance().Unbind();

	glPopAttrib();
}

void UnitDrawerStateGLSL4::EnableShadow(const CUnitDrawer* ud)
{
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);

	glPolygonMode(GL_FRONT_AND_BACK, ud->GetPolygonMode());

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	S3DModelVAO::GetInstance().Bind();

	SetActiveShader();
	activeShader->Enable();
	activeShader->SetUniform("cameraMode", 1); //shadow
	activeShader->SetUniform("alphaCtrl", 0.5f, 1.0f, 0.0f, 0.0f); // test > 0.5
}

void UnitDrawerStateGLSL4::DisableShadow(const CUnitDrawer* ud)
{
	assert(activeShader->IsBound());

	//don't reset shadow shader uniforms
	activeShader->Disable();

	S3DModelVAO::GetInstance().Unbind();

	glPopAttrib();
}