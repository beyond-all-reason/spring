/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "SyncObjectsSystem.h"

#include "Sim/Ecs/Registry.h"
#include "Sim/Objects/Components/SyncObjectsComponents.h"
#include "Sim/Objects/SolidObject.h"
#include "System/TimeProfiler.h"

using namespace Objects;

void SyncObjectsSystem::Init() {
    Sim::systemGlobals.CreateSystemComponent<SyncObjectsSystemComponent>();

    Sim::systemUtils.OnUpdate().connect<&SyncObjectsSystem::Update>();
}

template<class TSync, class TReal>
void sync(TSync& syncValue, TReal& realValue) {
    if (syncValue != realValue)
        syncValue = realValue;
}

template<>
void sync(SyncedFloat3& syncValue, float3& realValue) {
    if (! syncValue.bitExactEquals(realValue))
        syncValue = realValue;
}

void SyncObjectsSystem::Update() {
    SCOPED_TIMER("Ecs::SyncObjectsSystem::Update");

    auto& comp = Sim::systemGlobals.GetSystemComponent<SyncObjectsSystemComponent>();
    uint32_t objectCount = 0;

    auto view = Sim::registry.group<SolidObjectSync, SolidObjectRef>();
    view.each([&](SolidObjectSync& syncObject, SolidObjectRef& objectRef){
        CSolidObject& realObject = *objectRef.value;

        sync(syncObject.frontdir, realObject.frontdir);
        sync(syncObject.pos, realObject.pos);
        // sync(syncObject.rightdir, realObject.rightdir);
        // sync(syncObject.updir, realObject.updir);

        // sync(syncObject.relMidPos, realObject.relMidPos);
        // sync(syncObject.relAimPos, realObject.relAimPos);

        // sync(syncObject.midPos, realObject.midPos);
        // sync(syncObject.aimPos, realObject.aimPos);

        // sync(syncObject.heading, realObject.heading);
        // sync(syncObject.buildFacing, realObject.buildFacing);

        objectCount++;
    });

    comp.objectCount = static_cast<uint32_t>(view.size());

    LOG("SyncObjects checked: %i", objectCount);
}

void SyncObjectsSystem::Shutdown() {
    Sim::systemUtils.OnUpdate().disconnect<&SyncObjectsSystem::Update>();
}
