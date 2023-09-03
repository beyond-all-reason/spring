/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ExploSpikeParticle.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Projectiles/ProjectileDrawer.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"

CR_BIND_DERIVED(CExploSpikeParticle, CProjectile, )

CR_REG_METADATA(CExploSpikeParticle,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(length),
		CR_MEMBER(width),
		CR_MEMBER(alpha),
		CR_MEMBER(alphaDecay),
		CR_MEMBER(lengthGrowth),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config)
))

CExploSpikeParticle::CExploSpikeParticle()
	: length(0.0f)
	, width(0.0f)
	, alpha(0.0f)
	, alphaDecay(0.0f)
	, lengthGrowth(0.0f)
	, color(1.0f, 0.8f, 0.5f)
{
}

CExploSpikeParticle::CExploSpikeParticle(
	CUnit* owner,
	const float3& pos,
	const float3& spd,
	float length,
	float width,
	float alpha,
	float alphaDecay
):
	CProjectile(pos, spd, owner, false, false, false),
	length(length),
	width(width),
	alpha(alpha),
	alphaDecay(alphaDecay),
	color(1.0f, 0.8f, 0.5f)
{
	lengthGrowth = speed.w * (0.5f + guRNG.NextFloat() * 0.4f);

	checkCol  = false;
	useAirLos = true;

	SetRadiusAndHeight(length + lengthGrowth * alpha / alphaDecay, 0.0f);
}

void CExploSpikeParticle::Init(const CUnit* owner, const float3& offset)
{
	CProjectile::Init(owner, offset);

	lengthGrowth = dir.Length() * (0.5f + guRNG.NextFloat() * 0.4f);
	dir /= lengthGrowth;

	SetRadiusAndHeight(length + lengthGrowth * alpha / alphaDecay, 0.0f);
}

void CExploSpikeParticle::Update()
{
	pos += speed;
	length += lengthGrowth;
	alpha = std::max(0.0f, alpha - alphaDecay);

	deleteMe |= (alpha <= 0.0f);
}

void CExploSpikeParticle::Draw()
{
	const float3 dif = (pos - camera->GetPos()).ANormalize();
	const float3 dir2 = (dif.cross(dir)).ANormalize();

	unsigned char col[4];
	const float a = std::max(0.0f, alpha - alphaDecay * globalRendering->timeOffset) * 255.0f;
	col[0] = (unsigned char)(a * color.x);
	col[1] = (unsigned char)(a * color.y);
	col[2] = (unsigned char)(a * color.z);
	col[3] = 1;

	const float3 l = (dir * length) + (lengthGrowth * globalRendering->timeOffset);
	const float3 w = dir2 * width;

	#define let projectileDrawer->laserendtex
	AddEffectsQuad(
		{ drawPos - l - w, let->xstart, let->ystart, col },
		{ drawPos + l - w, let->xend,   let->ystart, col },
		{ drawPos + l + w, let->xend,   let->yend,   col },
		{ drawPos - l + w, let->xstart, let->yend,   col }
	);
	#undef let
}



int CExploSpikeParticle::GetProjectilesCount() const
{
	return 1;
}

bool CExploSpikeParticle::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_FLOAT (CExploSpikeParticle, length      )
	CHECK_MEMBER_INFO_FLOAT (CExploSpikeParticle, width       )
	CHECK_MEMBER_INFO_FLOAT (CExploSpikeParticle, alpha       )
	CHECK_MEMBER_INFO_FLOAT (CExploSpikeParticle, alphaDecay  )
	CHECK_MEMBER_INFO_FLOAT (CExploSpikeParticle, lengthGrowth)
	CHECK_MEMBER_INFO_FLOAT3(CExploSpikeParticle, color       )

	return false;
}
