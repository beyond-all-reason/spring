#include "FluxEconomySystem.h"

#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Ecs/Utils/SystemUtils.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Log/ILog.h"

using namespace SystemGlobals;

void FluxEconomySystem::Init() {
    if (modInfo.economySystem == ECONOMY_SYSTEM_ORIGINAL) {
        LOG("%s: flux economy system active", __func__);
        SystemUtils::systemUtils.OnUpdate().connect<&FluxEconomySystem::Update>();
    }
}

void FluxEconomySystem::Update() {
    // if (systemGlobals.IsSystemActive<FlowEconomySystemComponent>())
    //     return;

    teamHandler.UpdateResourceSnapshots();
}
