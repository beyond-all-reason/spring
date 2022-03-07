#ifndef SLOW_UPDATE_H__
#define SLOW_UPDATE_H__

constexpr int REALTIME_SYSTEM_UPDATE = 1; // 30 hz
constexpr int FAST_SYSTEM_UPDATE = 3; // 10 hz

// I had considered using 6 frames; however performance counters refresh every 15 frames, which
// would make it much harder to monitor peformance.
constexpr int SLOW_SYSTEM_UPDATE = 5; // 6 hz
constexpr int BACKGROUND_SYSTEM_UPDATE = 30; // 1 hz

// SLOW SYSTEMS
constexpr int BUILD_UPDATE_RATE = SLOW_SYSTEM_UPDATE;
constexpr int ENV_RESOURCE_UPDATE_RATE = SLOW_SYSTEM_UPDATE;
constexpr int FLOW_ECONOMY_UPDATE_RATE = SLOW_SYSTEM_UPDATE;

constexpr int BUILD_TICK = 2;
constexpr int ENV_RESOURCE_TICK = 1;
constexpr int FLOW_ECONOMY_TICK = 0;

#endif