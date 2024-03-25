/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SENSORHANDLER_H
#define SENSORHANDLER_H

#include <array>
#include <vector>

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/SimObjectIDPool.h"
#include "System/creg/STL_Map.h"

class CSensor;

class CSensorHandler
{
	//CR_DECLARE_STRUCT(CSensorHandler)

public:
	CSensorHandler() : idPool(MAX_SENSORS) {}

	void Init();
	void Kill();

	void Update();
	bool AddSensor(CSensor* sensor);

	bool CanAddSensor(int id) const {
		// do we want to be assigned a random ID and are any left in pool?
		if (id < 0)
			return (!idPool.IsEmpty());
		// is this ID not already in use *and* has it been recycled by pool?
		if (id < MaxSensors())
			return (sensors[id] == nullptr && idPool.HasID(id));
		// AddSensor will not make new room for us
		return false;
	}

	unsigned int MaxSensors() const { return maxSensors; }

	bool CanBuildSensor(int team) const;
	bool GarbageCollectSensor(unsigned int id);

	// note: negative ID's are implicitly converted
	CSensor* GetSensor(unsigned int id) const { return ((id < MaxSensors()) ? sensors[id] : nullptr); }

	static CSensor* NewSensor(int team_, float3 pos_, int LOSType_, float LOSDistance_, int duration_);

	const std::vector<CSensor*>& GetSensorsToBeRemoved() const { return sensorsToBeRemoved; }
	const std::vector<CSensor*>& GetActiveSensors() const { return activeSensors; }
	std::vector<CSensor*>& GetActiveSensors() { return activeSensors; }

private:
	void InsertActiveSensor(CSensor* sensor);
	bool QueueDeleteSensor(CSensor* sensor);
	void QueueDeleteSensors();
	void DeleteSensor(CSensor* sensor);
	void DeleteSensors();
	void UpdateSensors();

private:
	SimObjectIDPool idPool;

	std::vector<CSensor*> sensors;                                           ///< used to get units from IDs (0 if not created)

	std::vector<CSensor*> activeSensors;                                     ///< used to get all active units
	std::vector<CSensor*> sensorsToBeRemoved;                                ///< units that will be removed at start of next update

	size_t activeUpdateSensor = 0;      ///< first Sensor of batch that will be updated

	///< global unit-limit (derived from the per-team limit)
	///< units.size() is equal to this and constant at runtime
	unsigned int maxSensors = 0;

	bool inUpdateCall = false;
};

extern CSensorHandler sensorHandler;

#endif /* SENSORHANDLER_H */
