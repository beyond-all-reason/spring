#include "BeamLaserProjectile.h"
/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "BeamLaserProjectile.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Env/Particles/Generators/ParticleGeneratorHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include <cstring> //memset

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CBeamLaserProjectile, CWeaponProjectile, )

CR_REG_METADATA(CBeamLaserProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(ccsColor),
	CR_MEMBER(cceColor),
	CR_MEMBER(ecsColor),
	CR_MEMBER(eceColor),
	CR_MEMBER(pgOffset)
))


CBeamLaserProjectile::CBeamLaserProjectile(const ProjectileParams& params)
	: CWeaponProjectile(params)
	, pgOffset(-1)
{
	projectileType = WEAPON_BEAMLASER_PROJECTILE;

	assert(weaponDef->IsHitScanWeapon());
/*
	thickness = weaponDef->visuals.thickness;
	corethickness = weaponDef->visuals.corethickness;
	flaresize = weaponDef->visuals.laserflaresize;
	decay = weaponDef->visuals.beamdecay;

	midtexx =
		(weaponDef->visuals.texture2->xstart +
		(weaponDef->visuals.texture2->xend - weaponDef->visuals.texture2->xstart) * 0.5f);
*/
	ccsColor[0] = (weaponDef->visuals.color2.x * params.startAlpha);
	ccsColor[1] = (weaponDef->visuals.color2.y * params.startAlpha);
	ccsColor[2] = (weaponDef->visuals.color2.z * params.startAlpha);
	ccsColor[3] = 1;
	cceColor[0] = (weaponDef->visuals.color2.x * params.endAlpha);
	cceColor[1] = (weaponDef->visuals.color2.y * params.endAlpha);
	cceColor[2] = (weaponDef->visuals.color2.z * params.endAlpha);
	cceColor[3] = 1;
	ecsColor[0] = (weaponDef->visuals.color.x * params.startAlpha);
	ecsColor[1] = (weaponDef->visuals.color.y * params.startAlpha);
	ecsColor[2] = (weaponDef->visuals.color.z * params.startAlpha);
	ecsColor[3] = 1;
	eceColor[0] = (weaponDef->visuals.color.x * params.endAlpha);
	eceColor[1] = (weaponDef->visuals.color.y * params.endAlpha);
	eceColor[2] = (weaponDef->visuals.color.z * params.endAlpha);
	eceColor[3] = 1;

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<BeamLaserParticleGenerator>();
	pgOffset = pg.Add({
		.startPos = startPos,
		.ccsColor = SColor(ccsColor[0], ccsColor[1], ccsColor[2], ccsColor[3]),
		.targetPos = targetPos,
		.cceColor = SColor(cceColor[0], cceColor[1], cceColor[2], cceColor[3]),
		.animParams1 = animParams,
		.ecsColor = SColor(ecsColor[0], ecsColor[1], ecsColor[2], ecsColor[3]),
		.animParams2 = {},
		.eceColor = SColor(eceColor[0], eceColor[1], eceColor[2], eceColor[3]),
		.animParams3 = {},
		.drawOrder = drawOrder,
		.texCoord1 = *weaponDef->visuals.texture1,
		.texCoord2 = *weaponDef->visuals.texture2,
		.texCoord3 = *weaponDef->visuals.texture3,
		.thickness = weaponDef->visuals.thickness,
		.coreThickness = weaponDef->visuals.corethickness,
		.flareSize = weaponDef->visuals.laserflaresize,
		.midTexX2 = (weaponDef->visuals.texture2->xend + weaponDef->visuals.texture2->xstart) * 0.5f
	});
}

CBeamLaserProjectile::~CBeamLaserProjectile()
{
	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<BeamLaserParticleGenerator>();
	pg.Del(pgOffset);
}



void CBeamLaserProjectile::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;

	const auto& decay = weaponDef->visuals.beamdecay;

	if ((--ttl) <= 0) {
		deleteMe = true;
	} else {
		for (int i = 0; i < 3; i++) {
			ccsColor[i] *= decay;
			cceColor[i]   *= decay;
			ecsColor[i] *= decay;
			eceColor[i]   *= decay;
		}

		const auto& flaresize = weaponDef->visuals.laserflaresize;

		explGenHandler.GenExplosion(cegID, startPos + ((targetPos - startPos) / ttl), (targetPos - startPos), 0.0f, flaresize, 0.0f, owner(), nullptr);
	}

	UpdateInterception();

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<BeamLaserParticleGenerator>();
	auto& data = pg.Get(pgOffset);

	data.ccsColor = SColor(ccsColor[0], ccsColor[1], ccsColor[2], ccsColor[3]);
	data.cceColor = SColor(cceColor[0], cceColor[1], cceColor[2], cceColor[3]);
	data.ecsColor = SColor(ecsColor[0], ecsColor[1], ecsColor[2], ecsColor[3]);
	data.eceColor = SColor(eceColor[0], eceColor[1], eceColor[2], eceColor[3]);
}

void CBeamLaserProjectile::Draw()
{
	/*
	RECOIL_DETAILED_TRACY_ZONE;
	if (!validTextures[0])
		return;

	const float3 midPos = (targetPos + startPos) * 0.5f;
	const float3 cameraDir = (midPos - camera->GetPos()).SafeANormalize();

	CCamera* playerCamera = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	const float playerCamDistSq = (midPos - playerCamera->GetPos()).SqLength();

	// beam's coor-system; degenerate if targetPos == startPos
	const float3 zdir = (targetPos - startPos).SafeANormalize();
	const float3 xdir = (cameraDir.cross(zdir)).SafeANormalize();
	const float3 ydir = (cameraDir.cross(xdir));

	const float beamEdgeSize = thickness;
	const float beamCoreSize = beamEdgeSize * corethickness;
	const float flareEdgeSize = thickness * flaresize;
	const float flareCoreSize = flareEdgeSize * corethickness;

	const float3& pos1 = startPos;
	const float3& pos2 = targetPos;

	const auto* WT1 = weaponDef->visuals.texture1;
	const auto* WT2 = weaponDef->visuals.texture2;
	const auto* WT3 = weaponDef->visuals.texture3;

	if (playerCamDistSq < Square(1000.0f)) {
		if (validTextures[2]) {
			AddEffectsQuad(
				{ pos1 - xdir * beamEdgeSize,                       midtexx  , WT2->ystart, edgeColStart },
				{ pos1 - xdir * beamEdgeSize - ydir * beamEdgeSize, WT2->xend, WT2->ystart, edgeColStart },
				{ pos1 + xdir * beamEdgeSize - ydir * beamEdgeSize, WT2->xend, WT2->yend  , edgeColStart },
				{ pos1 + xdir * beamEdgeSize,                       midtexx  , WT2->yend  , edgeColStart }
			);
			AddEffectsQuad(
				{ pos1 - xdir * beamCoreSize,                       midtexx  , WT2->ystart, coreColStart },
				{ pos1 - xdir * beamCoreSize - ydir * beamCoreSize, WT2->xend, WT2->ystart, coreColStart },
				{ pos1 + xdir * beamCoreSize - ydir * beamCoreSize, WT2->xend, WT2->yend  , coreColStart },
				{ pos1 + xdir * beamCoreSize,                       midtexx  , WT2->yend  , coreColStart }
			);

		}
		if (validTextures[1]) {
			AddEffectsQuad(
				{ pos1 - xdir * beamEdgeSize,                       WT1->xstart, WT1->ystart, edgeColStart },
				{ pos2 - xdir * beamEdgeSize,                       WT1->xend  , WT1->ystart, edgeColEnd   },
				{ pos2 + xdir * beamEdgeSize,                       WT1->xend  , WT1->yend  , edgeColEnd   },
				{ pos1 + xdir * beamEdgeSize,                       WT1->xstart, WT1->yend  , edgeColStart }
			);

			AddEffectsQuad(
				{ pos1 - xdir * beamCoreSize,                       WT1->xstart, WT1->ystart, coreColStart },
				{ pos2 - xdir * beamCoreSize,                       WT1->xend  , WT1->ystart, coreColEnd   },
				{ pos2 + xdir * beamCoreSize,                       WT1->xend  , WT1->yend  , coreColEnd   },
				{ pos1 + xdir * beamCoreSize,                       WT1->xstart, WT1->yend  , coreColStart }
			);
		}
		if (validTextures[2]) {
			AddEffectsQuad(
				{ pos2 - xdir * beamEdgeSize,                       midtexx  , WT2->ystart, edgeColStart },
				{ pos2 - xdir * beamEdgeSize + ydir * beamEdgeSize, WT2->xend, WT2->ystart, edgeColStart },
				{ pos2 + xdir * beamEdgeSize + ydir * beamEdgeSize, WT2->xend, WT2->yend  , edgeColStart },
				{ pos2 + xdir * beamEdgeSize,                       midtexx  , WT2->yend  , edgeColStart }
			);

			AddEffectsQuad(
				{ pos2 - xdir * beamCoreSize,                       midtexx  , WT2->ystart, coreColStart },
				{ pos2 - xdir * beamCoreSize + ydir * beamCoreSize, WT2->xend, WT2->ystart, coreColStart },
				{ pos2 + xdir * beamCoreSize + ydir * beamCoreSize, WT2->xend, WT2->yend  , coreColStart },
				{ pos2 + xdir * beamCoreSize,                       midtexx  , WT2->yend  , coreColStart }
			);
		}
	} else {
		if (validTextures[1]) {
			AddEffectsQuad(
				{ pos1 - xdir * beamEdgeSize,                       WT1->xstart, WT1->ystart, edgeColStart },
				{ pos2 - xdir * beamEdgeSize,                       WT1->xend  , WT1->ystart, edgeColEnd   },
				{ pos2 + xdir * beamEdgeSize,                       WT1->xend  , WT1->yend  , edgeColEnd   },
				{ pos1 + xdir * beamEdgeSize,                       WT1->xstart, WT1->yend  , edgeColStart }
			);

			AddEffectsQuad(
				{ pos1 - xdir * beamCoreSize,                       WT1->xstart, WT1->ystart, coreColStart },
				{ pos2 - xdir * beamCoreSize,                       WT1->xend  , WT1->ystart, coreColEnd   },
				{ pos2 + xdir * beamCoreSize,                       WT1->xend  , WT1->yend  , coreColEnd   },
				{ pos1 + xdir * beamCoreSize,                       WT1->xstart, WT1->yend  , coreColStart }
			);
		}
	}

	// draw flare
	if (validTextures[3]) {
		AddEffectsQuad(
			{ pos1 - camera->GetRight() * flareEdgeSize - camera->GetUp() * flareEdgeSize, WT3->xstart, WT3->ystart, edgeColStart },
			{ pos1 + camera->GetRight() * flareEdgeSize - camera->GetUp() * flareEdgeSize, WT3->xend,   WT3->ystart, edgeColStart },
			{ pos1 + camera->GetRight() * flareEdgeSize + camera->GetUp() * flareEdgeSize, WT3->xend,   WT3->yend,   edgeColStart },
			{ pos1 - camera->GetRight() * flareEdgeSize + camera->GetUp() * flareEdgeSize, WT3->xstart, WT3->yend,   edgeColStart }
		);

		AddEffectsQuad(
			{ pos1 - camera->GetRight() * flareCoreSize - camera->GetUp() * flareCoreSize, WT3->xstart, WT3->ystart, coreColStart },
			{ pos1 + camera->GetRight() * flareCoreSize - camera->GetUp() * flareCoreSize, WT3->xend,   WT3->ystart, coreColStart },
			{ pos1 + camera->GetRight() * flareCoreSize + camera->GetUp() * flareCoreSize, WT3->xend,   WT3->yend,   coreColStart },
			{ pos1 - camera->GetRight() * flareCoreSize + camera->GetUp() * flareCoreSize, WT3->xstart, WT3->yend,   coreColStart }
		);
	}
	*/
}

void CBeamLaserProjectile::DrawOnMinimap() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const SColor color = { ecsColor[0], ecsColor[1], ecsColor[2], 255u };

	AddMiniMapVertices({ startPos , color }, { targetPos, color });
}

int CBeamLaserProjectile::GetProjectilesCount() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return
		2 * validTextures[1] +
		4 * validTextures[2] +
		2 * validTextures[3];
}
