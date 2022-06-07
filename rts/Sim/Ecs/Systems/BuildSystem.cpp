#include "BuildSystem.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Ecs/Utils/BuildUtils.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "System/TimeProfiler.h"

using namespace SystemGlobals;
using namespace Build;

void BuildSystem::Init() {
    systemGlobals.CreateSystemComponent<BuildSystemComponent>();
    systemGlobals.SetSystemActiveState<BuildSystemComponent>(true);
}


void BuildSystem::Update() {
    if (!BuildUtils::IsSystemActive())
        return;

    if ((gs->frameNum % BUILD_UPDATE_RATE) != BUILD_TICK)
       return;

    LOG("BuildSystem::%s: %d", __func__, gs->frameNum);

    SCOPED_TIMER("ECS::BuildSystem::Update");

    auto group = EcsMain::registry.group<ActiveBuild>(entt::get<Units::Team, Units::UnitId>);
    for (auto entity : group) {
        auto& buildDetails = group.get<ActiveBuild>(entity);
        auto buildPower = buildDetails.currentBuildpower;
        auto buildTarget = buildDetails.buildTarget;

        // currently paused
        if (buildPower == 0.f) continue;

        auto teamId = (group.get<Units::Team>(entity)).value;
        auto team = teamHandler.Team(teamId);
        auto& buildCost = EcsMain::registry.get<BuildCost>(buildTarget);

        float buildRate = 1.f;
        for (int i=0; i<SResourcePack::MAX_RESOURCES; i++){
            bool foundLowerProrationrate = (buildCost[i] > 0.f) && (team->resProrationRates[i] < buildRate);
            buildRate = foundLowerProrationrate ? team->resProrationRates[i] : buildRate;
        }

        // float buildRate = team->prorationRates[(int)buildDetails.prorationType];
        auto& buildProgress = (EcsMain::registry.get<BuildProgress>(buildTarget)).value;
        auto& health = (EcsMain::registry.get<SolidObject::Health>(buildTarget)).value;
        auto maxHealth = (EcsMain::registry.get<SolidObject::MaxHealth>(buildTarget)).value;
        auto buildTime = (EcsMain::registry.get<BuildTime>(buildTarget)).value;

        float buildStep = (buildPower*BUILD_UPDATE_RATE)/buildTime;
        float proratedBuildStep = buildStep * buildRate;
        float nextProgress = buildProgress + proratedBuildStep;
        float nextHealth = health + maxHealth * proratedBuildStep;

        SResourcePack resPull = buildCost * buildStep;
        SResourcePack resUsage(buildCost);
        resUsage *= (nextProgress >= 1.f) ? (1.f - buildProgress) : proratedBuildStep;

        LOG("BuildSystem::%s: %d -> %d (tid: %d m:%f e:%f)", __func__, (int)entity, (int)buildTarget, teamId, resUsage.metal, resUsage.energy);

        if (team->UseResources(resUsage)) {
            if (nextProgress < 1.f) team->recordFlowEcoPull(resPull);

            buildProgress = std::min(nextProgress, 1.f);
            health = std::min(nextHealth, maxHealth);
            if (buildProgress == 1.f){
                EcsMain::registry.emplace<BuildComplete>(buildTarget);
            }
            LOG("BuildSystem::%s: %d -> %d (%f%%)", __func__, (int)entity, (int)buildTarget, buildProgress*100.f);
        }
        else {
            team->recordFlowEcoPull(resPull);
        }
    }
}
