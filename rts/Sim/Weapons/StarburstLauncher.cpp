/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StarburstLauncher.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CStarburstLauncher, CWeapon, )
CR_REG_METADATA(CStarburstLauncher, (
	CR_MEMBER(uptime),
	CR_MEMBER(tracking)
))

CStarburstLauncher::CStarburstLauncher(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// null happens when loading
	if (def != nullptr) {
		tracking = weaponDef->turnrate * def->tracks;
		uptime = (def->uptime * GAME_SPEED);
	}
}


void CStarburstLauncher::FireImpl(const bool scriptCall)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 speed = ((weaponDef->fixedLauncher)? weaponDir: UpVector) * weaponDef->startvelocity;
	const float3 aimError = (gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos + UpVector * 2.0f;
	params.end = currentTargetPos;
	params.speed = speed;
	params.error = aimError;
	// Projectile TTL (params.ttl) is ignored by the Starburst Projectile and it only uses the weapondef.
	// I tried overriding the projectile TTL to 1 and it caused the starburst rocket to fly off into space
	params.ttl = (ttl > 0) ? ttl : weaponDef->flighttime;
	params.tracking = tracking;
	params.maxRange = (ttl > 0 || weaponDef->fixedLauncher) ? MAX_PROJECTILE_RANGE : range;

	WeaponProjectileFactory::LoadProjectile(params);
}

bool CStarburstLauncher::HaveFreeLineOfFire(const float3 srcPos, const float3 tgtPos, const SWeaponTarget& trg) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (!TraceRay::TestCone(srcPos, weaponDef->fixedLauncher? weaponDir: UpVector, 100.0f, 0.0f, owner->allyteam, avoidFlags, owner));
}

float CStarburstLauncher::GetRange2D(float boost, float ydiff) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return boost + range + (ydiff * weaponDef->heightmod);
}
