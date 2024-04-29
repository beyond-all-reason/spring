/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_MEMPOOL_H
#define UNIT_MEMPOOL_H

#include "UnitTypes/Builder.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"

#if (defined(__x86_64) || defined(__x86_64__) || defined(_M_X64))
// CBuilder is (currently) the largest derived unit-type
typedef StaticMemPoolT<MAX_UNITS, CBuilder> UnitMemPool;
#else
typedef FixedDynMemPoolT<MAX_UNITS / 1000, MAX_UNITS / 32, CBuilder> UnitMemPool;
#endif

extern UnitMemPool unitMemPool;

#endif

