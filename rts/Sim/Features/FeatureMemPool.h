/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATURE_MEMPOOL_H
#define FEATURE_MEMPOOL_H

#include "Feature.h"

#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"

#if (defined(__x86_64) || defined(__x86_64__) || defined(_M_X64))
typedef StaticMemPoolT<MAX_FEATURES, CFeature> FeatureMemPool;
#else
typedef FixedDynMemPoolT<MAX_FEATURES / 1000, MAX_FEATURES / 32, CFeature> FeatureMemPool;
#endif

extern FeatureMemPool featureMemPool;

#endif
