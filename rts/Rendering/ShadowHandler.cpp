/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <cfloat>

#include "ShadowHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameVersion.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GL/RenderBuffers.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"

#include "lib/fmt/format.h"

CONFIG(int, Shadows).defaultValue(2).headlessValue(-1).minimumValue(-1).safemodeValue(-1).description("Sets whether shadows are rendered.\n-1:=forceoff, 0:=off, 1:=full, 2:=fast (skip terrain)"); //FIXME document bitmask
CONFIG(int, ShadowMapSize).defaultValue(CShadowHandler::DEF_SHADOWMAP_SIZE).minimumValue(32).description("Sets the resolution of shadows. Higher numbers increase quality at the cost of performance.");
CONFIG(int, ShadowProjectionMode).defaultValue(CShadowHandler::SHADOWPROMODE_CAM_CENTER);
CONFIG(bool, ShadowColorMode).defaultValue(true).description("Whether the colorbuffer of shadowmap FBO is RGB vs greyscale(to conserve some VRAM)");

CShadowHandler shadowHandler;

void CShadowHandler::Reload(const char* argv)
{
	int nextShadowConfig = (shadowConfig + 1) & 0xF;
	int nextShadowMapSize = shadowMapSize;
	int nextShadowProMode = shadowProMode;
	int nextShadowColorMode = shadowColorMode;

	if (argv != nullptr)
		(void) sscanf(argv, "%i %i %i %i", &nextShadowConfig, &nextShadowMapSize, &nextShadowProMode, &nextShadowColorMode);

	// do nothing without a parameter change
	if (nextShadowConfig == shadowConfig && nextShadowMapSize == shadowMapSize && nextShadowProMode == shadowProMode && nextShadowColorMode == shadowColorMode)
		return;

	configHandler->Set("Shadows", nextShadowConfig & 0xF);
	configHandler->Set("ShadowMapSize", std::clamp(nextShadowMapSize, int(MIN_SHADOWMAP_SIZE), int(MAX_SHADOWMAP_SIZE)));
	configHandler->Set("ShadowProjectionMode", std::clamp(nextShadowProMode, int(SHADOWPROMODE_MAP_CENTER), int(SHADOWPROMODE_MIX_CAMMAP)));
	configHandler->Set("ShadowColorMode", static_cast<bool>(nextShadowColorMode));

	Kill();
	Init();
}

void CShadowHandler::Init()
{
	const bool tmpFirstInit = firstInit;
	firstInit = false;

	shadowConfig  = configHandler->GetInt("Shadows");
	shadowMapSize = configHandler->GetInt("ShadowMapSize");
	// disabled; other option usually produces worse resolution
	shadowProMode = configHandler->GetInt("ShadowProjectionMode");
	//shadowProMode = SHADOWPROMODE_CAM_CENTER;
	shadowColorMode = configHandler->GetInt("ShadowColorMode");
	shadowGenBits = SHADOWGEN_BIT_NONE;

	shadowsLoaded = false;
	inShadowPass = false;

	shadowDepthTexture = 0;
	shadowColorTexture = 0;

	if (!tmpFirstInit && !shadowsSupported)
		return;

	// possible values for the "Shadows" config-parameter:
	// < 0: disable and don't try to initialize
	//   0: disable, but create a fallback FBO
	// > 0: enabled (by default for all shadow-casting geometry if equal to 1)
	if (shadowConfig < 0) {
		LOG("[%s] shadow rendering is disabled (config-value %d)", __func__, shadowConfig);
		return;
	}

	if (shadowConfig > 0)
		shadowGenBits = SHADOWGEN_BIT_MODEL | SHADOWGEN_BIT_MAP | SHADOWGEN_BIT_PROJ | SHADOWGEN_BIT_TREE;

	if (shadowConfig > 1)
		shadowGenBits &= (~shadowConfig);

	// no warnings when running headless
	if (SpringVersion::IsHeadless())
		return;

	if (!InitFBOAndTextures()) {
		// free any resources allocated by InitFBOAndTextures()
		FreeFBOAndTextures();

		LOG_L(L_ERROR, "[%s] failed to initialize depth-texture FBO", __func__);
		return;
	}

	if (tmpFirstInit)
		shadowsSupported = true;

	if (shadowConfig > 0)
		LoadShadowGenShaders();
}

void CShadowHandler::Kill()
{
	FreeFBOAndTextures();
	shaderHandler->ReleaseProgramObjects("[ShadowHandler]");
	shadowGenProgs.fill(nullptr);
}


void CShadowHandler::Update()
{
	if (freezeFrustum)
		return;

	CCamera* playCam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	CCamera* shadCam = CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW);

	for (size_t i = 0; i < frustumPoints.size(); ++i)
		frustumPoints[i] = playCam->GetFrustumVert(i);

	CalcShadowMatrices(playCam, shadCam);
}

void CShadowHandler::SaveShadowMapTextures() const
{
	glSaveTexture(shadowDepthTexture, fmt::format("smDepth_{}.png", globalRendering->drawFrame).c_str());
	glSaveTexture(shadowColorTexture, fmt::format("smColor_{}.png", globalRendering->drawFrame).c_str());
}

void CShadowHandler::DrawFrustumDebugMiniMap() const
{
	if (!debugFrustum || !shadowsLoaded)
		return;

	CCamera* shadCam = CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW);

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_0>();
	rb.AssertSubmission();

	const auto ntl = VA_TYPE_0{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NTL) };
	const auto ntr = VA_TYPE_0{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NTR) };
	const auto nbr = VA_TYPE_0{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NBR) };
	const auto nbl = VA_TYPE_0{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NBL) };

	const auto ftl = VA_TYPE_0{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FTL) };
	const auto ftr = VA_TYPE_0{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FTR) };
	const auto fbr = VA_TYPE_0{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FBR) };
	const auto fbl = VA_TYPE_0{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FBL) };

	rb.AddVertices({ nbl, nbr }); // NBL - NBR
	rb.AddVertices({ nbr, ntr }); // NBR - NTR
	rb.AddVertices({ ntr, ntl }); // NTR - NTL
	rb.AddVertices({ ntl, nbl }); // NTL - NBL

	rb.AddVertices({ ntl, ftl }); // NTL - FTL
	rb.AddVertices({ ntr, ftr }); // NTR - FTR
	rb.AddVertices({ nbl, fbl }); // NBL - FBL
	rb.AddVertices({ nbr, fbr }); // NBR - FBR

	rb.AddVertices({ fbl, fbr }); // FBL - FBR
	rb.AddVertices({ fbr, ftr }); // FBR - FTR
	rb.AddVertices({ ftr, ftl }); // FTR - FTL
	rb.AddVertices({ ftl, fbl }); // FTL - FBL

	auto& sh = rb.GetShader();
	glLineWidth(2.0f);
	sh.Enable();
	sh.SetUniform("ucolor", 0.0f, 0.0f, 1.0f, 1.0f);
	rb.DrawArrays(GL_LINES);
	sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);
	sh.Disable();
	glLineWidth(1.0f);
}

void CShadowHandler::DrawFrustumDebugMap() const
{
	if (!debugFrustum || !shadowsLoaded || !freezeFrustum)
		return;

	static constexpr SColor NEAR_PLANE_COL = SColor{ 255,   0,   0, 255 };
	static constexpr SColor  FAR_PLANE_COL = SColor{ 0  , 255,   0, 255 };
	static constexpr SColor PLAYER_CAM_COL = SColor{ 255, 255, 255, 255 };

	CCamera* shadCam = CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW);

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();
	rb.AssertSubmission();

	// shadow frustum
	{
		const auto ntl = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NTL), NEAR_PLANE_COL };
		const auto ntr = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NTR), NEAR_PLANE_COL };
		const auto nbr = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NBR), NEAR_PLANE_COL };
		const auto nbl = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NBL), NEAR_PLANE_COL };

		const auto ftl = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FTL),  FAR_PLANE_COL };
		const auto ftr = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FTR),  FAR_PLANE_COL };
		const auto fbr = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FBR),  FAR_PLANE_COL };
		const auto fbl = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FBL),  FAR_PLANE_COL };

		rb.AddVertices({ nbl, nbr }); // NBL - NBR
		rb.AddVertices({ nbr, ntr }); // NBR - NTR
		rb.AddVertices({ ntr, ntl }); // NTR - NTL
		rb.AddVertices({ ntl, nbl }); // NTL - NBL

		rb.AddVertices({ ntl, ftl }); // NTL - FTL
		rb.AddVertices({ ntr, ftr }); // NTR - FTR
		rb.AddVertices({ nbl, fbl }); // NBL - FBL
		rb.AddVertices({ nbr, fbr }); // NBR - FBR

		rb.AddVertices({ fbl, fbr }); // FBL - FBR
		rb.AddVertices({ fbr, ftr }); // FBR - FTR
		rb.AddVertices({ ftr, ftl }); // FTR - FTL
		rb.AddVertices({ ftl, fbl }); // FTL - FBL
	}

	// player camera frustum
	{
		const auto ntl = VA_TYPE_C{ frustumPoints[CCamera::FRUSTUM_POINT_NTL], PLAYER_CAM_COL };
		const auto ntr = VA_TYPE_C{ frustumPoints[CCamera::FRUSTUM_POINT_NTR], PLAYER_CAM_COL };
		const auto nbr = VA_TYPE_C{ frustumPoints[CCamera::FRUSTUM_POINT_NBR], PLAYER_CAM_COL };
		const auto nbl = VA_TYPE_C{ frustumPoints[CCamera::FRUSTUM_POINT_NBL], PLAYER_CAM_COL };

		const auto ftl = VA_TYPE_C{ frustumPoints[CCamera::FRUSTUM_POINT_FTL], PLAYER_CAM_COL };
		const auto ftr = VA_TYPE_C{ frustumPoints[CCamera::FRUSTUM_POINT_FTR], PLAYER_CAM_COL };
		const auto fbr = VA_TYPE_C{ frustumPoints[CCamera::FRUSTUM_POINT_FBR], PLAYER_CAM_COL };
		const auto fbl = VA_TYPE_C{ frustumPoints[CCamera::FRUSTUM_POINT_FBL], PLAYER_CAM_COL };

		rb.AddVertices({ nbl, nbr }); // NBL - NBR
		rb.AddVertices({ nbr, ntr }); // NBR - NTR
		rb.AddVertices({ ntr, ntl }); // NTR - NTL
		rb.AddVertices({ ntl, nbl }); // NTL - NBL

		rb.AddVertices({ ntl, ftl }); // NTL - FTL
		rb.AddVertices({ ntr, ftr }); // NTR - FTR
		rb.AddVertices({ nbl, fbl }); // NBL - FBL
		rb.AddVertices({ nbr, fbr }); // NBR - FBR

		rb.AddVertices({ fbl, fbr }); // FBL - FBR
		rb.AddVertices({ fbr, ftr }); // FBR - FTR
		rb.AddVertices({ ftr, ftl }); // FTR - FTL
		rb.AddVertices({ ftl, fbl }); // FTL - FBL
	}

	auto& sh = rb.GetShader();
	glLineWidth(8.0f);
	sh.Enable();
	sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);
	rb.DrawArrays(GL_LINES);
	sh.Disable();
	glLineWidth(1.0f);
}

void CShadowHandler::FreeFBOAndTextures() {
	if (smOpaqFBO.IsValid()) {
		smOpaqFBO.Bind();
		smOpaqFBO.DetachAll();
		smOpaqFBO.Unbind();
	}

	smOpaqFBO.Kill();

	glDeleteTextures(1, &shadowDepthTexture); shadowDepthTexture = 0;
	glDeleteTextures(1, &shadowColorTexture); shadowColorTexture = 0;
}

void CShadowHandler::LoadShadowGenShaders()
{
	#define sh shaderHandler
	static const std::string shadowGenProgHandles[SHADOWGEN_PROGRAM_COUNT] = {
		"ShadowGenShaderProgModel",
		"ShadowGenShaderProgModelGL4",
		"ShadowGenshaderProgMap",
		"ShadowGenshaderProgProjectileOpaque",
	};
	static const std::string shadowGenProgDefines[SHADOWGEN_PROGRAM_COUNT] = {
		"#define SHADOWGEN_PROGRAM_MODEL\n",
		"#define SHADOWGEN_PROGRAM_MODEL_GL4\n",
		"#define SHADOWGEN_PROGRAM_MAP\n",
		"#define SHADOWGEN_PROGRAM_PROJ_OPAQ\n",
	};

	// #version has to be added here because it is conditional
	static const std::string versionDefs[3] = {
		"#version 130\n",
		"#version " + IntToString(globalRendering->supportFragDepthLayout? 420: 130) + "\n",
	};

	static const std::string extraDefs =
		("#define SUPPORT_CLIP_CONTROL " + IntToString(globalRendering->supportClipSpaceControl) + "\n") +
		("#define SUPPORT_DEPTH_LAYOUT " + IntToString(globalRendering->supportFragDepthLayout) + "\n");

	for (int i = 0; i < SHADOWGEN_PROGRAM_COUNT; i++) {
		if (i == SHADOWGEN_PROGRAM_MODEL_GL4)
			continue; //special path

		if (i == SHADOWGEN_PROGRAM_MAP)
			continue; //special path

		Shader::IProgramObject* po = sh->CreateProgramObject("[ShadowHandler]", shadowGenProgHandles[i] + "GLSL");

		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenVertProg.glsl", versionDefs[0] + shadowGenProgDefines[i] + extraDefs, GL_VERTEX_SHADER));
		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenFragProg.glsl", versionDefs[1] + shadowGenProgDefines[i] + extraDefs, GL_FRAGMENT_SHADER));

		po->Link();
		po->Enable();
		po->SetUniform("alphaMaskTex", 0);
		po->SetUniform("alphaParams", mapInfo->map.voidAlphaMin, 0.0f);
		po->Disable();
		po->Validate();

		if (!po->IsValid()) {
			po->RemoveShaderObject(GL_FRAGMENT_SHADER);
			po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenFragProg.glsl", versionDefs[0] + shadowGenProgDefines[i] + extraDefs, GL_FRAGMENT_SHADER));
			po->Link();
			po->Enable();
			po->SetUniform("alphaMaskTex", 0);
			po->SetUniform("alphaParams", mapInfo->map.voidAlphaMin, 0.0f);
			po->Disable();
			po->Validate();
		}

		shadowGenProgs[i] = po;
	}
	{
		Shader::IProgramObject* po = sh->CreateProgramObject("[ShadowHandler]", shadowGenProgHandles[SHADOWGEN_PROGRAM_MAP] + "GLSL");

		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenVertMapProg.glsl", versionDefs[0] + shadowGenProgDefines[SHADOWGEN_PROGRAM_MAP] + extraDefs, GL_VERTEX_SHADER));
		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenFragProg.glsl"   , versionDefs[1] + shadowGenProgDefines[SHADOWGEN_PROGRAM_MAP] + extraDefs, GL_FRAGMENT_SHADER));
		po->BindAttribLocation("vertexPos", 0);
		po->Link();
		po->Enable();
		po->SetUniform("alphaMaskTex", 0);
		po->SetUniform("heightMapTex", 1);
		po->SetUniform("alphaParams", mapInfo->map.voidAlphaMin, 0.0f);
		po->SetUniform("mapSize",
			static_cast<float>(mapDims.mapx * SQUARE_SIZE), static_cast<float>(mapDims.mapy * SQUARE_SIZE),
					   1.0f / (mapDims.mapx * SQUARE_SIZE),            1.0f / (mapDims.mapy * SQUARE_SIZE)
		);
		po->SetUniform("texSquare", 0, 0);
		po->Disable();
		po->Validate();

		if (!po->IsValid()) {
			po->RemoveShaderObject(GL_FRAGMENT_SHADER);
			po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenFragProg.glsl", versionDefs[0] + shadowGenProgDefines[SHADOWGEN_PROGRAM_MAP] + extraDefs, GL_FRAGMENT_SHADER));
			po->Link();
			po->Enable();
			po->SetUniform("alphaMaskTex", 0);
			po->SetUniform("heightMapTex", 1);
			po->SetUniform("alphaParams", mapInfo->map.voidAlphaMin, 0.0f);
			po->SetUniform("mapSize",
				static_cast<float>(mapDims.mapx * SQUARE_SIZE), static_cast<float>(mapDims.mapy * SQUARE_SIZE),
						   1.0f / (mapDims.mapx * SQUARE_SIZE),            1.0f / (mapDims.mapy * SQUARE_SIZE)
			);
			po->SetUniform("texSquare", 0, 0);
			po->Disable();
			po->Validate();
		}

		shadowGenProgs[SHADOWGEN_PROGRAM_MAP] = po;
	}
	if (globalRendering->haveGL4) {
		Shader::IProgramObject* po = sh->CreateProgramObject("[ShadowHandler]", shadowGenProgHandles[SHADOWGEN_PROGRAM_MODEL_GL4] + "GLSL");

		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenVertProgGL4.glsl", shadowGenProgDefines[SHADOWGEN_PROGRAM_MODEL_GL4] + extraDefs, GL_VERTEX_SHADER));
		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenFragProgGL4.glsl", shadowGenProgDefines[SHADOWGEN_PROGRAM_MODEL_GL4] + extraDefs, GL_FRAGMENT_SHADER));
		po->Link();
		po->Enable();
		po->SetUniform("alphaCtrl", 0.5f, 1.0f, 0.0f, 0.0f); // test > 0.5
		po->Disable();
		po->Validate();

		shadowGenProgs[SHADOWGEN_PROGRAM_MODEL_GL4] = po;
	}

	shadowsLoaded = true;
	#undef sh
}



bool CShadowHandler::InitFBOAndTextures()
{
	//create dummy textures / FBO in case shadowConfig is 0
	const int realShTexSize = shadowConfig > 0 ? shadowMapSize : 1;

	// smOpaqFBO is no-op constructed, has to be initialized manually
	smOpaqFBO.Init(false);

	if (!smOpaqFBO.IsValid()) {
		LOG_L(L_ERROR, "[%s] framebuffer not valid", __func__);
		return false;
	}

	// TODO: add bit depth?
	static constexpr struct {
		GLint clampMode;
		GLint filterMode;
		const char* name;
	} presets[] = {
		{GL_CLAMP_TO_BORDER, GL_LINEAR , "SHADOW-BEST"  },
		{GL_CLAMP_TO_EDGE  , GL_NEAREST, "SHADOW-COMPAT"},
	};

	static constexpr float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	bool status = false;
	for (const auto& preset : presets)
	{
		if (FBO::GetCurrentBoundFBO() == smOpaqFBO.GetId())
			smOpaqFBO.DetachAll();

		//depth
		glDeleteTextures(1, &shadowDepthTexture);
		glGenTextures(1, &shadowDepthTexture);
		glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);

		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, one);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, preset.clampMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, preset.clampMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, preset.filterMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, preset.filterMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); //no mips

		const int depthBits = std::min(globalRendering->supportDepthBufferBitDepth, 24);
		const GLint depthFormat = CGlobalRendering::DepthBitsToFormat(depthBits);

		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexImage2D(GL_TEXTURE_2D, 0, depthFormat, realShTexSize, realShTexSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);

		/// color
		glDeleteTextures(1, &shadowColorTexture);
		glGenTextures(1, &shadowColorTexture);
		glBindTexture(GL_TEXTURE_2D, shadowColorTexture);

		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, one);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, preset.clampMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, preset.clampMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, preset.filterMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, preset.filterMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); //no mips
		// TODO: Figure out if mips make sense here.

		if (static_cast<bool>(shadowColorMode)) {
			// seems like GL_RGB8 has enough precision
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, realShTexSize, realShTexSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			static constexpr GLint swizzleMask[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ONE };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

		}
		else {
			// Conserve VRAM
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, realShTexSize, realShTexSize, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
			static constexpr GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		// Mesa complains about an incomplete FBO if calling Bind before TexImage (?)
		smOpaqFBO.Bind();
		smOpaqFBO.AttachTexture(shadowDepthTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT);
		smOpaqFBO.AttachTexture(shadowColorTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		// test the FBO
		status = smOpaqFBO.CheckStatus(preset.name);

		if (status) //exit on the first occasion
			break;
	}

	glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	EnableColorOutput(true);
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	smOpaqFBO.Unbind();

	// revert to FBO = 0 default
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	return status;
}

void CShadowHandler::DrawShadowPasses()
{
	inShadowPass = true;

	glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	eventHandler.DrawWorldShadow();

	EnableColorOutput(true);
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	EnableColorOutput(false);

	if ((shadowGenBits & SHADOWGEN_BIT_TREE) != 0) {
		grassDrawer->DrawShadow();
	}

	if ((shadowGenBits & SHADOWGEN_BIT_PROJ) != 0){
		projectileDrawer->DrawShadowOpaque();
	}
	if ((shadowGenBits & SHADOWGEN_BIT_MODEL) != 0) {
		unitDrawer->DrawShadowPass();
		featureDrawer->DrawShadowPass();
	}

	// cull front-faces during the terrain shadow pass: sun direction
	// can be set so oblique that geometry back-faces are visible (eg.
	// from hills near map edges) from its POV
	//
	// not the best idea, causes acne when projecting the shadow-map
	// (rasterizing back-faces writes different depth values) and is
	// no longer required since border geometry will fully hide them
	// (could just disable culling of terrain faces entirely, but we
	// also want to prevent overdraw in low-angle passes)
	// glCullFace(GL_FRONT);

	// Restore GL_BACK culling, because Lua shadow materials might
	// have changed culling at their own discretion
	glCullFace(GL_BACK);
	if ((shadowGenBits & SHADOWGEN_BIT_MAP) != 0){
		ZoneScopedN("Draw::World::CreateShadows::Terrain");
		readMap->GetGroundDrawer()->DrawShadowPass();
	}

	//transparent pass, comes last
	if ((shadowGenBits & SHADOWGEN_BIT_PROJ) != 0) {
		projectileDrawer->DrawShadowTransparent();
		eventHandler.DrawShadowPassTransparent();
	}

	glPopAttrib();

	inShadowPass = false;
}

static CMatrix44f ComposeLightMatrix(const CCamera* playerCam, const ISkyLight* light)
{
	CMatrix44f lightMatrix;

	// sun direction is in world-space, invert it
	float3 zDir = -float3(light->GetLightDir().xyz);

	// Try to rotate LM's X and Y around Z direction to fit playerCam tightest

	// find the most orthogonal vector to zDir and call it xDir
	float minDot = 1.0f;
	float3 xDir;
	for (const auto* dir : { &playerCam->forward, &playerCam->right, &playerCam->up }) {
		const float dp = zDir.dot(*dir);
		if (math::fabs(dp) < minDot) {
			xDir = std::copysign(1.0f, dp) * (*dir);
			minDot = math::fabs(dp);
		}
	}

	xDir = UpVector;

	// orthonormalize
	xDir = (xDir - xDir.dot(zDir) * zDir).ANormalize();
	float3 yDir = xDir.cross(zDir).ANormalize();

	lightMatrix.SetZ(zDir);
	lightMatrix.SetY(yDir);
	lightMatrix.SetX(xDir);
	//lightMatrix.Transpose();

	return lightMatrix;
}

void CShadowHandler::CalcShadowMatrices(CCamera* playerCam, CCamera* shadowCam)
{
	float2 mapDimsWS = float2{
		static_cast<float>(mapDims.mapx * SQUARE_SIZE),
		static_cast<float>(mapDims.mapy * SQUARE_SIZE)
	};

	AABB worldBounds;
	worldBounds.mins = float3{ 0.0f        , readMap->GetCurrMinHeight(),         0.0f };
	worldBounds.maxs = float3{ mapDimsWS.x , readMap->GetCurrMaxHeight(),  mapDimsWS.y };

	worldBounds.Combine(unitDrawer->GetObjectsBounds());
	worldBounds.Combine(featureDrawer->GetObjectsBounds());

	const auto projMidPos = CalcShadowProjectionPos(playerCam, worldBounds);

	float3 camPos;
	// construct Camera World Matrix & View Matrix
	{
		CMatrix44f camWorldMat;

		float3 zAxis = float3{ ISky::GetSky()->GetLight()->GetLightDir().xyz };
		float3 xAxis = float3(1, 0, 0);
		xAxis = (xAxis - xAxis.dot(zAxis) * zAxis).Normalize();
		float3 yAxis = zAxis.cross(xAxis);

		camWorldMat.SetX(xAxis);
		camWorldMat.SetY(yAxis);
		camWorldMat.SetZ(zAxis);

		// convert camera "world" matrix into camera view matrix
		// https://www.3dgep.com/understanding-the-view-matrix/
		viewMatrix = camWorldMat.InvertAffine();
		// viewMatrix position will be added a bit later

		AABB worldBoundsLS;
		for (const auto& cornerPointLS : worldBounds.GetCorners(viewMatrix)) {
			worldBoundsLS.AddPoint(cornerPointLS);
		}

		bool hit = RayHitsAABB(worldBoundsLS, viewMatrix * projMidPos, float3{ 0, 0, 1 }, &camPos);

		// convert back to world-space
		camPos = camWorldMat * camPos;

		// do the camWorldMat.InvertAffine(); for the position part
		viewMatrix.col[3] = float4{ -xAxis.dot(camPos), -yAxis.dot(camPos), -zAxis.dot(camPos), 1.0f };

		camWorldMat = viewMatrix.InvertAffine(); // sanity check
		assert(camPos == camWorldMat.GetPos());
	}

	float camToProjPosDist = camPos.distance(projMidPos);

	lightAABB.Reset();

	for (size_t i = 0; i < 4; ++i) {
		lightAABB.AddPoint(viewMatrix * frustumPoints[4 + i]);
	}
	lightAABB.AddPoint(viewMatrix * playerCam->GetPos());

	lightAABB.maxs.z = 0.0f; // @camPos

	projMatrix = CMatrix44f::ClipOrthoProj(
		 lightAABB.mins.x,  lightAABB.maxs.x,
		 lightAABB.mins.y,  lightAABB.maxs.y,
		-lightAABB.maxs.z, -lightAABB.mins.z,
		 globalRendering->supportClipSpaceControl
	);

	viewProjMatrix = projMatrix * viewMatrix;
}

void CShadowHandler::SetShadowCamera(CCamera* shadowCam)
{
	const int realShTexSize = shadowConfig > 0 ? shadowMapSize : 1;

	// first set matrices needed by shaders (including ShadowGenVertProg)
	shadowCam->SetProjMatrix(projMatrix);
	shadowCam->SetViewMatrix(viewMatrix);

	// scales are in a space relative to the camera position and along worldspace camera's principal vectors
	// while lightAABB is in camera view space, so need to use relative (max - min) values
	float4 shadowProjScales{
		lightAABB.maxs.x - lightAABB.mins.x,
		lightAABB.maxs.y - lightAABB.mins.y,
		0.0f,
		-(lightAABB.maxs.z - lightAABB.mins.z) // shadowCam->forward is looking towards the light, so make far plane negative
	};

	// convert xy-length to half-length
	shadowCam->SetFrustumScales(shadowProjScales * float4(0.5f, 0.5f, 1.0f, 1.0f));
	shadowCam->UpdateFrustum();
	shadowCam->UpdateLoadViewport(0, 0, realShTexSize, realShTexSize);

	// load matrices into gl_{ModelView,Projection}Matrix
	shadowCam->Update({ false, false, false, false, false });

	shadowCam->SetAspectRatio(shadowProjScales.x / shadowProjScales.y);
}

void CShadowHandler::SetupShadowTexSampler(unsigned int texUnit, bool enable) const
{
	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);

	// support FFP context
	if (enable)
		glEnable(GL_TEXTURE_2D);

	SetupShadowTexSamplerRaw();
}

void CShadowHandler::SetupShadowTexSamplerRaw() const
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
	// glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
	// glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_ALPHA);
}

void CShadowHandler::ResetShadowTexSampler(unsigned int texUnit, bool disable) const
{
	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (disable)
		glDisable(GL_TEXTURE_2D);

	ResetShadowTexSamplerRaw();
}

void CShadowHandler::ResetShadowTexSamplerRaw() const
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
}


void CShadowHandler::CreateShadows()
{
	// NOTE:
	//   we unbind later in WorldDrawer::GenerateIBLTextures() to save render
	//   context switches (which are one of the slowest OpenGL operations!)
	//   together with VP restoration
	smOpaqFBO.Bind();

	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glShadeModel(GL_FLAT);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

	CCamera* prvCam = CCameraHandler::GetSetActiveCamera(CCamera::CAMTYPE_SHADOW);
	SetShadowCamera(camera); // shadowCam here

	if (ISky::GetSky()->GetLight()->GetLightIntensity() > 0.0f)
		DrawShadowPasses();

	CCameraHandler::SetActiveCamera(prvCam->GetCamType());
	prvCam->Update();


	glShadeModel(GL_SMOOTH);

	//revert to default, EnableColorOutput(true) is not enough
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void CShadowHandler::EnableColorOutput(bool enable) const
{
	assert(FBO::GetCurrentBoundFBO() == smOpaqFBO.GetId());

	const GLboolean b = static_cast<GLboolean>(enable);
	glColorMask(b, b, b, GL_FALSE);
}


float3 CShadowHandler::CalcShadowProjectionPos(CCamera* playerCam, const AABB& worldBounds)
{
	float3 projPos;
	//for (int i = 0; i < 8; ++i)
	//	frustumPoints[i] = playerCam->GetFrustumVert(i);

	const std::initializer_list<float4> clipPlanes = {
		float4{-UpVector ,  (worldBounds.maxs.y) },
		float4{ UpVector , -(worldBounds.mins.y) },
		//float4{-RgtVector,  (worldBounds.maxs.x) },
		//float4{ RgtVector, -(worldBounds.mins.x) },
		//float4{-FwdVector,  (worldBounds.maxs.z) },
		//float4{ FwdVector, -(worldBounds.mins.z) },
	};

	for (int i = 0; i < 4; ++i) {
		ClipRayByPlanes(playerCam->GetPos(), frustumPoints[4 + i], clipPlanes);
		/*
		//hard clamp xz
		frustumPoints[    i].x = std::clamp(frustumPoints[    i].x, worldBounds.mins.x, worldBounds.maxs.x);
		frustumPoints[    i].z = std::clamp(frustumPoints[    i].z, worldBounds.mins.z, worldBounds.maxs.z);
		frustumPoints[4 + i].x = std::clamp(frustumPoints[4 + i].x, worldBounds.mins.x, worldBounds.maxs.x);
		frustumPoints[4 + i].z = std::clamp(frustumPoints[4 + i].z, worldBounds.mins.z, worldBounds.maxs.z);
		*/

		projPos += frustumPoints[4 + i];
	}
	projPos += playerCam->GetPos();
	projPos *= 0.2f;

	return projPos;
}
