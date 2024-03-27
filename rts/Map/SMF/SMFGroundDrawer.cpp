/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SMFReadMap.h"
#include "SMFGroundDrawer.h"
#include "SMFGroundTextures.h"
#include "SMFRenderState.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/HeightMapTexture.h"
#include "Map/ReadMap.h"
#include "Map/SMF/Basic/BasicMeshDrawer.h"
#include "Map/SMF/ROAM/RoamMeshDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Shaders/Shader.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/FastMath.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

#include <tracy/Tracy.hpp>

//Basic, ROAM
static constexpr int MIN_GROUND_DETAIL[] = {                               0,   4};
static constexpr int MAX_GROUND_DETAIL[] = {CBasicMeshDrawer::LOD_LEVELS - 1, 200};

CONFIG(int, GroundDetail)
	.defaultValue(60)
	.headlessValue(0)
	.minimumValue(MIN_GROUND_DETAIL[1])
	.maximumValue(MAX_GROUND_DETAIL[1])
	.description("Controls how detailed the map geometry will be. On lowered settings, cliffs may appear to be jagged or \"melting\".");
CONFIG(bool, MapBorder).defaultValue(true).description("Draws a solid border at the edges of the map.");


CONFIG(int, MaxDynamicMapLights)
	.defaultValue(1)
	.minimumValue(0).description("Maximum number of map-global dynamic lights that will be rendered at once. High numbers of lights cost performance, as they affect every map fragment.");

CONFIG(bool, AdvMapShading).defaultValue(true).safemodeValue(false).description("Enable shaders for terrain rendering.");
CONFIG(bool, AllowDeferredMapRendering).defaultValue(false).safemodeValue(false).description("Enable rendering the map to the map deferred buffers.");
CONFIG(bool, AllowDrawMapPostDeferredEvents).defaultValue(false).description("Enable DrawGroundPostDeferred Lua callin.");
CONFIG(bool, AllowDrawMapDeferredEvents).defaultValue(false).description("Enable DrawGroundDeferred Lua callin.");


CONFIG(int, ROAM)
	.defaultValue(1)
	.minimumValue(0)
	.maximumValue(1)
	.description("Use ROAM for terrain mesh rendering: 0 to disable, 1=VBO mode to enable.");

CONFIG(bool, AlwaysSendDrawGroundEvents)
	.defaultValue(false)
	.description("Always send DrawGround{Pre,Post}{Forward,Deferred} events");

namespace Shader {
	struct IProgramObject;
}

CSMFGroundDrawer::CSMFGroundDrawer(CSMFReadMap* rm)
	: smfMap(rm)
	, meshDrawer(nullptr)
	, geomBuffer{"GROUNDDRAWER-GBUFFER"}
{
	alwaysDispatchEvents = configHandler->GetBool("AlwaysSendDrawGroundEvents");
	drawerMode = (configHandler->GetInt("ROAM") != 0)? SMF_MESHDRAWER_ROAM: SMF_MESHDRAWER_BASIC;
	groundDetail = configHandler->GetInt("GroundDetail");

	groundTextures = new CSMFGroundTextures(smfMap);
	meshDrawer = SwitchMeshDrawer(drawerMode);

	smfRenderStates = { nullptr };
	smfRenderStates[RENDER_STATE_SSP] = ISMFRenderState::GetInstance(false, false);
	smfRenderStates[RENDER_STATE_LUA] = ISMFRenderState::GetInstance( true, false);
	smfRenderStates[RENDER_STATE_NOP] = ISMFRenderState::GetInstance(false,  true);

	borderShader = shaderHandler->CreateProgramObject("[SMFGroundDrawer]", "Border");
	borderShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/SMFBorderVertProg.glsl", "", GL_VERTEX_SHADER));
	borderShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/SMFBorderFragProg.glsl", "", GL_FRAGMENT_SHADER));
	borderShader->BindAttribLocation("vertexPos", 0);
	borderShader->BindAttribLocation("vertexCol", 1);
	borderShader->Link();

	borderShader->Enable();
	borderShader->SetUniform("diffuseTex"  , 0);
	borderShader->SetUniform("heightMapTex", 1);
	borderShader->SetUniform("detailsTex"  , 2);
	borderShader->SetUniform("mapSize",
		static_cast<float>(mapDims.mapx * SQUARE_SIZE), static_cast<float>(mapDims.mapy * SQUARE_SIZE),
				   1.0f / (mapDims.mapx * SQUARE_SIZE),            1.0f / (mapDims.mapy * SQUARE_SIZE)
	);
	borderShader->Disable();

	borderShader->Validate();

	drawForward = true;
	drawDeferred = geomBuffer.Valid();
	drawMapEdges = configHandler->GetBool("MapBorder");
	postDeferredEvents = configHandler->GetBool("AllowDrawMapPostDeferredEvents");
	deferredEvents = configHandler->GetBool("AllowDrawMapDeferredEvents");


	if (smfRenderStates[RENDER_STATE_SSP]->Init(this))
		smfRenderStates[RENDER_STATE_SSP]->Update(this, nullptr);

	// always initialize this state; defer Update (allows re-use)
	smfRenderStates[RENDER_STATE_LUA]->Init(this);

	// note: state must be pre-selected before the first drawn frame
	// Sun*Changed can be called first, e.g. if DynamicSun is enabled
	smfRenderStates[RENDER_STATE_SEL] = SelectRenderState(DrawPass::Normal);

	if (drawDeferred) {
		drawDeferred &= UpdateGeometryBuffer(true);
	}
}

CSMFGroundDrawer::~CSMFGroundDrawer()
{
	//ZoneScoped;
	// remember which ROAM-mode was enabled (if any)
	configHandler->Set("ROAM", (dynamic_cast<CRoamMeshDrawer*>(meshDrawer) != nullptr)? 1: 0);

	smfRenderStates[RENDER_STATE_SSP]->Kill(); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_SSP]);
	smfRenderStates[RENDER_STATE_LUA]->Kill(); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_LUA]);
	smfRenderStates[RENDER_STATE_NOP]->Kill(); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_NOP]);

	smfRenderStates = { nullptr };

	shaderHandler->ReleaseProgramObject("[SMFGroundDrawer]", "Border");

	spring::SafeDelete(groundTextures);
	spring::SafeDelete(meshDrawer);
}



IMeshDrawer* CSMFGroundDrawer::SwitchMeshDrawer(int wantedMode)
{
	//ZoneScoped;
	// toggle
	if (wantedMode <= -1) {
		wantedMode = drawerMode + 1;
		wantedMode %= SMF_MESHDRAWER_LAST;
	}

	if ((wantedMode == drawerMode) && (meshDrawer != nullptr))
		return meshDrawer;

	spring::SafeDelete(meshDrawer);

	switch ((drawerMode = wantedMode)) {
		case SMF_MESHDRAWER_LEGACY: {
			LOG("Legacy Mesh Renderer is no longer available");
		} [[fallthrough]];
		case SMF_MESHDRAWER_BASIC: {
			LOG("Switching to Basic Mesh Rendering");
			meshDrawer = new CBasicMeshDrawer(this);
		} break;
		default: {
			LOG("Switching to ROAM Mesh Rendering");
			meshDrawer = new CRoamMeshDrawer(this);
		} break;
	}

	return meshDrawer;
}

ISMFRenderState* CSMFGroundDrawer::SelectRenderState(const DrawPass::e& drawPass)
{
	//ZoneScoped;
	// [0] := Lua GLSL, must have a valid shader for this pass
	// [1] := default ARB *or* GLSL, same condition
	const unsigned int stateEnums[2] = {RENDER_STATE_LUA, RENDER_STATE_SSP};

	for (unsigned int n = 0; n < 2; n++) {
		ISMFRenderState* state = smfRenderStates[ stateEnums[n] ];

		if (!state->HasValidShader(drawPass))
			continue;

		return (smfRenderStates[RENDER_STATE_SEL] = state);
	}

	// fallback
	return (smfRenderStates[RENDER_STATE_SEL] = smfRenderStates[RENDER_STATE_NOP]);
}

bool CSMFGroundDrawer::HaveLuaRenderState() const
{
	//ZoneScoped;
	return (smfRenderStates[RENDER_STATE_SEL] == smfRenderStates[RENDER_STATE_LUA]);
}



void CSMFGroundDrawer::DrawDeferredPass(const DrawPass::e& drawPass, bool alphaTest)
{
	//ZoneScoped;
	if (!geomBuffer.Valid())
		return;

	// some water renderers use FBO's for the reflection pass
	if (drawPass == DrawPass::WaterReflection)
		return;
	// some water renderers use FBO's for the refraction pass
	if (drawPass == DrawPass::WaterRefraction)
		return;
	// CubeMapHandler also uses an FBO for this pass
	if (drawPass == DrawPass::TerrainReflection)
		return;

	// deferred pass must be executed with GLSL shaders
	// if the FFP or ARB state was selected, bail early
	if (!SelectRenderState(DrawPass::TerrainDeferred)->CanDrawDeferred(this)) {
		geomBuffer.Bind();
		geomBuffer.SetDepthRange(1.0f, 0.0f);
		geomBuffer.Clear();
		geomBuffer.SetDepthRange(0.0f, 1.0f);
		geomBuffer.UnBind();
		return;
	}

	GL::GeometryBuffer::LoadViewport();

	{
		geomBuffer.Bind();
		geomBuffer.SetDepthRange(1.0f, 0.0f);
		geomBuffer.Clear();

		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(this, DrawPass::TerrainDeferred);
		smfRenderStates[RENDER_STATE_SEL]->Enable(this, DrawPass::TerrainDeferred);

		if (alphaTest) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, mapInfo->map.voidAlphaMin);
		}

		if (alwaysDispatchEvents || HaveLuaRenderState())
			eventHandler.DrawGroundPreDeferred();

		meshDrawer->DrawMesh(drawPass);

		if (alphaTest) {
			glDisable(GL_ALPHA_TEST);
		}

		smfRenderStates[RENDER_STATE_SEL]->Disable(this, drawPass);
		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(this, DrawPass::Normal);

		if (deferredEvents)
			eventHandler.DrawGroundDeferred();

		geomBuffer.SetDepthRange(0.0f, 1.0f);
		geomBuffer.UnBind();
	}

	globalRendering->LoadViewport();

	#if 0
	geomBuffer.DrawDebug(geomBuffer.GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_NORMTEX));
	#endif

	// send event if no forward pass will follow; must be done after the unbind
	if (!drawForward || postDeferredEvents)
		eventHandler.DrawGroundPostDeferred();
}

void CSMFGroundDrawer::DrawForwardPass(const DrawPass::e& drawPass, bool alphaTest)
{
	//ZoneScoped;
	if (!SelectRenderState(drawPass)->CanDrawForward(this))
		return;

	smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(this, drawPass);
	smfRenderStates[RENDER_STATE_SEL]->Enable(this, drawPass);

	glPushAttrib((GL_ENABLE_BIT * alphaTest) | (GL_POLYGON_BIT * wireframe));

	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (alphaTest) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, mapInfo->map.voidAlphaMin);
	}

	if (alwaysDispatchEvents || HaveLuaRenderState())
		eventHandler.DrawGroundPreForward();

	meshDrawer->DrawMesh(drawPass);

	glPopAttrib();

	smfRenderStates[RENDER_STATE_SEL]->Disable(this, drawPass);
	smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(this, DrawPass::Normal);

	if (alwaysDispatchEvents || HaveLuaRenderState())
		eventHandler.DrawGroundPostForward();
}

void CSMFGroundDrawer::Draw(const DrawPass::e& drawPass)
{
	//ZoneScoped;
	// must be here because water renderers also call us
	if (!globalRendering->drawGround)
		return;
	// if entire map is under voidwater, no need to draw *ground*
	if (readMap->HasOnlyVoidWater())
		return;

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	if (drawDeferred) {
		// do the deferred pass first, will allow us to re-use
		// its output at some future point and eventually draw
		// the entire map deferred
		DrawDeferredPass(drawPass, mapRendering->voidGround || (mapRendering->voidWater && drawPass != DrawPass::WaterReflection));
	}

	if (drawForward) {
		DrawForwardPass(drawPass, mapRendering->voidGround || (mapRendering->voidWater && drawPass != DrawPass::WaterReflection));
	}

	glDisable(GL_CULL_FACE);

	if (drawPass == DrawPass::Normal && drawMapEdges) {
		DrawBorder(drawPass);
	}
}


void CSMFGroundDrawer::DrawBorder(const DrawPass::e drawPass)
{
	//ZoneScoped;
	ISMFRenderState* prvState = smfRenderStates[RENDER_STATE_SEL];

	// no need to enable, does nothing
	smfRenderStates[RENDER_STATE_SEL] = smfRenderStates[RENDER_STATE_NOP];

	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glActiveTexture(GL_TEXTURE2); glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());

	glActiveTexture(GL_TEXTURE1); glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID());

	//for CSMFGroundTextures::BindSquareTexture()
	glActiveTexture(GL_TEXTURE0); glEnable(GL_TEXTURE_2D);

	glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

	borderShader->Enable();
	borderShader->SetUniform("borderMinHeight", std::min(readMap->GetInitMinHeight(), -500.0f));
	meshDrawer->DrawBorderMesh(drawPass);
	borderShader->Disable();

	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	smfRenderStates[RENDER_STATE_SEL] = prvState;
}


void CSMFGroundDrawer::DrawShadowPass()
{
	//ZoneScoped;
	if (!globalRendering->drawGround)
		return;
	if (readMap->HasOnlyVoidWater())
		return;

	shadowShader = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MAP);
	assert(shadowShader);
	glEnable(GL_POLYGON_OFFSET_FILL);

	//#pragma message "REMOVE ME, WHEN NOT NEEDED"
	//glDisable(GL_CULL_FACE);

	glPolygonOffset(spPolygonOffsetScale, spPolygonOffsetUnits); // dz*s + r*u

	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID());
	shadowShader->Enable();
	shadowShader->SetUniform("borderMinHeight", std::min(readMap->GetInitMinHeight(), -500.0f));
		meshDrawer->DrawMesh(DrawPass::Shadow);
		// also render the border geometry to prevent light-visible backfaces
		meshDrawer->DrawBorderMesh(DrawPass::Shadow);
	shadowShader->Disable();
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

	glDisable(GL_POLYGON_OFFSET_FILL);
	//glEnable(GL_CULL_FACE);
}



void CSMFGroundDrawer::SetLuaShader(const LuaMapShaderData* luaMapShaderData)
{
	//ZoneScoped;
	smfRenderStates[RENDER_STATE_LUA]->Update(this, luaMapShaderData);
}

void CSMFGroundDrawer::SetupBigSquare(const DrawPass::e& drawPass, const int bigSquareX, const int bigSquareY)
{
	//ZoneScoped;
	if (drawPass != DrawPass::Shadow) {
		groundTextures->BindSquareTexture(bigSquareX, bigSquareY);
		smfRenderStates[RENDER_STATE_SEL]->SetSquareTexGen(bigSquareX, bigSquareY);

		if (borderShader && borderShader->IsBound()) {
			borderShader->SetUniform("texSquare", bigSquareX, bigSquareY);
		}
	}
	else {
		if (shadowShader && shadowShader->IsBound()) {
			shadowShader->SetUniform("texSquare", bigSquareX, bigSquareY);
		}
	}
}



void CSMFGroundDrawer::Update()
{
	//ZoneScoped;
	if (readMap->HasOnlyVoidWater())
		return;

	groundTextures->DrawUpdate();
	// done by DrawMesh; needs to know the actual draw-pass
	// meshDrawer->Update();

	if (drawDeferred) {
		drawDeferred &= UpdateGeometryBuffer(false);
	}
}

void CSMFGroundDrawer::UpdateRenderState()
{
	//ZoneScoped;
	smfRenderStates[RENDER_STATE_SSP]->Update(this, nullptr);
}

void CSMFGroundDrawer::SunChanged() {
	//ZoneScoped;
	// Lua has gl.GetSun
	if (HaveLuaRenderState())
		return;

	smfRenderStates[RENDER_STATE_SEL]->UpdateShaderSkyUniforms();
}


bool CSMFGroundDrawer::UpdateGeometryBuffer(bool init)
{
	//ZoneScoped;
	static const bool drawDeferredAllowed = configHandler->GetBool("AllowDeferredMapRendering");

	if (!drawDeferredAllowed)
		return false;

	return (geomBuffer.Update(init));
}



void CSMFGroundDrawer::SetDetail(int newGroundDetail)
{
	//ZoneScoped;
	const int minGroundDetail = MIN_GROUND_DETAIL[drawerMode == SMF_MESHDRAWER_ROAM];
	const int maxGroundDetail = MAX_GROUND_DETAIL[drawerMode == SMF_MESHDRAWER_ROAM];

	configHandler->Set("GroundDetail", groundDetail = std::clamp(newGroundDetail, minGroundDetail, maxGroundDetail));
	LOG("GroundDetail%s set to %i", ((drawerMode != SMF_MESHDRAWER_ROAM)? "[Bias]": ""), groundDetail);
}



int CSMFGroundDrawer::GetGroundDetail(const DrawPass::e& drawPass) const
{
	//ZoneScoped;
	int detail = groundDetail;

	switch (drawPass) {
		case DrawPass::TerrainReflection:
			detail *= LODScaleTerrainReflection;
			break;
		case DrawPass::WaterReflection:
			detail *= LODScaleReflection;
			break;
		case DrawPass::WaterRefraction:
			detail *= LODScaleRefraction;
			break;
		case DrawPass::Shadow:
			// TODO:
			//   render a contour mesh for the SP? z-fighting / p-panning occur
			//   when the regular and shadow-mesh tessellations differ too much,
			//   more visible on larger or hillier maps
			//   detail *= LODScaleShadow;
			break;
		default:
			break;
	}

	return detail;
}
