/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SENSOR_MEMPOOL_H
#define SENSOR_MEMPOOL_H

#include "Sim/Misc/Sensor.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"

#if (defined(__x86_64) || defined(__x86_64__) || defined(_M_X64))
typedef StaticMemPoolT<MAX_SENSORS, CSensor> SensorMemPool;
#else
typedef FixedDynMemPoolT<MAX_SENSORS / 10, MAX_SENSORS / 32, CSensor> SensorMemPool;
#endif

extern SensorMemPool sensorMemPool;

#endif

