/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "SaveLoadUtils.h"

#include "cereal/archives/binary.hpp"

#include "System/float3.h"
#include "System/Ecs/Components/BaseComponents.h"
#include "System/Ecs/Utils/SystemUtils.h"
#include "System/Log/ILog.h"
#include "Sim/Misc/Resource.h"
#include "Sim/MoveTypes/Components/MoveTypesComponents.h"



// Partial Specialization of Function Templates are not allowed so we can't do this
//
// template<class Archive, class BaseComponentType>
// void serialize(Archive &ar, BaseComponentType &c) { ar(c.value); }
//
// and then the other type specializations

template<class Archive>
void serialize(Archive &ar, BasicComponentType<float> &c) { ar(c.value); }

template<class Archive>
void serialize(Archive &ar, BasicComponentType<int> &c) { ar(c.value); }

template<class Archive>
void serialize(Archive &ar, BasicComponentType<entt::entity> &c) { ar(c.value); }

template<class Archive>
void serialize(Archive &ar, BasicComponentType<std::size_t> &c) { ar(c.value); }

// template<class Archive>
// void serialize(Archive &ar, SResourcePack &c) { ar(c.res1, c.res2, c.res3, c.res4); }

template<class Archive>
void serialize(Archive &ar, float3 &c) { ar(c.x, c.y, c.z); }



template<class S, class T>
void ProcessComponents(T&& archive, S&& snapshot) {
    snapshot.entities(archive);

    MoveTypes::serializeComponents(archive, snapshot);
}

using namespace Sim;

void SaveLoadUtils::LoadComponents(std::stringstream &iss) {
    systemUtils.NotifyPreLoad();

    auto archive = cereal::BinaryInputArchive{iss};
    LOG_L(L_DEBUG, "%s: Entities before clear is %d", __func__, (int)registry.alive());
    registry.each([this](entt::entity entity) { registry.destroy(entity); });

    assert(registry.alive() == 0);

    LOG_L(L_DEBUG, "%s: Entities after clear is %d (%d)", __func__, (int)registry.alive(), (int)iss.tellg());
    {ProcessComponents<entt::snapshot_loader>(archive, entt::snapshot_loader{registry});}
    LOG_L(L_DEBUG, "%s: Entities after load is %d (%d)", __func__, (int)registry.alive(), (int)iss.tellg());

    {
        archive(systemGlobals);
    }
    systemUtils.NotifyPostLoad();
}

void SaveLoadUtils::SaveComponents(std::stringstream &oss) {
    auto archive = cereal::BinaryOutputArchive{oss};
    LOG_L(L_DEBUG, "%s: Entities before save is %d (%d)", __func__, (int)registry.alive(), (int)oss.tellp());
    {ProcessComponents<entt::snapshot>(archive, entt::snapshot{registry});}
    LOG_L(L_DEBUG, "%s: Save bytes writen %d", __func__, (int)oss.tellp());

    {
        archive(systemGlobals);
    }
}
