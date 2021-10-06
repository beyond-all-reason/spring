/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <array>
#include "Game/Camera.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Rendering/Common/ModelDrawer.h"
#include "Rendering/Features/FeatureDrawerData.h"
#include "System/EventHandler.h"

class CFeature;

class CFeatureDrawer: public CModelDrawerBase<CFeatureDrawerData, CFeatureDrawer>
{
public:
	CFeatureDrawer() {}
	virtual ~CFeatureDrawer() {}
public:
	static void InitStatic();
	//static void KillStatic(bool reload); will use base
	//static void UpdateStatic();
public:
	virtual void DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const = 0;
	virtual void DrawFeatureTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const = 0;

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	virtual void PushIndividualState(const CFeature* feature, bool deferredPass) const = 0;
	virtual void PopIndividualState(const CFeature* feature, bool deferredPass) const = 0;
	virtual void DrawIndividual(const CFeature* feature, bool noLuaCall) const = 0;
	virtual void DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall) const = 0;

	virtual void DrawOpaqueFeaturesShadow(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const = 0;
	virtual void DrawOpaqueFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const = 0;
	virtual void DrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction) const = 0;
	virtual void DrawAlphaFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const = 0;
	virtual void DrawAlphaFeature(CFeature* f, bool ffpMat) const = 0;
	virtual void DrawFarFeatures() const = 0;

	virtual bool SetTeamColor(int team, const float2 alpha = float2(1.0f, 0.0f)) const;
public:
	// modelDrawerData proxies
	void ConfigNotify(const std::string& key, const std::string& value) { modelDrawerData->ConfigNotify(key, value); }
	static const std::vector<CFeature*>& GetUnsortedFeatures() { return modelDrawerData->GetUnsortedObjects(); }
public:
	static void PushModelRenderState(int mdlType) {};
	static void PushModelRenderState(const S3DModel* m) {};
	static void PushModelRenderState(const CSolidObject* o) {};

	static void PopModelRenderState(int mdlType) {};
	static void PopModelRenderState(const S3DModel* m) {};
	static void PopModelRenderState(const CSolidObject* o) {};

	virtual void DrawFeatureModel(const CFeature* feature, bool noLuaCall) const = 0;
protected:
	bool ShouldDrawOpaqueFeature(const CFeature* f, bool drawReflection, bool drawRefraction) const;
};

#define featureDrawer (CFeatureDrawer::selectedModelDrawer)

class CFeatureDrawerCommon : public CFeatureDrawer
{
public:
	void Update() const override;
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const override;
protected:
	template<bool legacy>
	void DrawShadowPassImpl() const;

	template<bool legacy>
	void DrawImpl(bool drawReflection, bool drawRefraction) const;
};

class CFeatureDrawerLegacy : public CFeatureDrawerCommon
{
public:
	void Draw(bool drawReflection, bool drawRefraction) const override { DrawImpl<true>(drawReflection, drawRefraction); }
	void DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const override;
	void DrawFeatureTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const override;

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	void PushIndividualState(const CFeature* feature, bool deferredPass) const override {};
	void PopIndividualState(const CFeature* feature, bool deferredPass) const override {};
	void DrawIndividual(const CFeature* feature, bool noLuaCall) const override {};
	void DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall) const override {};

	void DrawOpaqueFeaturesShadow(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const override {};
	void DrawOpaqueFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const override;
	void DrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction) const;
	void DrawAlphaFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const override {};
	void DrawAlphaFeature(CFeature* f, bool ffpMat) const override {};
	void DrawFarFeatures() const override {};

	// Setup Fixed State
	void SetupOpaqueDrawing(bool deferredPass) const override;
	void ResetOpaqueDrawing(bool deferredPass) const override;

	void SetupAlphaDrawing(bool deferredPass) const override {};
	void ResetAlphaDrawing(bool deferredPass) const override {};

	// Inherited via CFeatureDrawerCommon
	void DrawShadowPass() const override { DrawShadowPassImpl<true>(); };
	void DrawAlphaPass() const override {};
	bool CanEnable() const override { return true; };

	void DrawFeatureModel(const CFeature* feature, bool noLuaCall) const override;

	void Enable(bool deferredPass, bool alphaPass) const override {}
	void Disable(bool deferredPass) const override {}
};

class CFeatureDrawerFFP : public CFeatureDrawerLegacy
{
public:
	bool SetTeamColor(int team, const float2 alpha = float2(1.0f, 0.0f)) const override { return false; }
};

class CFeatureDrawerARB : public CFeatureDrawerLegacy
{
public:
	bool SetTeamColor(int team, const float2 alpha = float2(1.0f, 0.0f)) const override { return false; }

};

class CFeatureDrawerGLSL : public CFeatureDrawerLegacy
{
public:
	bool SetTeamColor(int team, const float2 alpha = float2(1.0f, 0.0f)) const override { return false; }
};

class CFeatureDrawerGL4 : public CFeatureDrawerLegacy//CFeatureDrawerCommon
{
public:
	bool SetTeamColor(int team, const float2 alpha = float2(1.0f, 0.0f)) const override { return false; }
};