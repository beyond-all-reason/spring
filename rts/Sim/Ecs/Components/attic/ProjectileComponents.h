#ifndef PROJECTILE_COMPONENTS_H__
#define PROJECTILE_COMPONENTS_H__

#include "Sim/Projectiles/Projectile.h"

//#define OPTIMIZATION_VFLOAT

namespace Projectiles {

struct ID {
    int id;
};

struct ProjectileRef {
    CProjectile* projectileRef;
};

// World Object
struct UseAirLos {
    bool useAirLos;
};

// Projectile
struct Owner {
    unsigned int ownerID;
};

// Is present then the projectile is controlled by Lua
struct LuaMoveCtrl {
    bool luaMoveCtrl;
};

// If present, then collision is enabled
struct Collidable {};

// weapon projectile properties
struct Ttl {
    int ttl;
};

// Specific to piece projectile
struct PieceProjectile {
    int age;
};


#ifdef OPTIMIZATION_VFLOAT

#include "System/vfloat4.h"


typedef vfloat4 Position;
typedef vfloat4 Velocity;
typedef vfloat4 Direction;
typedef float Speed;
typedef float Gravity;

static constexpr vfloat4  UpVec(0.0f, 1.0f, 0.0f);

#else

#include "System/float3.h"

struct Position {
    float3 position;
};

struct Velocity {
    float3 velocity;
};

struct Direction {
    float3 direction;
};

struct Speed {
    float speed;
};

struct Gravity {
    float gravity;
};

static constexpr float3  UpVec(0.0f, 1.0f, 0.0f);

#endif

}

#endif