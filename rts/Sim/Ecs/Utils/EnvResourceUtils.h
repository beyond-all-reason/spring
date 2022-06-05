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
    static SystemGlobals::EnvResourceComponent& GetComp() {
        return SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>(); }

	static void LoadTidal(float curStrength) { GetComp().curTidalStrength = curStrength; }
	static void LoadWind(float minStrength, float maxStrength);

	static float GetMaxWindStrength() { return GetComp().maxWindStrength; }
	static float GetMinWindStrength() { return GetComp().minWindStrength; }

	static float GetAverageWindStrength() {
        auto& comp = GetComp(); return ((comp.minWindStrength + comp.maxWindStrength) * 0.5f); }

	static float GetCurrentWindStrength() { return GetComp().curWindStrength; }
	static float GetCurrentTidalStrength() { return GetComp().curTidalStrength; }
	static const float3& GetCurrentWindVec() { return GetComp().curWindVec; }
	static const float3& GetCurrentWindDir() { return GetComp().curWindDir; }

    static bool IsWindAboutToChange() {
        return ((gs->frameNum % WIND_DIRECTION_UPDATE_RATE) == (WIND_DIRECTION_UPDATE_RATE - 1)); }

};

}

#endif