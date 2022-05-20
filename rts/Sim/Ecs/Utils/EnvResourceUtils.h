#ifndef ENV_RESOURCE_UTILS_H__
#define ENV_RESOURCE_UTILS_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Misc/GlobalSynced.h"


namespace EnvResources {

class EnvResourceUtils {
public:
	void LoadTidal(float curStrength) {
        auto& comp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>();
        comp.curTidalStrength = curStrength; }
	void LoadWind(float minStrength, float maxStrength);

	float GetMaxWindStrength() const {
        auto& comp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>();
        return comp.maxWindStrength; }
	float GetMinWindStrength() const {
        auto& comp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>();
        return comp.minWindStrength; }
	float GetAverageWindStrength() const {
        auto& comp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>();
        return ((comp.minWindStrength + comp.maxWindStrength) * 0.5f); }
	float GetCurrentWindStrength() const {
        auto& comp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>();
        return comp.curWindStrength; }
	float GetCurrentTidalStrength() const {
        auto& comp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>();
        return comp.curTidalStrength; }

	const float3& GetCurrentWindVec() const {
        auto& comp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>();
        return comp.curWindVec; }
	const float3& GetCurrentWindDir() const {
        auto& comp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>();
        return comp.curWindDir; }

    bool IsWindAboutToChange() const {
    return ((gs->frameNum % WIND_DIRECTION_UPDATE_RATE) == (WIND_DIRECTION_UPDATE_RATE - 1));
}

};

extern EnvResourceUtils envResourceUtils;

}

#endif