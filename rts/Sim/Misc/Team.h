/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEAM_H
#define TEAM_H

#include <string>
#include <vector>
#include <list>

#include "TeamBase.h"
#include "TeamStatistics.h"
#include "Sim/Misc/Resource.h"
#include "System/Color.h"
#include "ExternalAI/SkirmishAIKey.h"
#include "Lua/LuaRulesParams.h"

class CUnit;

struct EconomyFlowSnapshot{
	SResourcePack fixedIncome, proratableIncome;	// unconditional income
	SResourcePack proratableExpense; // proratable expense not impacted by other resource availability
};

class CTeam : public TeamBase
{
	CR_DECLARE_DERIVED(CTeam)
public:
	CTeam();

	void ResetResourceState();
	void SlowUpdate();

	const SResourcePack GetResources() const {
		SResourcePack result;

		for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
			if (res[i] <= resStorage[i])
				result[i] = res[i];
			else
				result[i] = resStorage[i];
		}

		return result;
	}

	bool HaveResources(const SResourcePack& amount) const;
	void AddResources(SResourcePack res, bool useIncomeMultiplier = true);
	bool UseResources(const SResourcePack& res);

	void AddMetal(float amount, bool useIncomeMultiplier = true);
	void AddEnergy(float amount, bool useIncomeMultiplier = true);
	bool UseEnergy(float amount);
	bool UseMetal(float amount);

	void GiveEverythingTo(const unsigned toTeam);

	void Died(bool normalDeath = true);
	void AddPlayer(int playerNum);
	void KillAIs();

	void UpdateControllerName() override;

	void SetDefaultStartPos();
	void ClampStartPosInStartBox(float3* pos) const;

	void SetMaxUnits(unsigned int n) { maxUnits = n; }
	unsigned int GetMaxUnits() const { return maxUnits; }
	unsigned int GetNumUnits() const { return numUnits; }
	bool AtUnitLimit() const { return (numUnits >= maxUnits); }

	const TeamStatistics& GetCurrentStats() const { return statHistory.back(); }
	      TeamStatistics& GetCurrentStats()       { return statHistory.back(); }

	void recordFlowEcoPull(SResourcePack pull) {
		flowEcoPull += pull;
		flowEcoFullPull += pull;
		resPull += pull;
	}

	void applyExcessToShared();

	CTeam& operator = (const TeamBase& base) {
		TeamBase::operator = (base);
		return *this;
	}

	enum AddType {
		AddBuilt,
		AddCaptured,
		AddGiven
	};

	enum RemoveType {
		RemoveDied,
		RemoveCaptured,
		RemoveGiven
	};

	void AddUnit(CUnit* unit, AddType type);
	void RemoveUnit(CUnit* unit, RemoveType type);

public:
	int teamNum;
	unsigned int numUnits; // number of units this team controls
	unsigned int maxUnits; // maximum number of units this team can control

	bool isDead;
	bool gaia;

	SResourcePack res;
	SResourcePack resStorage;

	SResourcePack resPull,    resPrevPull;
	SResourcePack resIncome,  resPrevIncome;
	SResourcePack resExpense, resPrevExpense;
	SResourcePack resShare;
	SResourcePack resDelayedShare; //< excess that might be shared next SlowUpdate
	SResourcePack resSent,     resPrevSent;
	SResourcePack resReceived, resPrevReceived;
	SResourcePack resPrevExcess;

	// New Flow Eco Values

	EconomyFlowSnapshot resCurrent, resNext;
	SResourcePack flowEcoPull, flowEcoFullPull;
	SResourcePack resNextIncome;	// Resources created become available in the back frame.
	SResourcePack flowEcoReservedSupply; // Resource reserved exclusively for flow economy.
	float prorationRates[SResourcePack::MAX_RESOURCES+1];
	//SResourcePack resProrationRate;
	//float minResProrationRate;
	// --

	int nextHistoryEntry;
	std::vector<TeamStatistics> statHistory;

	/// mod controlled parameters
	LuaRulesParams::Params  modParams;

	/// unsynced
	float highlight;
};

#endif /* TEAM_H */
