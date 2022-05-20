#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"


#include "UnitSystem.h"
#include "EnvResourceSystem.h"

#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"

#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"

using namespace SystemGlobals;

EnvResourceSystem envResourceSystem;

using namespace Units;
using namespace EnvEconomy;

void EnvResourceSystem::Init()
{
    systemGlobals.InitSystemComponent<EnvResourceComponent>();
}

void EnvResourceSystem::Update()
{
    SCOPED_TIMER("ECS::EnvResourceSystem::Update");

    auto& comp = systemGlobals.GetSystemComponent<EnvResourceComponent>();

	// zero-strength wind does not need updates
	if (comp.maxWindStrength <= 0.0f)
		return;

	if ((gs->frameNum % WIND_DIRECTION_UPDATE_RATE) == WIND_DIRECTION_TICK) {
        comp.windDirTimer = 0;
		UpdateWindDirection(comp);
    }
    else {
        comp.windDirTimer++;
        UpdateWindStrength(comp);
    }
}

void EnvResourceSystem::UpdateWindDirection(EnvResourceComponent& comp)
{
    comp.oldWindVec = comp.curWindVec;
    comp.newWindVec = comp.oldWindVec;

    // generate new wind direction
    float newStrength = 0.0f;

    do {
        comp.newWindVec.x -= (gsRNG.NextFloat() - 0.5f) * comp.maxWindStrength;
        comp.newWindVec.z -= (gsRNG.NextFloat() - 0.5f) * comp.maxWindStrength;
        newStrength = comp.newWindVec.Length();
    } while (newStrength == 0.0f);

    // normalize and clamp s.t. minWindStrength <= strength <= maxWindStrength
    comp.newWindVec /= newStrength;
    comp.newWindVec *= (newStrength = Clamp(newStrength, comp.minWindStrength, comp.maxWindStrength));
    comp.newWindStrength = newStrength;

    auto group = EcsMain::registry.group<WindGenerator>(entt::get<UnitId>);
    for (auto entity : group) {
        auto unitId = group.get<UnitId>(entity).value;
        auto unit = (unitHandler.GetUnit(unitId));
        unit->UpdateWind(comp.newWindVec.x, comp.newWindVec.z, comp.newWindStrength);

        //LOG("%s: updated dir existing generator %d", __func__, unitId);
    }
}

void EnvResourceSystem::UpdateWindStrength(EnvResourceComponent& comp)
{
    const float mod = smoothstep(0.0f, 1.0f, comp.windDirTimer / float(WIND_DIRECTION_UPDATE_RATE));

    // blend between old & new wind directions
    // note: generators added on simframes when timer is 0
    // do not receive a snapshot of the blended direction
    comp.curWindVec = mix(comp.oldWindVec, comp.newWindVec, mod);
    comp.curWindStrength = comp.curWindVec.LengthNormalize();

    comp.curWindDir = comp.curWindVec;
    comp.curWindVec = comp.curWindDir * (comp.curWindStrength = Clamp(comp.curWindStrength, comp.minWindStrength, comp.maxWindStrength));

    //LOG("%s: wind strength: %f<-%d = %f", __func__, mod, windDirTimer, curWindStrength);

    // make newly added generators point in direction of wind
    auto group = EcsMain::registry.group<NewWindGenerator>(entt::get<UnitId>);
    for (auto entity : group) {
        auto unitId = group.get<UnitId>(entity).value;

        // direction
        auto unit = (unitHandler.GetUnit(unitId));
        unit->UpdateWind(comp.curWindDir.x, comp.curWindDir.z, comp.curWindStrength);

        EcsMain::registry.remove<NewWindGenerator>(entity);
        //LOG("%s: updated new dir generator %d", __func__, unitId);
    }
}


/*
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

void EnvResourceSystem::ActivateGenerator(CUnit* unit) {
    entt::entity entity = unit->entityReference;
    bool entityIsValid = EcsMain::registry.valid(entity);
    if (entityIsValid && EcsMain::registry.all_of<WindGenerator>(entity))
        EcsMain::registry.emplace_or_replace<WindGeneratorActive>(entity);
}

void EnvResourceSystem::DeactivateGenerator(CUnit* unit) {
    entt::entity entity = unit->entityReference;
    bool entityIsValid = EcsMain::registry.valid(entity);
    if (entityIsValid && EcsMain::registry.all_of<WindGenerator>(entity))
        EcsMain::registry.remove<WindGeneratorActive>(entity);
}

bool EnvResourceSystem::DelGenerator(CUnit* unit)
{
    entt::entity entity = unit->entityReference;
    bool entityIsValid = EcsMain::registry.valid(entity);

    if (entityIsValid){
        UnitEconomyHelper::UpdateUnitFixedEnergyIncome(unit, 0.f);
        EcsMain::registry.remove<NewWindGenerator>(entity);
        EcsMain::registry.remove<WindGenerator>(entity);
        EcsMain::registry.remove<WindGeneratorActive>(entity);
    }
    return entityIsValid;
}*/
