/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FlameProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CFlameProjectile, CWeaponProjectile, )

CR_REG_METADATA(CFlameProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(spread),
	CR_MEMBER(curTime),
	CR_MEMBER(physLife),
	CR_MEMBER(invttl)
))


CFlameProjectile::CFlameProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, curTime(0.0f)
	, physLife(0.0f)
	, invttl(1.0f / ttl)
	, spread(params.spread)
{
	projectileType = WEAPON_FLAME_PROJECTILE;

	if (weaponDef != nullptr) {
		SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);
		
		drawRadius = weaponDef->size;
		physLife = 1.0f / weaponDef->duration;
	}
}

void CFlameProjectile::Collision()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3& norm = CGround::GetNormal(pos.x, pos.z);
	const float ns = speed.dot(norm);

	SetVelocityAndSpeed(speed - (norm * ns));
	SetPosition(pos + UpVector * 0.05f);

	curTime += 0.05f;
}

void CFlameProjectile::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!luaMoveCtrl) {
		SetPosition(pos + speed);
		UpdateGroundBounce();
		SetVelocityAndSpeed(speed + spread);
	}

	UpdateInterception();
	drawRadius += weaponDef->sizeGrowth;

	curTime = std::min(curTime + invttl, 1.0f);
	checkCol &= (curTime <= physLife);
	deleteMe |= (curTime >= 1.0f);

	explGenHandler.GenExplosion(cegID, pos, speed, curTime, 0.0f, 0.0f, owner(), nullptr);
}

void CFlameProjectile::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!validTextures[0])
		return;

	UpdateWeaponAnimParams();

	unsigned char col[4];
	weaponDef->visuals.colorMap->GetColor(col, curTime);

	AddWeaponEffectsQuad<1>(
		{ drawPos - camera->GetRight() * radius - camera->GetUp() * radius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, col },
		{ drawPos + camera->GetRight() * radius - camera->GetUp() * radius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->ystart, col },
		{ drawPos + camera->GetRight() * radius + camera->GetUp() * radius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   col },
		{ drawPos - camera->GetRight() * radius + camera->GetUp() * radius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->yend,   col }
	);
}

int CFlameProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaMoveCtrl)
		return 0;

	const float3 rdir = (pos - shieldPos).Normalize();

	if (rdir.dot(speed) >= shieldMaxSpeed)
		return 0;

	SetVelocityAndSpeed(speed + (rdir * shieldForce));
	return 2;
}

int CFlameProjectile::GetProjectilesCount() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return 1 * validTextures[0];
}
