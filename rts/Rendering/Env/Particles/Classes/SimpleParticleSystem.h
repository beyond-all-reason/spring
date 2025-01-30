/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIMPLE_PARTICLE_SYSTEM_H
#define SIMPLE_PARTICLE_SYSTEM_H

#include "Sim/Projectiles/Projectile.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/float3.h"

class CUnit;
class CColorMap;

class CSimpleParticleSystem : public CProjectile
{
	CR_DECLARE_DERIVED(CSimpleParticleSystem)
	CR_DECLARE_SUB(Particle)

public:
	CSimpleParticleSystem();
	virtual ~CSimpleParticleSystem() { particles.clear(); }

	void Serialize(creg::ISerializer* s);

	void Draw() override;
	void Update() override;
	void Init(const CUnit* owner, const float3& offset) override;

	int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

protected:
	float3 emitVector;
	float3 emitMul;
	float3 gravity;
	float particleSpeed;
	float particleSpeedSpread;

	float emitRot;
	float emitRotSpread;

	AtlasedTexture* texture;
	CColorMap* colorMap;
	bool directional;

	float particleLife;
	float particleLifeSpread;
	float particleSize;
	float particleSizeSpread;
	float airdrag;
	float sizeGrowth;
	float sizeMod;

	int numParticles;

	struct Particle
	{
		CR_DECLARE_STRUCT(Particle)

		float3 pos;
		float3 speed;

		float rotVal;
		float rotVel;

		float life;
		float decayrate;
		float size;
		float sizeGrowth;
		float sizeMod;
	};

protected:
	 std::vector<Particle> particles;
};

/**
* old CSphereParticleSpawner (it used to spawns the particles as independent CProjectile objects)
* has proven to be slower
*/
class CSphereParticleSpawner : public CSimpleParticleSystem {
	CR_DECLARE_DERIVED(CSphereParticleSpawner)
public:
	CSphereParticleSpawner() {}
	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo) {
		return CSimpleParticleSystem::GetMemberInfo(memberInfo);
	}
};

#endif // SIMPLE_PARTICLE_SYSTEM_H
