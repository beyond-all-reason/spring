#include "FeatureDrawerData.h"

#include "System/Config/ConfigHandler.h"
#include "System/StringHash.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"

CONFIG(float, FeatureDrawDistance)
.defaultValue(6000.0f)
.minimumValue(0.0f)
.description("Maximum distance at which features will be drawn.");

CONFIG(float, FeatureFadeDistance)
.defaultValue(4500.0f)
.minimumValue(0.0f)
.description("Distance at which features will begin to fade from view.");


void CFeatureDrawerData::RenderFeatureCreated(const CFeature* feature)
{
	if (feature->def->drawType != DRAWTYPE_MODEL)
		return;

	UpdateDrawQuad(feature);
}

void CFeatureDrawerData::RenderFeatureDestroyed(const CFeature* feature)
{
}

CFeatureDrawerData::CFeatureDrawerData()
	: CFeatureRenderDataBase("[CFeatureDrawerData]", 313373)
{
	eventHandler.AddClient(this); //cannot be done in CModelRenderDataConcept, because object is not fully constructed
	configHandler->NotifyOnChange(this, { "FeatureDrawDistance", "FeatureFadeDistance" });

	featureDrawDistance = configHandler->GetFloat("FeatureDrawDistance");
	featureFadeDistance = std::min(configHandler->GetFloat("FeatureFadeDistance"), featureDrawDistance);
}

CFeatureDrawerData::~CFeatureDrawerData()
{
	configHandler->RemoveObserver(this);
}

void CFeatureDrawerData::ConfigNotify(const std::string& key, const std::string& value)
{
	switch (hashStringLower(key.c_str())) {
	case hashStringLower("FeatureDrawDistance"): {
		featureDrawDistance = std::strtof(value.c_str(), nullptr);
	} break;
	case hashStringLower("FeatureFadeDistance"): {
		featureFadeDistance = std::strtof(value.c_str(), nullptr);
	} break;
	default: {
	} break;
	}

	featureDrawDistance = std::max(0.0f, featureDrawDistance);
	featureFadeDistance = std::max(0.0f, featureFadeDistance);
	featureFadeDistance = std::min(featureDrawDistance, featureFadeDistance);

	LOG_L(L_INFO, "[FeatureDrawer::%s] {draw,fade}distance set to {%f,%f}", __func__, featureDrawDistance, featureFadeDistance);
}
