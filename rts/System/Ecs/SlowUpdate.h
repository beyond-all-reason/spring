/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SLOW_UPDATE_H__
#define SLOW_UPDATE_H__

// #ifndef LOG_SECTION_CURRENT
// #define LOG_SECTION_ECS "ECS"
// #define LOG_SECTION_CURRENT LOG_SECTION_ECS
// #define DEBUG
// #endif

#include "Sim/Misc/GlobalConstants.h"

constexpr int REALTIME_SYSTEM_UPDATE = 1;   // 30 hz
constexpr int FAST_SYSTEM_UPDATE = 3;       // 10 hz
constexpr int SLOW_SYSTEM_UPDATE = 15;      //  2 hz
//constexpr int BACKGROUND_SYSTEM_UPDATE = 30; // 1 hz


// FAST SYSTEMS
constexpr int BUILD_UPDATE_RATE = FAST_SYSTEM_UPDATE;
constexpr int ENV_RESOURCE_UPDATE_RATE = FAST_SYSTEM_UPDATE;
constexpr int FLOW_ECONOMY_UPDATE_RATE = FAST_SYSTEM_UPDATE;
constexpr int REPAIR_UPDATE_RATE = FAST_SYSTEM_UPDATE;
constexpr int UNIT_ECONOMY_UPDATE_RATE = FAST_SYSTEM_UPDATE;

constexpr int BUILD_TICK = 1;
constexpr int ENV_RESOURCE_TICK = 2;
constexpr int FLOW_ECONOMY_TICK = 0;
constexpr int REPAIR_TICK = 2;
constexpr int UNIT_ECONOMY_TICK = 1;


// SLOW UPDATE
constexpr int UNIT_ECONOMY_REPORT_UPDATE_RATE = SLOW_SYSTEM_UPDATE;

constexpr int UNIT_ECONOMY_REPORT_TICK = 2;


// BACKGROUND SYSTEMS
constexpr int WIND_DIRECTION_UPDATE_RATE = 15 * GAME_SPEED;

constexpr int WIND_DIRECTION_TICK = 0;

#endif