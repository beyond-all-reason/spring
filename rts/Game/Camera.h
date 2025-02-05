/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>

#include "System/AABB.hpp"
#include "System/float3.h"
#include "System/float4.h"
#include "System/Matrix44f.h"


class CCamera {
public:
	enum {
		CAMTYPE_PLAYER = 0, // main camera
		CAMTYPE_UWREFL = 1, // used for underwater reflections
		CAMTYPE_SHADOW = 2, // used for shadowmap generation
		CAMTYPE_ENVMAP = 3, // used for cubemap generation
		CAMTYPE_VISCUL = 4, // used for frustum culling
		CAMTYPE_ACTIVE = 5, // pointer to currently active camera
		CAMTYPE_COUNT  = 6,
	};

	enum {
		PROJTYPE_PERSP = 0,
		PROJTYPE_ORTHO = 1,
		PROJTYPE_COUNT = 2,
	};

	enum {
		FRUSTUM_PLANE_LFT = 0,
		FRUSTUM_PLANE_RGT = 1,
		FRUSTUM_PLANE_BOT = 2,
		FRUSTUM_PLANE_TOP = 3,
		FRUSTUM_PLANE_NEA = 4,
		FRUSTUM_PLANE_FAR = 5,
		FRUSTUM_PLANE_CNT = 6,
	};
	enum {
		FRUSTUM_POINT_NBL = 0,
		FRUSTUM_POINT_NBR = 1,
		FRUSTUM_POINT_NTR = 2,
		FRUSTUM_POINT_NTL = 3,
		FRUSTUM_POINT_FBL = 4,
		FRUSTUM_POINT_FBR = 5,
		FRUSTUM_POINT_FTR = 6,
		FRUSTUM_POINT_FTL = 7,
		FRUSTUM_POINT_CNT = 8,
	};
	enum {
		FRUSTUM_EDGE_NTR_NTL = 0,
		FRUSTUM_EDGE_NTL_NBL = 1,
		FRUSTUM_EDGE_FTL_NTL = 2,
		FRUSTUM_EDGE_FTR_NTR = 3,
		FRUSTUM_EDGE_FBR_NBR = 4,
		FRUSTUM_EDGE_FBL_NBL = 5,
		FRUSTUM_EDGE_CNT     = 6,
	};
	enum {
		FRUSTUM_SIDE_POS = 0,
		FRUSTUM_SIDE_NEG = 1,
	};

	enum {
		MOVE_STATE_FWD = 0 ,  // forward
		MOVE_STATE_BCK = 1 ,  // back
		MOVE_STATE_LFT = 2 ,  // left
		MOVE_STATE_RGT = 3 ,  // right
		MOVE_STATE_UP  = 4 ,  // up
		MOVE_STATE_DWN = 5 ,  // down
		MOVE_STATE_FST = 6 ,  // fast
		MOVE_STATE_SLW = 7 ,  // slow
		MOVE_STATE_TLT = 8 ,  // tilt
		MOVE_STATE_RST = 9 ,  // reset
		MOVE_STATE_RTT = 10,  // rotate
	};

	enum IntersectionTestResult {
		OUTSIDE   = 0,
		INSIDE    = 1,
		INTERSECT = 2,
	};

	struct Frustum {
	public:
		IntersectionTestResult IntersectSphere(float3 p, float radius) const;
		IntersectionTestResult IntersectAABB(const AABB& b) const;

	public:
		// corners
		std::array<float3, FRUSTUM_POINT_CNT> verts;
		// plane equations
		std::array<float4, FRUSTUM_PLANE_CNT> planes;
		// ntr - ntl, ntl - nbl, ftl - ntl, ftr - ntr, fbr - nbr, fbl - nbl
		std::array<float3, FRUSTUM_EDGE_CNT> edges;

		// xy-scales (for orthographic cameras only), .z := znear, .w := zfar
		float4 scales;
	};
	struct FrustumLine {
		int sign = 0;

		float base = 0.0f;
		float dir = 0.0f;

		float minz = 0.0f;
		float maxz = 0.0f;
	};

	struct UpdateParams {
		bool updateDirs;
		bool updateMats;
		bool updateFrustum;
		bool updateViewPort;
		bool updateViewRange;
	};

public:
	CCamera(uint32_t cameraType = CAMTYPE_PLAYER, uint32_t projectionType = PROJTYPE_PERSP);

	void CopyState(const CCamera*);
	void CopyStateReflect(const CCamera*);

	void Update(const UpdateParams& p);
	void Update(bool updateDirs = true, bool updateMats = true, bool updateViewPort = true, bool updateViewRange = true) {
		Update({updateDirs, updateMats, true, updateViewPort, updateViewRange});
	}

	/// @param fov in degree
	void SetPos(const float3& p) { pos = p; }
	void SetDir(const float3& dir);

	const float3& GetPos() const { return pos; }
	const float3& GetDir() const { return forward; }

	const float3& GetForward() const { return forward; }
	const float3& GetRight() const   { return right; }
	const float3& GetUp() const      { return up; }
	const float3& GetRot() const     { return rot; }

	void SetRot(const float3& r) { UpdateDirsFromRot(rot = r); }
	void SetRotX(const float x) { SetRot(float3(    x, rot.y, rot.z)); }
	void SetRotY(const float y) { SetRot(float3(rot.x,     y, rot.z)); }
	void SetRotZ(const float z) { SetRot(float3(rot.x, rot.y,     z)); }

	float3 CalcPixelDir(int x, int y) const;
	float3 CalcViewPortCoordinates(const float3& objPos) const;

	IntersectionTestResult InView(const float3& point, float radius = 0.0f) const;
	IntersectionTestResult InView(const float3& mins, const float3& maxs) const { return InView(AABB{mins, maxs}); }
	IntersectionTestResult InView(const AABB& aabb) const;

	void CalcFrustumLines(float miny, float maxy, float scale, bool neg = false);
	void CalcFrustumLine(
		const float4& face,
		const float3& params,
		uint32_t side
	);

	void ClipFrustumLines(const float zmin, const float zmax, bool neg);
	void SetFrustumScales(const float4 scales) { frustum.scales = scales; }

	const FrustumLine* GetPosFrustumLines() const { return &frustumLines[FRUSTUM_SIDE_POS][0]; }
	const FrustumLine* GetNegFrustumLines() const { return &frustumLines[FRUSTUM_SIDE_NEG][0]; }

	void SetClipCtrlMatrix(const CMatrix44f& mat) { clipControlMatrix = mat; }
	void SetProjMatrix(const CMatrix44f& mat) { projectionMatrix = mat; }
	void SetViewMatrix(const CMatrix44f& mat);

	const CMatrix44f& GetViewMatrix() const { return viewMatrix; }
	const CMatrix44f& GetViewMatrixInverse() const { return viewMatrixInverse; }
	const CMatrix44f& GetProjectionMatrix() const { return projectionMatrix; }
	const CMatrix44f& GetProjectionMatrixInverse() const { return projectionMatrixInverse; }
	const CMatrix44f& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const CMatrix44f& GetViewProjectionMatrixInverse() const { return viewProjectionMatrixInverse; }
	const CMatrix44f& GetBillBoardMatrix() const { return billboardMatrix; }
	const CMatrix44f& GetClipControlMatrix() const { return clipControlMatrix; }

	const Frustum& GetFrustum() const { return frustum; }
	const float3& GetFrustumVert (uint32_t i) const { return frustum.verts [i]; }
	const float4& GetFrustumPlane(uint32_t i) const { return frustum.planes[i]; }
	const float3& GetFrustumEdge (uint32_t i) const { return frustum.edges [i]; }
	const float4& GetFrustumScales() const { return frustum.scales; }

	void LoadMatrices() const;
	void LoadViewport() const;
	void UpdateLoadViewport(int px, int py, int sx, int sy);

	void SetVFOV(float angle);
	void SetAspectRatio(float ar) { aspectRatio = ar; }

	float GetVFOV() const { return fov; }
	float GetHFOV() const;
	float GetHalfFov() const { return halfFov; }
	float GetTanHalfFov() const { return tanHalfFov; }
	float GetLPPScale() const { return lppScale; }
	float GetNearPlaneDist() const { return frustum.scales.z; }
	float GetFarPlaneDist() const { return frustum.scales.w; }
	float GetAspectRatio() const { return aspectRatio; }

	float3 GetMoveVectorFromState(bool fromKeyState) const;

	void SetMovState(int idx, bool b) { movState[idx] = b; }
	void SetRotState(int idx, bool b) { rotState[idx] = b; }
	const bool* GetMovState() const { return movState; }
	const bool* GetRotState() const { return rotState; }

	static CCamera* GetActive();

	static float3 GetRotFromDir(float3 fwd);
	static float3 GetFwdFromRot(const float3& r);
	static float3 GetRgtFromRot(const float3& r);


	float ProjectedDistance(const float3& objPos) const {
		return (forward.dot(objPos - pos));
	}

	/*
	float ProjectedDistanceShadow(const float3& objPos, const float3& sunDir) const {
		// FIXME: fix it, cap it for shallow shadows?
		const float3 diff = pos - objPos;
		const float  dot  = diff.dot(sunDir);
		const float3 gap  = diff - (sunDir * dot);
		return (gap.Length());
	}
	*/

	uint32_t GetCamType() const { return camType; }
	uint32_t GetProjType() const { return projType; }
	void SetCamType(uint32_t ct);
	void SetProjType(uint32_t pt) { projType = pt; }
	void InitConfigNotify();
	void RemoveConfigNotify();

public:
	void UpdateViewRange();
	void UpdateFrustum();
	void UpdateMatrices(uint32_t vsx, uint32_t vsy, float var);
	void UpdateDerivedMatrices();
	void UpdateViewPort(int px, int py, int sx, int sy);

	void ConfigNotify(const std::string& key, const std::string& value);
	void ConfigUpdate();

private:
	void gluPerspectiveSpring(const float aspect, const float zn, const float zf);
	void glOrthoScaledSpring(const float sx, const float sy, const float zn, const float zf);

	void UpdateDirsFromRot(const float3& r);

public:
	float3 pos;
	float3 rot;                   ///< x = inclination, y = azimuth (to the -z axis!), z = roll
	float3 forward = FwdVector;   ///< local z-axis
	float3 right   = RgtVector;   ///< local x-axis
	float3 up      =  UpVector;   ///< local y-axis

	// Lua-controlled parameters, player-camera only
	float3 posOffset;
	float3 tiltOffset;

	float fov         = 0.0f;  ///< vertical viewing angle, in degrees
	float halfFov     = 0.0f;  ///< half the fov in radians
	float tanHalfFov  = 0.0f;  ///< math::tan(halfFov)
	float lppScale    = 0.0f;  ///< length-per-pixel scale
	float aspectRatio = 1.0f;  ///< horizontal

	int viewport[4];

	/**
	* @brief moveFastMult
	* The multiplier applied to speed when camera is in movefast state.
	*/
	float moveFastMult;

	/**
	* @brief moveSlowMult
	* The multiplier applied to speed when camera is in moveslow state.
	*/
	float moveSlowMult;

	int edgeMoveWidth;
	int useInterpolate;
	bool edgeMoveDynamic;

private:
	CMatrix44f projectionMatrix;
	CMatrix44f projectionMatrixInverse;
	CMatrix44f viewMatrix;
	CMatrix44f viewMatrixInverse;
	CMatrix44f viewProjectionMatrix;
	CMatrix44f viewProjectionMatrixInverse;
	CMatrix44f billboardMatrix;
	CMatrix44f clipControlMatrix;

	Frustum frustum;
	// positive (right-to-left) lines [0], negative (left-to-right) lines [1]
	FrustumLine frustumLines[2][4 + 1];

	// CAMTYPE_*
	uint32_t camType = -1u;
	// PROJTYPE_*
	uint32_t projType = -1u;

	bool movState[10]; // fwd, back, left, right, up, down, fast, slow, tilt, reset
	bool rotState[4]; // unused
};

#define camera (CCamera::GetActive())
