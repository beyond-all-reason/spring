#include "LegacyUtils.h"

#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyReportComponents.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Units/Unit.h"

using namespace SystemGlobals;

void LegacyUtils::AddFlowEconomyUnit(CUnit *unit) {

    auto entity = unit->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    if (modInfo.economySystem == ECONOMY_SYSTEM_ECS)
        return; // eco is only added when needed

    // force add the components needed when the system isn't active to ensure the original code still works.
    EcsMain::registry.emplace<UnitEconomy::ResourcesConditionalMake>(entity);
    EcsMain::registry.emplace<UnitEconomy::ResourcesConditionalUse>(entity);
    EcsMain::registry.emplace<UnitEconomy::ResourcesUnconditionalMake>(entity);
    EcsMain::registry.emplace<UnitEconomy::ResourcesUnconditionalUse>(entity);
    EcsMain::registry.emplace<UnitEconomy::ResourcesCurrentMake>(entity);
    EcsMain::registry.emplace<UnitEconomy::ResourcesCurrentUsage>(entity);
}