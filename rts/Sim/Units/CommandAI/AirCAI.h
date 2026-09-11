/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "MobileCAI.h"

class CUnit;
class CFeature;
class CStrafeAirMoveType;
struct Command;
class AAirMoveType;

class CAirCAI : public CMobileCAI
{
public:
	CR_DECLARE_DERIVED(CAirCAI)
	CAirCAI(CUnit* owner);
	CAirCAI();

	int GetDefaultCmd(const CUnit* pointed, const CFeature* feature) override;
	void SlowUpdate() override;
	void GiveCommandReal(const Command& c, bool fromSynced = true) override;
	void AddUnit(CUnit* unit);
	void FinishCommand(CommandEndReason reason = CommandEndReason::Completed) override;
	void BuggerOff(const float3& pos, float radius) override;
//	void StopMove();

	void ExecuteGuard(Command& c) override;
	void ExecuteAreaAttack(Command& c);
	void ExecuteAttack(Command& c) override;
	void ExecuteFight(Command& c) override;
	void ExecuteMove(Command& c) override;

	bool IsValidTarget(const CUnit* enemy, CWeapon* weapon) const override;

private:
	bool AirAutoGenerateTarget(AAirMoveType*);
	bool SelectNewAreaAttackTargetOrPos(const Command& ac) override;
	void PushOrUpdateReturnFight() {
		CCommandAI::PushOrUpdateReturnFight(commandPos1, commandPos2);
	}

	float3 basePos;
	float3 baseDir;

	int activeCommand;
	int targetAge;
//	unsigned int patrolTime;

	int lastPC1;
	int lastPC2;
};
