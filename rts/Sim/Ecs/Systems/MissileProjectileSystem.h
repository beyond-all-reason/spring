#ifndef MISSILE_PROJECTILE_SYSTEM_H__
#define MISSILE_PROJECTILE_SYSTEM_H__

#include "lib/entt/entt.hpp"

#include "Sim/Projectiles/Projectile.h"

class MissileProjectileSystem {
public:
    void Init();
    void Update();
    void AddProjectile(CProjectile* projectile);
    void RemoveProjectile(CProjectile* projectile);

private:

};

extern MissileProjectileSystem missileProjectileSystem;

#endif