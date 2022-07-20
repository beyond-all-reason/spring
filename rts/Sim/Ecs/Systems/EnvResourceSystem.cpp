#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Utils/SystemUtils.h"
#include "Sim/Ecs/Utils/UnitUtils.h"

#include "EnvResourceSystem.h"

#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"

#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/UnitHandler.h"

using namespace SystemGlobals;
using namespace Units;
using namespace EnvEconomy;

void UpdateWindDirection(EnvResourceComponent& comp)
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

        //LOG_L(L_DEBUG, "%s: updated dir existing generator %d", __func__, unitId);
    }
}

void UpdateWindStrength(EnvResourceComponent& comp)
{
    const float mod = smoothstep(0.0f, 1.0f, comp.windDirTimer / float(WIND_DIRECTION_UPDATE_RATE));

    // blend between old & new wind directions
    // note: generators added on simframes when timer is 0
    // do not receive a snapshot of the blended direction
    comp.curWindVec = mix(comp.oldWindVec, comp.newWindVec, mod);
    comp.curWindStrength = comp.curWindVec.LengthNormalize();

    comp.curWindDir = comp.curWindVec;
    comp.curWindVec = comp.curWindDir * (comp.curWindStrength = Clamp(comp.curWindStrength, comp.minWindStrength, comp.maxWindStrength));

    //LOG_L(L_DEBUG, "%s: wind strength: %f<-%d = %f", __func__, mod, windDirTimer, curWindStrength);

    // make newly added generators point in direction of wind
    auto group = EcsMain::registry.group<NewWindGenerator>(entt::get<UnitId>);
    for (auto entity : group) {
        auto unitId = group.get<UnitId>(entity).value;

        // direction
        auto unit = (unitHandler.GetUnit(unitId));
        unit->UpdateWind(comp.curWindDir.x, comp.curWindDir.z, comp.curWindStrength);

        EcsMain::registry.remove<NewWindGenerator>(entity);
        //LOG_L(L_DEBUG, "%s: updated new dir generator %d", __func__, unitId);
    }
}

void EnvResourceSystem::Init()
{
    systemGlobals.CreateSystemComponent<EnvResourceComponent>();

    entt::component_traits<EnvResourceComponent> systemComponentTraits;
    LOG_L(L_DEBUG, "%s: Component page size is %d", __func__, (int)systemComponentTraits.page_size);

    SystemUtils::systemUtils.OnUpdate().connect<&EnvResourceSystem::Update>();
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
