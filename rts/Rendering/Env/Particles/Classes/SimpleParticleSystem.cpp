/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimpleParticleSystem.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "System/creg/DefTypes.h"
#include "System/float3.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

#include <tracy/Tracy.hpp>

CR_BIND_DERIVED(CSimpleParticleSystem, CProjectile, )

CR_REG_METADATA(CSimpleParticleSystem,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(emitVector),
		CR_MEMBER(emitMul),
		CR_MEMBER(gravity),
		CR_MEMBER(colorMap),
		CR_IGNORED(texture),
		CR_MEMBER(airdrag),
		CR_MEMBER(particleLife),
		CR_MEMBER(particleLifeSpread),
		CR_MEMBER(numParticles),
		CR_MEMBER(particleSpeed),
		CR_MEMBER(particleSpeedSpread),
		CR_MEMBER(particleSize),
		CR_MEMBER(particleSizeSpread),
		CR_MEMBER(emitRot),
		CR_MEMBER(emitRotSpread),
		CR_MEMBER(directional),
		CR_MEMBER(sizeGrowth),
		CR_MEMBER(sizeMod),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(particles),
	CR_SERIALIZER(Serialize)
))

CR_BIND(CSimpleParticleSystem::Particle, )

CR_REG_METADATA_SUB(CSimpleParticleSystem, Particle,
(
	CR_MEMBER(pos),
	CR_MEMBER(speed),
	CR_MEMBER(life),
	CR_MEMBER(rotVal),
	CR_MEMBER(rotVel),
	CR_MEMBER(decayrate),
	CR_MEMBER(size),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(sizeMod)
))

CSimpleParticleSystem::CSimpleParticleSystem()
	: CProjectile()
	, emitVector(ZeroVector)
	, emitMul(1.0f, 1.0f, 1.0f)
	, gravity(ZeroVector)
	, particleSpeed(0.0f)
	, particleSpeedSpread(0.0f)
	, emitRot(0.0f)
	, emitRotSpread(0.0f)
	, texture(nullptr)
	, colorMap(nullptr)
	, directional(false)
	, particleLife(0.0f)
	, particleLifeSpread(0.0f)
	, particleSize(0.0f)
	, particleSizeSpread(0.0f)
	, airdrag(0.0f)
	, sizeGrowth(0.0f)
	, sizeMod(0.0f)
	, numParticles(0)
{
	checkCol = false;
	useAirLos = true;
}

void CSimpleParticleSystem::Serialize(creg::ISerializer* s)
{
	//ZoneScoped;
	std::string name;
	if (s->IsWriting())
		name = projectileDrawer->textureAtlas->GetTextureName(texture);
	creg::GetType(name)->Serialize(s, &name);
	if (!s->IsWriting())
		texture = projectileDrawer->textureAtlas->GetTexturePtr(name);
}

void CSimpleParticleSystem::Draw()
{
	//ZoneScoped;
	UpdateAnimParams();

	float3 zdir;
	float3 ydir;
	float3 xdir;

	const auto DoParticleDraw = [this](const float3& xdir, const float3& ydir, const float3& zdir, const Particle* p) {
		const float3 pDrawPos = p->pos + p->speed * globalRendering->timeOffset;
		const float size = p->size;

		unsigned char color[4];
		colorMap->GetColor(color, p->life);

		std::array<float3, 4> bounds = {
			-ydir * size - xdir * size,
			-ydir * size + xdir * size,
			 ydir * size + xdir * size,
			 ydir * size - xdir * size
		};

		if (math::fabs(p->rotVal) > 0.01f) {
			float3::rotate<false>(p->rotVal, zdir, bounds);
		}
		AddEffectsQuad(
			{ pDrawPos + bounds[0], texture->xstart, texture->ystart, color },
			{ pDrawPos + bounds[1], texture->xend,   texture->ystart, color },
			{ pDrawPos + bounds[2], texture->xend,   texture->yend,   color },
			{ pDrawPos + bounds[3], texture->xstart, texture->yend,   color }
		);
	};

	const bool shadowPass = (camera->GetCamType() == CCamera::CAMTYPE_SHADOW);
	if (directional && !shadowPass) {
		for (int i = 0; i < numParticles; i++) {
			const Particle* p = &particles[i];

			if (p->life >= 1.0f)
				continue;

			zdir = (p->pos - camera->GetPos()).SafeANormalize();
			ydir = zdir.cross(p->speed);
			if likely(ydir.SqLength() > 0.001f) {
				ydir.SafeANormalize();
				xdir = ydir.cross(zdir);
			}
			else {
				zdir = camera->GetForward();
				xdir = camera->GetRight();
				ydir = camera->GetUp();
			}

			DoParticleDraw(xdir, ydir, zdir, p);
		}
		return;
	}

	// !directional
	for (int i = 0; i < numParticles; i++) {
		const Particle* p = &particles[i];

		if (p->life >= 1.0f)
			continue;

		zdir = camera->GetForward();
		xdir = camera->GetRight();
		ydir = camera->GetUp();
		
		DoParticleDraw(xdir, ydir, zdir, p);
	}
}

void CSimpleParticleSystem::Update()
{
	//ZoneScoped;
	deleteMe = true;

	for (auto& p: particles) {
		if (p.life < 1.0f) {
			p.pos    += p.speed;
			p.speed  += gravity;
			p.speed  *= airdrag;
			p.rotVal += p.rotVel;
			p.rotVel += rotParams.y; //rot accel
			p.life   += p.decayrate;
			p.size    = p.size * sizeMod + sizeGrowth;

			deleteMe = false;
		}
	}
}

void CSimpleParticleSystem::Init(const CUnit* owner, const float3& offset)
{
	//ZoneScoped;
	CProjectile::Init(owner, offset);

	const float3 up = emitVector;
	const float3 right = up.cross(float3(up.y, up.z, -up.x));
	const float3 forward = up.cross(right);

	// FIXME: should catch these earlier and for more projectile-types
	if (colorMap == nullptr) {
		colorMap = CColorMap::LoadFromFloatVector(std::vector<float>(8, 1.0f));
		LOG_L(L_WARNING, "[CSimpleParticleSystem::%s] no color-map specified", __func__);
	}
	if (texture == nullptr) {
		texture = &projectileDrawer->textureAtlas->GetTexture("simpleparticle");
		LOG_L(L_WARNING, "[CSimpleParticleSystem::%s] no texture specified", __func__);
	}

	particles.resize(numParticles);
	for (auto& p: particles) {
		float az = guRNG.NextFloat() * math::TWOPI;
		float ay = (emitRot + (emitRotSpread * guRNG.NextFloat())) * math::DEG_TO_RAD;

		p.pos = offset;
		p.speed = ((up * emitMul.y) * fastmath::cos(ay) - ((right * emitMul.x) * fastmath::cos(az) - (forward * emitMul.z) * fastmath::sin(az)) * fastmath::sin(ay)) * (particleSpeed + (guRNG.NextFloat() * particleSpeedSpread));
		p.rotVal = rotParams.z; //initial rotation value
		p.rotVel = rotParams.x; //initial rotation velocity
		p.life = 0.0f;
		p.decayrate = 1.0f / (particleLife + (guRNG.NextFloat() * particleLifeSpread));
		p.size = particleSize + guRNG.NextFloat()*particleSizeSpread;
	}

	drawRadius = (particleSpeed + particleSpeedSpread) * (particleLife + particleLifeSpread);
}

int CSimpleParticleSystem::GetProjectilesCount() const
{
	return numParticles;
}



bool CSimpleParticleSystem::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	//ZoneScoped;
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_FLOAT3(CSimpleParticleSystem, emitVector         )
	CHECK_MEMBER_INFO_FLOAT3(CSimpleParticleSystem, emitMul            )
	CHECK_MEMBER_INFO_FLOAT3(CSimpleParticleSystem, gravity            )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleSpeed      )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleSpeedSpread)
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, emitRot            )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, emitRotSpread      )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleLife       )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleLifeSpread )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleSize       )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleSizeSpread )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, airdrag            )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, sizeGrowth         )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, sizeMod            )
	CHECK_MEMBER_INFO_INT   (CSimpleParticleSystem, numParticles       )
	CHECK_MEMBER_INFO_BOOL  (CSimpleParticleSystem, directional        )
	CHECK_MEMBER_INFO_PTR   (CSimpleParticleSystem, texture , projectileDrawer->textureAtlas->GetTexturePtr)
	CHECK_MEMBER_INFO_PTR   (CSimpleParticleSystem, colorMap, CColorMap::LoadFromDefString                 )

	return false;
}


CR_BIND_DERIVED(CSphereParticleSpawner, CSimpleParticleSystem, )

CR_REG_METADATA(CSphereParticleSpawner, )