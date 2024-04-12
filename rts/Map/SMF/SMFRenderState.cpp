/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SMFRenderState.h"
#include "SMFGroundDrawer.h"
#include "SMFReadMap.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/HeightMapTexture.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SkyLight.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/StringUtil.h"

#include "System/Misc/TracyDefs.h"

static constexpr float SMF_TEXSQUARE_SIZE = 1024.0f;


ISMFRenderState* ISMFRenderState::GetInstance(bool luaShaders, bool noop) {
	if (noop)
		return new SMFRenderStateNOOP();
	else
		return new SMFRenderStateGLSL(luaShaders);
}

bool SMFRenderStateGLSL::Init(const CSMFGroundDrawer* smfGroundDrawer) {
	RECOIL_DETAILED_TRACY_ZONE;
	const std::string names[GLSL_SHADER_COUNT] = {
		"SMFShaderGLSL-Forward-Std",
		"SMFShaderGLSL-Forward-Adv",
		"SMFShaderGLSL-Deferred-Adv"
	};
	const std::string defs =
		("#define SMF_TEXSQUARE_SIZE " + FloatToString(                  SMF_TEXSQUARE_SIZE) + "\n") +
		("#define SMF_INTENSITY_MULT " + FloatToString(CGlobalRendering::SMF_INTENSITY_MULT) + "\n");


	if (useLuaShaders) {
		for (uint32_t n = GLSL_SHADER_FWD_ADV; n < GLSL_SHADER_COUNT; n++) {
			glslShaders[n] = shaderHandler->CreateProgramObject("[SMFGroundDrawer::Lua]", names[n] + "-Lua");
			// release ID created by GLSLProgramObject's ctor; should be 0
			glslShaders[n]->Release();
		}
	} else {
		for (uint32_t n = GLSL_SHADER_FWD_STD; n < GLSL_SHADER_COUNT; n++) {
			// load from VFS files
			glslShaders[n] = shaderHandler->CreateProgramObject("[SMFGroundDrawer::VFS]", names[n]);
			glslShaders[n]->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/SMFVertProg.glsl", defs, GL_VERTEX_SHADER));
			glslShaders[n]->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/SMFFragProg.glsl", defs, GL_FRAGMENT_SHADER));
			glslShaders[n]->BindAttribLocation("vertexPos", 0);
		}
	}

	currShader = glslShaders[CanUseAdvShading(smfGroundDrawer, GLSL_SHADER_FWD_ADV) ? GLSL_SHADER_FWD_ADV : GLSL_SHADER_FWD_STD];
	return true;
}

void SMFRenderStateGLSL::Kill() {
	RECOIL_DETAILED_TRACY_ZONE;
	if (useLuaShaders) {
		// make sure SH deletes only the wrapper objects; programs are managed by LuaShaders
		for (uint32_t n = GLSL_SHADER_FWD_STD; n < GLSL_SHADER_COUNT; n++) {
			if (glslShaders[n] != nullptr) {
				glslShaders[n]->LoadFromID(0);
			}
		}

		shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer::Lua]");
	} else {
		shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer::VFS]");
	}
}

void SMFRenderStateGLSL::Update(
	const CSMFGroundDrawer* smfGroundDrawer,
	const LuaMapShaderData* luaMapShaderData
) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (useLuaShaders) {
		assert(luaMapShaderData != nullptr);

		// load from LuaShader ID; should be a linked and valid program (or 0)
		// NOTE: only non-custom shaders get to have engine flags and uniforms!
		for (uint32_t n = GLSL_SHADER_FWD_ADV; n <= GLSL_SHADER_DFR_ADV; n++) {
			glslShaders[n]->LoadFromID(luaMapShaderData->shaderIDs[n - 1]);
		}
	} else {
		assert(luaMapShaderData == nullptr);

		const CSMFReadMap* smfMap = smfGroundDrawer->GetReadMap();

		const int2 normTexSize = smfMap->GetTextureSize(MAP_BASE_NORMALS_TEX);
		// const int2 specTexSize = smfMap->GetTextureSize(MAP_SSMF_SPECULAR_TEX);

		for (uint32_t n = GLSL_SHADER_FWD_STD; n <= GLSL_SHADER_DFR_ADV; n++) {
			const bool isAdv = (n != GLSL_SHADER_FWD_STD);

			if (isAdv) {
				glslShaders[n]->SetFlag("SMF_ADV_SHADING",                      true);
				glslShaders[n]->SetFlag("SMF_VOID_WATER",                       mapRendering->voidWater);
				glslShaders[n]->SetFlag("SMF_VOID_GROUND",                      mapRendering->voidGround);
				glslShaders[n]->SetFlag("SMF_SPECULAR_LIGHTING",                smfMap->GetSpecularTexture() != 0);
				glslShaders[n]->SetFlag("SMF_DETAIL_TEXTURE_SPLATTING",        (smfMap->GetSplatDistrTexture() != 0 && smfMap->GetSplatDetailTexture() != 0));
				glslShaders[n]->SetFlag("SMF_DETAIL_NORMAL_TEXTURE_SPLATTING", (smfMap->GetSplatDistrTexture() != 0 && smfMap->HaveSplatNormalTexture()));
				glslShaders[n]->SetFlag("SMF_DETAIL_NORMAL_DIFFUSE_ALPHA",      mapRendering->splatDetailNormalDiffuseAlpha);
				glslShaders[n]->SetFlag("SMF_WATER_ABSORPTION",                 smfMap->HasVisibleWater());
				glslShaders[n]->SetFlag("SMF_SKY_REFLECTIONS",                  smfMap->GetSkyReflectModTexture() != 0);
				glslShaders[n]->SetFlag("SMF_BLEND_NORMALS",                    smfMap->GetBlendNormalsTexture() != 0);
				glslShaders[n]->SetFlag("SMF_LIGHT_EMISSION",                   smfMap->GetLightEmissionTexture() != 0);
				glslShaders[n]->SetFlag("SMF_PARALLAX_MAPPING",                 smfMap->GetParallaxHeightTexture() != 0);
			}

			// both are runtime set in ::Enable, but AMD drivers need values from the beginning
			glslShaders[n]->SetFlag("HAVE_SHADOWS", false);
			glslShaders[n]->SetFlag("HAVE_INFOTEX", false);

			if (n == GLSL_SHADER_DFR_ADV) {
				glslShaders[n]->SetFlag("DEFERRED_MODE", true);
				glslShaders[n]->SetFlag("GBUFFER_NORMTEX_IDX", GL::GeometryBuffer::ATTACHMENT_NORMTEX);
				glslShaders[n]->SetFlag("GBUFFER_DIFFTEX_IDX", GL::GeometryBuffer::ATTACHMENT_DIFFTEX);
				glslShaders[n]->SetFlag("GBUFFER_SPECTEX_IDX", GL::GeometryBuffer::ATTACHMENT_SPECTEX);
				glslShaders[n]->SetFlag("GBUFFER_EMITTEX_IDX", GL::GeometryBuffer::ATTACHMENT_EMITTEX);
				glslShaders[n]->SetFlag("GBUFFER_MISCTEX_IDX", GL::GeometryBuffer::ATTACHMENT_MISCTEX);
				glslShaders[n]->SetFlag("GBUFFER_ZVALTEX_IDX", GL::GeometryBuffer::ATTACHMENT_ZVALTEX);
			}

			glslShaders[n]->Link();
			glslShaders[n]->Enable();

			glslShaders[n]->SetUniform("diffuseTex",             0);
			glslShaders[n]->SetUniform("heightMapTex",           1);
			glslShaders[n]->SetUniform("detailTex",              2);
			glslShaders[n]->SetUniform("infoTex",               14);
			if (isAdv) {
				glslShaders[n]->SetUniform("shadowTex", 4);
				glslShaders[n]->SetUniform("normalsTex", 5);
				glslShaders[n]->SetUniform("specularTex", 6);
				glslShaders[n]->SetUniform("splatDetailTex", 7);
				glslShaders[n]->SetUniform("splatDistrTex", 8);
				glslShaders[n]->SetUniform("skyReflectTex", 9);
				glslShaders[n]->SetUniform("skyReflectModTex", 10);
				glslShaders[n]->SetUniform("blendNormalsTex", 11);
				glslShaders[n]->SetUniform("lightEmissionTex", 12);
				glslShaders[n]->SetUniform("parallaxHeightTex", 13);
				glslShaders[n]->SetUniform("splatDetailNormalTex1", 15);
				glslShaders[n]->SetUniform("splatDetailNormalTex2", 16);
				glslShaders[n]->SetUniform("splatDetailNormalTex3", 17);
				glslShaders[n]->SetUniform("splatDetailNormalTex4", 18);
				glslShaders[n]->SetUniform("shadowColorTex", 19);

				glslShaders[n]->SetUniform4v("lightDir", &ISky::GetSky()->GetLight()->GetLightDir()[0]);
				glslShaders[n]->SetUniform3v("cameraPos", &camera->GetPos()[0]);

				glslShaders[n]->SetUniform3v("groundAmbientColor", &sunLighting->groundAmbientColor[0]);
				glslShaders[n]->SetUniform3v("groundDiffuseColor", &sunLighting->groundDiffuseColor[0]);
				glslShaders[n]->SetUniform3v("groundSpecularColor", &sunLighting->groundSpecularColor[0]);
				glslShaders[n]->SetUniform("groundSpecularExponent", sunLighting->specularExponent);
				glslShaders[n]->SetUniform("groundShadowDensity", sunLighting->groundShadowDensity);

				glslShaders[n]->SetUniformMatrix4x4("shadowMat", false, shadowHandler.GetShadowMatrixRaw());

				glslShaders[n]->SetUniform3v("waterMinColor", &waterRendering->minColor[0]);
				glslShaders[n]->SetUniform3v("waterBaseColor", &waterRendering->baseColor[0]);
				glslShaders[n]->SetUniform3v("waterAbsorbColor", &waterRendering->absorb[0]);

				glslShaders[n]->SetUniform4v("splatTexScales", &mapRendering->splatTexScales[0]);
				glslShaders[n]->SetUniform4v("splatTexMults", &mapRendering->splatTexMults[0]);

				glslShaders[n]->SetUniform("normalTexGen", 1.0f / ((normTexSize.x - 1) * SQUARE_SIZE), 1.0f / ((normTexSize.y - 1) * SQUARE_SIZE));
			}
			else {
				glslShaders[n]->SetUniform("shadingTex", 3);
			}

			glslShaders[n]->SetUniform("infoTexGen", 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE));
			glslShaders[n]->SetUniform("infoTexIntensityMul", 1.0f);
			glslShaders[n]->SetUniform("specularTexGen", 1.0f / (mapDims.mapx * SQUARE_SIZE), 1.0f / (mapDims.mapy * SQUARE_SIZE));

			glslShaders[n]->Disable();
			glslShaders[n]->Validate();
		}
	}
}

bool SMFRenderStateGLSL::HasValidShader(const DrawPass::e& drawPass) const {
	RECOIL_DETAILED_TRACY_ZONE;
	Shader::IProgramObject* shader = (drawPass == DrawPass::TerrainDeferred) ? glslShaders[GLSL_SHADER_DFR_ADV] : currShader;
	return (shader != nullptr && shader->IsValid());
}

bool SMFRenderStateGLSL::CanDrawDeferred(const CSMFGroundDrawer* smfGroundDrawer) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return CanUseAdvShading(smfGroundDrawer, GLSL_SHADER_DFR_ADV);
}

void SMFRenderStateGLSL::Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (useLuaShaders) {
		// use raw, GLSLProgramObject::Enable also calls RecompileIfNeeded
		currShader->EnableRaw();
		// diffuse textures are always bound (SMFGroundDrawer::SetupBigSquare)
		glActiveTexture(GL_TEXTURE0);
		return;
	}

	const auto shaderStage = (drawPass == DrawPass::TerrainDeferred) ? GLSL_SHADER_DFR_ADV : GLSL_SHADER_FWD_ADV;
	const bool isAdv = CanUseAdvShading(smfGroundDrawer, shaderStage);

	const CSMFReadMap* smfMap = smfGroundDrawer->GetReadMap();

	if (isAdv)
		currShader->SetFlag("HAVE_SHADOWS", shadowHandler.ShadowsLoaded());

	currShader->SetFlag("HAVE_INFOTEX", infoTextureHandler->IsEnabled());

	currShader->Enable();
	currShader->SetUniform("mapHeights", readMap->GetCurrMinHeight(), readMap->GetCurrMaxHeight());
	currShader->SetUniform("infoTexIntensityMul", float(infoTextureHandler->InMetalMode()) + 1.0f);

	if (isAdv) {
		currShader->SetUniform3v("cameraPos", &camera->GetPos()[0]);
		if (shadowHandler.ShadowsLoaded())
			currShader->SetUniformMatrix4x4("shadowMat", false, shadowHandler.GetShadowMatrixRaw());
	}

	// already on the MV stack at this point
	glLoadIdentity();
	glMultMatrixf(camera->GetViewMatrix());

	if (isAdv && shadowHandler.ShadowsLoaded()) {
		shadowHandler.SetupShadowTexSampler(GL_TEXTURE4);
		glActiveTexture(GL_TEXTURE19); glBindTexture(GL_TEXTURE_2D, shadowHandler.GetColorTextureID());
	}

	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID());
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
	glActiveTexture(GL_TEXTURE14); glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
	if (isAdv) {
		glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, smfMap->GetNormalsTexture());
		glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, smfMap->GetSpecularTexture());
		glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatDetailTexture());
		glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatDistrTexture());
		glActiveTexture(GL_TEXTURE9); glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler.GetSkyReflectionTextureID());
		glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, smfMap->GetSkyReflectModTexture());
		glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, smfMap->GetBlendNormalsTexture());
		glActiveTexture(GL_TEXTURE12); glBindTexture(GL_TEXTURE_2D, smfMap->GetLightEmissionTexture());
		glActiveTexture(GL_TEXTURE13); glBindTexture(GL_TEXTURE_2D, smfMap->GetParallaxHeightTexture());

		for (int i = 0; i < CSMFReadMap::NUM_SPLAT_DETAIL_NORMALS; i++) {
			if (smfMap->GetSplatNormalTexture(i) != 0) {
				glActiveTexture(GL_TEXTURE15 + i); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatNormalTexture(i));
			}
		}
	}
	else {
		glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, smfMap->GetShadingTexture());
	}

	glActiveTexture(GL_TEXTURE0);
}

void SMFRenderStateGLSL::Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (useLuaShaders) {
		glActiveTexture(GL_TEXTURE0);
		currShader->DisableRaw();
		return;
	}

	const auto shaderStage = (drawPass == DrawPass::TerrainDeferred) ? GLSL_SHADER_DFR_ADV : GLSL_SHADER_FWD_ADV;
	const bool isAdv = CanUseAdvShading(smfGroundDrawer, shaderStage);

	if (isAdv && shadowHandler.ShadowsLoaded()) {
		glActiveTexture(GL_TEXTURE4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}

	glActiveTexture(GL_TEXTURE0);
	currShader->Disable();
}

void SMFRenderStateGLSL::SetSquareTexGen(const int sqx, const int sqy) const {
	RECOIL_DETAILED_TRACY_ZONE;
	// needs to be set even for Lua shaders, is unknowable otherwise
	// (works because SMFGroundDrawer::SetupBigSquare always calls us)
	currShader->SetUniform("texSquare", sqx, sqy);
}

void SMFRenderStateGLSL::SetCurrentShader(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (drawPass == DrawPass::TerrainDeferred) {
		currShader = glslShaders[GLSL_SHADER_DFR_ADV];
		return;
	}

	if (CanUseAdvShading(smfGroundDrawer, GLSL_SHADER_FWD_ADV))
		currShader = glslShaders[GLSL_SHADER_FWD_ADV];
	else
		currShader = glslShaders[GLSL_SHADER_FWD_STD];
}

void SMFRenderStateGLSL::UpdateShaderSkyUniforms()
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(currShader && !currShader->IsBound());

	for (uint32_t n = GLSL_SHADER_FWD_ADV; n < GLSL_SHADER_COUNT; n++) {
		glslShaders[n]->Enable();
		glslShaders[n]->SetUniform4v("lightDir", &ISky::GetSky()->GetLight()->GetLightDir().x);
		glslShaders[n]->SetUniform("groundShadowDensity", sunLighting->groundShadowDensity);
		glslShaders[n]->SetUniform3v("groundAmbientColor", &sunLighting->groundAmbientColor[0]);
		glslShaders[n]->SetUniform3v("groundDiffuseColor", &sunLighting->groundDiffuseColor[0]);
		glslShaders[n]->SetUniform3v("groundSpecularColor", &sunLighting->groundSpecularColor[0]);
		glslShaders[n]->Disable();
	}
}

bool SMFRenderStateGLSL::CanUseAdvShading(const CSMFGroundDrawer* smfGroundDrawer, ShaderStage shStage) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return smfGroundDrawer->UseAdvShading() && glslShaders[shStage]->IsValid();
}
