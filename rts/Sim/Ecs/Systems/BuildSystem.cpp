#include "BuildSystem.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Utils/BuildUtils.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"

using namespace SystemGlobals;
using namespace Build;

void BuildSystem::Init() {
    if (modInfo.economySystem == ECONOMY_SYSTEM_ECS)
        systemGlobals.CreateSystemComponent<BuildSystemComponent>();
}

void RequestBuildResources() {
    auto combinedGroup = EcsMain::registry.group<ActiveBuild>(entt::get<Units::Team, Units::UnitId, FlowEconomy::AllocatedUnusedResource>);
    auto group = EcsMain::registry.group<ActiveBuild>(entt::get<Units::Team, Units::UnitId>);
    auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    for (auto entity : group) {
        if (entitiesLeftToProcess-- == 0) break;

        auto& buildDetails = group.get<ActiveBuild>(entity);
        auto buildPower = buildDetails.currentBuildpower;
        auto buildTarget = buildDetails.buildTarget;

        // currently paused
        if (buildPower == 0.f) {
            EcsMain::registry.emplace_or_replace<FlowEconomy::ResourceUse>(entity);
            LOG("BuildSystem::%s: %d -> %d (%d) paused", __func__, (int)entity, (int)buildTarget, (int)buildPower);
            continue;
        }

        auto teamId = (group.get<Units::Team>(entity)).value;
        auto team = teamHandler.Team(teamId);
        auto& buildCost = EcsMain::registry.get<BuildCost>(buildTarget);
        auto buildTime = (EcsMain::registry.get<BuildTime>(buildTarget)).value;
        float buildStep = (buildPower*GAME_SPEED)/buildTime;
        SResourcePack resPull = buildCost * buildStep;

        EcsMain::registry.emplace_or_replace<FlowEconomy::ResourceUse>(entity, resPull);
        team->resPull += (resPull * ((float)BUILD_UPDATE_RATE/(float)GAME_SPEED));

        LOG("BuildSystem::%s: %d -> %d (%f,%f,%f,%f)", __func__, (int)entity, (int)buildTarget, resPull[0], resPull[1], resPull[2], resPull[3]);
    }
}

template<typename T>
class ReleaseComponentOnExit {
public:
    ReleaseComponentOnExit(entt::entity scopedEntity): entity(scopedEntity) {}
    ~ReleaseComponentOnExit() { EcsMain::registry.remove<T>(entity); }
private:
    entt::entity entity;
};

void BuildSystem::Update() {
    if (!BuildUtils::IsSystemActive())
        return;

    if ((gs->frameNum % BUILD_UPDATE_RATE) != BUILD_TICK)
       return;

    LOG("BuildSystem::%s: %d", __func__, gs->frameNum);

    SCOPED_TIMER("ECS::BuildSystem::Update");

    RequestBuildResources();

    auto group = EcsMain::registry.group<ActiveBuild>(entt::get<Units::Team, Units::UnitId, FlowEconomy::AllocatedUnusedResource>);
    for (auto entity : group) {
        ReleaseComponentOnExit<FlowEconomy::AllocatedUnusedResource> scopedExit(entity);

        auto& buildDetails = group.get<ActiveBuild>(entity);
        auto buildPower = buildDetails.currentBuildpower;
        auto buildTarget = buildDetails.buildTarget;

        // currently paused
        if (buildPower == 0.f) {
            EcsMain::registry.emplace_or_replace<FlowEconomy::ResourceUse>(entity);
            LOG("BuildSystem::%s: %d -> %d (%d) paused", __func__, (int)entity, (int)buildTarget, (int)buildPower);
            continue;
        }

        auto teamId = (group.get<Units::Team>(entity)).value;
        auto team = teamHandler.Team(teamId);
        auto& buildCost = EcsMain::registry.get<BuildCost>(buildTarget);
        auto buildTime = (EcsMain::registry.get<BuildTime>(buildTarget)).value;
        float buildStep = (buildPower*BUILD_UPDATE_RATE)/buildTime;
        SResourcePack resPull = buildCost * buildStep;

        // float buildRate = 1.f;
        // for (int i=0; i<SResourcePack::MAX_RESOURCES; i++){
        //     bool foundLowerProrationrate = (buildCost[i] > 0.f) && (team->resProrationRates[i] < buildRate);
        //     buildRate = foundLowerProrationrate ? team->resProrationRates[i] : buildRate;
        // }

        auto& resAllocated = EcsMain::registry.get<FlowEconomy::AllocatedUnusedResource>(entity); // EcsMain::registry.get_or_emplace<FlowEconomy::AllocatedUnusedResource>(entity);
        float buildRate = resAllocated.prorationRate;
        // if (buildRate == 0.f) {
        //     EcsMain::registry.get_or_emplace<FlowEconomy::ResourceUse>(entity, resPull);
        //     team->resPull += resPull;
        //     return;
        // }

        auto& buildProgress = (EcsMain::registry.get<BuildProgress>(buildTarget)).value;
        auto& health = (EcsMain::registry.get<SolidObject::Health>(buildTarget)).value;
        auto maxHealth = (EcsMain::registry.get<SolidObject::MaxHealth>(buildTarget)).value;
        
        float proratedBuildStep = buildStep * buildRate;
        float nextProgress = buildProgress + proratedBuildStep;
        float nextHealth = health + maxHealth * proratedBuildStep;
        float finalBuildStep = std::min(proratedBuildStep, std::max(1.f - buildProgress, 1.f));

        // SResourcePack resUsage(buildCost);
        SResourcePack resUsage(resAllocated.res);

        if (nextProgress > 1.f) {
            float allocRatioUsed = ((1.f - buildProgress) / proratedBuildStep);
            resUsage *= allocRatioUsed;
            resAllocated.res -= resUsage;
        }
        else
            resAllocated.res = SResourcePack();

        // resUsage *= (nextProgress >= 1.f) ? (1.f - buildProgress) : proratedBuildStep;

        // LOG("BuildSystem::%s: %d -> %d (tid: %d m:%f e:%f)", __func__, (int)entity, (int)buildTarget, teamId, resUsage.metal, resUsage.energy);

        // bool isResAvailable = team->UseFlowEcoResources(resUsage);
        // if (!isResAvailable) {
        //     buildRate = 1.f;
        //     for (int i=0; i<SResourcePack::MAX_RESOURCES; ++i){
        //         float resBuildRate = (resPull[i] > 0.f) ? (team->res[i] / resPull[i]) : 1.f;
        //         LOG("BuildSystem::%s: Retry: resBuildRate = %f [%f]", __func__, resBuildRate, resPull[i]);
        //         buildRate = std::min(resBuildRate, buildRate);
        //     }
        //     // trunc buildRate to avoid rounding error when this should actually work
        //     constexpr double truncAccuracy = 1000000.;
        //     LOG("BuildSystem::%s: Retry: BuildRate = %.10f", __func__, buildRate);
        //     buildRate = springmath::trunc((double)buildRate, truncAccuracy);
        //     LOG("BuildSystem::%s: Retry: BuildRate = %.10f (%f)", __func__, buildRate, truncAccuracy);
        //     if (buildRate > 0.000001f){
        //         for (int i=0; i<SResourcePack::MAX_RESOURCES; ++i)
        //             resUsage[i] = resPull[i] * buildRate * (resPull[i] > 0.f);

        //         LOG("BuildSystem::%s: Retry %d -> %d (tid: %d m:%f e:%f)", __func__, (int)entity, (int)buildTarget, teamId, resUsage.metal, resUsage.energy);

        //         isResAvailable = team->UseFlowEcoResources(resUsage);
        //     }
        // }

        // if (isResAvailable) {
            // if (nextProgress < 1.f) team->recordFlowEcoPull(resPull, resUsage);

            //if (nextProgress < 1.f) team->resPull += resPull;
            if (nextProgress >= 1.f) EcsMain::registry.remove<FlowEconomy::ResourceUse>(entity);

            auto comp = EcsMain::registry.try_get<UnitEconomy::ResourcesCurrentUsage>(entity);
            if (comp != nullptr)
                *comp += resUsage;

            buildProgress = std::min(nextProgress, 1.f);
            health = std::min(nextHealth, maxHealth);
            LOG("BuildSystem::%s: %d -> %d (%f%%)", __func__, (int)entity, (int)buildTarget, buildProgress*100.f);
        // }
        // else {
        //     team->recordFlowEcoPull(resPull, resUsage);
        // }
    }
}
