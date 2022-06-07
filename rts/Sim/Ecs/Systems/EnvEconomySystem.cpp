#include "EnvEconomySystem.h"

#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/EnvEconomyComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Systems/FlowEconomySystem.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/UnitDef.h"

#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"

using namespace SystemGlobals;

void EnvEconomySystem::Update()
{
    if (!systemGlobals.IsSystemActive<FlowEconomySystemComponent>())
        return;

    if ((gs->frameNum % ENV_RESOURCE_UPDATE_RATE) != ENV_RESOURCE_TICK)
       return;

    SCOPED_TIMER("ECS::EnvEconomySystem::Update");
    LOG("EnvResourceSystem::%s: %d", __func__, gs->frameNum);

    auto& envResourceComp = SystemGlobals::systemGlobals.GetSystemComponent<SystemGlobals::EnvResourceComponent>();

    auto group = EcsMain::registry.group<EnvEconomy::WindEnergy>(entt::get<Units::UnitDefRef, FlowEconomy::ResourceAdd>);
    for (auto entity : group) {
        auto unitDef = (group.get<Units::UnitDefRef>(entity).value);
        auto& income = group.get<FlowEconomy::ResourceAdd>(entity);

        income.energy = std::min(envResourceComp.curWindStrength, unitDef->windGenerator);

        LOG("%s: updated wind generator income", __func__);
    }
}
