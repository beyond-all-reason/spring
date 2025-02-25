/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_MEMPOOL_H
#define UNIT_MEMPOOL_H

#include "UnitTypes/Builder.h"
#include "UnitTypes/Building.h"
#include "UnitTypes/ExtractorBuilding.h"
#include "UnitTypes/Factory.h"

#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"

/* Needs manually listing all derived classes to find the biggest,
 * but we probably can't do better, at least without reflection */
union LargestDerivedFromCUnit {
	CBuilder builder;
	CFactory factory;
	CBuilding building;
	CExtractorBuilding extractorBuilding;
};

#if (defined(__x86_64) || defined(__x86_64__) || defined(_M_X64))
typedef StaticMemPoolT<MAX_UNITS, LargestDerivedFromCUnit> UnitMemPool;
#else
typedef FixedDynMemPoolT<MAX_UNITS / 1000, MAX_UNITS / 32, LargestDerivedFromCUnit> UnitMemPool;
#endif

extern UnitMemPool unitMemPool;

#endif

