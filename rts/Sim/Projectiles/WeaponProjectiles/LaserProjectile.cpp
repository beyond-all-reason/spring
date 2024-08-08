/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "LaserProjectile.h"
#include "Map/Ground.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Env/Particles/Generators/ParticleGeneratorHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CLaserProjectile, CWeaponProjectile, )

CR_REG_METADATA(CLaserProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(intensity),
	CR_MEMBER(speedf),
	CR_MEMBER(maxLength),
	CR_MEMBER(curLength),
	CR_MEMBER(stayTime),
	CR_MEMBER(intensityFalloff),
	CR_MEMBER(midtexx),
	CR_MEMBER(pgOffset)
))


CLaserProjectile::CLaserProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	// NB: constant, assumes |speed=dir*projectileSpeed| never changes after creation
	, speedf(speed.w)
	, maxLength(0.0f)
	, curLength(0.0f)
	, intensity(0.0f)
	, intensityFalloff(0.0f)
	, midtexx(0.0f)

	, stayTime(0)
{
	projectileType = WEAPON_LASER_PROJECTILE;

	SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);

	maxLength = weaponDef->duration * (speedf * GAME_SPEED);
	intensity = weaponDef->intensity;
	intensityFalloff = intensity * weaponDef->falloffRate;

	midtexx =
		(weaponDef->visuals.texture2->xstart +
		(weaponDef->visuals.texture2->xend - weaponDef->visuals.texture2->xstart) * 0.5f);

	drawRadius = maxLength;

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<LaserParticleGenerator>();
	pgOffset = pg.Add(LaserData{
		.drawPos = pos,
		.curLength = curLength,
		.dir = dir,
		.maxLength = maxLength,
		.thickness = weaponDef->visuals.thickness,
		.coreThickness = weaponDef->visuals.corethickness,
		.color1 = SColor{},
		.color2 = SColor{},
		.lodDistance = static_cast<float>(weaponDef->visuals.lodDistance),
		.drawOrder = drawOrder,
		.checkCol = static_cast<int32_t>(checkCol),
		.stayTime = static_cast<float>(stayTime),
		.speedf = speedf,
		.texCoord1 = *weaponDef->visuals.texture1,
		.texCoord2 = *weaponDef->visuals.texture2
	});
}

CLaserProjectile::~CLaserProjectile()
{
	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<LaserParticleGenerator>();
	pg.Del(pgOffset);
}

void CLaserProjectile::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float4 oldSpeed = speed;

	UpdateIntensity();
	UpdateLength();
	UpdateInterception();
	UpdatePos(oldSpeed);

	// pre-decrement ttl: if projectile has to live for N frames
	// we want to check for collisions only N (not N + 1) times!
	checkCol &= ((ttl -= 1) >= 0);
	deleteMe |= ((curLength <= 0.01f) && (weaponDef->laserHardStop));
	deleteMe |= ((intensity <= 0.01f) && (!weaponDef->laserHardStop));

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<LaserParticleGenerator>();
	const auto [token, data] = pg.Get(pgOffset);

	const auto& c1 = weaponDef->visuals.color;
	const auto& c2 = weaponDef->visuals.color2;

	data->drawPos = pos;
	data->curLength = curLength;
	data->dir = dir;
	data->color1 = SColor(intensity * c1.r, intensity * c1.g, intensity * c1.b, 1.0f / 255.0f);
	data->color2 = SColor(intensity * c2.r, intensity * c2.g, intensity * c2.b, 1.0f / 255.0f);
	data->checkCol = static_cast<int32_t>(checkCol);
	data->stayTime = static_cast<float>(stayTime);
	data->speedf = speedf;
}

void CLaserProjectile::UpdateIntensity() {
	RECOIL_DETAILED_TRACY_ZONE;
	if (ttl > 0) {
		explGenHandler.GenExplosion(cegID, pos, speed, ttl, intensity, 0.0f, owner(), nullptr);
		return;
	}

	if (weaponDef->laserHardStop) {
		// bolt reached its max-range but wasn't fully extended yet
		if (curLength < maxLength && speed != ZeroVector)
			stayTime = 1 + int((maxLength - curLength) / speedf);

		SetVelocityAndSpeed(ZeroVector);
	} else {
		// fade out over the next 5 frames at most
		intensity -= std::max(intensityFalloff * 0.2f, 0.2f);
		intensity = std::max(intensity, 0.0f);
	}
}

void CLaserProjectile::UpdateLength() {
	RECOIL_DETAILED_TRACY_ZONE;
	if (speed != ZeroVector) {
		// expand bolt to maximum length if not
		// stopped / collided OR after hardstop
		curLength = std::min(curLength + speedf, maxLength);
	} else {
		// contract bolt to zero length after stayTime
		// expires (can be immediately if not hardstop)
		curLength = std::max(curLength - speedf * (stayTime == 0), 0.0f);
	}

	stayTime = std::max(stayTime - 1, 0);
}

void CLaserProjectile::UpdatePos(const float4& oldSpeed) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaMoveCtrl)
		return;

	SetPosition(pos + speed);
	// note: this can change pos *and* speed
	UpdateGroundBounce();

	if (oldSpeed != speed) {
		SetVelocityAndSpeed(speed);
	}
}



void CLaserProjectile::CollisionCommon(const float3& oldPos) {
	RECOIL_DETAILED_TRACY_ZONE;
	// we will fade out over some time
	deleteMe = false;

	if (weaponDef->noExplode)
		return;

	checkCol = false;

	SetPosition(oldPos);
	SetVelocityAndSpeed(ZeroVector);

	if (curLength < maxLength) {
		stayTime = 1 + int((maxLength - curLength) / speedf);
	}
}

void CLaserProjectile::Collision(CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 oldPos = pos;

	CWeaponProjectile::Collision(unit);
	CollisionCommon(oldPos);
}

void CLaserProjectile::Collision(CFeature* feature)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 oldPos = pos;

	CWeaponProjectile::Collision(feature);
	CollisionCommon(oldPos);
}

void CLaserProjectile::Collision()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 oldPos = pos;

	CWeaponProjectile::Collision();
	CollisionCommon(oldPos);
}



void CLaserProjectile::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// dont draw if a 3d model has been defined for us
	assert(!model);

	if (!validTextures[0])
		return;

	float3 dif(pos - camera->GetPos());
	const float camDist = dif.LengthNormalize();

	float3 dir1(dif.cross(dir));
	dir1.Normalize();
	float3 dir2(dif.cross(dir1));

	const auto& color  = weaponDef->visuals.color;
	const auto& color2 = weaponDef->visuals.color2;

	const uint8_t col[4] = {
		(uint8_t)(color.x * intensity * 255),
		(uint8_t)(color.y * intensity * 255),
		(uint8_t)(color.z * intensity * 255),
		1 //intensity*255;
	};

	const uint8_t col2[4] = {
		(uint8_t)(color2.x * intensity * 255),
		(uint8_t)(color2.y * intensity * 255),
		(uint8_t)(color2.z * intensity * 255),
		1 //intensity*255;
	};

	const float& size = weaponDef->visuals.thickness;
	const float& coresize = size * weaponDef->visuals.corethickness;

	if (camDist < weaponDef->visuals.lodDistance) {
		const float3 pos2 = drawPos - (dir * curLength);
		float texStartOffset;
		float texEndOffset;
		if (checkCol) { // expanding or contracting?
			texStartOffset = 0.0f;
			texEndOffset   = (1.0f - (curLength / maxLength)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		} else {
			texStartOffset = (-1.0f + (curLength / maxLength) + ((float)stayTime * (speedf / maxLength))) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
			texEndOffset   = ((float)stayTime * (speedf / maxLength)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		}

		if (validTextures[2]) {
			AddEffectsQuad(
				{ drawPos - (dir1 * size) - (dir2 * size),        weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->ystart, col },
				{ drawPos - (dir1 * size),                        midtexx,                             weaponDef->visuals.texture2->ystart, col },
				{ drawPos + (dir1 * size),                        midtexx,                             weaponDef->visuals.texture2->yend  , col },
				{ drawPos + (dir1 * size) - (dir2 * size),        weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->yend  , col }
			);

			AddEffectsQuad(
				{ drawPos - (dir1 * coresize) - (dir2 * coresize), weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->ystart, col2 },
				{ drawPos - (dir1 * coresize),                     midtexx,                             weaponDef->visuals.texture2->ystart, col2 },
				{ drawPos + (dir1 * coresize),                     midtexx,                             weaponDef->visuals.texture2->yend  , col2 },
				{ drawPos + (dir1 * coresize) - (dir2 * coresize), weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->yend  , col2 }
			);
		}
		if (validTextures[1]) {
			AddEffectsQuad(
				{ drawPos - (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col },
				{ pos2    - (dir1 * size),     weaponDef->visuals.texture1->xend   + texEndOffset  , weaponDef->visuals.texture1->ystart, col },
				{ pos2    + (dir1 * size),     weaponDef->visuals.texture1->xend   + texEndOffset  , weaponDef->visuals.texture1->yend  , col },
				{ drawPos + (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend  , col }
			);

			AddEffectsQuad(
				{ drawPos - (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col2 },
				{ pos2    - (dir1 * coresize), weaponDef->visuals.texture1->xend   + texEndOffset  , weaponDef->visuals.texture1->ystart, col2 },
				{ pos2    + (dir1 * coresize), weaponDef->visuals.texture1->xend   + texEndOffset  , weaponDef->visuals.texture1->yend  , col2 },
				{ drawPos + (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend  , col2 }
			);
		}
		if (validTextures[2]) {
			AddEffectsQuad(
				{ pos2 - (dir1 * size),                         midtexx,                           weaponDef->visuals.texture2->ystart, col },
				{ pos2 - (dir1 * size) + (dir2 * size),         weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col },
				{ pos2 + (dir1 * size) + (dir2 * size),         weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend  , col },
				{ pos2 + (dir1 * size),                         midtexx,                           weaponDef->visuals.texture2->yend  , col }
			);

			AddEffectsQuad(
				{ pos2 - (dir1 * coresize),                     midtexx,                           weaponDef->visuals.texture2->ystart, col2 },
				{ pos2 - (dir1 * coresize) + (dir2 * coresize), weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col2 },
				{ pos2 + (dir1 * coresize) + (dir2 * coresize), weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend  , col2 },
				{ pos2 + (dir1 * coresize),                     midtexx,                           weaponDef->visuals.texture2->yend  , col2 }
			);
		}
	} else {
		const float3 pos1 = drawPos + (dir * (size * 0.5f));
		const float3 pos2 = pos1 - (dir * (curLength + size));
		float texStartOffset;
		float texEndOffset;

		if (checkCol) { // expanding or contracting?
			texStartOffset = 0;
			texEndOffset   = (1.0f - (curLength / maxLength)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		} else {
			texStartOffset = (-1.0f + (curLength / maxLength) + ((float)stayTime * (speedf / maxLength))) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
			texEndOffset   = ((float)stayTime * (speedf / maxLength)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		}
		if (validTextures[1]) {
			AddEffectsQuad(
				{ pos1 - (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col },
				{ pos2 - (dir1 * size),     weaponDef->visuals.texture1->xend +     texEndOffset, weaponDef->visuals.texture1->ystart, col },
				{ pos2 + (dir1 * size),     weaponDef->visuals.texture1->xend +     texEndOffset, weaponDef->visuals.texture1->yend  , col },
				{ pos1 + (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend  , col }
			);

			AddEffectsQuad(
				{ pos1 - (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col2 },
				{ pos2 - (dir1 * coresize), weaponDef->visuals.texture1->xend +     texEndOffset, weaponDef->visuals.texture1->ystart, col2 },
				{ pos2 + (dir1 * coresize), weaponDef->visuals.texture1->xend +     texEndOffset, weaponDef->visuals.texture1->yend  , col2 },
				{ pos1 + (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend  , col2 }
			);
		}
	}
}

int CLaserProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaMoveCtrl)
		return 0;

	const float3 rdir = (pos - shieldPos).Normalize();

	if (rdir.dot(speed) < 0.0f) {
		SetVelocityAndSpeed(speed - (rdir * rdir.dot(speed) * 2.0f));
		return 1;
	}

	return 0;
}

int CLaserProjectile::GetProjectilesCount() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return
		2 * validTextures[1] +
		4 * validTextures[2];
}
