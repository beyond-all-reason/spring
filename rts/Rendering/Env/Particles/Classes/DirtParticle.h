/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class CDirtParticle : public CProjectile
{
	CR_DECLARE_DERIVED(CDirtParticle)
public:
	CDirtParticle();
	CDirtParticle(
		CUnit* owner,
		const float3& pos,
		const float3& speed,
		float ttl,
		float size,
		float expansion,
		float slowdown,
		const float3& color
	);

	void Serialize(creg::ISerializer* s);

	void Draw() override;
	void Update() override;

	int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	float alpha;
	float alphaFalloff;
	float size;
	float sizeExpansion;
	float slowdown;
	float3 color;

	AtlasedTexture* texture;
};

using CDirtProjectile = CDirtParticle;
