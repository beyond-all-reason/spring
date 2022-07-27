#ifndef ENV_ECONOMY_COMPONENTS_H__
#define ENV_ECONOMY_COMPONENTS_H__

#include "Sim/Ecs/EcsMain.h"
#include "BaseComponents.h"

namespace EnvEconomy {

// holds values that always matter to a wind generator
struct WindGenerator {};

// indicates that this is a wind generator that needs to be given a
// starting wind value while the next wind update is coming.
struct NewWindGenerator {};

// Only present when the wind generator is active
struct WindGeneratorActive {};

struct WindEnergy {};

ALIAS_COMPONENT(WindEconomyTaskRef, entt::entity)


template<class Archive, class Snapshot>
void serializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component
        < NewWindGenerator, WindEconomyTaskRef, WindEnergy, WindGenerator, WindGeneratorActive
        >(archive);
}

}

#endif