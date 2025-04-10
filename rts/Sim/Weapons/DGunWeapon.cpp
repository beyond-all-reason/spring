/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DGunWeapon.h"

#include "WeaponDef.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CDGunWeapon, CWeapon, )
CR_REG_METADATA(CDGunWeapon, )

void CDGunWeapon::FireImpl(const bool scriptCall)
{
	RECOIL_DETAILED_TRACY_ZONE;
	float3 dir = currentTargetPos - weaponMuzzlePos;
	dir.Normalize();

	dir += (gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.end = currentTargetPos;
	params.speed = dir * projectileSpeed;
	params.ttl = 1;

	WeaponProjectileFactory::LoadProjectile(params);
}

void CDGunWeapon::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CWeapon::Init();
	muzzleFlareSize = 1.5f;
}
