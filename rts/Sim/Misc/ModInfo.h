/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOD_INFO_H
#define MOD_INFO_H

#include <string>
#include "Sim/Path/PFSTypes.h"

class CModInfo
{
public:
	CModInfo() { ResetState(); }

	void ResetState();
	void Init(const std::string& modFileName);

	/**
	 * The archive file name.
	 * examples: "Supreme Annihilation U32 V1.0.sdz", "BA704.sd7", "133855d253e657e9406122f346cfe8f1.sdp"
	 */
	std::string filename;

	/**
	 * The human readable name (including version).
	 * The lower-case version of this is used for dependency checking.
	 * examples: "Supreme Annihilation U32 V1.0", "Balanced Annihilation V7.04", "Balanced Annihilation V7.11"
	 */
	std::string humanName;
	std::string humanNameVersioned;
	/**
	 * The short name (not including version).
	 * examples: "SA", "BA", "BA"
	 */
	std::string shortName;
	/**
	 * The version
	 * examples: "U32 V1.0", "7.04", "7.11"
	 */
	std::string version;
	std::string mutator;
	std::string description;

	// Movement behaviour
	bool allowAircraftToLeaveMap;    //< determines if gunships are allowed to leave map boundaries
	bool allowAircraftToHitGround;   //< determines if aircraft (both types) can collide with terrain
	bool allowPushingEnemyUnits;     //< determines if enemy (ground-)units can be pushed during collisions
	bool allowCrushingAlliedUnits;   //< determines if allied (ground-)units can be crushed during collisions
	bool allowUnitCollisionDamage;   //< determines if units take damage from (skidding) collisions
	bool allowUnitCollisionOverlap;  //< determines if unit footprints are allowed to semi-overlap during collisions
	bool allowSepAxisCollisionTest;  //< determines if (ground-)units perform collision-testing via the SAT
	bool allowGroundUnitGravity;     //< determines if (ground-)units experience gravity during regular movement
	bool allowHoverUnitStrafing;     //< determines if (hover-)units carry their momentum sideways when turning

	// relative to a unit's maxspeed (default: inf)
	float maxCollisionPushMultiplier;

	// rate in sim frames that a unit's position in the quad grid is updated (default: 3)
	// a lower number will increase CPU load, but increase accuracy of collision detection
	int unitQuadPositionUpdateRate;

	// rate in sim frames that ground/sea units update their unit collision avoidance vectors (default: 3)
	// a lower number will increase CPU load, but improve reaction time of collision avoidance
	int groundUnitCollisionAvoidanceUpdateRate;

	// Build behaviour
	/// Should constructions without builders decay?
	bool constructionDecay;
	/// How long until they start decaying?
	int constructionDecayTime;
	/// How fast do they decay?
	float constructionDecaySpeed;
	/// When units are created, issue a move command off of the factory pad.
	bool insertBuiltUnitMoveCommand;

	// Damage behaviour
	/// unit pieces flying off (usually on death)
	float debrisDamage;

	/* FIXME: ideally things like debris / forest fire AoE would also
	 * be configurable, but it would be best to implement it as a fake
	 * weapon def as opposed to a loose collection of modrules. */

	// Reclaim behaviour
	/// 0 = 1 reclaimer per feature max, otherwise unlimited
	int multiReclaim;
	/// 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks
	int reclaimMethod;
	/// 0 = Revert to wireframe and gradual reclaim, 1 = Subtract HP and give full metal at end, default 1
	int reclaimUnitMethod;
	/// How much energy should reclaiming a unit cost, default 0.0
	float reclaimUnitEnergyCostFactor;
	/// How much metal should reclaim return, default 1.0
	float reclaimUnitEfficiency;
	/// How much should energy should reclaiming a feature cost, default 0.0
	float reclaimFeatureEnergyCostFactor;
	/// Does wireframe reclaim drain health? default true
	bool reclaimUnitDrainHealth;
	/// Allow reclaiming enemies? default true
	bool reclaimAllowEnemies;
	/// Allow reclaiming allies? default true
	bool reclaimAllowAllies;

	// Repair behaviour
	/// How much should energy should repair cost, default 0.0
	float repairEnergyCostFactor;

	// Resurrect behaviour
	/// How much should energy should resurrect cost, default 0.5
	float resurrectEnergyCostFactor;

	// Capture behaviour
	/// How much should energy should capture cost, default 0.0
	float captureEnergyCostFactor;


	float unitExpMultiplier;
	float unitExpPowerScale;
	float unitExpHealthScale;
	float unitExpReloadScale;
	float unitExpGrade;


	// Paralyze behaviour
	/// time it takes for paralysis to decay by 100% in seconds
	float paralyzeDeclineRate;

	/// paralyze unit depending on maxHealth? if not depending on current health, default true
	bool paralyzeOnMaxHealth;


	// Transportation behaviour
	/// If false, every unit using a tank or kbot movedef gets `cantBeTransported = true` override in its unit def. Defaults to true.
	bool transportGround;
	/// If false, every unit using a hovercraft movedef gets `cantBeTransported = true` override in its unit def. Defaults to false.
	bool transportHover;
	/// If false, every unit using a ship movedef gets `cantBeTransported = true` override in its unit def. Defaults to false.
	bool transportShip;
	/// If false, every aircraft gets `cantBeTransported = true` override in its unit def. Defaults to false.
	bool transportAir;
	/// If false, transported units cannot be manually or automatically targeted
	bool targetableTransportedUnits;

	// Fire-on-dying-units behaviour
	/// Do units fire at enemies running Killed() script?
	bool fireAtKilled;
	/// Do units fire at crashing aircraft?
	bool fireAtCrashing;

	/// 0=no flanking bonus;  1=global coords, mobile;  2=unit coords, mobile;  3=unit coords, locked
	int flankingBonusModeDefault;

	// maximum damage bonus granted by flanking bonus. Can use a number less than 1 to reduce damage.
	float flankingBonusMaxDefault;

	// minimum damage bonus granted by flnaking bonus. Can use a number less than 1 to reduce damage.
	float flankingBonusMinDefault;

	// Sensor behaviour
	/// miplevel for los
	int losMipLevel;
	/// miplevel to use for airlos
	int airMipLevel;
	/// miplevel to use for radar, sonar, seismic, jammer, ...
	int radarMipLevel;

	/// when underwater, units are not in LOS unless also in sonar
	bool requireSonarUnderWater;
	/// when unit->alwaysVisible is true, it is visible even when cloaked
	bool alwaysVisibleOverridesCloaked;
	/// ignore enemies when checking decloak if they are further than their spherical sight range
	bool decloakRequiresLineOfSight;
	/// should _all_ allyteams share the same jammermap
	bool separateJammers;


	enum {
		FEATURELOS_NONE = 0,
		FEATURELOS_GAIAONLY,
		FEATURELOS_GAIAALLIED,
		FEATURELOS_ALL,
	};

	/// feature visibility style: 0 - no LOS for features, 1 - gaia features visible
	/// 2 - gaia/allied features visible, 3 - all features visible
	int featureVisibility;

	// PFS
	/// which pathfinder system (NOP, DEFAULT/legacy, or QT) the mod will use
	int pathFinderSystem;

	/// Minimum delay after unit has made progress to next waypoint before allowing repath
	int pfRepathDelayInFrames;

	/// Minimum wait time after the the last repath before a unit is permitted to request a new one.
	int pfRepathMaxRateInFrames;

	/// Point at which a region is considered bad for raw path tracing.
	float pfRawMoveSpeedThreshold;

	/// Limits how many nodes the QTPFS pathing system is permitted to search. A smaller number
	/// improves CPU performance, but a larger number will resolve longer paths better, without
	/// needing to refresh the path.
	int qtMaxNodesSearched;

	/// Limits how many nodes the QTPFS pathing system is permitted to search, like
	/// qtMaxNodesSearched, except that it calculated based off a relative to walkable nodes
	/// in the map. The larger of this and qtMaxNodesSearched will be used.
	float qtMaxNodesSearchedRelativeToMapOpenNodes;

	/// Minimum size, in elmos, an incomplete path has to be to allow the path to be refreshed.
	/// Once the path is smaller than this distance then the system assumes the path cannot be
	/// improved further. A larger number reduces CPU usage, but also increases the chance that
	/// a unit will become trapped in a complex terrain/base setup even if there's a route that
	/// would bring the unit nearer to the goal.
	float qtRefreshPathMinDist;

	/// Enable to reduce CPU usage, but also reduce quality of resultant paths.
	bool qtLowerQualityPaths;

	float pfRawDistMult;
	float pfUpdateRateScale;

	bool enableSmoothMesh;

	/// Reduce the resolution of the smooth mesh by the divider value. Increasing the value reduces
	/// the accuracy of the smooth mesh, but improves performance. Minimum 1, default 2.
	int smoothMeshResDivider;

	/// Radius in heightmap squares to use the smooth the mesh gradients. Increasing value
	/// increases the area that a given point uses to find the local highest point, and the
	/// distance of the slope. Default is 40.
	int smoothMeshSmoothRadius;

	int quadFieldQuadSizeInElmos;

	bool allowTake;
	bool allowEnginePlayerlist;

	// how often to report wind speed/direction to wind gens
	int windChangeReportPeriod;
};

extern CModInfo modInfo;

#endif // MOD_INFO_H

