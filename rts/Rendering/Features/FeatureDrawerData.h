#pragma once

#include "System/float3.h"
#include "Rendering/Common/ModelRenderData.h"

class CFeature;

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
	void FeatureMoved(const CFeature* feature, const float3& oldpos) {};
public:
	CFeatureDrawerData();
	virtual ~CFeatureDrawerData();
public:
	void ConfigNotify(const std::string& key, const std::string& value);
/////
public:
	void Update() override {};
	bool IsAlpha(const CFeature* co) const override { return true; };
public:
	float featureDrawDistance;
	float featureFadeDistance;
};