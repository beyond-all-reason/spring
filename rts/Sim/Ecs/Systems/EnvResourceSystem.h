#ifndef ENV_RESOURCE_SYSTEM_H__
#define ENV_RESOURCE_SYSTEM_H__

#include "Sim/Misc/GlobalConstants.h"
#include "System/float3.h"

// #include "Sim/Ecs/Components/SystemGlobalComponents.h"

// struct SystemComponentTraits : public entt::component_traits<SystemGlobals::EnvResourceComponent> {
// 	static constexpr bool in_place_delete = false;
//     static constexpr std::size_t page_size = 1;
// };

namespace SystemGlobals {
    struct EnvResourceComponent;
}

// TODO: save/restore components
// save/restore entity in unit
class EnvResourceSystem {
    CR_DECLARE_STRUCT(EnvResourceSystem)

public:
    void Init();
    void Update();

	// update all generators every 15 seconds
	static constexpr int WIND_UPDATE_RATE = 15 * GAME_SPEED;

private:
    void UpdateWindDirection(SystemGlobals::EnvResourceComponent& comp);
    void UpdateWindStrength(SystemGlobals::EnvResourceComponent& comp);
};

extern EnvResourceSystem envResourceSystem;

#endif