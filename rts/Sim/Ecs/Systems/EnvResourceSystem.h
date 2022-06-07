#ifndef ENV_RESOURCE_SYSTEM_H__
#define ENV_RESOURCE_SYSTEM_H__

#include "Sim/Misc/GlobalConstants.h"

class EnvResourceSystem {
public:
    static void Init();
    static void Update();

	// update all generators every 15 seconds
	static constexpr int WIND_UPDATE_RATE = 15 * GAME_SPEED;
};

#endif