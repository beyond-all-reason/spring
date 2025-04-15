/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_MEMPOOL_H
#define PROJECTILE_MEMPOOL_H

#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"
#include "System/SpringMath.h"

#include "Sim/Projectiles/WeaponProjectiles/StarburstProjectile.h"

static constexpr size_t PMP_ALIGN = 16; // smallest that fits the needs of all the various projectile types
static constexpr size_t PMP_S = AlignUp(sizeof(CStarburstProjectile), PMP_ALIGN); //biggest in size

#if (defined(__x86_64) || defined(__x86_64__) || defined(_M_X64))
typedef StaticMemPool<MAX_PROJECTILES, PMP_S, PMP_ALIGN> ProjMemPool;
#else
typedef FixedDynMemPool<PMP_S, MAX_PROJECTILES / 2000, MAX_PROJECTILES / 64, PMP_ALIGN> ProjMemPool;
#endif

extern ProjMemPool projMemPool;

#endif

