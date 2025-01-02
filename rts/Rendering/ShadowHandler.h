/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHADOW_HANDLER_H
#define SHADOW_HANDLER_H

#include <array>
#include <limits>

#include "Rendering/GL/FBO.h"
#include "System/float4.h"
#include "System/Matrix44f.h"

namespace Shader {
	struct IProgramObject;
}

class CCamera;
class CShadowHandler
{
public:
	void Init();
	void Kill();
	void Reload(const char* argv);
	void Update();

	void SetupShadowTexSampler(unsigned int texUnit, bool enable = false) const;
	void SetupShadowTexSamplerRaw() const;
	void ResetShadowTexSampler(unsigned int texUnit, bool disable = false) const;
	void ResetShadowTexSamplerRaw() const;
	void CreateShadows();

	void EnableColorOutput(bool enable) const;

	enum ShadowGenerationBits {
		SHADOWGEN_BIT_NONE  = 0,
		SHADOWGEN_BIT_MAP   = 2,
		SHADOWGEN_BIT_MODEL = 4,
		SHADOWGEN_BIT_PROJ  = 8,
		SHADOWGEN_BIT_TREE  = 16,
	};
	enum ShadowProjectionMode {
		SHADOWPROMODE_MAP_CENTER = 0, // use center of map-geometry as projection target (constant res.)
		SHADOWPROMODE_CAM_CENTER = 1, // use center of camera-frustum as projection target (variable res.)
		SHADOWPROMODE_MIX_CAMMAP = 2, // use whichever mode maximizes resolution this frame
	};
	enum ShadowMapSizes {
		MIN_SHADOWMAP_SIZE =   512,
		DEF_SHADOWMAP_SIZE =  2048,
		MAX_SHADOWMAP_SIZE = 16384,
	};
	enum ShadowMapCascades {
		MIN_SHADOWMAP_CASCADES = 1,
		DEF_SHADOWMAP_CASCADES = 1,
		MAX_SHADOWMAP_CASCADES = 8,
	};

	enum ShadowGenProgram {
		SHADOWGEN_PROGRAM_MODEL      = 0,
		SHADOWGEN_PROGRAM_MODEL_GL4  = 1,
		SHADOWGEN_PROGRAM_MAP        = 2,
		SHADOWGEN_PROGRAM_PROJECTILE = 3,
		SHADOWGEN_PROGRAM_COUNT      = 4,
	};

	Shader::IProgramObject* GetShadowGenProg(ShadowGenProgram p) {
		return shadowGenProgs[p];
	}

	const CMatrix44f& GetShadowViewMatrix(size_t cascade = 0) const { return  viewMatrices[cascade]; }
	const CMatrix44f& GetShadowProjMatrix(size_t cascade = 0) const { return  projMatrices[cascade]; }

	const float4& GetShadowParams() const { return shadowTexProjCenter; }

	uint32_t GetShadowTextureID(size_t cascade = 0) const { return shadowDepthTextures[cascade]; }
	uint32_t GetColorTextureID(size_t cascade = 0) const { return shadowColorTextures[cascade]; }

	uint32_t GetTextureType() const;

	static bool ShadowsInitialized() { return firstInit; }
	static bool ShadowsSupported() { return shadowsSupported; }

	bool ShadowsLoaded() const { return shadowsLoaded; }
	bool InShadowPass() const { return inShadowPass; }

	void SaveShadowMapTextures() const;
	void DrawFrustumDebug() const;

	bool& DebugFrustumRef() { return debugFrustum; }
private:
	void FreeFBOAndTextures();
	bool InitFBOAndTextures();

	void DrawShadowPasses();
	void LoadProjectionMatrix(const CCamera* shadowCam);
	void LoadShadowGenShaders();

	void SetShadowMatrix(CCamera* playerCam, CCamera* shadowCam);
	void SetShadowCamera(CCamera* shadowCam);

	float4 GetShadowProjectionScales(CCamera*, const CMatrix44f&);
	float3 CalcShadowProjectionPos(CCamera*, float3*);

	float GetOrthoProjectedFrustumRadius(CCamera*, const CMatrix44f&, float3&);

public:
	int shadowConfig;
	int shadowMapSize;
	int shadowMapCascades;
	int shadowGenBits;
private:
	bool shadowsLoaded = false;
	bool inShadowPass = false;
	bool debugFrustum = false;

	inline static bool firstInit = true;
	inline static bool shadowsSupported = false;

	// these project geometry into light-space
	// to write the (FBO) depth-buffer texture
	std::array<Shader::IProgramObject*, SHADOWGEN_PROGRAM_COUNT> shadowGenProgs;

	float3 projMidPos[2 + 1];
	float3 sunProjDir;

	float4 shadowProjScales;

	// culling and drawing versions of both matrices
	CMatrix44f projMatrices[MAX_SHADOWMAP_CASCADES];
	CMatrix44f viewMatrices[MAX_SHADOWMAP_CASCADES];

	std::array<uint32_t, MAX_SHADOWMAP_CASCADES> shadowDepthTextures;
	std::array<uint32_t, MAX_SHADOWMAP_CASCADES> shadowColorTextures;

	std::array<FBO, MAX_SHADOWMAP_CASCADES> shadowFBOs;

	/// xmid, ymid, p17, p18
	static constexpr float4 shadowTexProjCenter = {
		// .xy are used to bias the SM-space projection; the values
		// of .z and .w are such that (invsqrt(xy + zz) + ww) ~= 1
		0.5f                             , //x
		0.5f                             , //y
		std::numeric_limits<float>::max(), //z
		1.0f                               //w
	};
};

extern CShadowHandler shadowHandler;

#endif /* SHADOW_HANDLER_H */
