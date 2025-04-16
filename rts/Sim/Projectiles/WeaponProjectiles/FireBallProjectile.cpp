#include "FireBallProjectile.h"
/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FireBallProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/SpringMath.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CFireBallProjectile, CWeaponProjectile, )

CR_REG_METADATA(CFireBallProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(sparks),
	CR_MEMBER(numSparks)
))


CR_BIND(CFireBallProjectile::Spark, )
CR_REG_METADATA_SUB(CFireBallProjectile, Spark, (
	CR_MEMBER(pos),
	CR_MEMBER(speed),
	CR_MEMBER(size),
	CR_MEMBER(ttl)
))


CFireBallProjectile::CFireBallProjectile(const ProjectileParams& params): CWeaponProjectile(params)
{
	RECOIL_DETAILED_TRACY_ZONE;
	projectileType = WEAPON_FIREBALL_PROJECTILE;

	if (weaponDef != nullptr) {
		SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);
		drawRadius = weaponDef->size;
		castShadow = weaponDef->visuals.castShadow;
	}

	validTextures[1] = IsValidTexture(projectileDrawer->explotex);
	validTextures[2] = IsValidTexture(projectileDrawer->dguntex);
	validTextures[0] = validTextures[1] || validTextures[2];
}

void CFireBallProjectile::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!validTextures[0])
		return;

	UpdateWeaponAnimParams();

	unsigned char col[4] = {255, 150, 100, 1};

	const float3 interPos = mix(pos, drawPos, checkCol);
	const float size = drawRadius;

	const unsigned int numFire = std::min(10u, numSparks);
	const unsigned int maxCol = mix(numFire, 10u, checkCol);

	if (validTextures[1])
	for (unsigned int i = 0; i < numSparks; i++) {
		col[0] = (numSparks - i) * 12;
		col[1] = (numSparks - i) *  6;
		col[2] = (numSparks - i) *  4;

		const auto* ept = projectileDrawer->explotex;
		AddWeaponEffectsQuad<1>(
			{ sparks[i].pos - camera->GetRight() * sparks[i].size - camera->GetUp() * sparks[i].size, ept->xstart, ept->ystart, col },
			{ sparks[i].pos + camera->GetRight() * sparks[i].size - camera->GetUp() * sparks[i].size, ept->xend  , ept->ystart, col },
			{ sparks[i].pos + camera->GetRight() * sparks[i].size + camera->GetUp() * sparks[i].size, ept->xend  , ept->yend  , col },
			{ sparks[i].pos - camera->GetRight() * sparks[i].size + camera->GetUp() * sparks[i].size, ept->xstart, ept->yend  , col }
		);
	}

	if (validTextures[2])
	for (unsigned int i = 0; i < numFire; i++) {
		col[0] = (maxCol - i) * 25;
		col[1] = (maxCol - i) * 15;
		col[2] = (maxCol - i) * 10;
		const auto* dgt = projectileDrawer->dguntex;
		AddWeaponEffectsQuad<2>(
			{ interPos - (speed * 0.5f * i) - camera->GetRight() * size - camera->GetUp() * size, dgt->xstart, dgt->ystart, col },
			{ interPos - (speed * 0.5f * i) + camera->GetRight() * size - camera->GetUp() * size, dgt->xend ,  dgt->ystart, col },
			{ interPos - (speed * 0.5f * i) + camera->GetRight() * size + camera->GetUp() * size, dgt->xend ,  dgt->yend  , col },
			{ interPos - (speed * 0.5f * i) - camera->GetRight() * size + camera->GetUp() * size, dgt->xstart, dgt->yend  , col }
		);
	}
}

void CFireBallProjectile::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (checkCol) {
		pos += (speed * (1 - luaMoveCtrl));
		speed.y += (mygravity * weaponDef->gravityAffected * (1 - luaMoveCtrl));

		checkCol = !(weaponDef->noExplode && TraveledRange());

		EmitSpark();
	} else {
		deleteMe |= (numSparks == 0);
	}

	TickSparks();
	UpdateGroundBounce();
	UpdateInterception();
}


void CFireBallProjectile::EmitSpark()
{
	RECOIL_DETAILED_TRACY_ZONE;
	constexpr unsigned int maxSparks = sizeof(sparks) / sizeof(sparks[0]);

	if (numSparks == maxSparks)
		return;

	Spark& spark = sparks[numSparks++];

	const float x = guRNG.NextFloat() - 0.5f;
	const float y = guRNG.NextFloat() - 0.5f;
	const float z = guRNG.NextFloat() - 0.5f;

	spark.speed = float3(speed * 0.95f) + float3(x, y, z);
	spark.pos = pos - speed * (guRNG.NextFloat() + 3.0f) + spark.speed * 3.0f;
	spark.size = 5.0f;
	spark.ttl = maxSparks;
}

void CFireBallProjectile::TickSparks()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (unsigned int i = 0; i < numSparks; ) {
		if ((--sparks[i].ttl) <= 0) {
			sparks[i] = sparks[--numSparks];
			continue;
		}

		sparks[i].pos += (sparks[i].speed * checkCol);
		sparks[i].speed *= 0.95f;

		i++;
	}

	explGenHandler.GenExplosion(cegID, pos, speed, ttl, (numSparks > 0)? sparks[0].size: 0.0f, 0.0f, owner(), nullptr);
}


void CFireBallProjectile::Collision()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CWeaponProjectile::Collision();
	deleteMe = false;
}

int CFireBallProjectile::GetProjectilesCount() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return
		validTextures[1] * numSparks +
		validTextures[2] * std::min(10u, numSparks);
}

