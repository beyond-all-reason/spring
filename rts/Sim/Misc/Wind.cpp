/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Wind.h"
#include "GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "System/ContainerUtil.h"
#include "System/SpringMath.h"

#include "System/Misc/TracyDefs.h"

CR_BIND(EnvResourceHandler, )

CR_REG_METADATA(EnvResourceHandler, (
	CR_MEMBER(curTidalStrength),
	CR_MEMBER(curWindStrength),
	CR_MEMBER(minWindStrength),
	CR_MEMBER(maxWindStrength),

	CR_MEMBER(curWindVec),
	CR_MEMBER(curWindDir),

	CR_MEMBER(newWindVec),
	CR_MEMBER(oldWindVec),

	CR_MEMBER(windDirTimer),

	CR_MEMBER(allGeneratorIDs),
	CR_MEMBER(newGeneratorIDs)
))


EnvResourceHandler envResHandler;


void EnvResourceHandler::ResetState()
{
	RECOIL_DETAILED_TRACY_ZONE;
	curTidalStrength = 0.0f;
	curWindStrength = 0.0f;
	minWindStrength = 0.0f;
	maxWindStrength = 100.0f;

	curWindDir = RgtVector;
	curWindVec = ZeroVector;
	newWindVec = ZeroVector;
	oldWindVec = ZeroVector;

	windDirTimer = 0;

	allGeneratorIDs.clear();
	allGeneratorIDs.reserve(256);
	newGeneratorIDs.clear();
	newGeneratorIDs.reserve(256);
}

void EnvResourceHandler::LoadWind(float minStrength, float maxStrength)
{
	RECOIL_DETAILED_TRACY_ZONE;
	minWindStrength = std::min(minStrength, maxStrength);
	maxWindStrength = std::max(minStrength, maxStrength);

	// generate initial wind direction
	float strength = 0.0f;

	do {
		curWindDir.x = gsRNG.NextFloat();
		curWindDir.z = gsRNG.NextFloat();
		strength = curWindDir.LengthNormalize2D();
	} while (strength == 0.0f);

	curWindVec = curWindDir * GetAverageWindStrength();
	oldWindVec = curWindVec;
}


bool EnvResourceHandler::AddGenerator(CUnit* u) {
	RECOIL_DETAILED_TRACY_ZONE;
	// duplicates should never happen, no need to check
	return (spring::VectorInsertUnique(newGeneratorIDs, u->id));
}

bool EnvResourceHandler::DelGenerator(CUnit* u) {
	RECOIL_DETAILED_TRACY_ZONE;
	// id is never present in both
	return (spring::VectorErase(newGeneratorIDs, u->id) || spring::VectorErase(allGeneratorIDs, u->id));
}



void EnvResourceHandler::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// zero-strength wind does not need updates
	if (maxWindStrength <= 0.0f)
		return;

	if (windDirTimer == 0) {
		oldWindVec = curWindVec;
		newWindVec = oldWindVec;

		// generate new wind direction
		float newStrength = 0.0f;

		do {
			newWindVec.x -= (gsRNG.NextFloat() - 0.5f) * maxWindStrength;
			newWindVec.z -= (gsRNG.NextFloat() - 0.5f) * maxWindStrength;
			newStrength = newWindVec.Length();
		} while (newStrength == 0.0f);

		// normalize and clamp s.t. minWindStrength <= strength <= maxWindStrength
		newWindVec /= newStrength;
		newStrength = std::clamp(newStrength, minWindStrength, maxWindStrength);
		newWindVec *= newStrength;
	}

	const float mod = smoothstep(0.0f, 1.0f, windDirTimer / float(WIND_UPDATE_RATE));

	// blend between old & new wind directions
	curWindVec = mix(oldWindVec, newWindVec, mod);
	curWindStrength = std::clamp(curWindVec.LengthNormalize(), minWindStrength, maxWindStrength);

	curWindDir = curWindVec;
	curWindVec = curWindDir * curWindStrength;

	if (const auto& wcrp = modInfo.windChangeReportPeriod; wcrp > 0 && gs->frameNum % wcrp == 0) {
		// update generators every modInfo.windChangeReportPeriod frames
		for (auto unitID : allGeneratorIDs) {
			unitHandler.GetUnit(unitID)->UpdateWind(curWindDir.x, curWindDir.z, curWindStrength);
		}
	}

	// needs to be done immediately to rotate the generator according to the wind direction correctly
	for (auto unitID : newGeneratorIDs) {
		// make newly added generators point in direction of wind
		unitHandler.GetUnit(unitID)->UpdateWind(curWindDir.x, curWindDir.z, curWindStrength);
		allGeneratorIDs.push_back(unitID);
	}
	newGeneratorIDs.clear();

	windDirTimer = (windDirTimer + 1) % (WIND_UPDATE_RATE + 1);
}

