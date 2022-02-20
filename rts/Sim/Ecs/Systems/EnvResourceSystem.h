#ifndef ENV_RESOURCE_SYSTEM_H__
#define ENV_RESOURCE_SYSTEM_H__

#include "Sim/Misc/GlobalConstants.h"
#include "System/float3.h"

// TODO: save/restore components
// save/restore entity in unit
class EnvResourceSystem {
    CR_DECLARE_STRUCT(EnvResourceSystem)

public:
    void Init();
    void Update();

	bool AddGenerator(CUnit* u);

	/* Not expected to be called unless a unit loses its ability to be a wind generator */
	bool DelGenerator(CUnit* u);

	void LoadTidal(float curStrength) { curTidalStrength = curStrength; }
	void LoadWind(float minStrength, float maxStrength);

	float GetMaxWindStrength() const { return maxWindStrength; }
	float GetMinWindStrength() const { return minWindStrength; }
	float GetAverageWindStrength() const { return ((minWindStrength + maxWindStrength) * 0.5f); }
	float GetCurrentWindStrength() const { return curWindStrength; }
	float GetCurrentTidalStrength() const { return curTidalStrength; }

	const float3& GetCurrentWindVec() const { return curWindVec; }
	const float3& GetCurrentWindDir() const { return curWindDir; }

private:
	// update all generators every 15 seconds
	static constexpr int WIND_UPDATE_RATE = 15 * GAME_SPEED;

	float curTidalStrength = 0.0f;
	float curWindStrength = 0.0f;

	float minWindStrength = 0.0f;
	float maxWindStrength = 0.0f;

	float3 curWindDir;
	float3 curWindVec; // curWindDir * curWindStrength
	float3 newWindVec;
	float3 oldWindVec;

	int windDirTimer = 0;

    void UpdateWindTimer();
    void UpdateWindDirection();
    void UpdateNewEnvResources();
};

extern EnvResourceSystem envResourceSystem;

#endif