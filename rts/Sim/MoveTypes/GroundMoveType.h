/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDMOVETYPE_H
#define GROUNDMOVETYPE_H

#include <array>
#include <tuple>

#include "MoveType.h"
#include "Sim/Path/IPathController.h"
#include "System/Sync/SyncedFloat3.h"

struct UnitDef;
struct MoveDef;
class CSolidObject;

class CGroundMoveType : public AMoveType
{
	CR_DECLARE_DERIVED(CGroundMoveType)

public:
	CGroundMoveType(CUnit* owner);
	~CGroundMoveType();

	static constexpr int HEADING_CHANGED_NONE = 0;
	static constexpr int HEADING_CHANGED_MOVE = 1;
	static constexpr int HEADING_CHANGED_STOP = 2;
	static constexpr int HEADING_CHANGED_STUN = 3;

	struct MemberData {
		std::array<std::pair<unsigned int,  bool*>, 3>  bools;
		std::array<std::pair<unsigned int, short*>, 1> shorts;
		std::array<std::pair<unsigned int, float*>, 9> floats;
	};

	void PostLoad();
	void* GetPreallocContainer() { return owner; }  // creg

	bool Update() override;
	void SlowUpdate() override;

	// Decide how the unit should move to carry out obsctacle avoidance and path following decisions. Actual movement
	// must be deferred to UpdateUnitPosition() because unit heading, speed, and position will impact other
	// units' obsctacle avoidance decision making.
	// This is should be MT safe.
	void UpdateTraversalPlan();

	// Update the unit's movement according to obsctacle avoidance and path following decisions from UpdateTraversalPlan().
	// This is should be MT safe.
	void UpdateUnitPosition();

	// Resolves post UpdateTraversalPlan() and UpdateUnitPosition() tasks that must be carried out in a single
	// thread.
	void UpdatePreCollisions();

	// Carry out unit collision detections and resolution. Actual movement will be carried in Update() later because
	// moving units will impact further collisions during these checks. All collision events have to be recorded in the
	// appropriate GroundMoveSystemComponent event list for the current thread. These events will be issued afterwards,
	// single threaded, before Update() is called. This is to ensure units responding to collision events are
	// responding to the collision as the collision state, not post collision state.
	// This is should be MT safe.
	void UpdateCollisionDetections();


	void UpdateObstacleAvoidance();

	void StartMovingRaw(const float3 moveGoalPos, float moveGoalRadius) override;
	void StartMoving(float3 pos, float moveGoalRadius) override;
	void StartMoving(float3 pos, float moveGoalRadius, float speed) override { StartMoving(pos, moveGoalRadius); }
	void StopMoving(bool callScript = false, bool hardStop = false, bool cancelRaw = false) override;
	bool IsMovingTowards(const float3& pos, float radius, bool checkProgress) const override {
		return (goalPos == pos * XZVector && goalRadius == radius && (!checkProgress || progressState == Active));
	}

	void KeepPointingTo(float3 pos, float distance, bool aggressive) override;
	void KeepPointingTo(CUnit* unit, float distance, bool aggressive) override;

	void TestNewTerrainSquare();
	bool CanApplyImpulse(const float3&) override;
	void LeaveTransport() override;
	void Connect() override;
	void Disconnect() override;

	void InitMemberPtrs(MemberData* memberData);
	bool SetMemberValue(unsigned int memberHash, void* memberValue) override;

	bool OnSlope(float minSlideTolerance);
	bool IsReversing() const override { return reversing; }
	bool IsPushResistant() const override { return pushResistant; }
	bool IsPushResitanceBlockActive() const override { return pushResistanceBlockActive; }
	bool WantToStop() const { return (pathID == 0 && (!useRawMovement || atEndOfPath)); }

	void TriggerSkipWayPoint() {
		earlyCurrWayPoint.y = -2.0f;
	}
	void TriggerCallArrived() {
		atEndOfPath = true;
		atGoal = true;
		pathingArrived = true;
	}


	float GetTurnRate() const { return turnRate; }
	float GetTurnSpeed() const { return turnSpeed; }
	float GetTurnAccel() const { return turnAccel; }

	float GetAccRate() const { return accRate; }
	float GetDecRate() const { return decRate; }
	float GetMyGravity() const { return myGravity; }
	float GetOwnerRadius() const { return ownerRadius; }

	float GetMaxReverseSpeed() const { return maxReverseSpeed; }
	float GetWantedSpeed() const { return wantedSpeed; }
	float GetCurrentSpeed() const { return currentSpeed; }
	float GetDeltaSpeed() const { return deltaSpeed; }

	float GetCurrWayPointDist() const { return currWayPointDist; }
	float GetPrevWayPointDist() const { return prevWayPointDist; }
	float GetGoalRadius(float s = 0.0f) const override { return (goalRadius + extraRadius * s); }

	unsigned int GetPathID() const { return pathID; }

	const SyncedFloat3& GetCurrWayPoint() const { return currWayPoint; }
	const SyncedFloat3& GetNextWayPoint() const { return nextWayPoint; }

	const float3& GetFlatFrontDir() const { return flatFrontDir; }
	const float3& GetGroundNormal(const float3&) const;
	float GetGroundHeight(const float3&) const;

	void SyncWaypoints() {
		// Synced vars trigger a checksum update on change, which is expensive so we should check
		// that there has been a change before triggering an update to the checksum.
		if (!currWayPoint.bitExactEquals(earlyCurrWayPoint))
			currWayPoint = earlyCurrWayPoint;
		if (!nextWayPoint.bitExactEquals(earlyNextWayPoint))
			nextWayPoint = earlyNextWayPoint;
	}
	unsigned int GetPathId() { return pathID; }

	float GetTurnRadius() {
		const float absTurnSpeed = std::max(0.0001f, math::fabs(turnRate));
		const float framesToTurn = SPRING_CIRCLE_DIVS / absTurnSpeed;
		return std::max((currentSpeed * framesToTurn) * math::INVPI2, currentSpeed * 1.05f);
	}

	bool IsAtGoal() const override { return atGoal; }
	void OwnerMayBeStuck() { forceStaticObjectCheck = true; };
	void SetMtJobId(int _jobId) { jobId = _jobId; }

private:
	float3 GetObstacleAvoidanceDir(const float3& desiredDir);
	float3 Here() const;

	// Start skidding if the angle between the vel and dir vectors is >arccos(2*sqSkidSpeedMult-1)/2
	bool StartSkidding(const float3& vel, const float3& dir) const { return ((SignedSquare(vel.dot(dir)) + 0.01f) < (vel.SqLength() * sqSkidSpeedMult)); }
	bool StopSkidding(const float3& vel, const float3& dir) const { return ((SignedSquare(vel.dot(dir)) + 0.01f) >= (vel.SqLength() * sqSkidSpeedMult)); }
	bool StartFlying(const float3& vel, const float3& dir) const { return (vel.dot(dir) > 0.2f); }
	bool StopFlying(const float3& vel, const float3& dir) const { return (vel.dot(dir) <= 0.2f); }

	float Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal = 0.0f);

	unsigned int GetNewPath();

	void SetNextWayPoint(int thread);
	bool CanSetNextWayPoint(int thread);
	void ReRequestPath(bool forceRequest);

	void StartEngine(bool callScript);
	void StopEngine(bool callScript, bool hardStop = false);

	void Arrived(bool callScript);
	void Fail(bool callScript);

	void HandleObjectCollisions();
	bool HandleStaticObjectCollision(
		CUnit* collider,
		CSolidObject* collidee,
		const MoveDef* colliderMD,
		const float colliderRadius,
		const float collideeRadius,
		const float3& separationVector,
		bool canRequestPath,
		bool checkYardMap,
		bool checkTerrain,
		int curThread
	);

    void HandleUnitCollisions(
        CUnit *collider,
        const float3 &colliderParams,
        const UnitDef *colliderUD,
        const MoveDef *colliderMD,
        int curThread);
    float3 CalculatePushVector(const float3 &colliderParams, const float2 &collideeParams, const bool allowUCO, const float4 &separationVect, CUnit *collider, CUnit *collidee);
    void HandleFeatureCollisions(
        CUnit *collider,
        const float3 &colliderParams,
        const UnitDef *colliderUD,
        const MoveDef *colliderMD,
        int curThread);

public:
    void SetMainHeading();
    void ChangeSpeed(float, bool, bool = false);
	void ChangeHeading(short newHeading);
private:
	void UpdateSkid();
	void UpdateControlledDrop();
	void CheckCollisionSkid();
	void CalcSkidRot();

	void AdjustPosToWaterLine();
	bool UpdateDirectControl();
	void UpdateOwnerAccelAndHeading();
	void UpdatePos(const CUnit* unit, const float3&, float3& resultantMove, int thread) const;
	void UpdateOwnerPos(const float3&, const float3&);
	bool UpdateOwnerSpeed(float oldSpeedAbs, float newSpeedAbs, float newSpeedRaw);
	bool OwnerMoved(const short, const float3&, const float3&);
	bool FollowPath(int thread);
	bool WantReverse(const float3& wpDir, const float3& ffDir) const;
	void SetWaypointDir(const float3& cwp, const float3 &opos);

private:
	GMTDefaultPathController pathController;

	int jobId = 0;

	SyncedFloat3 currWayPoint;
	SyncedFloat3 nextWayPoint;

	float3 earlyCurrWayPoint;
	float3 earlyNextWayPoint;

	float3 waypointDir;
	float3 flatFrontDir;
	float3 lastAvoidanceDir;
	float3 mainHeadingPos;
	float3 skidRotVector;                   /// vector orthogonal to skidDir

	float turnRate = 0.1f;                  /// maximum angular speed (angular units/frame)
	float turnSpeed = 0.0f;                 /// current angular speed (angular units/frame)
	float turnAccel = 0.0f;                 /// angular acceleration (angular units/frame^2)

	float accRate = 0.01f;
	float decRate = 0.01f;
	float myGravity = 0.0f;

	float maxReverseDist = 0.0f;
	float minReverseAngle = 0.0f;
	float maxReverseSpeed = 0.0f;
	float sqSkidSpeedMult = 0.95f;

	float wantedSpeed = 0.0f;
	float currentSpeed = 0.0f;
	float deltaSpeed = 0.0f;

	float currWayPointDist = 0.0f;
	float prevWayPointDist = 0.0f;

	float goalRadius = 0.0f;                /// original radius passed to StartMoving*
	float ownerRadius = 0.0f;               /// owner MoveDef footprint radius
	float extraRadius = 0.0f;               /// max(0, ownerRadius - goalRadius) if goal-pos is valid, 0 otherwise

	float skidRotSpeed = 0.0f;              /// rotational speed when skidding (radians / (GAME_SPEED frames))
	float skidRotAccel = 0.0f;              /// rotational acceleration when skidding (radians / (GAME_SPEED frames^2))

	float3 forceFromMovingCollidees;
	float3 forceFromStaticCollidees;
	float3 resultantForces;

	unsigned int pathID = 0;
	unsigned int nextPathId = 0;
	unsigned int deletePathId = 0;

	unsigned int numIdlingUpdates = 0;      /// {in, de}creased every Update if idling is true/false and pathId != 0
	unsigned int numIdlingSlowUpdates = 0;  /// {in, de}creased every SlowUpdate if idling is true/false and pathId != 0

	short wantedHeading = 0;
	short minScriptChangeHeading = 0;       /// minimum required turn-angle before script->ChangeHeading is called

	int wantRepathFrame = std::numeric_limits<int>::min();
	int lastRepathFrame = std::numeric_limits<int>::min();
	float bestLastWaypointDist = std::numeric_limits<float>::infinity();
	float bestReattemptedLastWaypointDist = std::numeric_limits<float>::infinity();
	int setHeading = 0; // 1 = Regular (use setHeadingDir), 2 = Main
	short setHeadingDir = 0;
	short limitSpeedForTurning = 0;			/// if set, take extra care to prevent overshooting while turning for the next N waypoints.

	float oldSpeed = 0.f;
	float newSpeed = 0.f;

	bool atGoal = true;
	bool atEndOfPath = true;
	bool wantRepath = false;
	bool lastWaypoint = false;

	bool reversing = false;
	bool idling = false;
	bool pushResistant = false;
	bool pushResistanceBlockActive = false;
	bool canReverse = false;
	bool useMainHeading = false;            /// if true, turn toward mainHeadingPos until weapons[0] can TryTarget() it
	bool useRawMovement = false;            /// if true, move towards goal without invoking PFS (unrelated to MoveDef::allowRawMovement)
	bool pathingFailed = false;
	bool pathingArrived = false;
	bool positionStuck = false;
	bool forceStaticObjectCheck = false;
	bool avoidingUnits = false;
};

#endif // GROUNDMOVETYPE_H

