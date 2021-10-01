#include "FeatureDrawerData.h"

#include "System/Config/ConfigHandler.h"
#include "System/StringHash.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/Common/ModelDrawerHelpers.h"
#include "Rendering/Env/IWater.h"

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

	SetFeatureDrawAlpha(feature, nullptr);
	UpdateObject(feature, true);
}

void CFeatureDrawerData::RenderFeatureDestroyed(const CFeature* feature)
{
	DelObject(feature, feature->def->drawType == DRAWTYPE_MODEL);
	LuaObjectDrawer::SetObjectLOD(const_cast<CFeature*>(feature), LUAOBJ_FEATURE, 0);
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
	default: {} break;
	}

	featureDrawDistance = std::max(0.0f, featureDrawDistance);
	featureFadeDistance = std::max(0.0f, featureFadeDistance);
	featureFadeDistance = std::min(featureDrawDistance, featureFadeDistance);

	LOG_L(L_INFO, "[FeatureDrawer::%s] {draw,fade}distance set to {%f,%f}", __func__, featureDrawDistance, featureFadeDistance);
}

void CFeatureDrawerData::Update()
{
	for (CFeature* f : unsortedObjects) {
		UpdateDrawPos(f);
		SetFeatureDrawAlpha(f, nullptr);
	}

	for (uint32_t camType = CCamera::CAMTYPE_PLAYER; camType < CCamera::CAMTYPE_ENVMAP; ++camType) {
		CCamera* cam = CCameraHandler::GetCamera(camType);
		UpdateVisibleQuads(cam, modelDrawDist);
		//do not do GetVisibleFeatures() here
	}
}

bool CFeatureDrawerData::IsAlpha(const CFeature* co) const
{
	return (co->drawAlpha < 1.0f);
}

void CFeatureDrawerData::FlagVisibleFeatures(const CCamera* currCamera, bool drawShadowPass, bool drawReflection, bool drawRefraction, bool drawFarFeatures)
{
	const CCamera* playerCam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

	const auto& quads = GetCamVisibleQuads(currCamera->GetCamType());

	const float sqFadeDistBegin = featureFadeDistance * featureFadeDistance;
	const float sqFadeDistEnd   = featureDrawDistance * featureDrawDistance;

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; ++modelType) {
		const auto& rdrContProxies = GetRdrContProxies(modelType);
		for (int quad : quads) {
			const auto& rdrCntProxy = rdrContProxies[quad];

			// non visible quad
			if (!rdrCntProxy.IsQuadVisible())
				continue;

			// quad has no objects
			if (!rdrCntProxy.HasObjects())
				continue;

			for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
				for (CFeature* f : rdrCntProxy.GetObjectBin(i)) {
					assert(quad == f->drawQuad);

					// clear marker; will be set at most once below
					f->SetDrawFlag(CFeature::FD_NODRAW_FLAG);

					if (f->noDraw)
						continue;

					if (f->IsInVoid())
						continue;

					assert(f->def->drawType == DRAWTYPE_MODEL);

					if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(gu->myAllyTeam))
						continue;

					if (drawShadowPass) {
						if (SetFeatureDrawAlpha(f, playerCam, sqFadeDistBegin, sqFadeDistEnd)) {
							// no shadows for fully alpha-faded features from player's POV
							f->UpdateTransform(f->drawPos, false);
							f->SetDrawFlag(CFeature::FD_SHADOW_FLAG);
						}
						continue;
					}

					if (drawRefraction && !f->IsInWater())
						continue;

					if (drawReflection && !CModelDrawerHelper::ObjectVisibleReflection(f->drawMidPos, currCamera->GetPos(), f->GetDrawRadius()))
						continue;

					if (SetFeatureDrawAlpha(f, currCamera, sqFadeDistBegin, sqFadeDistEnd)) {
						f->UpdateTransform(f->drawPos, false);
						f->SetDrawFlag(mix(int(CFeature::FD_OPAQUE_FLAG), int(CFeature::FD_ALPHAF_FLAG), f->drawAlpha < 1.0f));
						continue;
					}

					// note: it looks pretty bad to first alpha-fade and then
					// draw a fully *opaque* fartex, so restrict impostors to
					// non-fading features
					f->SetDrawFlag(CFeature::FD_FARTEX_FLAG * drawFarFeatures * (!f->alphaFade));
				}
			}
		}
	}
}

void CFeatureDrawerData::GetVisibleFeatures(CCamera* cam, int extraSize, bool drawFar) {
	FlagVisibleFeatures(cam, cam->GetCamType() == CCamera::CAMTYPE_SHADOW, water->DrawReflectionPass(), water->DrawRefractionPass(), drawFar);
}

void CFeatureDrawerData::UpdateDrawPos(CFeature* f)
{
	f->drawPos    = f->GetDrawPos(globalRendering->timeOffset);
	f->drawMidPos = f->GetMdlDrawMidPos();
}

bool CFeatureDrawerData::SetFeatureDrawAlpha(const CFeature* cf, const CCamera* cam, float sqFadeDistMin, float sqFadeDistMax)
{
	CFeature* f = const_cast<CFeature*>(cf);
	// always reset outside ::Draw
	if (cam == nullptr)
		return (f->drawAlpha = 0.0f, false);

	const float sqrCamDist = (f->pos - cam->GetPos()).SqLength();
	const float farTexDist = Square(f->GetDrawRadius() * CModelRenderDataConcept::modelDrawDist);

	// first test if feature should be rendered as a fartex
	if (sqrCamDist >= farTexDist)
		return false;

	// special case for non-fading features
	if (!f->alphaFade)
		return (f->drawAlpha = 1.0f, true);

	const float sqFadeDistBeg = mix(sqFadeDistMin, farTexDist * (sqFadeDistMin / sqFadeDistMax), (farTexDist < sqFadeDistMax));
	const float sqFadeDistEnd = mix(sqFadeDistMax, farTexDist, (farTexDist < sqFadeDistMax));

	// draw feature as normal, no fading
	if (sqrCamDist < sqFadeDistBeg)
		return (f->drawAlpha = 1.0f, true);

	// otherwise save it for the fade-pass
	if (sqrCamDist < sqFadeDistEnd)
		return (f->drawAlpha = 1.0f - ((sqrCamDist - sqFadeDistBeg) / (sqFadeDistEnd - sqFadeDistBeg)), true);

	// do not draw at all, fully faded
	return false;
}
