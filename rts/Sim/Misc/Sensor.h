
#ifndef SENSOR_H
#define SENSOR_H

#include <vector>
#include "System/type2.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Misc/LosHandler.h"

class CSensor : public CWorldObject
{
public:
	//CR_DECLARE(CSensor)

	CSensor();
	virtual ~CSensor();

	void Init(int team_, float3 pos_, int LOSType_, float LOSDistance_, int duration_);
	void Update();
	void Kill();
	bool ChangeTeam(int newTeamID);

public:
	int team;
	int allyteam;

	float LOSDistance;

	int createdOn;
	int expiresOn;

	bool isInit;
	bool isExpired;
	bool isDead;

	int losRadius;
	int airLosRadius;
	int radarRadius;
	int sonarRadius;
	int jammerRadius;
	int seismicRadius;
	int sonarJamRadius;

	float losHeight;
	float radarHeight;


	// which squares the sensor can currently observe, per los-type
	std::array<SLosInstance*, /*ILosType::LOS_TYPE_COUNT*/ 7> los{ {nullptr} };
};

#endif // SENSOR_H