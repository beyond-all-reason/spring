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
	virtual void Update() const = 0;

	virtual void Draw() const = 0;

	virtual void DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) = 0;
	virtual void DrawFeatureTrans(const CFeature*, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) = 0;

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	virtual void PushIndividualState(const CFeature* feature, bool deferredPass) = 0;
	virtual void PopIndividualState(const CFeature* feature, bool deferredPass) = 0;
	virtual void DrawIndividual(const CFeature* feature, bool noLuaCall) = 0;
	virtual void DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall) = 0;

	virtual void DrawOpaqueFeatures(int modelType) = 0;
	virtual void DrawAlphaFeatures(int modelType) = 0;
	virtual void DrawAlphaFeature(CFeature* f, bool ffpMat) = 0;
	virtual void DrawFarFeatures() = 0;
public:
	// modelDrawerData proxies
	void ConfigNotify(const std::string& key, const std::string& value) { modelDrawerData->ConfigNotify(key, value); }
public:
	static void PushModelRenderState(int mdlType) {};
	static void PushModelRenderState(const S3DModel* m) {};
	static void PushModelRenderState(const CSolidObject* o) {};

	static void PopModelRenderState(int mdlType) {};
	static void PopModelRenderState(const S3DModel* m) {};
	static void PopModelRenderState(const CSolidObject* o) {};

	virtual void DrawFeatureModel(const CFeature* feature, bool noLuaCall) const = 0;
protected:
	bool CanDrawFeature(const CFeature*) const;
};

#define featureDrawer (CFeatureDrawer::selectedModelDrawer)

class CFeatureDrawerCommon : public CFeatureDrawer
{
public:
	void Update() const override;
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const override;
};

class CFeatureDrawerLegacy : public CFeatureDrawerCommon
{
public:
	void Draw() const override {};
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const override {};
	void DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) override {};
	void DrawFeatureTrans(const CFeature*, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) override {};

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	void PushIndividualState(const CFeature* feature, bool deferredPass) override {};
	void PopIndividualState(const CFeature* feature, bool deferredPass) override {};
	void DrawIndividual(const CFeature* feature, bool noLuaCall) override {};
	void DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall) override {};

	void DrawOpaqueFeatures(int modelType) override {};
	void DrawAlphaFeatures(int modelType)override {};
	void DrawAlphaFeature(CFeature* f, bool ffpMat) override {};
	void DrawFarFeatures() override {};

	// Setup Fixed State
	void SetupOpaqueDrawing(bool deferredPass) const override {};
	void ResetOpaqueDrawing(bool deferredPass) const override {};

	void SetupAlphaDrawing(bool deferredPass) const override {};
	void ResetAlphaDrawing(bool deferredPass) const override {};

	// Inherited via CFeatureDrawerCommon
	void DrawShadowPass() const override {};
	void DrawAlphaPass() const override {};
	bool CanEnable() const override { return true; };
	void DrawFeatureModel(const CFeature* feature, bool noLuaCall) const override {};
};

class CFeatureDrawerFFP : public CFeatureDrawerLegacy
{
};

class CFeatureDrawerARB : public CFeatureDrawerLegacy
{
};

class CFeatureDrawerGLSL : public CFeatureDrawerLegacy
{
};

class CFeatureDrawerGL4 : public CFeatureDrawerLegacy//CFeatureDrawerCommon
{
};