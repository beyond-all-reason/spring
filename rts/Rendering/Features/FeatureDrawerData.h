#pragma once

#include "System/float3.h"
#include "Rendering/Common/ModelRenderData.h"

class CFeature;
class CCamera;

class CFeatureDrawerData : public CFeatureRenderDataBase {
public:
	// CEventClient interface
	bool WantsEvent(const std::string & eventName) {
		return
			eventName == "RenderFeatureCreated"   ||
			eventName == "RenderFeatureDestroyed" ||
			eventName == "FeatureMoved";
	}
	void RenderFeatureCreated(const CFeature* feature);
	void RenderFeatureDestroyed(const CFeature* feature);
	void FeatureMoved(const CFeature* feature, const float3& oldpos) { UpdateObject(feature, false); };
public:
	CFeatureDrawerData();
	virtual ~CFeatureDrawerData();
public:
	void ConfigNotify(const std::string& key, const std::string& value);
/////
public:
	void Update() override;
	bool IsAlpha(const CFeature* co) const override;
private:
	void FlagVisibleFeatures(const CCamera* currCamera, bool drawShadowPass, bool drawReflection, bool drawRefraction, bool drawFarFeatures);
	void GetVisibleFeatures(CCamera* cam, int extraSize, bool drawFar);
private:
	static void UpdateDrawPos(CFeature* f);
	static bool SetFeatureDrawAlpha(const CFeature* cf, const CCamera* cam, float sqFadeDistMin = -1.0f, float sqFadeDistMax = -1.0f);
public:
	float featureDrawDistance;
	float featureFadeDistance;
};