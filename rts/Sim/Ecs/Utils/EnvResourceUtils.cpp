#include "EnvResourceUtils.h"

#include "System/SpringMath.h"

using namespace EnvResources;

EnvResourceUtils EnvResources::envResourceUtils;

void EnvResourceUtils::LoadWind(float minStrength, float maxStrength)
{
    auto& comp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>();

	comp.minWindStrength = std::min(minStrength, maxStrength);
	comp.maxWindStrength = std::max(minStrength, maxStrength);

	comp.curWindVec = mix(comp.curWindDir * GetAverageWindStrength(), RgtVector * GetAverageWindStrength(), comp.curWindDir == RgtVector);
	comp.oldWindVec = comp.curWindVec;
}