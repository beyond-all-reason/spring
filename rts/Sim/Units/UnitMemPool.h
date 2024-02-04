/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_MEMPOOL_H
#define UNIT_MEMPOOL_H

#include "UnitTypes/Building.h"
#include "UnitTypes/Factory.h"
#include "UnitTypes/ExtractorBuilding.h"

#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"

static constexpr auto BiggestCUnitDerivative = std::max({ sizeof(CBuilding), sizeof(CFactory), sizeof(CExtractorBuilding) });

#if (defined(__x86_64) || defined(__x86_64__) || defined(_M_X64))
typedef StaticMemPool<MAX_UNITS, BiggestCUnitDerivative> UnitMemPool;
#else
typedef FixedDynMemPool<BiggestCUnitDerivative, MAX_UNITS / 1000, MAX_UNITS / 32> UnitMemPool;
#endif

extern UnitMemPool unitMemPool;

#endif

