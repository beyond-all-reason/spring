/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVE_TYPE_EVENTS_H__
#define MOVE_TYPE_EVENTS_H__

#include "System/float3.h"

class CUnit;
class CFeature;

namespace MoveTypes {

struct FeatureCollisionEvent {
    CUnit* collider;
    CFeature* collidee;
    int id;

    FeatureCollisionEvent(int _id, CUnit* _collider, CFeature* _collidee)
    : id(_id)
    , collider(_collider)
    , collidee(_collidee)
    {}
};

struct FeatureCrushEvent {
    CUnit* collider;
    CFeature* collidee;
    float3 crushImpulse;
    int id;

    FeatureCrushEvent(int _id, CUnit* _collider, CFeature* _collidee, float3 _crushImpulse)
    : id(_id)
    , collider(_collider)
    , collidee(_collidee)
    , crushImpulse(_crushImpulse)
    {}
};

struct FeatureMoveEvent {
    CUnit* collider;
    CFeature* collidee;
    float3 moveImpulse;
    int id;

    FeatureMoveEvent(int _id, CUnit* _collider, CFeature* _collidee, float3 _moveImpulse)
    : id(_id)
    , collider(_collider)
    , collidee(_collidee)
    , moveImpulse(_moveImpulse)
    {}
};

struct UnitCollisionEvent {
    CUnit* collider;
    CUnit* collidee;
    int id;

    UnitCollisionEvent(int _id, CUnit* _collider, CUnit* _collidee)
    : id(_id)
    , collider(_collider)
    , collidee(_collidee)
    {}
};

struct UnitCrushEvent {
    CUnit* collider;
    CUnit* collidee;
    float3 crushImpulse;
    int id;

    UnitCrushEvent(int _id, CUnit* _collider, CUnit* _collidee, float3 _crushImpulse)
    : id(_id)
    , collider(_collider)
    , collidee(_collidee)
    , crushImpulse(_crushImpulse)
    {}
};

struct UnitMovedEvent {
    int id;
    CUnit* unit;

    UnitMovedEvent(int _id, CUnit* _unit)
    : id(_id)
    , unit(_unit)
    {}
};

}

#endif