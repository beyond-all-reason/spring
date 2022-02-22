#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Units/Unit.h"

#include "UnitSystem.h"
#include "EnvResourceSystem.h"
#include "FlowEconomySystem.h"

#include "System/TimeProfiler.h"


#include "System/Log/ILog.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"

CR_BIND(EnvResourceSystem, )

CR_REG_METADATA(EnvResourceSystem, (
	CR_MEMBER(curTidalStrength),
	CR_MEMBER(curWindStrength),
	CR_MEMBER(minWindStrength),
	CR_MEMBER(maxWindStrength),

	CR_MEMBER(curWindVec),
	CR_MEMBER(curWindDir),

	CR_MEMBER(newWindVec),
	CR_MEMBER(oldWindVec),

	CR_MEMBER(windDirTimer)
))

EnvResourceSystem envResourceSystem;

using namespace Units;
using namespace EnvEconomy;

void EnvResourceSystem::Init()
{
	curTidalStrength = 0.0f;
	curWindStrength = 0.0f;
	minWindStrength = 0.0f;
	maxWindStrength = 100.0f;

	curWindDir = RgtVector;
	curWindVec = ZeroVector;
	newWindVec = ZeroVector;
	oldWindVec = ZeroVector;

	windDirTimer = 0;
}

void EnvResourceSystem::Update()
{
    SCOPED_TIMER("ECS::EnvResourceSystem::Update");

	// zero-strength wind does not need updates
	if (maxWindStrength <= 0.0f)
		return;

	if (windDirTimer == 0)
		UpdateWindDirection();
    else
        UpdateWind();

    UpdateWindTimer();
}

void EnvResourceSystem::UpdateWindTimer()
{
    windDirTimer = (windDirTimer + 1) % (WIND_UPDATE_RATE + 1);
}

void EnvResourceSystem::UpdateWindDirection()
{
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
    newWindVec *= (newStrength = Clamp(newStrength, minWindStrength, maxWindStrength));

    // update generators
    auto envResourcesToUpdate = EcsMain::registry.group<WindGenerator>(entt::get<UnitId>);
    for (auto entity : envResourcesToUpdate) {
        auto unitId = envResourcesToUpdate.get<UnitId>(entity);

        // Update energy values
        float unitWindValue = 1.f;
        float newWindValue = std::max(unitWindValue, newStrength);

        auto unit = (unitHandler.GetUnit(unitId.unitId));
        unit->UpdateWind(newWindVec.x, newWindVec.z, newStrength);

        //LOG("%s: updated existing generator %d", __func__, unitId.unitId);
    }
}

void EnvResourceSystem::UpdateWind()
{
    const float mod = smoothstep(0.0f, 1.0f, windDirTimer / float(WIND_UPDATE_RATE));

    // blend between old & new wind directions
    // note: generators added on simframes when timer is 0
    // do not receive a snapshot of the blended direction
    curWindVec = mix(oldWindVec, newWindVec, mod);
    curWindStrength = curWindVec.LengthNormalize();

    curWindDir = curWindVec;
    curWindVec = curWindDir * (curWindStrength = Clamp(curWindStrength, minWindStrength, maxWindStrength));

    // make newly added generators point in direction of wind
    auto envResourcesToUpdate = EcsMain::registry.group<NewWindGenerator>(entt::get<UnitId>);

    for (auto entity : envResourcesToUpdate) {
        auto unitId = envResourcesToUpdate.get<UnitId>(entity);

        auto unit = (unitHandler.GetUnit(unitId.unitId));
        unit->UpdateWind(curWindDir.x, curWindDir.z, curWindStrength);
        EcsMain::registry.remove<NewWindGenerator>(entity);
        //LOG("%s: updated new generator %d", __func__, unitId.unitId);
    }

    // Update income
    // FIXME: this can definately be done better
    if (flowEconomySystem.IsSystemActive()){
        auto view = EcsMain::registry.group<WindGeneratorActive>(entt::get<UnitDefRef>);
        for (auto entity : view){
            const auto& unitDef = view.get<UnitDefRef>(entity);
            float energyGenerated = std::min(curWindStrength, unitDef.unitDefRef->windGenerator);
            flowEconomySystem.UpdateUnitFixedEnergyCreation(entity, energyGenerated);
        }
    }
}

void EnvResourceSystem::LoadWind(float minStrength, float maxStrength)
{
	minWindStrength = std::min(minStrength, maxStrength);
	maxWindStrength = std::max(minStrength, maxStrength);

	curWindVec = mix(curWindDir * GetAverageWindStrength(), RgtVector * GetAverageWindStrength(), curWindDir == RgtVector);
	oldWindVec = curWindVec;
}

bool EnvResourceSystem::AddGenerator(CUnit* unit)
{
    if (!EcsMain::registry.valid(unit->entityReference)){
        LOG("%s: cannot add generator unit to %d because it hasn't been registered yet.", __func__, unit->id);
        return false;
    }

    EcsMain::registry.emplace_or_replace<WindGenerator>(unit->entityReference);
    if (windDirTimer != 0)
        EcsMain::registry.emplace_or_replace<NewWindGenerator>(unit->entityReference);

    LOG("%s: added wind generator unit %d", __func__, unit->id);

    return true;
}

void EnvResourceSystem::ActivateGenerator(CUnit* unit){
    if (!EcsMain::registry.valid(unit->entityReference)){
        LOG("%s: cannot add generator unit to %d because it hasn't been registered yet.", __func__, unit->id);
        return;
    }

    EcsMain::registry.emplace_or_replace<WindGeneratorActive>(unit->entityReference);
}

void EnvResourceSystem::DeactivateGenerator(CUnit* unit){
    if (!EcsMain::registry.valid(unit->entityReference)){
        LOG("%s: cannot add generator unit to %d because it hasn't been registered yet.", __func__, unit->id);
        return;
    }

    EcsMain::registry.remove<WindGeneratorActive>(unit->entityReference);
}

bool EnvResourceSystem::DelGenerator(CUnit* unit)
{
    entt::entity entity = unit->entityReference;
    bool entityIsValid = EcsMain::registry.valid(entity);

    if (entityIsValid){
        EcsMain::registry.remove<NewWindGenerator>(entity);
        EcsMain::registry.remove<WindGenerator>(entity);
        EcsMain::registry.remove<WindGeneratorActive>(entity);
    }
    return entityIsValid;
}
