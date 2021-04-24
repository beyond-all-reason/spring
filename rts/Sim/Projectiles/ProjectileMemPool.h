/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_MEMPOOL_H
#define PROJECTILE_MEMPOOL_H

#include "Sim/Projectiles/WeaponProjectiles/StarburstProjectile.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"

static constexpr int PROJ_MEMPOOL_S = sizeof(CStarburstProjectile); //CStarburstProjectile is biggest in size
#if (defined(__x86_64) || defined(__x86_64__))
typedef StaticMemPool<MAX_PROJECTILES, PROJ_MEMPOOL_S> ProjMemPool;
#else
typedef FixedDynMemPool<PROJ_MEMPOOL_S, MAX_PROJECTILES / 2000, MAX_PROJECTILES / 64> ProjMemPool;
#endif

extern ProjMemPool projMemPool;

#endif

