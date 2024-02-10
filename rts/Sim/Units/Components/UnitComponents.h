/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_COMPONENTS_H__
#define UNIT_COMPONENTS_H__

#include "System/Ecs/Components/BaseComponents.h"
#include "System/Ecs/Utils/DoubleLinkedList.h"
#include "System/type2.h"


namespace Unit {

struct TerraformBuildTask {
    int2 mapPos;
    int2 size;
    float targetHeight = 0.f;
    float originalHeight = 0.f;
};

DLL_COMPONENT(TerraformTaskChain);
ALIAS_COMPONENT(TerraformTaskBuildPower, float);

// NOTE: add this the save/load utility
template<class Archive, class Snapshot>
void serializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component
        < TerraformBuildTask, TerraformTaskChain, TerraformTaskBuildPower
        >(archive);
}

}

#endif