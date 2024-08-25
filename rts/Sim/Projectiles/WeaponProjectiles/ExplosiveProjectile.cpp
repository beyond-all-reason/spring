/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ExplosiveProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Env/Particles/Generators/ParticleGeneratorHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CExplosiveProjectile, CWeaponProjectile, )

CR_REG_METADATA(CExplosiveProjectile, (
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(invttl),
	CR_MEMBER(curTime),
	CR_MEMBER(pgOffset)
))


CExplosiveProjectile::CExplosiveProjectile(const ProjectileParams& params)
	: CWeaponProjectile(params)
	, invttl(0.0f)
	, curTime(0.0f)
	, pgOffset(-1)
{
	RECOIL_DETAILED_TRACY_ZONE;
	projectileType = WEAPON_EXPLOSIVE_PROJECTILE;

	mygravity = params.gravity;
	useAirLos = true;

	SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);
	drawRadius = weaponDef->size;
	castShadow = weaponDef->visuals.castShadow;

	if (ttl <= 0) {
		invttl = 1.0f;
	} else {
		invttl = 1.0f / ttl;
	}

	const WeaponDef::Visuals& wdVisuals = weaponDef->visuals;
	auto DefColor = SColor(wdVisuals.color.x, wdVisuals.color.y, wdVisuals.color.z, weaponDef->intensity);

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<ExplosiveParticleGenerator>();
	pgOffset = pg.Add({
		.pos = pos,
		.radius = drawRadius,

		.dir = dir,
		.drawOrder = drawOrder,

		.color0 = DefColor,
		.color1 = DefColor,
		.numStages = static_cast<uint32_t>(weaponDef->visuals.stages),
		.noGap = weaponDef->visuals.noGap,

		.alphaDecay = weaponDef->visuals.alphaDecay,
		.sizeDecay = weaponDef->visuals.sizeDecay,
		.separation = weaponDef->visuals.separation,

		.colEdge0 = 0.0f,
		.colEdge1 = 1.0f,
		.curTime = curTime,

		.texCoord = weaponDef->visuals.texture1 ? *weaponDef->visuals.texture1 : AtlasedTexture::DefaultAtlasTexture
	});
}

CExplosiveProjectile::~CExplosiveProjectile()
{
	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<ExplosiveParticleGenerator>();
	pg.Del(pgOffset);
}

void CExplosiveProjectile::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CProjectile::Update();

	if (--ttl == 0) {
		Collision();
	} else {
		if (ttl > 0)
			explGenHandler.GenExplosion(cegID, pos, speed, ttl, damages->damageAreaOfEffect, 0.0f, owner(), nullptr);
	}

	curTime += invttl;
	curTime = std::min(curTime, 1.0f);

	if (weaponDef->noExplode && TraveledRange()) {
		CProjectile::Collision();
		return;
	}

	UpdateGroundBounce();
	UpdateInterception();

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<ExplosiveParticleGenerator>();

	auto& data = pg.Get(pgOffset);

	data.pos = pos;
	data.curTime = curTime;

	if (const auto* cm = weaponDef->visuals.colorMap; cm) {
		auto [i0, i1]  = cm->GetIndices(curTime);
		data.color0   = cm->GetColor(i0);
		data.color1   = cm->GetColor(i1);
		data.colEdge0 = cm->GetColorPos(i0);
		data.colEdge1 = cm->GetColorPos(i1);
	}
}

void CExplosiveProjectile::Draw()
{
	/*
	RECOIL_DETAILED_TRACY_ZONE;
	// do not draw if a 3D model has been defined for us
	if (model != nullptr)
		return;

	if (!validTextures[0])
		return;

	uint8_t col[4] = {0};

	const WeaponDef::Visuals& wdVisuals = weaponDef->visuals;
	const AtlasedTexture* tex = wdVisuals.texture1;

	if (wdVisuals.colorMap != nullptr) {
		wdVisuals.colorMap->GetColor(col, curTime);
	} else {
		col[0] = wdVisuals.color.x    * 255;
		col[1] = wdVisuals.color.y    * 255;
		col[2] = wdVisuals.color.z    * 255;
		col[3] = weaponDef->intensity * 255;
	}

	const auto& alphaDecay = wdVisuals.alphaDecay;
	const auto& sizeDecay  = wdVisuals.sizeDecay;
	const auto& separation = wdVisuals.separation;
	const auto& noGap      = wdVisuals.noGap;
	const auto& stages     = wdVisuals.stages;
	const float invStages  = 1.0f / std::max(1, stages);

	const float3 ndir = dir * separation * 0.6f;

	for (int stage = 0; stage < stages; ++stage) {
		const float stageDecay = (stages - (stage * alphaDecay)) * invStages;
		const float stageSize  = drawRadius * (1.0f - (stage * sizeDecay));

		const float3 ydirCam  = camera->GetUp()    * stageSize;
		const float3 xdirCam  = camera->GetRight() * stageSize;
		const float3 stageGap = (noGap)? (ndir * stageSize * stage): (ndir * drawRadius * stage);
		const float3 stagePos = drawPos - stageGap;

		col[0] = stageDecay * col[0];
		col[1] = stageDecay * col[1];
		col[2] = stageDecay * col[2];
		col[3] = stageDecay * col[3];

		AddEffectsQuad(
			{ stagePos - xdirCam - ydirCam, tex->xstart, tex->ystart, col },
			{ stagePos + xdirCam - ydirCam, tex->xend,   tex->ystart, col },
			{ stagePos + xdirCam + ydirCam, tex->xend,   tex->yend,   col },
			{ stagePos - xdirCam + ydirCam, tex->xstart, tex->yend,   col }
		);
	}
	*/
}

int CExplosiveProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
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

int CExplosiveProjectile::GetProjectilesCount() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return weaponDef->visuals.stages * validTextures[0];
}
