/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNC_OBJECTS_COMPONENTS_H__
#define SYNC_OBJECTS_COMPONENTS_H__

#include "System/Ecs/Components/BaseComponents.h"
#include "System/Sync/SyncedFloat3.h"

struct CSolidObject;

namespace Objects {

struct SolidObjectSync {
	SyncedFloat3 frontdir =  FwdVector;
    SyncedFloat3 pos;
	// SyncedFloat3 rightdir = -RgtVector;
	// SyncedFloat3    updir =   UpVector;

	// SyncedFloat3 relMidPos;
	// SyncedFloat3 relAimPos;
	// SyncedFloat3 midPos;
	// SyncedFloat3 aimPos;

	// SyncedSshort heading = 0;
	// SyncedSshort buildFacing = 0;
};

ALIAS_COMPONENT(SolidObjectRef, CSolidObject*);

// template<class Archive>
// void serialize(Archive &ar, UnitTrapCheck &c) { ar(c.type, c.id); }

// template<class Archive, class Snapshot>
// void serializeComponents(Archive &archive, Snapshot &snapshot) {
//     snapshot.template component
//         < GeneralMoveType, GroundMoveType, UnitTrapCheck
//         >(archive);
// }

struct SyncObjectsSystemComponent {
	static constexpr std::size_t page_size = 1;
    SyncedUint objectCount;
};

}

#endif