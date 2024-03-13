#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/SensorMemPool.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "System/creg/DefTypes.h"
#include "System/creg/STL_List.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/SyncedPrimitive.h"

#include <tracy/Tracy.hpp>


CSensor::CSensor()
{
	createdOn = gs->frameNum;
	drawFlag = SO_NODRAW_FLAG;
	isInit = false;
	isExpired = false;
	isDead = false;
}

CSensor::~CSensor()
{
	// Don't think anything is needed here...?
}

void CSensor::Init(int team_, float3 pos_, int LOSType_, float LOSDistance_, int duration_)
{
	team = team_;
	allyteam = teamHandler.AllyTeam(team);
	pos = pos_;

	switch (LOSType_) {
	case 1: 
		losRadius = LOSDistance_;
		airLosRadius = LOSDistance_;
		radarRadius = 0;
		sonarRadius = 0;
		break;
	case 2: 
		losRadius = 0;
		airLosRadius = 0;
		radarRadius = LOSDistance_;
		sonarRadius = LOSDistance_;
		break;
	case 4: 
		losRadius = LOSDistance_;
		airLosRadius = LOSDistance_;
		radarRadius = LOSDistance_;
		sonarRadius = LOSDistance_;
		break;
	default: 
		losRadius = 0;
		airLosRadius = 0;
		radarRadius = 0;
		sonarRadius = 0;
		break;
	}

	jammerRadius = 0;
	seismicRadius = 0;
	sonarJamRadius = 0;

	losHeight = 0;
	radarHeight = 0;

	LOSDistance = LOSDistance_;

	if (duration_ == -1) {
		expiresOn = -1;
	}
	else {
		expiresOn = createdOn + duration_;
	}

	eventHandler.SensorCreated(this);
	isInit = true;
}


void CSensor::Update()
{
	if (expiresOn != -1 && gs->frameNum > expiresOn) {
		isExpired = true;
	}
}

void CSensor::Kill() 
{
	if (isDead)
		return;

	isExpired = true;
	isDead = true;

	eventHandler.SensorExpired(this);
}

bool CSensor::ChangeTeam(int newTeamID)
{
	if (isExpired || isDead)
		return false;

	eventHandler.SensorTaken(this, team, newTeamID);
	teamHandler.Team(team)->RemoveSensor(this);
	teamHandler.Team(team)->AddSensor(this);
}

//CR_BIND_DERIVED_POOL(CSensor, CWorldObject, , sensorMemPool.allocMem, sensorMemPool.freeMem)
//CR_REG_METADATA(CSensor, (
//	CR_MEMBER(LOSDistance),
//	CR_MEMBER(createdOn),
//	CR_MEMBER(expiresOn),
//	CR_MEMBER(isInit),
//	CR_MEMBER(isExpired),
//	CR_MEMBER(isDead),
//	CR_MEMBER(losRadius),
//	CR_MEMBER(airLosRadius),
//	CR_MEMBER(radarRadius),
//	CR_MEMBER(sonarRadius),
//	CR_MEMBER(jammerRadius),
//	CR_MEMBER(seismicRadius),
//	CR_MEMBER(sonarJamRadius),
//	CR_MEMBER(losHeight),
//	CR_MEMBER(radarHeight),
//	CR_MEMBER(los)
//	))