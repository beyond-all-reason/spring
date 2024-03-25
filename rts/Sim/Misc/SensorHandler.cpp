/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "SensorHandler.h"
#include "Sim/Misc/SensorMemPool.h"

#include "Sim/Ecs/Registry.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeProfiler.h"
#include "System/creg/STL_Deque.h"
#include "System/creg/STL_Set.h"


//CR_BIND(CSensorHandler, )
//CR_REG_METADATA(CSensorHandler, (
//	CR_MEMBER(idPool),
//
//	CR_MEMBER(sensorsToBeRemoved),
//	CR_MEMBER(activeUpdateSensor),
//
//	CR_MEMBER(maxSensors),
//
//	CR_MEMBER(inUpdateCall)
//))


SensorMemPool sensorMemPool;
CSensorHandler sensorHandler;


CSensor* CSensorHandler::NewSensor(int team_, float3 pos_, int LOSType_, float LOSDistance_, int duration_)
{
	CSensor* sensor = sensorMemPool.alloc<CSensor>();
	sensor->Init(team_, pos_, LOSType_, LOSDistance_, duration_);
	return sensor;
}



void CSensorHandler::Init() {
	{
		maxSensors = MAX_SENSORS;
	}
	{
		sensors.resize(maxSensors, nullptr);
		activeSensors.reserve(maxSensors);

		sensorMemPool.reserve(128);

		idPool.Clear();
		idPool.Expand(0, MAX_SENSORS);
	}
}


void CSensorHandler::Kill()
{
	for (CSensor* s : activeSensors) {
		sensorMemPool.free(s);
	}
	{
		// do not clear in ctor because creg-loaded objects would be wiped out
		sensorMemPool.clear();

		sensors.clear();

		activeSensors.clear();
		sensorsToBeRemoved.clear();
	}
	{
		maxSensors = 0;
	}
}

void CSensorHandler::InsertActiveSensor(CSensor* sensor)
{
	idPool.AssignID(sensor);

	assert(sensor->id < sensors.size());
	assert(sensors[sensor->id] == nullptr);

	activeSensors.push_back(sensor);
	sensors[sensor->id] = sensor;
}


bool CSensorHandler::AddSensor(CSensor* sensor)
{
	assert(CanAddSensor(sensor->id));

	InsertActiveSensor(sensor);

	teamHandler.Team(sensor->team)->AddSensor(sensor);

	return true;
}


bool CSensorHandler::GarbageCollectSensor(unsigned int id)
{
	if (inUpdateCall)
		return false;

	assert(sensorsToBeRemoved.empty());

	if (!QueueDeleteSensor(sensors[id]))
		return false;

	// only processes sensors[id]
	DeleteSensors();

	return (idPool.RecycleID(id));
}

void CSensorHandler::QueueDeleteSensors()
{
	ZoneScoped;
	// gather up dead sensors
	for (activeUpdateSensor = 0; activeUpdateSensor < activeSensors.size(); ++activeUpdateSensor) {
		QueueDeleteSensor(activeSensors[activeUpdateSensor]);
	}
}


bool CSensorHandler::QueueDeleteSensor(CSensor* sensor)
{
	if (!sensor->isExpired)
		return false;
	sensor->Kill();
	sensorsToBeRemoved.push_back(sensor);
	eventHandler.SensorExpired(sensor);
	losHandler->SensorExpired(sensor);
	return true;
}

void CSensorHandler::DeleteSensors()
{
	while (!sensorsToBeRemoved.empty()) {
		DeleteSensor(sensorsToBeRemoved.back());
		sensorsToBeRemoved.pop_back();
	}
}


void CSensorHandler::DeleteSensor(CSensor* delSensor)
{
	assert(delSensor->isDead);

	const auto it = std::find(activeSensors.begin(), activeSensors.end(), delSensor);

	if (it == activeSensors.end()) {
		assert(false);
		return;
	}

	const int delSensorTeam = delSensor->team;

	teamHandler.Team(delSensorTeam)->RemoveSensor(delSensor);

	activeSensors.erase(it);

	idPool.FreeID(delSensor->id, true);

	sensors[delSensor->id] = nullptr;

	sensorMemPool.free(delSensor);
}


void CSensorHandler::Update()
{
	inUpdateCall = true;

	DeleteSensors();
	QueueDeleteSensors();
	UpdateSensors();

	inUpdateCall = false;
}


void CSensorHandler::UpdateSensors()
{
	SCOPED_TIMER("Sim::Sensor::Update");

	size_t activeSensorCount = activeSensors.size();
	for (size_t i = 0; i < activeSensorCount; ++i) {
		CSensor* sensor = activeSensors[i];
		if (sensor->isDead)
			continue;

		sensor->Update();

		assert(activeSensors[i] == sensor);
	}
}


bool CSensorHandler::CanBuildSensor(int team) const
{
	if (teamHandler.Team(team)->AtSensorLimit())
		return false;

	return true;
}