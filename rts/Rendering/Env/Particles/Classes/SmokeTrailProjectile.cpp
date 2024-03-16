/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SmokeTrailProjectile.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/SpringMath.h"

#include <tracy/Tracy.hpp>

CR_BIND_DERIVED(CSmokeTrailProjectile, CProjectile, )

CR_REG_METADATA(CSmokeTrailProjectile,(
	CR_MEMBER(pos1),
	CR_MEMBER(pos2),
	CR_MEMBER(origSize),
	CR_MEMBER(creationTime),
	CR_MEMBER(lifeTime),
	CR_MEMBER(lifePeriod),
	CR_MEMBER(color),
	CR_MEMBER(dir1),
	CR_MEMBER(dir2),
	CR_MEMBER(dirpos1),
	CR_MEMBER(dirpos2),
	CR_MEMBER(midpos),
	CR_MEMBER(middir),
	CR_MEMBER(drawSegmented),
	CR_MEMBER(firstSegment),
	CR_MEMBER(lastSegment),
	CR_IGNORED(texture),
	CR_SERIALIZER(Serialize)
))


CSmokeTrailProjectile::CSmokeTrailProjectile(
	const CUnit* owner,
	const float3& pos1,
	const float3& pos2,
	const float3& dir1,
	const float3& dir2,
	bool firstSegment,
	bool lastSegment,
	float size,
	int time,
	int period,
	float color,
	AtlasedTexture* texture,
	bool castShadowIn
):
	CProjectile((pos1 + pos2) * 0.5f, ZeroVector, owner, false, false, false),

	pos1(pos1),
	pos2(pos2),
	origSize(size),
	creationTime(gs->frameNum),
	lifeTime(time),
	lifePeriod(period),
	color(color),
	dir1(dir1),
	dir2(dir2),
	drawSegmented(false),
	firstSegment(firstSegment),
	lastSegment(lastSegment),
	texture((texture == nullptr) ? projectileDrawer->smoketrailtex : texture)
{
	checkCol = false;
	castShadow = castShadowIn;

	UpdateEndPos(pos1, dir1);
	SetRadiusAndHeight(pos1.distance(pos2), 0.0f);

	useAirLos |= ((pos.y - CGround::GetApproximateHeight(pos.x, pos.z)) > 10.0f);
}

void CSmokeTrailProjectile::Serialize(creg::ISerializer* s)
{
	//ZoneScoped;
	if (!s->IsWriting())
		texture = projectileDrawer->smoketrailtex;
}

void CSmokeTrailProjectile::UpdateEndPos(const float3 pos, const float3 dir)
{
	//ZoneScoped;
	pos1 = pos;
	dir1 = dir;

	const float dist = pos1.distance(pos2);

	drawSegmented = false;
	SetPosition((pos1 + pos2) * 0.5f);
	SetRadiusAndHeight(dist, 0.0f);
	sortDistOffset = 10.f + dist * 0.5f; // so that missile's engine flame gets rendered above the trail

	if (dir1.dot(dir2) < 0.98f) {
		dirpos1 = pos1 - dir1 * dist * 0.33f;
		dirpos2 = pos2 + dir2 * dist * 0.33f;
		midpos = CalcBeizer(0.5f, pos1, dirpos1, dirpos2, pos2);
		middir = (dir1 + dir2).ANormalize();
		drawSegmented = true;
	}
}


void CSmokeTrailProjectile::Draw()
{
	//ZoneScoped;
	const float age = gs->frameNum + globalRendering->timeOffset - creationTime;
	const float invLifeTime = (1.0f / lifeTime);

	const bool shadowPass = (camera->GetCamType() == CCamera::CAMTYPE_SHADOW);

	const float3 dif1 = shadowPass ? camera->GetForward() : (pos1 - camera->GetPos()).ANormalize();
	const float3 dif2 = shadowPass ? camera->GetForward() : (pos2 - camera->GetPos()).ANormalize();

	const float3 odir1 = (dif1.cross(dir1)).ANormalize();
	const float3 odir2 = (dif2.cross(dir2)).ANormalize();

	const float t1 = (age                    ) * invLifeTime;
	const float tm = (age + 0.5f * lifePeriod) * invLifeTime;
	const float t2 = (age +        lifePeriod) * invLifeTime;

	const float lerp1 = ((1.0f - t1) * (0.7f + std::fabs(dif1.dot(dir1)))) * (1 - lastSegment );
	const float lerp2 = ((1.0f - t2) * (0.7f + std::fabs(dif2.dot(dir2)))) * (1 - firstSegment);

	const float size1 = 1.0f + t1 * origSize;
	const float size2 = 1.0f + t2 * origSize;


	const SColor colBase = { color, color, color, 1.0f };
	const SColor col1 = colBase * std::clamp(lerp1, 0.0f, 1.0f);
	const SColor col2 = colBase * std::clamp(lerp2, 0.0f, 1.0f);

	if (drawSegmented) {

		const float3 difm = shadowPass ? camera->GetForward() : (midpos - camera->GetPos()).ANormalize();
		const float3 odirm = (difm.cross(middir)).ANormalize();

		const float lerpm = (1.0f - tm) * (0.7f + std::fabs(difm.dot(middir)));
		const float sizem = (0.2f + tm) * origSize;
		const float midtexx = mix(texture->xstart, texture->xend, 0.5f);

		const SColor colm = colBase * std::clamp(lerpm, 0.0f, 1.0f);

		AddEffectsQuad(
			{ pos1   - (odir1 * size1), texture->xstart, texture->ystart, col1  },
			{ midpos - (odirm * sizem), midtexx        , texture->ystart, colm },
			{ midpos + (odirm * sizem), midtexx        , texture->yend  , colm },
			{ pos1   + (odir1 * size1), texture->xstart, texture->yend  , col1  }
		);

		AddEffectsQuad(
			{ midpos - (odirm * sizem), midtexx      ,   texture->ystart, colm },
			{ pos2   - (odir2 * size2), texture->xend,   texture->ystart, col2 },
			{ pos2   + (odir2 * size2), texture->xend,   texture->yend  , col2 },
			{ midpos + (odirm * sizem), midtexx      ,   texture->yend  , colm }
		);
	} else {
		AddEffectsQuad(
			{ pos1 - (odir1 * size1), texture->xstart, texture->ystart, col1 },
			{ pos2 - (odir2 * size2), texture->xend  , texture->ystart, col2 },
			{ pos2 + (odir2 * size2), texture->xend  , texture->yend  , col2 },
			{ pos1 + (odir1 * size1), texture->xstart, texture->yend  , col1 }
		);
	}
}

void CSmokeTrailProjectile::Update()
{
	//ZoneScoped;
	deleteMe |= (gs->frameNum >= (creationTime + lifeTime));
}

int CSmokeTrailProjectile::GetProjectilesCount() const
{
	return 2;
}
