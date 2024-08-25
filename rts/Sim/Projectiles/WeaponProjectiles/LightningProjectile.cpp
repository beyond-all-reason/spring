#include "LightningProjectile.h"
/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "LightningProjectile.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Env/Particles/Generators/ParticleGeneratorHandler.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CLightningProjectile, CWeaponProjectile, )

CR_REG_METADATA(CLightningProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(displacements),
	CR_MEMBER(pgOffset)
))


CLightningProjectile::CLightningProjectile(const ProjectileParams& params): CWeaponProjectile(params)
{
	projectileType = WEAPON_LIGHTNING_PROJECTILE;
	useAirLos = false;

	assert(weaponDef->IsHitScanWeapon());

	displacements[0][0] = 0.0f;
	displacements[1][0] = 0.0f;

	for (auto& dispArray : displacements) {
		for (size_t d = 1; d < dispArray.size(); ++d) {
			auto& dispItem = dispArray[d];
			dispItem = (gsRNG.NextFloat() - 0.5f) * drawRadius * 0.05f;
		}
	}

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<LightningParticleGenerator>();
	pgOffset = pg.Add();
	auto& data = pg.Get(pgOffset);

	data.startPos = startPos;
	data.targetPos = targetPos;
	data.thickness = weaponDef->visuals.thickness;

	for (size_t d = 0; d < displacements[0].size(); ++d) {
		data.displacements[d                          ] = displacements[0][d];
	}
	for (size_t d = 0; d < displacements[1].size(); ++d) {
		data.displacements[d + displacements[0].size()] = displacements[1][d];
	}

	data.texCoord = *weaponDef->visuals.texture1;
	const auto& color = weaponDef->visuals.color;
	data.col = SColor{ color.x, color.y, color.z, 1.0f / 255.0f };
	data.drawOrder = drawOrder;


	static_assert(sizeof(displacements[0]) + sizeof(displacements[1]) == sizeof(LightningData::displacements));
}

CLightningProjectile::~CLightningProjectile()
{
	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<LightningParticleGenerator>();
	pg.Del(pgOffset);
}

void CLightningProjectile::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (--ttl <= 0) {
		deleteMe = true;
	} else {
		explGenHandler.GenExplosion(cegID, startPos + ((targetPos - startPos) / ttl), (targetPos - startPos), 0.0f, displacements[0][0], 0.0f, owner(), nullptr);
	}

	for (auto& dispArray : displacements) {
		for (size_t d = 1; d < dispArray.size(); ++d) {
			auto& dispItem = dispArray[d];
			dispItem += (gsRNG.NextFloat() - 0.5f) * 0.3f;
		}
	}

	UpdateInterception();

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<LightningParticleGenerator>();
	auto& data = pg.Get(pgOffset);

	// needed?
	data.startPos = startPos;
	data.targetPos = targetPos;

	for (size_t d = 1; d < displacements[0].size(); ++d) {
		data.displacements[d                          ] = displacements[0][d];
	}
	for (size_t d = 1; d < displacements[1].size(); ++d) {
		data.displacements[d + displacements[0].size()] = displacements[1][d];
	}
}

void CLightningProjectile::Draw()
{
	/*
	RECOIL_DETAILED_TRACY_ZONE;
	if (!validTextures[0])
		return;

	const auto& color = weaponDef->visuals.color;
	uint8_t col[4] {
		(uint8_t)(color.x * 255),
		(uint8_t)(color.y * 255),
		(uint8_t)(color.z * 255),
		1 //intensity*255;
	};

	const float3 ddir = (targetPos - startPos).Normalize();
	const float3 dif  = (startPos - camera->GetPos()).Normalize();
	const float3 dir1 = (dif.cross(ddir)).Normalize();

	for (auto& displacement : displacements) {
		float3 tempPos = startPos;
		for (size_t d = 1; d < displacement.size() - 1; ++d) {
			float f = (d + 1) * static_cast<float>(1.0f / (displacement.size() - 1));
			const float3 tempPosO = tempPos;
			tempPos  = (startPos * (1.0f - f)) + (targetPos * f);

			const auto& WDV = weaponDef->visuals;
			AddEffectsQuad(
				{ tempPosO + (dir1 * (displacement[d    ] + WDV.thickness)), WDV.texture1->xstart, WDV.texture1->ystart, col },
				{ tempPos  + (dir1 * (displacement[d + 1] + WDV.thickness)), WDV.texture1->xend,   WDV.texture1->ystart, col },
				{ tempPos  + (dir1 * (displacement[d + 1] - WDV.thickness)), WDV.texture1->xend,   WDV.texture1->yend,   col },
				{ tempPosO + (dir1 * (displacement[d    ] - WDV.thickness)), WDV.texture1->xstart, WDV.texture1->yend,   col }
			);
		}
	}
	*/
}

void CLightningProjectile::DrawOnMinimap() const
{
	RECOIL_DETAILED_TRACY_ZONE;

	const auto& color = weaponDef->visuals.color;
	const SColor lcolor = SColor{
		color[0],
		color[1],
		color[2]
	};

	AddMiniMapVertices({ startPos,  lcolor }, { targetPos, lcolor });
}

int CLightningProjectile::GetProjectilesCount() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (displacements[0].size() + displacements[1].size()) * validTextures[0];
}