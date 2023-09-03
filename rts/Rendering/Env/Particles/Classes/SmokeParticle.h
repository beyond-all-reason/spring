/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Sim/Projectiles/Projectile.h"
#include "System/float3.h"

class CUnit;

class CSmokeParticle : public CProjectile
{
	CR_DECLARE_DERIVED(CSmokeParticle)

public:
	CSmokeParticle();
	CSmokeParticle(
		CUnit* owner,
		const float3& pos,
		const float3& speed,
		float ttl,
		float startSize,
		float sizeExpansion,
		float color
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
public:
	float size;
private:
	float startSize;
	float sizeExpansion;
	int textureNum;
};

using CSmokeProjectile = CSmokeParticle;