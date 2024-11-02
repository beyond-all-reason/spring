/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "EmgProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CEmgProjectile, CWeaponProjectile, )

CR_REG_METADATA(CEmgProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(intensity),
	CR_MEMBER(color)
))


CEmgProjectile::CEmgProjectile(const ProjectileParams& params): CWeaponProjectile(params)
{
	RECOIL_DETAILED_TRACY_ZONE;
	projectileType = WEAPON_EMG_PROJECTILE;

	if (weaponDef != nullptr) {
		SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);
		drawRadius = weaponDef->size;

		intensity = weaponDef->intensity;
		color = weaponDef->visuals.color;

		castShadow = weaponDef->visuals.castShadow;
	} else {
		intensity = 0.0f;
	}
}

void CEmgProjectile::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// disable collisions when ttl reaches 0 since the
	// projectile will travel far past its range while
	// fading out
	checkCol &= (ttl >= 0);
	deleteMe |= (intensity <= 0.0f);

	pos += (speed * (1 - luaMoveCtrl));

	if (ttl <= 0) {
		// fade out over the next 10 frames at most
		intensity -= 0.1f;
		intensity = std::max(intensity, 0.0f);
	} else {
		explGenHandler.GenExplosion(cegID, pos, speed, ttl, intensity, 0.0f, owner(), nullptr);
	}

	UpdateGroundBounce();
	UpdateInterception();

	--ttl;
}

void CEmgProjectile::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!validTextures[0])
		return;

	UpdateWeaponAnimParams();

	const uint8_t col[4] {
		(uint8_t)(color.x * intensity * 255),
		(uint8_t)(color.y * intensity * 255),
		(uint8_t)(color.z * intensity * 255),
		(uint8_t)(          intensity * 255)
	};

	const auto* tex = weaponDef->visuals.texture1;

	AddWeaponEffectsQuad<1>(
		{ drawPos - camera->GetRight() * drawRadius - camera->GetUp() * drawRadius, tex->xstart, tex->ystart, col },
		{ drawPos + camera->GetRight() * drawRadius - camera->GetUp() * drawRadius, tex->xend,   tex->ystart, col },
		{ drawPos + camera->GetRight() * drawRadius + camera->GetUp() * drawRadius, tex->xend,   tex->yend,   col },
		{ drawPos - camera->GetRight() * drawRadius + camera->GetUp() * drawRadius, tex->xstart, tex->yend,   col }
	);
}

int CEmgProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaMoveCtrl)
		return 0;

	const float3 rdir = (pos - shieldPos).Normalize();

	if (rdir.dot(speed) < shieldMaxSpeed) {
		SetVelocityAndSpeed(speed + (rdir * shieldForce));
		return 2;
	}

	return 0;
}

int CEmgProjectile::GetProjectilesCount() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return 1 * validTextures[0];
}
