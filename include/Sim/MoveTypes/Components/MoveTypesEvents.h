/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVE_TYPE_EVENTS_H__
#define MOVE_TYPE_EVENTS_H__

#include "System/float3.h"

class CUnit;
class CFeature;
class CGroundMoveType;

namespace MoveTypes {

struct FeatureCollisionEvent {
    CUnit* collider;
    CFeature* collidee;

    FeatureCollisionEvent(CUnit* _collider, CFeature* _collidee)
    : collider(_collider)
    , collidee(_collidee)
    {}
};

struct FeatureCrushEvent {
    CUnit* collider;
    CFeature* collidee;
    float3 crushImpulse;

    FeatureCrushEvent(CUnit* _collider, CFeature* _collidee, float3 _crushImpulse)
    : collider(_collider)
    , collidee(_collidee)
    , crushImpulse(_crushImpulse)
    {}
};

struct FeatureMoveEvent {
    CUnit* collider;
    CFeature* collidee;
    float3 moveImpulse;

    FeatureMoveEvent(CUnit* _collider, CFeature* _collidee, float3 _moveImpulse)
    : collider(_collider)
    , collidee(_collidee)
    , moveImpulse(_moveImpulse)
    {}
};

struct UnitCollisionEvent {
    CUnit* collider;
    CUnit* collidee;

    UnitCollisionEvent(CUnit* _collider, CUnit* _collidee)
    : collider(_collider)
    , collidee(_collidee)
    {}
};

struct UnitCrushEvent {
    CUnit* collider;
    CUnit* collidee;
    float3 crushImpulse;

    UnitCrushEvent(CUnit* _collider, CUnit* _collidee, float3 _crushImpulse)
    : collider(_collider)
    , collidee(_collidee)
    , crushImpulse(_crushImpulse)
    {}
};

struct UnitMovedEvent {
    CUnit* unit = nullptr;
    bool moved = false;
};

struct ChangeHeadingEvent {
    int unitId;
    short deltaHeading = 0;
    bool changed = false;

    ChangeHeadingEvent(int _unitId)
    : unitId(_unitId)
    {}
};

struct ChangeMainHeadingEvent {
    int unitId;
    bool changed = false;

    ChangeMainHeadingEvent(int _unitId)
    : unitId(_unitId)
    {}
};

}

#endif