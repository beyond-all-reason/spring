/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Sim/Projectiles/Projectile.h"
#include "System/float3.h"

class CUnit;

class CSmokeParticle2 : public CProjectile
{
	CR_DECLARE_DERIVED(CSmokeParticle2)

public:
	CSmokeParticle2();
	CSmokeParticle2(
		CUnit* owner,
		const float3& pos,
		const float3& wantedPos,
		const float3& speed,
		float ttl,
		float startSize,
		float sizeExpansion,
		float color = 0.7f
	);

	void Update() override;
	void Draw() override;
	void Init(const CUnit* owner, const float3& offset) override;

	int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	float color;
	float age;
	float ageSpeed;
	float size;
	float startSize;
	float sizeExpansion;
	int textureNum;
	float3 wantedPos;
	float glowFalloff;
};

using CSmokeProjectile2 = CSmokeParticle2;