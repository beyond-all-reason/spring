

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/ProjectileComponents.h"
#include "MissileProjectileSystem.h"

#include "System/TimeProfiler.h"

#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"

#include "System/Log/ILog.h"

MissileProjectileSystem missileProjectileSystem;

using namespace Projectiles;

void MissileProjectileSystem::Init()
{
}

void MissileProjectileSystem::Update()
{
    auto projectilesToUpdate = EcsMain::registry.group<ID, ProjectileRef, Position, Velocity, Direction, Speed, Gravity>();//(entt::get<Ttl, LuaMoveCtrl>);

    LOG("%s: projectile to update is %lld", __func__, projectilesToUpdate.size());

    for(auto entity: projectilesToUpdate) {
        auto [pref, pos, vel, dir, speed, grav] = projectilesToUpdate.get<ProjectileRef, Position, Velocity, Direction, Speed, Gravity>(entity);

        auto ref = pref.projectileRef;

        dir.direction = ref->dir;
        pos.position = ref->pos;
        vel.velocity = ref->speed;
        speed.speed = ref->speed.w;
        grav.gravity = ref->mygravity;
    }

    {
    //SCOPED_TIMER("Sim::Projectiles::Update");

    // for(auto entity: projectilesToUpdate) {
    //     auto [ttl, luaMoveCtrl, pos, vel, dir, speed, grav] = projectilesToUpdate.get<Ttl, LuaMoveCtrl, Position, Velocity, Direction, Speed, Gravity>(entity);
    //     //if (ttl.ttl > 0)
    // }

    // This is the deafult projectiles update
    for(auto entity: projectilesToUpdate) {
        LOG("%s: projectile to update is %d", __func__, (uint32_t)entity);
        auto [pos, vel, dir, speed, grav] = projectilesToUpdate.get<Position, Velocity, Direction, Speed, Gravity>(entity);

        vel.velocity += UpVec*grav.gravity;
        speed.speed = vel.velocity.Length();
        if (speed.speed >= 0.0f)
            dir.direction = vel.velocity / speed.speed;
        pos.position += vel.velocity; 
    }
    }

    for(auto entity: projectilesToUpdate) {
        auto [pref, pos, vel, dir, speed] = projectilesToUpdate.get<ProjectileRef, Position, Velocity, Direction, Speed>(entity);

        auto ref = pref.projectileRef;

        ref->dir = dir.direction;
        ref->pos = pos.position;
        ref->speed = vel.velocity;
        ref->speed.w = speed.speed;
    }
}

void MissileProjectileSystem::AddProjectile(CProjectile* projectile)
{
    auto entity = EcsMain::registry.create();

    EcsMain::registry.emplace<ID>(entity, projectile->id);
    EcsMain::registry.emplace<ProjectileRef>(entity, projectile);
    EcsMain::registry.emplace<Position>(entity, projectile->pos);
    EcsMain::registry.emplace<Velocity>(entity, projectile->speed);
    EcsMain::registry.emplace<Direction>(entity, projectile->dir);
    EcsMain::registry.emplace<Speed>(entity, projectile->speed.w);
    EcsMain::registry.emplace<Gravity>(entity, projectile->mygravity);
    EcsMain::registry.emplace<LuaMoveCtrl>(entity, projectile->luaMoveCtrl);

    auto mproj = dynamic_cast<CMissileProjectile*>(projectile);
    if (mproj){
        EcsMain::registry.emplace<Ttl>(entity, mproj->ttl);
    }

    LOG("%s: added projectile %d", __func__, projectile->id);
}

void MissileProjectileSystem::RemoveProjectile(CProjectile* projectile)
{
    auto view = EcsMain::registry.view<const ID>();
    entt::entity entity;

    for(auto ientity: view) {
        // a component at a time ...
        auto &id = view.get<const ID>(ientity);
        if (id.id == projectile->id){
            entity = ientity;
            break;
        }
    }
    
    if (EcsMain::registry.valid(entity))
        EcsMain::registry.destroy(entity);
}
