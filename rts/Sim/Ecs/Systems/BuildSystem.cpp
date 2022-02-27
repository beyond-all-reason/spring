#include "BuildSystem.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"

#include "Sim/Misc/TeamHandler.h"

#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

// go through all builders
// get team prorationrate
// get build target
// apply progress to buildtarget (buildPower*prorationRate)


// make sure build action don't get resources

using namespace Build;

void returnExcessEco(entt::entity entity, CTeam *team, float excessBuild){
    if (excessBuild > 0.f) {
        SResourcePack resToReturn;
        if (EcsMain::registry.all_of<BuildCostEnergy>(entity)){
            resToReturn.energy = (EcsMain::registry.get<BuildCostEnergy>(entity)).value;
        }
        if (EcsMain::registry.all_of<BuildCostMetal>(entity)) {
            resToReturn.metal = (EcsMain::registry.get<BuildCostMetal>(entity)).value;
        }
        team->resNextIncome += resToReturn * excessBuild;
    }
}

void BuildSystem::Update() {
    auto group = EcsMain::registry.group<BuildPower, ActiveBuild>(entt::get<Units::Team, Units::UnitId>);

    for (auto entity : group) {
        auto buildPower = (group.get<BuildPower>(entity)).value;
        auto buildDetails = group.get<ActiveBuild>(entity);
        auto teamId = (group.get<Units::Team>(entity)).value;

        auto team = teamHandler.Team(teamId);

        float buildRate = team->prorationRates[(int)buildDetails.prorationType];

        // method 2
        auto& buildProgress = (EcsMain::registry.get<BuildProgress>(buildDetails.buildTarget)).value;
        auto buildTime = (EcsMain::registry.get<BuildTime>(buildDetails.buildTarget)).value;

        float buildStep = buildPower*buildRate / buildTime;
        float nextProgress = buildProgress + buildStep;

        returnExcessEco(buildDetails.buildTarget, team, (nextProgress - 1.f));

        buildProgress = std::min(nextProgress, 1.f);
        if (buildProgress == 1.f){
            EcsMain::registry.emplace<BuildComplete>(buildDetails.buildTarget);
        }
    }
}
