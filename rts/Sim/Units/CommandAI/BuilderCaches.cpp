/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "Sim/Units/CommandAI/BuilderCaches.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"

// not adding to creg, should repopulate itself
spring::unordered_set<int> CBuilderCaches::reclaimers;
spring::unordered_set<int> CBuilderCaches::featureReclaimers;
spring::unordered_set<int> CBuilderCaches::resurrecters;

std::vector<int> CBuilderCaches::removees;

void CBuilderCaches::InitStatic()
{
	spring::clear_unordered_set(reclaimers);
	spring::clear_unordered_set(featureReclaimers);
	spring::clear_unordered_set(resurrecters);
}

void CBuilderCaches::AddUnitToReclaimers(CUnit* unit) { reclaimers.insert(unit->id); }
void CBuilderCaches::RemoveUnitFromReclaimers(CUnit* unit) { reclaimers.erase(unit->id); }

void CBuilderCaches::AddUnitToFeatureReclaimers(CUnit* unit) { featureReclaimers.insert(unit->id); }
void CBuilderCaches::RemoveUnitFromFeatureReclaimers(CUnit* unit) { featureReclaimers.erase(unit->id); }

void CBuilderCaches::AddUnitToResurrecters(CUnit* unit) { resurrecters.insert(unit->id); }
void CBuilderCaches::RemoveUnitFromResurrecters(CUnit* unit) { resurrecters.erase(unit->id); }


/**
 * Checks if a unit is being reclaimed by a friendly con.
 *
 * We assume that there will not be a lot of reclaimers, because performance
 * would suck if there were. Ideally, reclaimers should be assigned on a
 * per-unit basis, but this requires tracking of deaths, which albeit
 * already done, is not exactly simple to follow.
 *
 * TODO easy: store reclaiming units per allyteam
 * TODO harder: update reclaimers as they start/finish reclaims and/or die
 */
bool CBuilderCaches::IsUnitBeingReclaimed(const CUnit* unit, const CUnit* friendUnit)
{
	bool retval = false;

	removees.clear();
	removees.reserve(reclaimers.size());

	for (auto it = reclaimers.begin(); it != reclaimers.end(); ++it) {
		const CUnit* u = unitHandler.GetUnit(*it);
		const CCommandAI* cai = u->commandAI;
		const CCommandQueue& cq = cai->commandQue;

		if (cq.empty()) {
			removees.push_back(u->id);
			continue;
		}
		const Command& c = cq.front();
		if (c.GetID() != CMD_RECLAIM || (c.GetNumParams() != 1 && c.GetNumParams() != 5)) {
			removees.push_back(u->id);
			continue;
		}
		const int cmdUnitId = (int)c.GetParam(0);
		if (cmdUnitId == unit->id && (friendUnit == nullptr || teamHandler.Ally(friendUnit->allyteam, u->allyteam))) {
			retval = true;
			break;
		}
	}

	for (auto it = removees.begin(); it != removees.end(); ++it)
		RemoveUnitFromReclaimers(unitHandler.GetUnit(*it));

	return retval;
}



bool CBuilderCaches::IsFeatureBeingReclaimed(int featureId, const CUnit* friendUnit)
{
	bool retval = false;

	removees.clear();
	removees.reserve(featureReclaimers.size());

	for (auto it = featureReclaimers.begin(); it != featureReclaimers.end(); ++it) {
		const CUnit* u = unitHandler.GetUnit(*it);
		const CCommandAI* cai = u->commandAI;
		const CCommandQueue& cq = cai->commandQue;

		if (cq.empty()) {
			removees.push_back(u->id);
			continue;
		}
		const Command& c = cq.front();
		if (c.GetID() != CMD_RECLAIM || (c.GetNumParams() != 1 && c.GetNumParams() != 5)) {
			removees.push_back(u->id);
			continue;
		}
		const int cmdFeatureId = (int)c.GetParam(0);
		if ((cmdFeatureId - unitHandler.MaxUnits()) == featureId && (friendUnit == nullptr || teamHandler.Ally(friendUnit->allyteam, u->allyteam))) {
			retval = true;
			break;
		}
	}

	for (auto it = removees.begin(); it != removees.end(); ++it)
		RemoveUnitFromFeatureReclaimers(unitHandler.GetUnit(*it));

	return retval;
}


bool CBuilderCaches::IsFeatureBeingResurrected(int featureId, const CUnit* friendUnit)
{
	bool retval = false;

	removees.clear();
	removees.reserve(resurrecters.size());

	for (auto it = resurrecters.begin(); it != resurrecters.end(); ++it) {
		const CUnit* u = unitHandler.GetUnit(*it);
		const CCommandAI* cai = u->commandAI;
		const CCommandQueue& cq = cai->commandQue;

		if (cq.empty()) {
			removees.push_back(u->id);
			continue;
		}
		const Command& c = cq.front();
		if (c.GetID() != CMD_RESURRECT || c.GetNumParams() != 1) {
			removees.push_back(u->id);
			continue;
		}
		const int cmdFeatureId = (int)c.GetParam(0);
		if ((cmdFeatureId - unitHandler.MaxUnits()) == featureId && (friendUnit == nullptr || teamHandler.Ally(friendUnit->allyteam, u->allyteam))) {
			retval = true;
			break;
		}
	}

	for (auto it = removees.begin(); it != removees.end(); ++it)
		RemoveUnitFromResurrecters(unitHandler.GetUnit(*it));

	return retval;
}



