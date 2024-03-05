/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_COMPONENTS_H__
#define UNIT_COMPONENTS_H__

#include "System/Ecs/Components/BaseComponents.h"
#include "System/type2.h"


namespace Unit {

ALIAS_COMPONENT(UnitId, int);
VOID_COMPONENT(InBuildStance);


// Used by Build Task Entity
struct TerraformBuildTask {
    // Region to terraform - names are a carry-over from original code.
    int tz1, tz2, tx1, tx2;

    // Terraform effort to carry out
    float remaining = 0.f;

    // Used to indicate that the terraform task is for flattening (if a buildee is assigned.)
    // rather than restore.
    entt::entity buildee = entt::null;
};
VOID_COMPONENT(TerraformTaskComplete);


// Used by Unit and Build Task Entities
DOUBLE_LINKED_LIST_COMPONENT(TerraformTaskChain);


// Used by Unit Entities
ALIAS_COMPONENT(TerraformTaskReference, entt::entity);
ALIAS_COMPONENT(TerraformTaskBuildPower, float);



// NOTE: add this to the save/load utility
template<class Archive, class Snapshot>
void serializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component
        < UnitId, InBuildStance
        , TerraformBuildTask, TerraformTaskChain, TerraformTaskBuildPower, TerraformTaskComplete, TerraformTaskReference
        >(archive);
}

}

#endif