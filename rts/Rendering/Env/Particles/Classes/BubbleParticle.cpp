/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "BubbleParticle.h"

#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Projectiles/ProjectileDrawer.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CBubbleParticle, CProjectile, )

CR_REG_METADATA(CBubbleParticle, (
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(ttl),
		CR_MEMBER(alpha),
		CR_MEMBER(startSize),
		CR_MEMBER(sizeExpansion),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(size)
))


CBubbleParticle::CBubbleParticle()
	: ttl(0)
	, alpha(0.0f)
	, size(0.0f)
	, startSize(0.0f)
	, sizeExpansion(0.0f)
{ }

CBubbleParticle::CBubbleParticle(
	CUnit* owner,
	float3 pos,
	float3 speed,
	int ttl,
	float startSize,
	float sizeExpansion,
	float alpha
):
	CProjectile(pos, speed, owner, false, false, false),
	ttl(ttl),
	alpha(alpha),
	size(startSize * 0.4f),
	startSize(startSize),
	sizeExpansion(sizeExpansion)
{
	checkCol = false;
}


void CBubbleParticle::Update()
{
	pos += speed;
	--ttl;
	size += sizeExpansion;
	if (size < startSize) {
		size += (startSize - size) * 0.2f;
	}
	drawRadius=size;

	if (pos.y > (-size * 0.7f)) {
		pos.y = -size * 0.7f;
		alpha -= 0.03f;
	}
	if (ttl < 0) {
		alpha -= 0.03f;
	}
	if (alpha < 0) {
		alpha = 0;
		deleteMe = true;
	}
}

void CBubbleParticle::Draw()
{
	unsigned char col[4];
	col[0] = (unsigned char)(255 * alpha);
	col[1] = (unsigned char)(255 * alpha);
	col[2] = (unsigned char)(255 * alpha);
	col[3] = (unsigned char)(255 * alpha);

	const float interSize = size + sizeExpansion * globalRendering->timeOffset;

	#define bt projectileDrawer->bubbletex
	AddEffectsQuad(
		{ drawPos - camera->GetRight() * interSize - camera->GetUp() * interSize, bt->xstart, bt->ystart, col },
		{ drawPos + camera->GetRight() * interSize - camera->GetUp() * interSize, bt->xend,   bt->ystart, col },
		{ drawPos + camera->GetRight() * interSize + camera->GetUp() * interSize, bt->xend,   bt->yend,   col },
		{ drawPos - camera->GetRight() * interSize + camera->GetUp() * interSize, bt->xstart, bt->yend,   col }
	);
	#undef bt
}

int CBubbleParticle::GetProjectilesCount() const
{
	return 1;
}


bool CBubbleParticle::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_FLOAT(CBubbleParticle, alpha        )
	CHECK_MEMBER_INFO_FLOAT(CBubbleParticle, startSize    )
	CHECK_MEMBER_INFO_FLOAT(CBubbleParticle, sizeExpansion)
	CHECK_MEMBER_INFO_INT  (CBubbleParticle, ttl          )

	return false;
}
