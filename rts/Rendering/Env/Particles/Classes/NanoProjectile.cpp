/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "NanoProjectile.h"

#include "Game/Camera.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Env/Particles/Generators/ParticleGeneratorHandler.h"
#include "Rendering/Colors.h"
#include "Rendering/GlobalRendering.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"
#include "Sim/Projectiles/ProjectileHandler.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CNanoProjectile, CProjectile, )

CR_REG_METADATA(CNanoProjectile,
(
	CR_MEMBER(rotAcc),
	CR_MEMBER(rotVal0x),
	CR_MEMBER(rotVel0x),
	CR_MEMBER(rotAcc0x),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(deathFrame),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config)
))


CNanoProjectile::CNanoProjectile()
{
	RECOIL_DETAILED_TRACY_ZONE;
	deathFrame = 0;
	color[0] = color[1] = color[2] = color[3] = 255;

	checkCol = false;
	drawSorted = false;
}

CNanoProjectile::CNanoProjectile(float3 pos, float3 speed, int lifeTime, SColor c)
	: CProjectile(pos, speed, nullptr, false, false, false)
	, deathFrame(gs->frameNum + lifeTime)
	, color(c)
{
	RECOIL_DETAILED_TRACY_ZONE;
	checkCol = false;
	drawSorted = false;
	drawRadius = 3;

	projectileHandler.currentNanoParticles += 1;

	rotVal0x = rotValRng0 * (guRNG.NextFloat() * 2.0 - 1.0);
	rotVel0x = rotVelRng0 * (guRNG.NextFloat() * 2.0 - 1.0);
	rotAcc0x = rotAccRng0 * (guRNG.NextFloat() * 2.0 - 1.0);

	rotVal = rotVal0 + rotVal0x;
	rotVel = rotVel0 + rotVel0x;
	rotAcc = rotAcc0 + rotAcc0x;

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<NanoParticleGenerator>();
	pgOffset = pg.Add({
		.partPos = pos,
		.createFrame = createFrame,
		.partSpeed = speed,
		.color = color,
		.animParams = animParams,
		.partSize = drawRadius,
		.rotParams = float3(rotVel, rotAcc, rotVal),
		.drawOrder = drawOrder,
		.texCoord = *projectileDrawer->gfxtex
	});
}

CNanoProjectile::~CNanoProjectile()
{
	RECOIL_DETAILED_TRACY_ZONE;
	projectileHandler.currentNanoParticles -= 1;
	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<NanoParticleGenerator>();
	pg.Del(pgOffset);
}

void CNanoProjectile::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	pos += speed;

	deleteMe |= (gs->frameNum >= deathFrame);

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<NanoParticleGenerator>();
	const auto [token, data] = pg.Get(pgOffset);
	data->partPos = pos;
}

void CNanoProjectile::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	{
		const float t = (gs->frameNum - createFrame + globalRendering->timeOffset);
		// rotParams.y is acceleration in angle per frame^2
		rotVel = rotVel0 + rotAcc * t;
		rotVal = rotVal0 + rotVel * t;
	}

	const float3 ri = camera->GetRight() * drawRadius;
	const float3 up = camera->GetUp() * drawRadius;
	std::array<float3, 4> bounds = {
		-ri - up,
		 ri - up,
		 ri + up,
		-ri + up
	};

	if (math::fabs(rotVal) > 0.01f) {
		float3::rotate<false>(rotVal, camera->GetForward(), bounds);
	}

	const auto* gfxt = projectileDrawer->gfxtex;
	AddEffectsQuad(
		{ drawPos + bounds[0], gfxt->xstart, gfxt->ystart, color },
		{ drawPos + bounds[1], gfxt->xend  , gfxt->ystart, color },
		{ drawPos + bounds[2], gfxt->xend  , gfxt->yend  , color },
		{ drawPos + bounds[3], gfxt->xstart, gfxt->yend  , color }
	);
}

void CNanoProjectile::DrawOnMinimap() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	AddMiniMapVertices({ pos        , color4::green }, { pos + speed, color4::green });
}

int CNanoProjectile::GetProjectilesCount() const
{
	return 0; // nano particles use their own counter
}


bool CNanoProjectile::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_INT   (CNanoProjectile, deathFrame)
	CHECK_MEMBER_INFO_SCOLOR(CNanoProjectile, color     )

	return false;
}
