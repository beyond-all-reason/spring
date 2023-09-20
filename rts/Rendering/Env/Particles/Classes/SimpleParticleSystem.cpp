/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimpleParticleSystem.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Units/Unit.h"
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
#include "Rendering/Colors.h"
#include "Game/CameraHandler.h"

#include "System/BranchPrediction.h"


CSimpleParticleSystemCollection simpleParticleSystem; 

void AddEffectsQuad(const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl,
					const float3& animParams, const float& animProgress)
{
	float minS = std::numeric_limits<float>::max()   ; float minT = std::numeric_limits<float>::max()   ;
	float maxS = std::numeric_limits<float>::lowest(); float maxT = std::numeric_limits<float>::lowest();
	std::invoke([&](auto&&... arg) {
		((minS = std::min(minS, arg.s)), ...);
		((minT = std::min(minT, arg.t)), ...);
		((maxS = std::max(maxS, arg.s)), ...);
		((maxT = std::max(maxT, arg.t)), ...);
	}, tl, tr, br, bl);

	
	auto& rb = CProjectile::GetPrimaryRenderBuffer();

	const auto uvInfo = float4{ minS, minT, maxS - minS, maxT - minT };
	const auto animInfo = float3{ animParams.x, animParams.y, animProgress };
	constexpr float layer = 0.0f; //for future texture arrays

	//pos, uvw, uvmm, col
	rb.AddQuadTriangles(
		{ tl.pos, float3{ tl.s, tl.t, layer }, uvInfo, animInfo, tl.c },
		{ tr.pos, float3{ tr.s, tr.t, layer }, uvInfo, animInfo, tr.c },
		{ br.pos, float3{ br.s, br.t, layer }, uvInfo, animInfo, br.c },
		{ bl.pos, float3{ bl.s, bl.t, layer }, uvInfo, animInfo, bl.c }
	);
}

void AddQuadOrder(uint64_t order)
{
	auto& rb = CProjectile::GetPrimaryRenderBuffer();
	if (rb.GetSortMode()) {
		rb.AddQuadOrder(order);
	}
}

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
	std::string name;
	if (s->IsWriting())
		name = projectileDrawer->textureAtlas->GetTextureName(texture);
	creg::GetType(name)->Serialize(s, &name);
	if (!s->IsWriting())
		texture = projectileDrawer->textureAtlas->GetTexturePtr(name);
}

void CSimpleParticleSystem::Draw()
{
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

		if (std::fabs(p->rotVal) > 0.01f) {
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

	drawRadius = (particleSpeed + particleSpeedSpread) * (particleLife * particleLifeSpread);
}

int CSimpleParticleSystem::GetProjectilesCount() const
{
	return numParticles;
}



bool CSimpleParticleSystem::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
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

void CSphereParticleSpawner::Draw()
{
}

void CSphereParticleSpawner::Update()
{
}

int CSphereParticleSpawner::GetProjectilesCount() const
{
	return 0;
}

void CSphereParticleSpawner::Init(const CUnit* owner, const float3& offset)
{
	CProjectile::Init(owner, offset);
	
	// FIXME: should catch these earlier and for more projectile-types
	if (colorMap == nullptr) {
		colorMap = CColorMap::LoadFromFloatVector(std::vector<float>(8, 1.0f));
		LOG_L(L_WARNING, "[CSphereParticleSpawner::%s] no color-map specified", __FUNCTION__);
	}
	if (texture == nullptr) {
		texture = &projectileDrawer->textureAtlas->GetTexture("sphereparticle");
		LOG_L(L_WARNING, "[CSphereParticleSpawner::%s] no texture specified", __FUNCTION__);
	}
	
	drawRadius = (particleSpeed + particleSpeedSpread) * (particleLife * particleLifeSpread);
	
	this->GenerateParticles(offset);
	deleteMe = true;
}

bool CSphereParticleSpawner::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	return CSimpleParticleSystem::GetMemberInfo(memberInfo);
}

CSphereParticleSpawner::CSphereParticleSpawner()
{
}

void CSphereParticleSpawner::GenerateParticles(const float3& pos)
{
	simpleParticleSystem.Add(*this, pos);
}

void CSimpleParticleSystemCollection::Add(CSimpleParticleSystem& p, float3 offset) {
	const float3 up = p.emitVector;
	const float3 right = up.cross(float3(up.y, up.z, -up.x));
	const float3 forward = up.cross(right);
	
	for (int i = 0 ; i < p.numParticles; ++i) {
		float az = guRNG.NextFloat() * math::TWOPI;
		float ay = (p.emitRot + (p.emitRotSpread * guRNG.NextFloat())) * math::DEG_TO_RAD;
		
		auto& e = d.emplace_back();

		e.pos = offset;
		e.speed =((up * p.emitMul.y) * fastmath::cos(ay) - ((right * p.emitMul.x) * fastmath::cos(az) - (forward * p.emitMul.z) * fastmath::sin(az)) * fastmath::sin(ay)) * (p.particleSpeed + (guRNG.NextFloat() * p.particleSpeedSpread));
		
		e.rotVal = (p.rotParams.z);
		e.rotVel = (p.rotParams.x); //initial rotation velocity
		e.rotParams = (p.rotParams.y);
		
		e.life=(0.0f);
		e.decayrate=(1.0f / (p.particleLife + (guRNG.NextFloat() * p.particleLifeSpread)));
		
		e.size=(p.particleSize + guRNG.NextFloat()*p.particleSizeSpread);
		e.sizeGrowth=(p.sizeGrowth);
		e.sizeMod=(p.sizeMod);
		
		e.gravity=(p.gravity);
		e.airdrag=(p.airdrag);
		
		e.visible=(true);
		e.visibleShadow=(true);
		e.visibleRefraction=(true);
		e.visibleReflection=(true);
		e.allyTeam=(p.allyteamID);
					
		// draw
		e.colorMap=(p.colorMap);
		
		e.texture=(p.texture);
		
		e.anims=(p.animParams);
		e.aprogress=(p.animProgress);
		e.createFrame=(p.createFrame);
		
		e.drawRadius=(p.drawRadius);
		e.drawOrder=(p.drawOrder);
		
		e.directional=(p.directional);
		
		e.alwaysVisible=(p.alwaysVisible);
		e.castShadow=(p.castShadow);	
	}
}
	
void CSimpleParticleSystemCollection::Update() {		
	CheckDead();
		
	for (int i =0; i < d.size(); ++i) {
		auto& e = d[i];
		e.pos    += e.speed;
		e.speed  += e.gravity;
		e.speed  *= e.airdrag;

		e.rotVal += e.rotVel;
		e.rotVel += e.rotParams;

		e.life += e.decayrate;
		
		e.size = e.size * e.sizeMod + e.sizeGrowth;	
	}

}
	
void CSimpleParticleSystemCollection::CheckDead() {
	for (int i =0; i < d.size();) {
		auto& e = d[i];
		if unlikely(e.life >= 1.0) {
			this->Erase(i);		
		} else 
		{
			++i;
		}			
	}
}

template <typename T>
void remove_from_container(int idx, T&& cont) {
	cont[idx] = cont.back();
	cont.pop_back();
}

void CSimpleParticleSystemCollection::Erase(int idx) {
	remove_from_container(idx, d);
}

void CSimpleParticleSystemCollection::PreDraw() {
	auto spectatingFullView = gu->spectatingFullView;
	auto myAllyTeam = gu->myAllyTeam;
	auto& th = teamHandler;
	auto& lh = losHandler;
	
	const CCamera* cameras[] = {
		CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER),
		CCameraHandler::GetCamera(CCamera::CAMTYPE_UWREFL),
		CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW)
	};
	
	float timeOffset = globalRendering->timeOffset;
	for (int i =0; i < d.size(); ++i) {
		auto& e = d[i];
		e.colorMap->GetColor(e.color.data(), e.life);
		e.interPos = e.pos + e.speed * timeOffset;
	
		this->UpdateAnimParams(i);	

		e.visible = 
			e.alwaysVisible || (spectatingFullView || (th.IsValidAllyTeam(e.allyTeam) && 
						th.Ally(e.allyTeam, myAllyTeam) ||
						(lh->InLos(e.pos, myAllyTeam) || 
						 lh->InAirLos(e.pos, myAllyTeam))));			

		e.visibleRefraction = e.visible && e.interPos.y <= e.drawRadius && cameras[0]->InView(e.interPos, e.drawRadius);
		e.visibleReflection = e.visible && cameras[1]->InView(e.interPos, e.drawRadius);
		e.visibleShadow = e.castShadow && e.visible && cameras[2]->InView(e.interPos, e.drawRadius);	
	}
}

void CSimpleParticleSystemCollection::Draw(bool drawRefraction)
{
	bool drawReflection = !drawRefraction;

	auto* cam = camera;

	// streflop faster alternative
	const static auto safeANormalize = [&](const auto& ydir) {	
		const float sql = ydir.SqLength();
		if likely(sql > float3::nrm_eps()) {
			return ydir * fastmath::isqrt_nosse(sql);
		}
		return ydir;
	};

	auto cpos = cam->GetPos();
	auto fwd = camera->GetForward();
	
	// TODO branchless processing?
	for (int i =0; i < d.size(); ++i) {
		auto& e = d[i];
		
		if (drawRefraction && !e.visibleRefraction) {
			continue;
		} else if (drawReflection && !e.visibleReflection) {
			continue;
		}
		
		const float3 zdir = safeANormalize(e.pos - cpos);
		float3 ydir = zdir.cross(e.speed);
		float yDirLen2 = ydir.SqLength();
		ydir = safeANormalize(ydir);
		const float3 xdir = ydir.cross(zdir);

		if (e.directional && yDirLen2 > 0.001f)
		{
			e.bounds = {
				-ydir * e.size - xdir * e.size,
				-ydir * e.size + xdir * e.size,
				 ydir * e.size + xdir * e.size,
				 ydir * e.size - xdir * e.size
			};
			if (std::fabs(e.rotVal) > 0.01f) {
				float3::rotate<false>(e.rotVal, zdir, e.bounds);
			}
		}
		else
		{
			const float3 cameraRight = camera->GetRight() * e.size;
			const float3 cameraUp    = camera->GetUp()    * e.size;				
			e.bounds = {
				-cameraRight - cameraUp,
				 cameraRight - cameraUp,
				 cameraRight + cameraUp,
				-cameraRight + cameraUp
			};
			if (std::fabs(e.rotVal) > 0.01f) {
				float3::rotate<false>(e.rotVal, fwd, e.bounds);
			}
		}

		AddEffectsQuad(
			{ e.interPos + e.bounds[0], e.texture->xstart, e.texture->ystart, e.color.data() },
			{ e.interPos + e.bounds[1], e.texture->xend,   e.texture->ystart, e.color.data() },
			{ e.interPos + e.bounds[2], e.texture->xend,   e.texture->yend,   e.color.data() },
			{ e.interPos + e.bounds[3], e.texture->xstart, e.texture->yend,   e.color.data() },
			e.anims, e.aprogress
		);
		
		
		// add order so we can sort them later
		// TODO drawOrder can be negative?
		if (rb.GetSortMode()) {
			uint64_t order (static_cast<uint64_t>(e.drawOrder) << 32 | static_cast<uint32_t>(-cam->ProjectedDistance(e.pos)));
			AddQuadOrder(order);		
		}
	}	
}

void CSimpleParticleSystemCollection::DrawShadow() {
	// visibility
	const bool shadowPass = (camera->GetCamType() == CCamera::CAMTYPE_SHADOW);
	assert(shadowPass);
	
	auto* cam = camera;
	
	// streflop alternative
	const static auto safeANormalize = [&](const auto& ydir) {	
		const float sql = ydir.SqLength();
		if likely(sql > float3::nrm_eps()) {
			return ydir * fastmath::isqrt_nosse(sql);
		}
		return ydir;
	};

	auto cpos = cam->GetPos();
	auto fwd = camera->GetForward();
	
	for (int i =0; i < d.size(); ++i) {
		auto& e = d[i];
		if (!e.visibleShadow) {
			continue;
		}
		const float3 zdir = safeANormalize(e.pos - cpos);
		float3 ydir = zdir.cross(e.speed);
		ydir = safeANormalize(ydir);


		const float3 cameraRight = camera->GetRight() * e.size;
		const float3 cameraUp    = camera->GetUp()    * e.size;				
		e.bounds = {
			-cameraRight - cameraUp,
			 cameraRight - cameraUp,
			 cameraRight + cameraUp,
			-cameraRight + cameraUp
		};
		if (std::fabs(e.rotVal) > 0.01f) {
			float3::rotate<false>(e.rotVal, fwd, e.bounds);
		}


		AddEffectsQuad(
			{ e.interPos + e.bounds[0], e.texture->xstart, e.texture->ystart, e.color.data() },
			{ e.interPos + e.bounds[1], e.texture->xend,   e.texture->ystart, e.color.data() },
			{ e.interPos + e.bounds[2], e.texture->xend,   e.texture->yend,   e.color.data() },
			{ e.interPos + e.bounds[3], e.texture->xstart, e.texture->yend,   e.color.data() },
			e.anims, e.aprogress
		);
		
		// shadow not sorted
	}	
}

void AddMiniMapVertices(VA_TYPE_C&& v1, VA_TYPE_C&& v2)
{
	auto& mmLnsRB = CProjectile::GetMiniMapLinesRB();
	auto& mmPtsRB = CProjectile::GetMiniMapPointsRB();
	if (v1.pos.equals(v2.pos)) {
		mmPtsRB.AddVertex(std::move(v1));
	}
	else {
		mmLnsRB.AddVertex(std::move(v1));
		mmLnsRB.AddVertex(std::move(v2));
	}
}

void CSimpleParticleSystemCollection::DrawOnMinimap() {
	for (int i =0; i < d.size(); ++i) {
		auto& e = d[i];
		if (!e.visible)
			continue;
		AddMiniMapVertices({ e.pos, color4::whiteA }, { e.pos + e.speed, color4::whiteA });
	}
}
	
void CSimpleParticleSystemCollection::UpdateAnimParams(int idx) {
	int gameFrame = gs->frameNum;
	float timeOffset = globalRendering->timeOffset;
	
	auto& e = d[idx];

	auto& animParams = e.anims;
	auto& animProgress = e.aprogress;
	if (static_cast<int>(animParams.x) <= 1 && static_cast<int>(animParams.y) <= 1) {
		animProgress = 0.0f;
		return;
	}

	const float t = (gameFrame + timeOffset - e.createFrame);
	const float animSpeed = std::fabs(animParams.z);

	if (animParams.z < 0.0f) {
		animProgress = 1.0f - std::fabs(std::fmod(t, 2.0f * animSpeed) / animSpeed - 1.0f);
	}
	else {
		animProgress = std::fmod(t, animSpeed) / animSpeed;
	}
}
