/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>

#include "Camera.h"
#include "CameraHandler.h"
#include "UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "System/SpringMath.h"
#include "System/float3.h"
#include "System/Matrix44f.h"
#include "System/Config/ConfigHandler.h"

#include "System/Misc/TracyDefs.h"


CONFIG(float, EdgeMoveWidth)
	.defaultValue(0.02f)
	.minimumValue(0.0f)
	.description("The width (in percent of screen size) of the EdgeMove scrolling area.");
CONFIG(bool, EdgeMoveDynamic)
	.defaultValue(true)
	.description("If EdgeMove scrolling speed should fade with edge distance.");
CONFIG(float, CameraMoveFastMult)
	.defaultValue(10.0f)
	.minimumValue(1.0f)
	.description("The multiplier applied to speed when camera is in movefast state.");
CONFIG(float, CameraMoveSlowMult)
	.defaultValue(0.1f)
	.maximumValue(1.0f)
	.description("The multiplier applied to speed when camera is in moveslow state.");
CONFIG(int, CamFrameTimeCorrection)
    .defaultValue(0)
	.minimumValue(0)
	.description("Sets wether the camera interpolation factor should be the inverse of fps or last draw frame time (0 = lastdrawframetime, 1 = fpsinv)");


CCamera::CCamera(uint32_t cameraType, uint32_t projectionType)
	: camType(cameraType)
	, projType(projectionType)
	, inViewPlanesMask((camType == CCamera::CAMTYPE_SHADOW) ? 0xF : 0x3F) // 0x3F - all planes, 0xF - all planes but NEAR/FAR
{
	assert(cameraType < CAMTYPE_COUNT);

	memset(viewport, 0, 4 * sizeof(int));
	memset(movState, 0, sizeof(movState));
	memset(rotState, 0, sizeof(rotState));

	frustum.scales.z = CGlobalRendering::MIN_ZNEAR_DIST;
	frustum.scales.w = CGlobalRendering::MAX_VIEW_RANGE;

	SetVFOV(45.0f);
	UpdateFrustum();
}

void CCamera::SetCamType(uint32_t ct)
{
	RECOIL_DETAILED_TRACY_ZONE;
	camType = ct;
	// 0x3F - all planes, 0xF - all planes but NEAR/FAR
	inViewPlanesMask = (camType == CCamera::CAMTYPE_SHADOW) ? 0xF : 0x3F;
}

void CCamera::InitConfigNotify(){
	RECOIL_DETAILED_TRACY_ZONE;
	configHandler->NotifyOnChange(this, {"CameraMoveFastMult", "CameraMoveSlowMult", "CamFrameTimeCorrection", "EdgeMoveDynamic", "EdgeMoveWidth"});
	ConfigUpdate();
}

void CCamera::RemoveConfigNotify(){
	RECOIL_DETAILED_TRACY_ZONE;
	configHandler->RemoveObserver(this);
}


void CCamera::ConfigUpdate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	moveFastMult = configHandler->GetFloat("CameraMoveFastMult");
	moveSlowMult = configHandler->GetFloat("CameraMoveSlowMult");
	useInterpolate = configHandler->GetInt("CamFrameTimeCorrection");
	edgeMoveDynamic = configHandler->GetBool("EdgeMoveDynamic");
	edgeMoveWidth = configHandler->GetFloat("EdgeMoveWidth");
}

void CCamera::ConfigNotify(const std::string & key, const std::string & value)
{
	RECOIL_DETAILED_TRACY_ZONE;
	ConfigUpdate();
}


CCamera* CCamera::GetActive()
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (CCameraHandler::GetActiveCamera());
}


void CCamera::CopyState(const CCamera* cam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// note: xy-scales are only relevant for CAMTYPE_SHADOW
	frustum     = cam->frustum;

	forward     = cam->GetForward();
	right       = cam->GetRight();
	up          = cam->GetUp();

	pos         = cam->GetPos();
	rot         = cam->GetRot();

	fov         = cam->GetVFOV();
	halfFov     = cam->GetHalfFov();
	tanHalfFov  = cam->GetTanHalfFov();

	lppScale    = cam->GetLPPScale();
	aspectRatio = cam->GetAspectRatio();

	// do not copy this, each camera knows its own type
	// camType = cam->GetCamType();
}

void CCamera::CopyStateReflect(const CCamera* cam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(cam->GetCamType() != CAMTYPE_UWREFL);
	assert(     GetCamType() == CAMTYPE_UWREFL);

	SetDir(cam->GetDir() * float3(1.0f, -1.0f, 1.0f));
	SetPos(cam->GetPos() * float3(1.0f, -1.0f, 1.0f));
	SetRotZ(-cam->GetRot().z);
	SetVFOV(cam->GetVFOV());

	Update(false, true, false);
}

void CCamera::Update(const UpdateParams& p)
{
	RECOIL_DETAILED_TRACY_ZONE;
	lppScale = (2.0f * tanHalfFov) * globalRendering->pixelY;
	aspectRatio = globalRendering->aspectRatio;

	// should be set before UpdateMatrices
	if (p.updateViewRange)
		UpdateViewRange();
	if (p.updateDirs)
		UpdateDirsFromRot(rot);
	if (p.updateMats)
		UpdateMatrices(globalRendering->viewSizeX, globalRendering->viewSizeY, aspectRatio);
	if (p.updateViewPort)
		UpdateViewPort(globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);
	if (p.updateFrustum)
		UpdateFrustum();

	LoadMatrices();
	// not done here
	// LoadViewPort();
}

void CCamera::UpdateFrustum()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// scale-factors for {x,y}-axes
	float2 nAxisScales;
	float2 fAxisScales;

	assert(projType <= PROJTYPE_ORTHO);
	if (projType == PROJTYPE_PERSP) {
		const float2 tanHalfFOVs = {math::tan(GetHFOV() * 0.5f * math::DEG_TO_RAD), tanHalfFov}; // horz, vert
		nAxisScales = {frustum.scales.z * tanHalfFOVs.x, frustum.scales.z * tanHalfFOVs.y}; // x, y
		fAxisScales = {frustum.scales.w * tanHalfFOVs.x, frustum.scales.w * tanHalfFOVs.y}; // x, y
	} else { //PROJTYPE_ORTHO
		nAxisScales = {frustum.scales.x, frustum.scales.y};
		fAxisScales = {frustum.scales.x, frustum.scales.y};
	}

	frustum.verts[FRUSTUM_POINT_NBL] = pos + (forward * frustum.scales.z) + (right * -nAxisScales.x) + (up * -nAxisScales.y); // nbl
	frustum.verts[FRUSTUM_POINT_NBR] = pos + (forward * frustum.scales.z) + (right *  nAxisScales.x) + (up * -nAxisScales.y); // nbr
	frustum.verts[FRUSTUM_POINT_NTR] = pos + (forward * frustum.scales.z) + (right *  nAxisScales.x) + (up *  nAxisScales.y); // ntr
	frustum.verts[FRUSTUM_POINT_NTL] = pos + (forward * frustum.scales.z) + (right * -nAxisScales.x) + (up *  nAxisScales.y); // ntl
	frustum.verts[FRUSTUM_POINT_FBL] = pos + (forward * frustum.scales.w) + (right * -fAxisScales.x) + (up * -fAxisScales.y); // fbl
	frustum.verts[FRUSTUM_POINT_FBR] = pos + (forward * frustum.scales.w) + (right *  fAxisScales.x) + (up * -fAxisScales.y); // fbr
	frustum.verts[FRUSTUM_POINT_FTR] = pos + (forward * frustum.scales.w) + (right *  fAxisScales.x) + (up *  fAxisScales.y); // ftr
	frustum.verts[FRUSTUM_POINT_FTL] = pos + (forward * frustum.scales.w) + (right * -fAxisScales.x) + (up *  fAxisScales.y); // ftl

	const auto SetFrustumPlane = [this](uint32_t i, uint32_t v1i, uint32_t v2i, uint32_t v3i) {
		const auto& v1 = frustum.verts[v1i];
		const auto& v2 = frustum.verts[v2i];
		const auto& v3 = frustum.verts[v3i];

		const float3 u = v1 - v2;
		const float3 v = v3 - v2;

		const float3 n = v.cross(u).UnsafeANormalize();
		const float  d = -n.dot(v2);
		frustum.planes[i] = float4(n, d);
	};

	SetFrustumPlane(FRUSTUM_PLANE_LFT, FRUSTUM_POINT_NTL, FRUSTUM_POINT_NBL, FRUSTUM_POINT_FBL);
	SetFrustumPlane(FRUSTUM_PLANE_RGT, FRUSTUM_POINT_NBR, FRUSTUM_POINT_NTR, FRUSTUM_POINT_FBR);
	SetFrustumPlane(FRUSTUM_PLANE_BOT, FRUSTUM_POINT_NBL, FRUSTUM_POINT_NBR, FRUSTUM_POINT_FBR);
	SetFrustumPlane(FRUSTUM_PLANE_TOP, FRUSTUM_POINT_NTR, FRUSTUM_POINT_NTL, FRUSTUM_POINT_FTL);
	SetFrustumPlane(FRUSTUM_PLANE_NEA, FRUSTUM_POINT_NTL, FRUSTUM_POINT_NTR, FRUSTUM_POINT_NBR);
	SetFrustumPlane(FRUSTUM_PLANE_FAR, FRUSTUM_POINT_FTR, FRUSTUM_POINT_FTL, FRUSTUM_POINT_FBL);

	frustum.edges[FRUSTUM_EDGE_NTR_NTL] = (frustum.verts[FRUSTUM_POINT_NTR] - frustum.verts[FRUSTUM_POINT_NTL]).UnsafeANormalize(); // ntr - ntl (same as ftr - ftl)
	frustum.edges[FRUSTUM_EDGE_NTL_NBL] = (frustum.verts[FRUSTUM_POINT_NTL] - frustum.verts[FRUSTUM_POINT_NBL]).UnsafeANormalize(); // ntl - nbl (same as ftl - fbl)
	frustum.edges[FRUSTUM_EDGE_FTL_NTL] = (frustum.verts[FRUSTUM_POINT_FTL] - frustum.verts[FRUSTUM_POINT_NTL]).UnsafeANormalize(); // ftl - ntl
	frustum.edges[FRUSTUM_EDGE_FTR_NTR] = (frustum.verts[FRUSTUM_POINT_FTR] - frustum.verts[FRUSTUM_POINT_NTR]).UnsafeANormalize(); // ftr - ntr
	frustum.edges[FRUSTUM_EDGE_FBR_NBR] = (frustum.verts[FRUSTUM_POINT_FBR] - frustum.verts[FRUSTUM_POINT_NBR]).UnsafeANormalize(); // fbr - nbr
	frustum.edges[FRUSTUM_EDGE_FBL_NBL] = (frustum.verts[FRUSTUM_POINT_FBL] - frustum.verts[FRUSTUM_POINT_NBL]).UnsafeANormalize(); // fbl - nbl

	if (camType == CAMTYPE_VISCUL)
		return;

	// vis-culling is always performed from player's (or light's)
	// POV but also happens during e.g. cubemap generation; copy
	// over the frustum planes we just calculated above such that
	// GetFrustumSides can be called by all parties interested in
	// VC
	//
	// note that this is the only place where VISCUL is updated!
	CCamera* visCam = CCameraHandler::GetCamera(CAMTYPE_VISCUL);
	CCamera* curCam = CCameraHandler::GetCamera(camType);

	visCam->CopyState(curCam);
}

void CCamera::UpdateMatrices(uint32_t vsx, uint32_t vsy, float var)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// recalculate the projection transform
	switch (projType) {
		case PROJTYPE_PERSP: {
			gluPerspectiveSpring(var, frustum.scales.z, frustum.scales.w);
		} break;
		case PROJTYPE_ORTHO: {
			glOrthoScaledSpring(vsx, vsy, frustum.scales.z, frustum.scales.w);
		} break;
		default: {
			assert(false);
		} break;
	}


	// FIXME:
	//   should be applying the offsets to pos/up/right/forward/etc,
	//   but without affecting the real positions (need an intermediary)
	const float3 fShake = ((forward * (1.0f + tiltOffset.z)) + (right * tiltOffset.x) + (up * tiltOffset.y)).ANormalize();
	const float3 camPos = pos + posOffset;
	const float3 center = camPos + fShake;

	// recalculate the view transform
	viewMatrix = CMatrix44f::LookAtView(camPos, center, up);


	// create extra matrices (useful for shaders)
	viewProjectionMatrix = projectionMatrix * viewMatrix;
	viewMatrixInverse = viewMatrix.InvertAffine();
	projectionMatrixInverse = projectionMatrix.Invert();
	viewProjectionMatrixInverse = viewProjectionMatrix.Invert();

	billboardMatrix = viewMatrix;
	billboardMatrix.SetPos(ZeroVector);
	billboardMatrix.Transpose(); // viewMatrix is affine, equals inverse
}

void CCamera::UpdateViewPort(int px, int py, int sx, int sy)
{
	RECOIL_DETAILED_TRACY_ZONE;
	viewport[0] = px;
	viewport[1] = py;
	viewport[2] = sx;
	viewport[3] = sy;
}

void CCamera::UpdateLoadViewport(int px, int py, int sx, int sy)
{
	RECOIL_DETAILED_TRACY_ZONE;
	UpdateViewPort(px, py, sx, sy);
	LoadViewport();
}


void CCamera::LoadMatrices() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&projectionMatrix.m[0]);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(&viewMatrix.m[0]);
}

void CCamera::LoadViewport() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}


void CCamera::UpdateViewRange()
{
	RECOIL_DETAILED_TRACY_ZONE;
	#if 0
	// horizon-probe direction
	const float3 hpPixelDir = (forward * XZVector + UpVector * -0.01f).Normalize();

	const float3 tlPixelDir = CalcPixelDir(                         0,                          0);
	const float3 trPixelDir = CalcPixelDir(globalRendering->viewSizeX,                          0);
	const float3 brPixelDir = CalcPixelDir(globalRendering->viewSizeX, globalRendering->viewSizeY);
	const float3 blPixelDir = CalcPixelDir(                         0, globalRendering->viewSizeY);
	#endif

	#if 0
	constexpr float SQ_MAX_VIEW_RANGE = Square(CGlobalRendering::MAX_VIEW_RANGE);
	#endif
	constexpr float ZFAR_ZNEAR_FACTOR = 0.001f;

	const float maxEdgeDistX = std::max(pos.x, float3::maxxpos - pos.x);
	const float maxEdgeDistZ = std::max(pos.z, float3::maxzpos - pos.z);
	const float maxEdgeDist  = math::sqrt(Square(maxEdgeDistX) + Square(maxEdgeDistZ));
	const float mapMinHeight = readMap->GetCurrMinHeight();

	float wantedViewRange = 0.0f;

	#if 0
	// only pick horizon probe-dir if between bottom and top planes
	if (hpPixelDir.y >= (blPixelDir.y + brPixelDir.y) * 0.5f && hpPixelDir.y <= (tlPixelDir.y + trPixelDir.y) * 0.5f)
		wantedViewRange = CGround::LinePlaneCol(pos, hpPixelDir, SQ_MAX_VIEW_RANGE, mapMinHeight);
	#endif

	// camera-height dependence (i.e. TAB-view)
	wantedViewRange = std::max(wantedViewRange, (pos.y - std::max(0.0f, mapMinHeight)) * 2.0f);
	// view-angle dependence (i.e. FPS-view)
	// forward normally points down, so 1-min(0, dot(f,u))
	// will be >= 1 and increase the effective maxEdgeDist
	wantedViewRange = std::max(wantedViewRange, (1.0f - std::min(0.0f, forward.dot(UpVector))) * maxEdgeDist);

	#if 0
	wantedViewRange = std::max(wantedViewRange, CGround::LinePlaneCol(pos, tlPixelDir, SQ_MAX_VIEW_RANGE, mapMinHeight));
	wantedViewRange = std::max(wantedViewRange, CGround::LinePlaneCol(pos, trPixelDir, SQ_MAX_VIEW_RANGE, mapMinHeight));
	wantedViewRange = std::max(wantedViewRange, CGround::LinePlaneCol(pos, brPixelDir, SQ_MAX_VIEW_RANGE, mapMinHeight));
	wantedViewRange = std::max(wantedViewRange, CGround::LinePlaneCol(pos, blPixelDir, SQ_MAX_VIEW_RANGE, mapMinHeight));
	wantedViewRange = std::clamp(wantedViewRange, CGlobalRendering::MIN_ZNEAR_DIST, CGlobalRendering::MAX_VIEW_RANGE);
	#endif

	frustum.scales.z = std::max(wantedViewRange * ZFAR_ZNEAR_FACTOR, globalRendering->minViewRange);
	frustum.scales.w = std::min(wantedViewRange                    , globalRendering->maxViewRange);
}

bool CCamera::InView(const float3& point, float radius) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return frustum.IntersectSphere(point, radius, inViewPlanesMask);
}

bool CCamera::InView(const AABB& aabb) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return InView(aabb.CalcCenter(), aabb.CalcRadius()) && frustum.IntersectAABB(aabb, inViewPlanesMask);
}

#if 0
// axis-aligned bounding box test (AABB)
bool CCamera::InView(const AABB& aabb) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// orthographic plane offsets along each respective normal; [0] = LFT&RGT, [1] = TOP&BOT
	const float xyPlaneOffsets[2] = {frustum.scales.x, frustum.scales.y};
	const float zwPlaneOffsets[2] = {frustum.scales.z, frustum.scales.w};


	// [i*2+0] := point, [i*2+1] := normal
	const float3 boxFaces[6 * 2] = {
		boxCenter + FwdVector * boxScales.z,  FwdVector, // front
		boxCenter - FwdVector * boxScales.z, -FwdVector, // back
		boxCenter + RgtVector * boxScales.x,  RgtVector, // right
		boxCenter - RgtVector * boxScales.x, -RgtVector, // left
		boxCenter +  UpVector * boxScales.y,   UpVector, // top
		boxCenter -  UpVector * boxScales.y,  -UpVector, // bottom
	};
	const float3 boxVerts[8] = {
		// bottom
		{mins.x,  mins.y,  mins.z},
		{maxs.x,  mins.y,  mins.z},
		{maxs.x,  mins.y,  maxs.z},
		{mins.x,  mins.y,  maxs.z},
		// top
		{mins.x,  maxs.y,  mins.z},
		{maxs.x,  maxs.y,  mins.z},
		{maxs.x,  maxs.y,  maxs.z},
		{mins.x,  maxs.y,  maxs.z},
	};

	{
		// test box planes
		for (uint32_t i = 0; i < 6; i++) {
			uint32_t n = 0;

			for (uint32_t j = 0; j < 8; j++) {
				n += (boxFaces[i * 2 + 1].dot(frustum.verts[j] - boxFaces[i * 2 + 0]) > 0.0f);
			}

			if (n == 8)
				return false;
		}
	}
	{
		// test cam planes (LRTB)
		for (uint32_t i = FRUSTUM_PLANE_LFT; i < FRUSTUM_PLANE_FAR; i++) {
			uint32_t n = 0;

			for (uint32_t j = 0; j < 8; j++) {
				n += (frustum.planes[i].dot(boxVerts[j] - pos) > xyPlaneOffsets[i >> 1]);
			}

			// fully in front of this plane, so outside frustum (normals point outward)
			if (n == 8)
				return false;
		}
	}
	{
		// test cam planes (NF)
		for (uint32_t i = FRUSTUM_PLANE_FAR; i < FRUSTUM_PLANE_CNT; i++) {
			uint32_t n = 0;

			for (uint32_t j = 0; j < 8; j++) {
				n += (frustum.planes[i].dot(boxVerts[j] - (pos + forward * zwPlaneOffsets[i & 1])) > 0.0f);
			}

			if (n == 8)
				return false;
		}
	}
	{
		for (uint32_t i = 0; i < 6; i++) {
			for (uint32_t j = 0; j < 6; j++) {
				if (boxFaces[i * 2 + 1] == frustum.planes[j])
					continue;

				float3 testAxis  = boxFaces[i * 2 + 1].cross(frustum.planes[j]);
				float3 testAxisN = testAxis.Normalize();

				float2   boxAxisDists = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()}; // .x=min,.y=max
				float2 frustAxisDists = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()}; // .x=min,.y=max
				float4  projAxisDists;

				for (uint32_t k = 0; k < 8; k++) {
					boxAxisDists.x = std::min(boxAxisDists.x, boxVerts[k].dot(testAxisN));
					boxAxisDists.y = std::max(boxAxisDists.y, boxVerts[k].dot(testAxisN));

					frustAxisDists.x = std::min(frustAxisDists.x, frustum.verts[k].dot(testAxisN));
					frustAxisDists.y = std::max(frustAxisDists.y, frustum.verts[k].dot(testAxisN));
				}

				projAxisDists.x = std::min(boxAxisDists.x, frustAxisDists.x); // min(minDists)
				projAxisDists.y = std::min(boxAxisDists.y, frustAxisDists.y); // min(maxDists)
				projAxisDists.z = std::max(boxAxisDists.x, frustAxisDists.x); // max(minDists)
				projAxisDists.w = std::max(boxAxisDists.y, frustAxisDists.y); // max(maxDists)

				if ((projAxisDists.y >= projAxisDists.z) && (projAxisDists.x <= projAxisDists.w))
					continue;

				return false;
			}
		}
	}

	return true;
}
#endif



void CCamera::SetVFOV(const float angle)
{
	RECOIL_DETAILED_TRACY_ZONE;
	fov = angle;
	halfFov = (fov * 0.5f) * math::DEG_TO_RAD;
	tanHalfFov = math::tan(halfFov);
}

float CCamera::GetHFOV() const {
	RECOIL_DETAILED_TRACY_ZONE;
	return (2.0f * math::atan(tanHalfFov * aspectRatio) * math::RAD_TO_DEG);
}
#if 0
float CCamera::CalcTanHalfHFOV() const {
	RECOIL_DETAILED_TRACY_ZONE;
	const float half_h_fov_deg = math::atan(thvfov * h_aspect_ratio) * math::RAD_TO_DEG;
	const float half_h_fov_rad = half_h_fov_deg * math::DEG_TO_RAD;
	return (math::tan(half_h_fov_rad));
}
#endif



float3 CCamera::GetRotFromDir(float3 fwd)
{
	RECOIL_DETAILED_TRACY_ZONE;
	fwd.Normalize();

	// NOTE:
	//   atan2(0.0,  0.0) returns 0.0
	//   atan2(0.0, -0.0) returns PI
	//   azimuth (yaw) 0 is on negative z-axis
	//   roll-angle (rot.z) is always 0 by default
	float3 r;
	r.x = math::acos(fwd.y);
	r.y = math::atan2(fwd.x, -fwd.z);
	r.z = 0.0f;
	return r;
}

float3 CCamera::GetFwdFromRot(const float3& r)
{
	RECOIL_DETAILED_TRACY_ZONE;
	float3 fwd;
	fwd.x = std::sin(r.x) *   std::sin(r.y);
	fwd.z = std::sin(r.x) * (-std::cos(r.y));
	fwd.y = std::cos(r.x);
	return fwd;
}

float3 CCamera::GetRgtFromRot(const float3& r)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// FIXME:
	//   right should always be "right" relative to forward
	//   (i.e. up should always point "up" in WS and camera
	//   can not flip upside down) but is not
	//
	//   fwd=(0,+1,0) -> rot=GetRotFromDir(fwd)=(0.0, PI, 0.0) -> GetRgtFromRot(rot)=(-1.0, 0.0, 0.0)
	//   fwd=(0,-1,0) -> rot=GetRotFromDir(fwd)=( PI, PI, 0.0) -> GetRgtFromRot(rot)=(+1.0, 0.0, 0.0)
	//
	float3 rgt;
	rgt.x = std::sin(math::HALFPI - r.z) *   std::sin(r.y + math::HALFPI);
	rgt.z = std::sin(math::HALFPI - r.z) * (-std::cos(r.y + math::HALFPI));
	rgt.y = std::cos(math::HALFPI - r.z);
	return rgt;
}


void CCamera::UpdateDirsFromRot(const float3& r)
{
	RECOIL_DETAILED_TRACY_ZONE;
	forward  = GetFwdFromRot(r);
	right    = GetRgtFromRot(r);
	up       = (right.cross(forward)).Normalize();
}

void CCamera::SetDir(const float3& dir)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// if (dir == forward) return;
	// update our axis-system from the angles
	SetRot(GetRotFromDir(dir) + (FwdVector * rot.z));
	assert(dir.dot(forward) > 0.9f);
}



float3 CCamera::CalcPixelDir(int x, int y) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int vsx = std::max(1, globalRendering->viewSizeX);
	const int vsy = std::max(1, globalRendering->viewSizeY);

	const float dx = float(x - globalRendering->viewPosX - (vsx >> 1)) / vsy * (tanHalfFov * 2.0f);
	const float dy = float(y -                             (vsy >> 1)) / vsy * (tanHalfFov * 2.0f);

	return ((forward - up * dy + right * dx).Normalize());
}


float3 CCamera::CalcViewPortCoordinates(const float3& objPos) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// same as gluProject()
	const float4 projPos = viewProjectionMatrix * float4(objPos, 1.0f);
	const float3 clipPos = float3(projPos) / projPos.w;

	float3 vpPos;
	vpPos.x = viewport[2] * (clipPos.x + 1.0f) * 0.5f;
	vpPos.y = viewport[3] * (clipPos.y + 1.0f) * 0.5f;
	vpPos.z =               (clipPos.z + 1.0f) * 0.5f;
	return vpPos;
}




inline void CCamera::gluPerspectiveSpring(float aspect, float zn, float zf) {
	const float t = zn * tanHalfFov;
	const float b = -t;
	const float l = b * aspect;
	const float r = t * aspect;

	projectionMatrix = clipControlMatrix * CMatrix44f::PerspProj(l, r,  b, t,  zn, zf);
}

// same as glOrtho(-1, 1, -1, 1, zn, zf) plus glScalef(sx, sy, 1)
inline void CCamera::glOrthoScaledSpring(
	const float sx,
	const float sy,
	const float zn,
	const float zf
) {
	const float l = -1.0f * sx;
	const float r =  1.0f * sx;
	const float b = -1.0f * sy;
	const float t =  1.0f * sy;

	projectionMatrix = clipControlMatrix * CMatrix44f::OrthoProj(l, r,  b, t,  zn, zf);
}

void CCamera::CalcFrustumLines(float miny, float maxy, float scale, bool neg) {
	const float3 isectParams = {miny, maxy, 1.0f / scale};

	// reset counts per side
	frustumLines[FRUSTUM_SIDE_POS][4].sign = 0;
	frustumLines[FRUSTUM_SIDE_NEG][4].sign = 0;

	// Note: order does not matter
	for (uint32_t i = FRUSTUM_PLANE_LFT; i < FRUSTUM_PLANE_NEA; i++) {
		CalcFrustumLine(frustum.planes[i], isectParams, neg ? FRUSTUM_SIDE_NEG : FRUSTUM_SIDE_POS);
	}

	assert(!neg || frustumLines[FRUSTUM_SIDE_NEG][4].sign == 4);
}

void CCamera::CalcFrustumLine(
	const float4& face,
	const float3& params,
	uint32_t side
) {

	std::pair<float3, float3> iLine;
	const float4 xzPlane = { 0, 1, 0, -params[face.y > 0.0f] };

	if (!IntersectPlanes(xzPlane, face, iLine))
		return;

	float3& xdir = iLine.first;
	const float3& pInt = iLine.second;

	// prevent DIV0 when calculating line.dir
	xdir.z *= (std::fabs(xdir.z) > 0.0001f);
	xdir.z = std::max(std::fabs(xdir.z), 0.0001f) * std::copysign(1.0f, xdir.z);

	// <line.dir> is the direction coefficient (0 ==> parallel to z-axis, inf ==> parallel to x-axis)
	// in the xz-plane; <line.base> is the x-coordinate at which line intersects x-axis; <line.sign>
	// indicates line direction, ie. left-to-right (whenever <xdir.z> is negative) or right-to-left
	// NOTE:
	//   (b.x / b.z) is actually the reciprocal of the DC (ie. the number of steps along +x for
	//   one step along +y); the world z-axis is inverted wrt. a regular Carthesian grid, so the
	//   DC is also inverted
	FrustumLine line;

	line.sign = Sign(int(xdir.z <= 0.0f));
	line.dir  = (xdir.x / xdir.z);
	line.base = (pInt.x - pInt.z * line.dir) * params.z;

	line.minz = (                      0.0f) - (mapDims.mapy);
	line.maxz = (mapDims.mapy * SQUARE_SIZE) + (mapDims.mapy);

	int  index = (line.sign == 1 || side == FRUSTUM_SIDE_NEG);
	int& count = frustumLines[index][4].sign;

	// store all lines in [NEG] (regardless of actual sign) if wanted by caller
	frustumLines[index][count++] = line;
}

void CCamera::ClipFrustumLines(const float zmin, const float zmax, bool neg)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto& lines = frustumLines[neg];

	for (int i = 0, cnt = lines[4].sign; i < cnt; i++) {
		for (int j = 0; j < cnt; j++) {
			if (i == j)
				continue;

			FrustumLine& fli = lines[i];
			FrustumLine& flj = lines[j];

			const float dbase = fli.base - flj.base;
			const float ddir = fli.dir - flj.dir;

			if (ddir == 0.0f)
				continue;

			const float colz = -(dbase / ddir);

			if ((flj.sign * ddir) > 0.0f) {
				if ((colz > fli.minz) && (colz < zmax))
					fli.minz = colz;
			} else {
				if ((colz < fli.maxz) && (colz > zmin))
					fli.maxz = colz;
			}
		}
	}
}


float3 CCamera::GetMoveVectorFromState(bool fromKeyState) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	float camDeltaTime = globalRendering->lastFrameTime;

	if (useInterpolate > 0)
		camDeltaTime = 1000.0f / std::fmax(globalRendering->FPS, 1.0f);
	
	float camMoveSpeed = 1.0f;

	camMoveSpeed *= movState[MOVE_STATE_SLW] ? moveSlowMult : 1.0f;
	camMoveSpeed *= movState[MOVE_STATE_FST] ? moveFastMult : 1.0f;

	float3 v = FwdVector * camMoveSpeed;

	if (fromKeyState) {
		v.y += (camDeltaTime * 0.001f * movState[MOVE_STATE_FWD]);
		v.y -= (camDeltaTime * 0.001f * movState[MOVE_STATE_BCK]);
		v.x += (camDeltaTime * 0.001f * movState[MOVE_STATE_RGT]);
		v.x -= (camDeltaTime * 0.001f * movState[MOVE_STATE_LFT]);

		return v;
	}

	const int windowW = globalRendering->winSizeX;
	int mouseY = mouse->lasty;
	int viewH;

	if (globalRendering->dualScreenMode && (mouse->lastx >= globalRendering->dualViewPosX) && (mouse->lastx <= globalRendering->dualViewPosX + globalRendering->dualViewSizeX)) {
		viewH = globalRendering->dualViewSizeY;
		// Translate mouseY so it maps mousecoords to bottom of dual view to 0 and top of dualview to dualViewSize
		mouseY -= globalRendering->dualWindowOffsetY - globalRendering->viewWindowOffsetY;
	} else {
		viewH = globalRendering->viewSizeY;
	}

	int2 border;
	border.x = std::max<int>(1, windowW * edgeMoveWidth);
	border.y = std::max<int>(1, viewH * edgeMoveWidth);

	float2 move;
	// must be float, ints don't save the sign in case of 0 and we need it for copysign()
	float2 distToEdge = {std::clamp(mouse->lastx, 0, windowW) * 1.0f, std::clamp(mouseY, 0, viewH) * 1.0f};

	if (((windowW - 1) - distToEdge.x) < distToEdge.x) distToEdge.x = -((windowW - 1) - distToEdge.x);
	if (((viewH - 1) - distToEdge.y) < distToEdge.y) distToEdge.y = -((viewH - 1) - distToEdge.y);

	if (edgeMoveDynamic) {
		move.x = std::clamp(float(border.x - std::abs(distToEdge.x)) / border.x, 0.0f, 1.0f);
		move.y = std::clamp(float(border.y - std::abs(distToEdge.y)) / border.y, 0.0f, 1.0f);
	} else {
		move.x = int(std::abs(distToEdge.x) < border.x);
		move.y = int(std::abs(distToEdge.y) < border.y);
	}

	move.x = std::copysign(move.x, -distToEdge.x);
	move.y = std::copysign(move.y,  distToEdge.y);

	v.x = (camDeltaTime * 0.001f * move.x);
	v.y = (camDeltaTime * 0.001f * move.y);

	return v;
}

// http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-points-and-spheres/
bool CCamera::Frustum::IntersectSphere(float3 p, float radius, uint8_t testMask) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (size_t i = 0; i < FRUSTUM_PLANE_CNT; ++i) {
		if ((testMask & (1 << i)) == 0)
			continue;

		const auto& plane = planes[i];
		const float dist = plane.dot(p) + plane.w;
		if (dist < -radius)
			return false; // outside
		/*
		else if (dist < radius)
			return true;  // intersect
		*/
	}

	return true; // inside or intersect
}

/*
bool CCamera::Frustum::IntersectAABB(const AABB& b) const
{
	return true;

	// edge axes and normals are identical for AABBs
	constexpr float3 aabbPlanes[3] = {
		RgtVector,
		UpVector,
		FwdVector
	};

	float3 aabbVerts[8];
	float3 crossAxes[3 * 6];

	b.CalcCorners(aabbVerts);

	const auto IsSepAxis = [](const float3& axis, const float3* frustVerts, const float3* aabbVerts) {
		float2 frustProjRange = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
		float2  aabbProjRange = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};

		float frustProjDists[8];
		float  aabbProjDists[8];

		for (int i = 0; i < 8; i++) {
			frustProjDists[i] = axis.dot(frustVerts[i]);

			frustProjRange.x = std::min(frustProjRange.x, frustProjDists[i]);
			frustProjRange.y = std::max(frustProjRange.y, frustProjDists[i]);

			aabbProjDists[i] = axis.dot(aabbVerts[i]);

			aabbProjRange.x = std::min(aabbProjRange.x, aabbProjDists[i]);
			aabbProjRange.y = std::max(aabbProjRange.y, aabbProjDists[i]);
		}

		return (!AABB::RangeOverlap(frustProjRange, aabbProjRange));
	};
	const auto AxisTestPred = [&](const float3& testAxis) {
		return (IsSepAxis(testAxis, &verts[0], aabbVerts));
	};

	if (std::find_if(&aabbPlanes[0], &aabbPlanes[0] + 3, AxisTestPred) != (&aabbPlanes[0] + 3))
		return false;
	if (std::find_if(&planes[0], &planes[0] + 6, AxisTestPred) != (&planes[0] + 6))
		return false;

	for (uint32_t i = 0; i < 3; i++) {
		for (uint32_t j = 0; j < 6; j++) {
			crossAxes[i * 6 + j] = (aabbPlanes[i].cross(edges[j])).SafeNormalize();
		}
	}

	return (std::find_if(&crossAxes[0], &crossAxes[0] + 3 * 6, AxisTestPred) == (&crossAxes[0] + 3 * 6));
}
*/

// http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-boxes-ii/
bool CCamera::Frustum::IntersectAABB(const AABB& b, uint8_t testMask) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (size_t i = 0; i < FRUSTUM_PLANE_CNT; ++i) {
		if ((testMask & (1 << i)) == 0)
			continue;

		const auto& plane = planes[i];
		if (plane.dot(b.GetVertexP(plane)) + plane.w < 0)
			return false; // outside
		/*
		else if (plane.dot(b.GetVertexN(plane)) + plane.w < 0)
			return true;  // intersects
		*/
	}
	return true; // inside or intersect
}
