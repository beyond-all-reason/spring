#include "EnvResourceUtils.h"

#include "System/SpringMath.h"

using namespace EnvResources;

void EnvResourceUtils::LoadWind(float minStrength, float maxStrength)
{
    auto& comp = GetComp();
	auto avgWindStrength = GetAverageWindStrength();

	comp.minWindStrength = std::min(minStrength, maxStrength);
	comp.maxWindStrength = std::max(minStrength, maxStrength);

	comp.curWindVec = mix(comp.curWindDir * avgWindStrength, RgtVector * avgWindStrength, comp.curWindDir == RgtVector);
	comp.oldWindVec = comp.curWindVec;
}