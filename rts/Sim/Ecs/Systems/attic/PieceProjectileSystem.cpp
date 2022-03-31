#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/ProjectileComponents.h"
#include "PieceProjectileSystem.h"

#include "System/TimeProfiler.h"

PieceProjectileSystem projectileSystem;

using namespace Projectiles;

void PieceProjectileSystem::Init()
{
}

void PieceProjectileSystem::Update()
{
    UpdatePhysics();
    UpdateState();
}

void PieceProjectileSystem::UpdatePhysics()
{
    auto projectilesToUpdate = EcsMain::registry.group<Position, Velocity, Direction, Speed, Gravity>(entt::exclude<LuaMoveCtrl>);

    for(auto entity: projectilesToUpdate) {
        auto [pos, vel, dir, speed, grav] = projectilesToUpdate.get<Position, Velocity, Direction, Speed, Gravity>(entity);

        // speed.y += mygravity;
        vel.velocity.y += grav.gravity;

        // SetVelocityAndSpeed(speed * 0.997f);
        vel.velocity *= 0.997f;
        speed.speed = vel.velocity.Length();
        dir.direction = vel.velocity / speed.speed;

        // SetPosition(pos + speed);
		pos.position += vel.velocity;
	}
}

void PieceProjectileSystem::UpdateState()
{
    auto projectilesToUpdate = EcsMain::registry.view<PieceProjectile>(entt::exclude<Collidable>);

    for(auto entity: projectilesToUpdate) {
        auto& proj = projectilesToUpdate.get<PieceProjectile>(entity);

        proj.age += 1;
        if (proj.age > 10)
            EcsMain::registry.emplace<Collidable>(entity);
    }
}

void PieceProjectileSystem::AddProjectile(CProjectile* projectile)
{
    auto entity = EcsMain::registry.create();

    EcsMain::registry.emplace<ID>(entity, projectile->id);
    EcsMain::registry.emplace<ProjectileRef>(entity, projectile);
    EcsMain::registry.emplace<Position>(entity, projectile->pos);
    EcsMain::registry.emplace<Velocity>(entity, projectile->speed);
    EcsMain::registry.emplace<Direction>(entity, projectile->dir);
    EcsMain::registry.emplace<Speed>(entity, projectile->speed.w);
    EcsMain::registry.emplace<Gravity>(entity, projectile->mygravity);
    EcsMain::registry.emplace<Owner>(entity, projectile->GetOwnerID());
    EcsMain::registry.emplace<UseAirLos>(entity, projectile->useAirLos);

    auto pieceProjectile = dynamic_cast<CPieceProjectile*>(projectile);
    if (pieceProjectile) {
        EcsMain::registry.emplace<PieceProjectile>(entity, pieceProjectile->age);
    }

    // Temporary setup
    projectile->entityId = (uint32_t)entity;
}

void PieceProjectileSystem::RemoveProjectile(CProjectile* projectile)
{
    auto view = EcsMain::registry.view<const ID>();
    entt::entity entity = (entt::entity)projectile->entityId;

    // for(auto ientity: view) {
    //     // a component at a time ...
    //     auto &id = view.get<const ID>(ientity);
    //     if (id.id == projectile->id){
    //         entity = ientity;
    //         break;
    //     }
    // }
    
    if (EcsMain::registry.valid(entity))
        EcsMain::registry.destroy(entity);
}
