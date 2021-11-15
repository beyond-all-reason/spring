#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/PieceProjectileRenderingComponents.h"
#include "Sim/Ecs/Components/ProjectileComponents.h"
#include "PieceProjectileSimBasedRenderingSystem.h"

#include "Game/GlobalUnsynced.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "Rendering/Env/Particles/Classes/SmokeTrailProjectile.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Models/3DModel.h"

#define SMOKE_TIME 40

PieceProjectileSimBasedRenderingSystem projectileSystem;

using namespace Projectiles;

void PieceProjectileSimBasedRenderingSystem::Init()
{
}

float3 PieceProjectileSimBasedRenderingSystem::RandomVertexPos(SimBasedPieceProjectileRendering& sbr) const
{
	if (sbr.omp == nullptr)
		return ZeroVector;
	#define rf guRNG.NextFloat()
	return mix(sbr.omp->mins, sbr.omp->maxs, float3(rf,rf,rf));
}

void PieceProjectileSimBasedRenderingSystem::Update()
{
    auto projectilesToUpdate = EcsMain::registry.group<SimBasedPieceProjectileRendering>(entt::get<Position, Velocity, Direction, UseAirLos, Owner, PieceProjectile>);

    for(auto entity: projectilesToUpdate) {
        auto [pos, sbr] = projectilesToUpdate.get<Position, SimBasedPieceProjectileRendering>(entity);

        sbr.spinAngle += sbr.spinSpeed;

        if ((sbr.explFlags & PF_NoCEGTrail) == 0) {
            auto& vel = projectilesToUpdate.get<Velocity>(entity);
            // TODO: pass a more sensible ttl to the CEG (age-related?)
            explGenHandler.GenExplosion(sbr.cegID, pos.position, vel.velocity, 100, 0.0f, 0.0f, nullptr, nullptr);
            return;
        }

        if (sbr.explFlags & PF_Fire) {
            for (int a = CPieceProjectile::NUM_TRAIL_PARTS - 2; a >= 0; --a) {
                sbr.fireTrailPoints[a + 1] = sbr.fireTrailPoints[a];
            }

            CMatrix44f m(pos.position);
            m.Rotate(sbr.spinAngle * math::DEG_TO_RAD, sbr.spinVec);
            m.Translate(RandomVertexPos(sbr));

            sbr.fireTrailPoints[0].pos  = m.GetPos();
            sbr.fireTrailPoints[0].size = 1 + guRNG.NextFloat();
        }

        if (sbr.explFlags & PF_Smoke) {
            auto [dir, proj, airLos, owner] = projectilesToUpdate.get<Direction, PieceProjectile, UseAirLos, Owner>(entity);

            if (sbr.smokeTrail) {
                sbr.smokeTrail->UpdateEndPos(pos.position, dir.direction);
                sbr.oldSmokePos = pos.position;
                sbr.oldSmokeDir = dir.direction;
            }

            if ((proj.age % 8) == 0) {
                sbr.smokeTrail = projMemPool.alloc<CSmokeTrailProjectile>(
                    unitHandler.GetUnit(owner.ownerID),
                    pos, sbr.oldSmokePos,
                    dir, sbr.oldSmokeDir,
                    proj.age == (CPieceProjectile::NUM_TRAIL_PARTS - 1),
                    false,
                    14,
                    SMOKE_TIME,
                    0.5f,
                    projectileDrawer->smoketrailtex
                );

                airLos.useAirLos = sbr.smokeTrail->useAirLos;
            }
        }
    }
}


void PieceProjectileSimBasedRenderingSystem::AddProjectile(CProjectile* projectile)
{
    SimBasedPieceProjectileRendering sbr;
    auto entity = EcsMain::registry.create();

    auto pieceProjectile = dynamic_cast<CPieceProjectile*>(projectile);
    if (!pieceProjectile)
        return;

    sbr.cegID = pieceProjectile->cegID;
    sbr.explFlags = pieceProjectile->explFlags;
    sbr.fireTrailPoints = pieceProjectile->fireTrailPoints;

    EcsMain::registry.emplace<SimBasedPieceProjectileRendering>(entity, projectile->id);
}

void PieceProjectileSimBasedRenderingSystem::RemoveProjectile(CProjectile* projectile)
{
    // Nothing to do because the projectile will already be remvoed by the Simulated Projectile System.
}
