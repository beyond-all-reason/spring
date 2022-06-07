#include "LegacyUtils.h"

#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Units/Unit.h"

using namespace SystemGlobals;

void LegacyUtils::AddFlowEconomyUnit(CUnit *unit) {

    auto entity = unit->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    if (systemGlobals.IsSystemActive<FlowEconomySystemComponent>())
        return; // eco is only added when needed

    // force add the components needed when the system isn't active to ensure the original code still works.
}