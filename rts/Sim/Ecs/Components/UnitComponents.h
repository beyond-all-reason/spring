#ifndef UNIT_COMPONENTS_H__
#define UNIT_COMPONENTS_H__

#include "Sim/Units/Unit.h"
#include "BaseComponents.h"


class UnitDef;

namespace Units {

ALIAS_COMPONENT(UnitId, int)
ALIAS_COMPONENT(Team, int)
ALIAS_COMPONENT(UnitDefRef, int)
ALIAS_COMPONENT(OwningEntity, entt::entity)
ALIAS_COMPONENT(EconomyTasks, std::size_t)

struct ChainEntity {
    entt::entity prev{entt::null};
    entt::entity next{entt::null};
};

ALIAS_COMPONENT(MetalUpKeepEconomyTaskRef, entt::entity)
ALIAS_COMPONENT(EnergyUpKeepEconomyTaskRef, entt::entity)
ALIAS_COMPONENT(ConditionalMetalUseEconomyTaskRef, entt::entity)
ALIAS_COMPONENT(ConditionalEnergyUseEconomyTaskRef, entt::entity)
ALIAS_COMPONENT(MakeResourcesEconomyTaskRef, entt::entity)
ALIAS_COMPONENT(MakeDrainResourcesEconomyTaskRef, entt::entity)

}

#endif