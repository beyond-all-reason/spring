#ifndef SOLID_OBJECT_COMPONENTS_H__
#define SOLID_OBJECT_COMPONENTS_H__

#include "BaseComponents.h"

namespace SolidObject {

ALIAS_COMPONENT(Health, float)
ALIAS_COMPONENT(MaxHealth, float)

template<class Archive, class Snapshot>
void serializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component
        < Health, MaxHealth
        >(archive);
}

}

#endif