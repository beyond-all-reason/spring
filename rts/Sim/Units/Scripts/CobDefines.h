/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

/* update LuaConstCOB.cpp anytime this is updated! */


// Indices for emit-sfx
static constexpr int SFX_VTOL = 0;
static constexpr int SFX_WAKE = 2;
static constexpr int SFX_WAKE_2 = 3; // same as SFX_WAKE
static constexpr int SFX_REVERSE_WAKE = 4;
static constexpr int SFX_REVERSE_WAKE_2 = 5; // same as SFX_REVERSE_WAKE
static constexpr int SFX_WHITE_SMOKE = 257;
static constexpr int SFX_BLACK_SMOKE = 258;
static constexpr int SFX_BUBBLE = 259;
static constexpr int SFX_CEG = 1024;
static constexpr int SFX_FIRE_WEAPON = 2048;
static constexpr int SFX_DETONATE_WEAPON = 4096;
static constexpr int SFX_GLOBAL = 16384;


// Indices for set/get value
static constexpr int ACTIVATION = 1;          // set or get
static constexpr int STANDINGMOVEORDERS = 2;  // set or get
static constexpr int STANDINGFIREORDERS = 3;  // set or get
static constexpr int HEALTH = 4;              // get (0-100%)
static constexpr int INBUILDSTANCE = 5;       // set or get
static constexpr int BUSY = 6;                // set or get (used by misc. special case missions like transport ships)
static constexpr int PIECE_XZ = 7;            // get
static constexpr int PIECE_Y = 8;             // get
static constexpr int UNIT_XZ = 9;             // get
static constexpr int UNIT_Y = 10;             // get
static constexpr int UNIT_HEIGHT = 11;        // get
static constexpr int XZ_ATAN = 12;            // get atan of packed x,z coords
static constexpr int XZ_HYPOT = 13;           // get hypot of packed x,z coords
static constexpr int ATAN = 14;               // get ordinary two-parameter atan
static constexpr int HYPOT = 15;              // get ordinary two-parameter hypot
static constexpr int GROUND_HEIGHT = 16;      // get land height, 0 if below water
static constexpr int BUILD_PERCENT_LEFT = 17; // get 0 = unit is built and ready, 1-100 = How much is left to build
static constexpr int YARD_OPEN = 18;  // set or get (change which plots we occupy when building opens and closes)
static constexpr int BUGGER_OFF = 19; // set or get (ask other units to clear the area)
static constexpr int ARMORED = 20;    // set or get

/*
static constexpr int WEAPON_AIM_ABORTED = 21;
static constexpr int WEAPON_READY       = 22;
static constexpr int WEAPON_LAUNCH_NOW  = 23;
static constexpr int FINISHED_DYING     = 26;
static constexpr int ORIENTATION        = 27;
*/
static constexpr int IN_WATER = 28;
static constexpr int CURRENT_SPEED = 29;
// static constexpr int MAGIC_DEATH        = 31;
static constexpr int VETERAN_LEVEL = 32;
static constexpr int ON_ROAD = 34;

static constexpr int MAX_ID = 70;
static constexpr int MY_ID = 71;
static constexpr int UNIT_TEAM = 72;
static constexpr int UNIT_BUILD_PERCENT_LEFT = 73;
static constexpr int UNIT_ALLIED = 74;
static constexpr int MAX_SPEED = 75;
static constexpr int CLOAKED = 76;
static constexpr int WANT_CLOAK = 77;
static constexpr int GROUND_WATER_HEIGHT = 78;       // get land height, negative if below water
static constexpr int UPRIGHT = 79;                   // set or get
static constexpr int POW = 80;                       // get
static constexpr int PRINT = 81;                     // get, so multiple args can be passed
static constexpr int HEADING = 82;                   // get
static constexpr int TARGET_ID = 83;                 // get
static constexpr int LAST_ATTACKER_ID = 84;          // get
static constexpr int LOS_RADIUS = 85;                // set or get
static constexpr int AIR_LOS_RADIUS = 86;            // set or get
static constexpr int RADAR_RADIUS = 87;              // set or get
static constexpr int JAMMER_RADIUS = 88;             // set or get
static constexpr int SONAR_RADIUS = 89;              // set or get
static constexpr int SONAR_JAM_RADIUS = 90;          // set or get
static constexpr int SEISMIC_RADIUS = 91;            // set or get
static constexpr int DO_SEISMIC_PING = 92;           // get
static constexpr int CURRENT_FUEL = 93;              // set or get
static constexpr int TRANSPORT_ID = 94;              // get
static constexpr int SHIELD_POWER = 95;              // set or get
static constexpr int STEALTH = 96;                   // set or get
static constexpr int CRASHING = 97;                  // set or get, returns whether aircraft isCrashing state
static constexpr int CHANGE_TARGET = 98;             // set, the value it's set to determines the affected weapon
static constexpr int CEG_DAMAGE = 99;                // set
static constexpr int COB_ID = 100;                   // get
static constexpr int PLAY_SOUND = 101;               // get, so multiple args can be passed
static constexpr int KILL_UNIT = 102;                // get KILL_UNIT(unitId, SelfDestruct=true, Reclaimed=false)
static constexpr int SET_WEAPON_UNIT_TARGET = 106;   // get (fake set)
static constexpr int SET_WEAPON_GROUND_TARGET = 107; // get (fake set)
static constexpr int SONAR_STEALTH = 108;            // set or get
static constexpr int REVERSING = 109;                // get

// NOTE: [LUA0 - LUA9] are defined in CobThread.cpp as [110 - 119]

static constexpr int FLANK_B_MODE = 120;            // set or get
static constexpr int FLANK_B_DIR = 121;             // set or get, set is through get for multiple args
static constexpr int FLANK_B_MOBILITY_ADD = 122;    // set or get
static constexpr int FLANK_B_MAX_DAMAGE = 123;      // set or get
static constexpr int FLANK_B_MIN_DAMAGE = 124;      // set or get
static constexpr int WEAPON_RELOADSTATE = 125;      // get (with fake set)
static constexpr int WEAPON_RELOADTIME = 126;       // get (with fake set)
static constexpr int WEAPON_ACCURACY = 127;         // get (with fake set)
static constexpr int WEAPON_SPRAY = 128;            // get (with fake set)
static constexpr int WEAPON_RANGE = 129;            // get (with fake set)
static constexpr int WEAPON_PROJECTILE_SPEED = 130; // get (with fake set)
static constexpr int COB_MIN = 131;                 // get
static constexpr int COB_MAX = 132;                 // get
static constexpr int ABS = 133;                     // get
static constexpr int GAME_FRAME = 134;              // get
static constexpr int KSIN = 135;                    // get (kiloSine    : 1024*sin(x))
static constexpr int KCOS = 136;                    // get (kiloCosine  : 1024*cos(x))
static constexpr int KTAN = 137;                    // get (kiloTangent : 1024*tan(x))
static constexpr int SQRT = 138;                    // get (square root)

// NOTE: shared variables use codes [1024 - 5119]
