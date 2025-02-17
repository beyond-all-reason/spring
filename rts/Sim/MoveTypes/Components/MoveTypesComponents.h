/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVE_TYPE_COMPONENTS_H__
#define MOVE_TYPE_COMPONENTS_H__

#include "MoveTypesEvents.h"
#include "System/Ecs/Components/BaseComponents.h"
#include <System/Threading/ThreadPool.h>

struct CUnit;
struct CFeature;

namespace MoveTypes {

// For move types that need to be handled single threaded.
ALIAS_COMPONENT(GeneralMoveType, int);

// Special multi-thread ground move type.
ALIAS_COMPONENT(GroundMoveType, int);

// Used by units that have updated the ground collision map and may have trapped units as a result.
// This is used to allow such a situation to be detected immediately. The fall-back checks are too
// slow in practice.
enum UnitTrapCheckType {
    TRAPPER_IS_UNIT,
    TRAPPER_IS_FEATURE
};
struct UnitTrapCheck {
    UnitTrapCheckType type;
    int id;
};

template<class Archive>
void serialize(Archive &ar, UnitTrapCheck &c) { ar(c.type, c.id); }

template<class Archive, class Snapshot>
void serializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component
        < GeneralMoveType, GroundMoveType, UnitTrapCheck
        >(archive);
}

struct YardmapTrapCheckSystemComponent {
	static constexpr std::size_t page_size = 1;
    static constexpr std::size_t INITIAL_TRAP_UNIT_LIST_ALLOC_SIZE = 8;

	std::array<std::vector<CUnit*>, ThreadPool::MAX_THREADS> trappedUnitLists;
};

constexpr size_t UNIT_EVENT_VECTOR_RESERVE = 4;

ALIAS_COMPONENT_LIST_RESERVE(FeatureCollisionEvents, std::vector<FeatureCollisionEvent>, UNIT_EVENT_VECTOR_RESERVE);
ALIAS_COMPONENT_LIST_RESERVE(UnitCollisionEvents, std::vector<UnitCollisionEvent>, UNIT_EVENT_VECTOR_RESERVE);
ALIAS_COMPONENT_LIST_RESERVE(FeatureCrushEvents, std::vector<FeatureCrushEvent>, UNIT_EVENT_VECTOR_RESERVE);
ALIAS_COMPONENT_LIST_RESERVE(UnitCrushEvents, std::vector<UnitCrushEvent>, UNIT_EVENT_VECTOR_RESERVE);
ALIAS_COMPONENT_LIST_RESERVE(FeatureMoveEvents, std::vector<FeatureMoveEvent>, UNIT_EVENT_VECTOR_RESERVE);

}

#endif