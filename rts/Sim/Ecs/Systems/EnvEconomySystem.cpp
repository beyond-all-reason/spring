#include "EnvEconomySystem.h"

#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Systems/FlowEconomySystem.h"
#include "Sim/Ecs/Systems/EnvResourceSystem.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/UnitDef.h"

#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"


EnvEconomySystem envEconomySystem;

void EnvEconomySystem::Update()
{
    SCOPED_TIMER("ECS::EnvEconomySystem::Update");

    if (!flowEconomySystem.IsSystemActive())
        return;

    if ((gs->frameNum % ENV_RESOURCE_UPDATE_RATE) != ENV_RESOURCE_TICK)
       return;

    LOG("EnvResourceSystem::%s: %d", __func__, gs->frameNum);

    auto group = EcsMain::registry.group<EnvEconomy::WindEnergy>(entt::get<Units::UnitDefRef, FlowEconomy::ResourceAdd>);
    for (auto entity : group) {
        auto unitDef = (group.get<Units::UnitDefRef>(entity).value);
        auto& energyIncome = group.get<FlowEconomy::ResourceAdd>(entity);

        energyIncome.energy = std::min(envResourceSystem.GetCurrentWindStrength(), unitDef->windGenerator);

        LOG("%s: updated wind value generator", __func__);
    }
}
