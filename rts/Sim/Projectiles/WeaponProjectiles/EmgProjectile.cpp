#include "EmgProjectile.h"
/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "EmgProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Env/Particles/Generators/ParticleGeneratorHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Misc/GlobalSynced.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CEmgProjectile, CWeaponProjectile, )

CR_REG_METADATA(CEmgProjectile, (
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(intensity),
	CR_MEMBER(color),
	CR_MEMBER(pgOffset)
))


CEmgProjectile::CEmgProjectile(const ProjectileParams& params): CWeaponProjectile(params)
{
	RECOIL_DETAILED_TRACY_ZONE;
	projectileType = WEAPON_EMG_PROJECTILE;

	SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);
	drawRadius = weaponDef->size;

	intensity = weaponDef->intensity;
	color = weaponDef->visuals.color;

	castShadow = weaponDef->visuals.castShadow;

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<EmgParticleGenerator>();
	
	pgOffset = pg.Add({
		.pos = drawPos,
		.radius = drawRadius,
		.animParams = animParams,
		.color = SColor(color.x, color.x, color.z, 1.0f),
		.rotParams = rotParams,
		.drawOrder = drawOrder,
		.texCoord = *weaponDef->visuals.texture1,
	});
}

CEmgProjectile::~CEmgProjectile()
{
	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<EmgParticleGenerator>();
	pg.Del(pgOffset);
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

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<EmgParticleGenerator>();

	const auto [token, data] = pg.Get(pgOffset);
	data->pos = pos;
	data->color = SColor(color.x, color.x, color.z, 1.0f) * intensity;
	data->animParams = animParams;
	data->rotParams = rotParams;
}

void CEmgProjectile::Draw()
{
	/*
	RECOIL_DETAILED_TRACY_ZONE;
	if (!validTextures[0])
		return;

	const uint8_t col[4] {
		(uint8_t)(color.x * intensity * 255),
		(uint8_t)(color.y * intensity * 255),
		(uint8_t)(color.z * intensity * 255),
		(uint8_t)(          intensity * 255)
	};

	AddEffectsQuad(
		{ drawPos - camera->GetRight() * drawRadius - camera->GetUp() * drawRadius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, col },
		{ drawPos + camera->GetRight() * drawRadius - camera->GetUp() * drawRadius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->ystart, col },
		{ drawPos + camera->GetRight() * drawRadius + camera->GetUp() * drawRadius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   col },
		{ drawPos - camera->GetRight() * drawRadius + camera->GetUp() * drawRadius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->yend,   col }
	);
	*/
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
