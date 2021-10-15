/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <vector>
#include <array>

#include "Game/Camera.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Rendering/Common/ModelDrawer.h"
#include "Rendering/Features/FeatureDrawerData.h"

class CFeature;

class CFeatureDrawer: public CModelDrawerBase<CFeatureDrawerData, CFeatureDrawer>
{
public:
	static void InitStatic();
	//static void KillStatic(bool reload); will use base
	//static void UpdateStatic();
public:
	// DrawFeature*
	virtual void DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const = 0;
	virtual void DrawFeatureTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const = 0;

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	virtual void DrawIndividual(const CFeature* feature, bool noLuaCall) const = 0;
	virtual void DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall) const = 0;
protected:
	virtual void DrawFeaturesShadow(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const = 0;
	virtual void DrawOpaqueFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const = 0;
	virtual void DrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction) const = 0;
	virtual void DrawAlphaFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const = 0;
	virtual void DrawAlphaFeature(CFeature* f) const = 0;
public:
	// modelDrawerData proxies
	void ConfigNotify(const std::string& key, const std::string& value) { modelDrawerData->ConfigNotify(key, value); }
	static const std::vector<CFeature*>& GetUnsortedFeatures() { return modelDrawerData->GetUnsortedObjects(); }
public:
	virtual void DrawFeatureModel(const CFeature* feature, bool noLuaCall) const = 0;
protected:
	static bool ShouldDrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction);
	static bool ShouldDrawAlphaFeature(CFeature* f);
	static bool ShouldDrawFeatureShadow(CFeature* f);

	void PushIndividualState(const CFeature* feature, bool deferredPass) const;
	void PopIndividualState(const CFeature* feature, bool deferredPass) const;
};

class CFeatureDrawerBase : public CFeatureDrawer
{
public:
	void Update() const override;
protected:
	template<bool legacy>
	void DrawShadowPassImpl() const;

	template<bool legacy>
	void DrawOpaquePassImpl(bool deferredPass, bool drawReflection, bool drawRefraction) const;

	template<bool legacy>
	void DrawAlphaPassImpl() const;
};

class CFeatureDrawerLegacy : public CFeatureDrawerBase
{
public:
	void Draw(bool drawReflection, bool drawRefraction) const override {
		DrawImpl<true, LuaObjType::LUAOBJ_FEATURE>(drawReflection, drawRefraction);
	}
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const override {
		DrawOpaquePassImpl<true>(deferredPass, drawReflection, drawRefraction);
	}
	void DrawAlphaPass() const override { DrawAlphaPassImpl<true>(); };

	void DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const override;
	void DrawFeatureTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const override;

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	void DrawIndividual(const CFeature* feature, bool noLuaCall) const override;
	void DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall) const override;
protected:
	void DrawFeaturesShadow(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const override;
	void DrawOpaqueFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const override;
	void DrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction) const;
	void DrawAlphaFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const override;
	void DrawAlphaFeature(CFeature* f) const override;

	void DrawOpaqueFeatureShadow(CFeature* f) const;

	// Inherited via CFeatureDrawerBase
	void DrawShadowPass() const override { DrawShadowPassImpl<true>(); };

	void DrawFeatureModel(const CFeature* feature, bool noLuaCall) const override;
};

class CFeatureDrawerFFP  final : public CFeatureDrawerLegacy {};
class CFeatureDrawerARB  final : public CFeatureDrawerLegacy {};
class CFeatureDrawerGLSL final : public CFeatureDrawerLegacy {};

//TODO remove CFeatureDrawerLegacy inheritance
class CFeatureDrawerGL4 final: public CFeatureDrawerLegacy//CFeatureDrawerBase
{
public:
	void Draw(bool drawReflection, bool drawRefraction) const override {
		DrawImpl<false, LuaObjType::LUAOBJ_FEATURE>(drawReflection, drawRefraction);
	}
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const override {
		DrawOpaquePassImpl<false>(deferredPass, drawReflection, drawRefraction);
	}
	void DrawAlphaPass() const override { DrawAlphaPassImpl<false>(); };
	void DrawShadowPass() const override { DrawShadowPassImpl<false>(); }
protected:
	void DrawFeaturesShadow(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const override;
	void DrawOpaqueFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const override;
	void DrawAlphaFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const override;
};

#define featureDrawer (CFeatureDrawer::modelDrawer)