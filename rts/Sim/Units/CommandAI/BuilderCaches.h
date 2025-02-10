/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BUILDER_CACHES_H_
#define _BUILDER_CACHES_H_

#include "System/UnorderedSet.hpp"

#include <vector>

class CUnit;

class CBuilderCaches
{
public:
	/**
	 * Checks if a unit is being reclaimed by a friendly con.
	 */
	static void InitStatic();
	static bool IsUnitBeingReclaimed(const CUnit* unit, const CUnit* friendUnit = nullptr);
	static bool IsFeatureBeingReclaimed(int featureId, const CUnit* friendUnit = nullptr);
	static bool IsFeatureBeingResurrected(int featureId, const CUnit* friendUnit = nullptr);
	static spring::unordered_set<int> reclaimers;
	static spring::unordered_set<int> featureReclaimers;
	static spring::unordered_set<int> resurrecters;

	static std::vector<int> removees;

	/// fix for patrolling cons repairing/resurrecting stuff that's being reclaimed
	static void AddUnitToReclaimers(CUnit*);
	static void RemoveUnitFromReclaimers(CUnit*);

	/// fix for cons wandering away from their target circle
	static void AddUnitToFeatureReclaimers(CUnit*);
	static void RemoveUnitFromFeatureReclaimers(CUnit*);

	/// fix for patrolling cons reclaiming stuff that is being resurrected
	static void AddUnitToResurrecters(CUnit*);
	static void RemoveUnitFromResurrecters(CUnit*);
};

#endif // _BUILDER_CACHES_H_
