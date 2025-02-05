/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <cfloat>

#include "ShadowHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameVersion.h"
#include "Game/Game.h"
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
#include "Rendering/GL/SubState.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GL/RenderBuffers.h"
#include "System/Config/ConfigHandler.h"
#include "System/Geometry/Polygon.hpp"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"

#include "lib/fmt/format.h"

CONFIG(int, Shadows).defaultValue(2).headlessValue(-1).minimumValue(-1).safemodeValue(-1).description(R"(
Sets whether shadows are rendered.
Use numbers:
	-1:=forceoff, 0:=off, 1:=full
Or use bitmask combination to disable particular parts of shadows rendering:
	2:=shadows for terrain, 4:=shadows for solid models, 8:=semitransparent shadows for particles, 16:=grass
)");

CONFIG(int, ShadowMapSize)
	.defaultValue(CShadowHandler::DEF_SHADOWMAP_SIZE)
	.minimumValue(CShadowHandler::MIN_SHADOWMAP_SIZE)
	.maximumValue(CShadowHandler::MAX_SHADOWMAP_SIZE)
	.description("Sets the resolution of shadows. Higher numbers increase quality at the cost of performance.");

CONFIG(int, ShadowProjectionMode).deprecated(true);
CONFIG(bool, ShadowColorMode).deprecated(true);

CShadowHandler shadowHandler;

void CShadowHandler::Reload(const char* argv)
{
	int nextShadowConfig = (shadowConfig + 1) & 0xF;
	int nextShadowMapSize = shadowMapSize;

	if (argv != nullptr)
		(void) sscanf(argv, "%i %i", &nextShadowConfig, &nextShadowMapSize);

	// do nothing without a parameter change
	if (nextShadowConfig == shadowConfig && nextShadowMapSize == shadowMapSize)
		return;

	configHandler->Set("Shadows", nextShadowConfig & 0xF);
	configHandler->Set("ShadowMapSize", nextShadowMapSize);

	Kill();
	Init();
}

void CShadowHandler::Init()
{
	const bool tmpFirstInit = firstInit;
	firstInit = false;

	shadowConfig  = configHandler->GetInt("Shadows");
	shadowMapSize = configHandler->GetInt("ShadowMapSize");
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

namespace {
	enum {
		NBL = 1,
		FBL = 0,
		NBR = 3,
		FBR = 2,
		NTL = 5,
		FTL = 4,
		NTR = 7,
		FTR = 6,
	};
}

void CShadowHandler::DrawFrustumDebugMap() const
{
	if (!debugFrustum || !shadowsLoaded || !freezeFrustum)
		return;

	static constexpr SColor SHADOW_CAM_COL = SColor{ 255,   0,   0, 255 };
	static constexpr SColor WORLD_BNDS_COL = SColor{   0,   0, 255, 255 };
	static constexpr SColor PLAYER_CAM_COL = SColor{ 255, 255, 255, 255 };
	static constexpr SColor CLIPPD_CAM_COL = SColor{   0, 255,   0, 255 };
	static constexpr SColor CSHADC_CAM_COL = SColor{   0, 255, 255, 255 };

	CCamera* shadCam = CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW);
	{
		auto mat = shadCam->GetViewMatrixInverse();
		mat.Scale(32.0f);
		GL::shapes.DrawSolidSphere(10, 10, mat, float4{ 1, 0, 0, 1 });
	}

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();
	rb.AssertSubmission();
	auto& sh = rb.GetShader();

	// shadow frustum
	{
		const auto ntl = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NTL), SHADOW_CAM_COL };
		const auto ntr = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NTR), SHADOW_CAM_COL };
		const auto nbr = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NBR), SHADOW_CAM_COL };
		const auto nbl = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NBL), SHADOW_CAM_COL };

		const auto ftl = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FTL), SHADOW_CAM_COL };
		const auto ftr = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FTR), SHADOW_CAM_COL };
		const auto fbr = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FBR), SHADOW_CAM_COL };
		const auto fbl = VA_TYPE_C{ shadCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FBL), SHADOW_CAM_COL };

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

		glLineWidth(6.0f);
		sh.Enable();
		sh.SetUniform("ucolor", 1.0f, 1.0f, 1.0f, 1.0f);
		rb.DrawArrays(GL_LINES);
		sh.Disable();
	}

	// world bounds
	{
		const auto wcs = game->GetWorldBounds().GetCorners();

		const auto ntl = VA_TYPE_C{ wcs[NTL], WORLD_BNDS_COL };
		const auto ntr = VA_TYPE_C{ wcs[NTR], WORLD_BNDS_COL };
		const auto nbr = VA_TYPE_C{ wcs[NBR], WORLD_BNDS_COL };
		const auto nbl = VA_TYPE_C{ wcs[NBL], WORLD_BNDS_COL };

		const auto ftl = VA_TYPE_C{ wcs[FTL], WORLD_BNDS_COL };
		const auto ftr = VA_TYPE_C{ wcs[FTR], WORLD_BNDS_COL };
		const auto fbr = VA_TYPE_C{ wcs[FBR], WORLD_BNDS_COL };
		const auto fbl = VA_TYPE_C{ wcs[FBL], WORLD_BNDS_COL };

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

		glLineWidth(2.0f);
		sh.Enable();
		rb.DrawArrays(GL_LINES);
		sh.Disable();
	}

	// player's camera frustum
	{
		const auto ntl = VA_TYPE_C{ playCamFrustum[CCamera::FRUSTUM_POINT_NTL], PLAYER_CAM_COL };
		const auto ntr = VA_TYPE_C{ playCamFrustum[CCamera::FRUSTUM_POINT_NTR], PLAYER_CAM_COL };
		const auto nbr = VA_TYPE_C{ playCamFrustum[CCamera::FRUSTUM_POINT_NBR], PLAYER_CAM_COL };
		const auto nbl = VA_TYPE_C{ playCamFrustum[CCamera::FRUSTUM_POINT_NBL], PLAYER_CAM_COL };

		const auto ftl = VA_TYPE_C{ playCamFrustum[CCamera::FRUSTUM_POINT_FTL], PLAYER_CAM_COL };
		const auto ftr = VA_TYPE_C{ playCamFrustum[CCamera::FRUSTUM_POINT_FTR], PLAYER_CAM_COL };
		const auto fbr = VA_TYPE_C{ playCamFrustum[CCamera::FRUSTUM_POINT_FBR], PLAYER_CAM_COL };
		const auto fbl = VA_TYPE_C{ playCamFrustum[CCamera::FRUSTUM_POINT_FBL], PLAYER_CAM_COL };

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

		glLineWidth(2.0f);
		sh.Enable();
		rb.DrawArrays(GL_LINES);
		sh.Disable();
	}

	// clipped world cube
	{
		size_t frstIdx = 0;
		for (size_t currIdx = 0; currIdx < clippedWorldCube.size() - 1; /*NOOP*/) {
			size_t nextIdx = (currIdx + 1);
			rb.AddVertices({ { clippedWorldCube[currIdx], CLIPPD_CAM_COL}, { clippedWorldCube[nextIdx], CLIPPD_CAM_COL} });
			if (clippedWorldCube[frstIdx] == clippedWorldCube[nextIdx]) {
				currIdx += 2; // skip one
				frstIdx = currIdx;
			} else {
				currIdx += 1;
			}
		}

		glLineWidth(4.0f);
		sh.Enable();
		rb.DrawArrays(GL_LINES);
		sh.Disable();
	}

	// clipped extended shadow cube
	{
		size_t frstIdx = 0;
		for (size_t currIdx = 0; currIdx < clippedShadowCube.size() - 1; /*NOOP*/) {
			size_t nextIdx = (currIdx + 1);
			rb.AddVertices({ { clippedShadowCube[currIdx], CSHADC_CAM_COL}, { clippedShadowCube[nextIdx], CSHADC_CAM_COL} });
			if (clippedShadowCube[frstIdx] == clippedShadowCube[nextIdx]) {
				currIdx += 2; // skip one
				frstIdx = currIdx;
			}
			else {
				currIdx += 1;
			}
		}

		glLineWidth(2.0f);
		sh.Enable();
		rb.DrawArrays(GL_LINES);
		sh.Disable();
		glLineWidth(1.0f);
	}
}
/*
void CShadowHandler::DumpFrustumData() const
{
	const auto& worldBounds = game->GetWorldBounds();
	const CCamera* playerCam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

	const auto ntl = playerCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NTL);
	const auto ntr = playerCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NTR);
	const auto nbr = playerCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NBR);
	const auto nbl = playerCam->GetFrustumVert(CCamera::FRUSTUM_POINT_NBL);

	const auto ftl = playerCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FTL);
	const auto ftr = playerCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FTR);
	const auto fbr = playerCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FBR);
	const auto fbl = playerCam->GetFrustumVert(CCamera::FRUSTUM_POINT_FBL);

	std::ostringstream ss;

	{
		const auto wcs = worldBounds.GetCorners();

		Geometry::Polygon worldCube;

		// Left Face
		worldCube.AddFace(wcs[0], wcs[1], wcs[5], wcs[4]);
		// Right Face
		worldCube.AddFace(wcs[2], wcs[6], wcs[7], wcs[3]);
		// Near Face
		worldCube.AddFace(wcs[0], wcs[4], wcs[6], wcs[2]);
		// Far Face
		worldCube.AddFace(wcs[1], wcs[3], wcs[7], wcs[5]);
		// Top Face
		worldCube.AddFace(wcs[4], wcs[5], wcs[7], wcs[6]);
		// Bottom Face
		worldCube.AddFace(wcs[0], wcs[2], wcs[3], wcs[1]);

		Geometry::Polygon cameraFrustum;

		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_LFT));
		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_RGT));
		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_BOT));
		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_TOP));
		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_NEA));
		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_FAR));

		worldCube.ClipByInPlace(cameraFrustum);

		for (const auto& face : worldCube.GetFaces()) {
			ss << "EdgeForm[Directive[Thick, Dashed, Yellow]], Opacity[0.2], Green, Polygon[{";
			for (const auto& pnt : face.GetPoints()) {
				ss << "{" << pnt.x << ", " << pnt.y << ", " << pnt.z << "}, ";
			}
			ss.seekp(-2, std::ios_base::end);
			ss << "}],\n";
		}
		ss.seekp(-2, std::ios_base::end);
		ss << '\0';
	}

	LOG(R"(
Graphics3D[{
{EdgeForm[Directive[Thick, Dashed, Blue]], Opacity[0], Cuboid[{%f, %f, %f}, {%f, %f, %f}]},
{EdgeForm[Directive[Thick, Dashed, Red]], Opacity[0], Polygon[{{%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}}]},
{EdgeForm[Directive[Thick, Dashed, Red]], Opacity[0], Polygon[{{%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}}]},
{EdgeForm[Directive[Thick, Dashed, Red]], Opacity[0], Polygon[{{%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}}]},
{EdgeForm[Directive[Thick, Dashed, Red]], Opacity[0], Polygon[{{%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}}]},
{EdgeForm[Directive[Thick, Dashed, Red]], Opacity[0], Polygon[{{%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}}]},
{EdgeForm[Directive[Thick, Dashed, Red]], Opacity[0], Polygon[{{%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}}]},
%s
}])"
	, worldBounds.mins.x, worldBounds.mins.y, worldBounds.mins.z, worldBounds.maxs.x, worldBounds.maxs.y, worldBounds.maxs.z
	, ntl.x, ntl.y, ntl.z, ntr.x, ntr.y, ntr.z, nbr.x, nbr.y, nbr.z, nbl.x, nbl.y, nbl.z
	, ftl.x, ftl.y, ftl.z, ftr.x, ftr.y, ftr.z, fbr.x, fbr.y, fbr.z, fbl.x, fbl.y, fbl.z
	, ntl.x, ntl.y, ntl.z, nbl.x, nbl.y, nbl.z, fbl.x, fbl.y, fbl.z, ftl.x, ftl.y, ftl.z
	, ntr.x, ntr.y, ntr.z, nbr.x, nbr.y, nbr.z, fbr.x, fbr.y, fbr.z, ftr.x, ftr.y, ftr.z
	, ntl.x, ntl.y, ntl.z, ntr.x, ntr.y, ntr.z, ftr.x, ftr.y, ftr.z, ftl.x, ftl.y, ftl.z
	, nbl.x, nbl.y, nbl.z, nbr.x, nbr.y, nbr.z, fbr.x, fbr.y, fbr.z, fbl.x, fbl.y, fbl.z
	, ss.str().c_str()
	);
}
*/

void CShadowHandler::FreeFBOAndTextures() {
	if (shadowsFBO.IsValid()) {
		shadowsFBO.Bind();
		shadowsFBO.DetachAll();
		shadowsFBO.Unbind();
	}

	shadowsFBO.Kill();

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

	// shadowsFBO is no-op constructed, has to be initialized manually
	shadowsFBO.Init(false);

	if (!shadowsFBO.IsValid()) {
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
		if (FBO::GetCurrentBoundFBO() == shadowsFBO.GetId())
			shadowsFBO.DetachAll();

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

		// seems like GL_RGB8 has enough precision
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, realShTexSize, realShTexSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		static constexpr GLint swizzleMask[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ONE };
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

		glBindTexture(GL_TEXTURE_2D, 0);

		// Mesa complains about an incomplete FBO if calling Bind before TexImage (?)
		shadowsFBO.Bind();
		shadowsFBO.AttachTexture(shadowDepthTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT);
		shadowsFBO.AttachTexture(shadowColorTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		// test the FBO
		status = shadowsFBO.CheckStatus(preset.name);

		if (status) //exit on the first occasion
			break;
	}

	glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	EnableColorOutput(true);
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	shadowsFBO.Unbind();

	// revert to FBO = 0 default
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	return status;
}

void CShadowHandler::DrawShadowPasses()
{
	inShadowPass = true;

	using namespace GL::State;
	auto state = GL::SubState(
		Blending(GL_FALSE),
		Lighting(GL_FALSE),
		Texture2D(GL_FALSE),
		ShadeModel(GL_FLAT),
		DepthMask(GL_TRUE),
		DepthTest(GL_TRUE),
		CullFace(GL_BACK),
		Culling(GL_TRUE),
		PolygonOffsetFill(GL_TRUE)
	);

	EnableColorOutput(false);

	eventHandler.DrawWorldShadow();

	{
		auto polyOffset = GL::SubState(
			PolygonOffset(objPolygonOffsetScale, objPolygonOffsetUnits) // factor * DZ + r * units
		);

		if ((shadowGenBits & SHADOWGEN_BIT_TREE) != 0) {
			grassDrawer->DrawShadow();
		}

		if ((shadowGenBits & SHADOWGEN_BIT_PROJ) != 0) {
			projectileDrawer->DrawShadowOpaque();
		}
		if ((shadowGenBits & SHADOWGEN_BIT_MODEL) != 0) {
			unitDrawer->DrawShadowPass();
			featureDrawer->DrawShadowPass();
		}
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

	{
		auto polyOffset = GL::SubState(
			CullFace(GL_FRONT),
			PolygonOffset(mapPolygonOffsetScale, mapPolygonOffsetUnits) // factor * DZ + r * units
		);

		if ((shadowGenBits & SHADOWGEN_BIT_MAP) != 0) {
			ZoneScopedN("Draw::World::CreateShadows::Terrain");
			readMap->GetGroundDrawer()->DrawShadowPass();
		}
	}

	//transparent pass, comes last
	{
		auto polyOffset = GL::SubState(
			PolygonOffset(1.0f, 1.0f) // factor * DZ + r * units
		);

		if ((shadowGenBits & SHADOWGEN_BIT_PROJ) != 0) {
			projectileDrawer->DrawShadowTransparent();
			eventHandler.DrawShadowPassTransparent();
		}
	}

	inShadowPass = false;
}

namespace Impl {
	float3 GetLightXDir(const CCamera* playerCam, const float3 toLightDir)
	{
#if 0
		// align with the world's X axis
		float3 xAxis = float3(1, 0, 0);
		return (xAxis - xAxis.dot(toLightDir) * toLightDir).Normalize();
#else
		// Try to rotate LM's X and Y around Z direction to fit playerCam tightest
		// find the most orthogonal vector to zDir and call it xDir
		float minDot = 1.0f;
		float3 xDir;
		for (const auto* dir : { &playerCam->forward, &playerCam->right, &playerCam->up }) {
			const float dp = toLightDir.dot(*dir);
			if (math::fabs(dp) < minDot) {
				xDir = *dir;
				minDot = math::fabs(dp);
			}
		}

		xDir = (xDir - xDir.dot(toLightDir) * toLightDir).Normalize();

		return xDir;
#endif
	}
};

void CShadowHandler::CalcShadowMatrices(CCamera* playerCam, CCamera* shadowCam)
{
	SCOPED_TIMER("CShadowHandler::CalcShadowMatrices");

	// save the player's camera frustum verts in case we need them in CShadowHandler::DrawFrustumDebugMap()
	playCamFrustum = playerCam->GetFrustum().verts;

	// original world cube
	Geometry::Polygon worldCube;
	worldCube.MakeFrom(game->GetWorldBounds());

	{
		Geometry::Polygon cameraFrustum;

		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_LFT));
		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_RGT));
		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_BOT));
		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_TOP));
		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_NEA));
		cameraFrustum.AddFace().SetPlane(playerCam->GetFrustumPlane(CCamera::FRUSTUM_PLANE_FAR));

		Geometry::Polygon clipperWorldCubePolygon = worldCube.ClipBy(cameraFrustum);
		clippedWorldCube = clipperWorldCubePolygon.GetAllLines();
	}

	// construct Camera World Matrix & View Matrix
	CMatrix44f viewMatrixInv;
	{
		float3 zAxis = float3{ ISky::GetSky()->GetLight()->GetLightDir().xyz };
		float3 xAxis = Impl::GetLightXDir(playerCam, zAxis);
		float3 yAxis = zAxis.cross(xAxis);

		viewMatrixInv.SetX(xAxis);
		viewMatrixInv.SetY(yAxis);
		viewMatrixInv.SetZ(zAxis);

		// convert camera "world" matrix into camera view matrix
		// https://www.3dgep.com/understanding-the-view-matrix/
		viewMatrix = viewMatrixInv.InvertAffine();
	}

	lightAABB.Reset();

	for (const auto& p : clippedWorldCube) {
		lightAABB.AddPoint(viewMatrix * p);
	}

	float3 lsMidPos = lightAABB.CalcCenter();
	float3 lsDims = lightAABB.CalcScales();

	viewMatrix.SetPos(-(lsMidPos + float3{ 0.0f, 0.0f, lsDims.z }));
	viewMatrixInv.Translate(-viewMatrix.GetPos());

	lightAABB.mins += viewMatrix.GetPos();
	lightAABB.maxs += viewMatrix.GetPos();

	// this is required when shadow camera position is roughly behind the player's camera
	// in such cases without adjustment objects behind the near plane of the player's camera
	// won't be included into the shadow cuboid bounds
	float extraCamHeight = 0.0f;
	{
		AABB lightAABBExt = lightAABB;

		// extend lightAABBExt.z in both directions so they're guaranteed to extend past the worldCube boundaries 
		lightAABBExt.maxs.z += 2.0f * readMap->GetBoundingRadius();
		lightAABBExt.mins.z -= 2.0f * readMap->GetBoundingRadius();

		Geometry::Polygon unclippedShadowCubePoly;
		unclippedShadowCubePoly.MakeFrom(lightAABBExt, viewMatrixInv);
		unclippedShadowCubePoly.FlipFacesDirection(); // figure out why it's needed

		Geometry::Polygon clippedShadowCubePoly = worldCube.ClipBy(unclippedShadowCubePoly);
		clippedShadowCube = clippedShadowCubePoly.GetAllLines();

		// reuse lightAABBExt variable
		lightAABBExt = clippedShadowCubePoly.GetAABB(viewMatrix);
		extraCamHeight = std::max(extraCamHeight, lightAABBExt.maxs.z);
	}

	// shift camera further away to account for the calculation above
	viewMatrix.col[3].z -= extraCamHeight;

	// translate mins.z accordingly
	lightAABB.mins.z -= extraCamHeight;
	// Make sure maxs.z is at the camera position
	lightAABB.maxs.z = 0.0f; // @ camPos

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
	shadowCam->UpdateDerivedMatrices();

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
	shadowsFBO.Bind();

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
	assert(FBO::GetCurrentBoundFBO() == shadowsFBO.GetId());

	const GLboolean b = static_cast<GLboolean>(enable);
	glColorMask(b, b, b, GL_FALSE);
}