/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class HeatCloudParticle : public CProjectile
{
	CR_DECLARE_DERIVED(HeatCloudParticle)
public:
	HeatCloudParticle();
	/// projectile starts at size 0 and ends at size \<size\>
	HeatCloudParticle(
		CUnit* owner,
		const float3& pos,
		const float3& speed,
		const float temperature,
		const float size
	);

	void Serialize(creg::ISerializer* s);

	void Draw() override;
	void Update() override;

	void Init(const CUnit* owner, const float3& offset) override;

	int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	float heat;
	float maxheat;
	float heatFalloff;
public:
	float size;
private:
	float sizeGrowth;
	float sizemod;
	float sizemodmod;

	AtlasedTexture* texture;
};

using HeatCloudProjectile = HeatCloudParticle;
