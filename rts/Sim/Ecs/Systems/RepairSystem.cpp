#include "RepairSystem.h"

#include "Sim/Ecs/Utils/SystemUtils.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/Log/DefaultFilter.h"

LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_ECS)

void RepairSystem::Init() {
    if (modInfo.economySystem == ECONOMY_SYSTEM_ECS) {
        SystemUtils::systemUtils.OnUpdate().connect<&RepairSystem::Update>();
        log_filter_section_setMinLevel(LOG_LEVEL_DEBUG, LOG_SECTION_CURRENT);
    }
}

void RequestRepairResources() {

}

void RepairTasks() {
    
}

void RepairSystem::Update() {
    if ((gs->frameNum % REPAIR_UPDATE_RATE) != REPAIR_TICK)
       return;

    LOG_L(L_DEBUG, "RepairSystem::%s: %d", __func__, gs->frameNum);

    SCOPED_TIMER("ECS::RepairSystem::Update");
}
