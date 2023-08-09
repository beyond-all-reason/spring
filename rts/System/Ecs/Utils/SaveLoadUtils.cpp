#include "SaveLoadUtils.h"

#include "cereal/archives/binary.hpp"

// #include "Sim/Ecs/Components/BuildComponents.h"
// #include "Sim/Ecs/Components/EnvEconomyComponents.h"
// #include "Sim/Ecs/Components/FlowEconomyComponents.h"
// #include "Sim/Ecs/Components/SolidObjectComponent.h"
// #include "Sim/Ecs/Components/SystemGlobalComponents.h"
// #include "Sim/Ecs/Components/UnitComponents.h"
// #include "Sim/Ecs/Components/UnitEconomyComponents.h"
// #include "Sim/Ecs/Components/UnitEconomyReportComponents.h"
#include "System/Ecs/Utils/SystemUtils.h"
#include "Sim/Misc/Resource.h"
#include "System/Log/ILog.h"

#include "SystemGlobalUtils.h"


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

template<class Archive>
void serialize(Archive &ar, SResourcePack &c) { ar(c.res1, c.res2, c.res3, c.res4); }

template<class Archive>
void serialize(Archive &ar, float3 &c) { ar(c.x, c.y, c.z); }



template<class S, class T>
void ProcessComponents(T&& archive, S&& regSnapshot) {
    regSnapshot.entities(archive);

    // Build::serializeComponents(archive, regSnapshot);
    // EnvEconomy::serializeComponents(archive, regSnapshot);
    // FlowEconomy::serializeComponents(archive, regSnapshot);
    // SolidObject::serializeComponents(archive, regSnapshot);
    // SystemGlobals::serializeComponents(archive, regSnapshot);
    // Units::serializeComponents(archive, regSnapshot);
    // UnitEconomy::serializeComponents(archive, regSnapshot);
    // UnitEconomyReport::serializeComponents(archive, regSnapshot);
}


using namespace SystemGlobals;

void SaveLoadUtils::LoadComponents(std::stringstream &iss) {
    SystemUtils::systemUtils.NotifyPreLoad();

    auto archive = cereal::BinaryInputArchive{iss};
    LOG_L(L_DEBUG, "%s: Entities before clear is %d", __func__, (int)EcsMain::registry.alive());
    EcsMain::registry.each([](entt::entity entity) { EcsMain::registry.destroy(entity); });
    LOG_L(L_DEBUG, "%s: Entities after clear is %d (%d)", __func__, (int)EcsMain::registry.alive(), (int)iss.tellg());
    {ProcessComponents<entt::snapshot_loader>(archive, entt::snapshot_loader{EcsMain::registry});}
    LOG_L(L_DEBUG, "%s: Entities after load is %d (%d)", __func__, (int)EcsMain::registry.alive(), (int)iss.tellg());

    {
        using namespace SystemGlobals;
        archive(systemGlobals);
    }
    SystemUtils::systemUtils.NotifyPostLoad();
}

void SaveLoadUtils::SaveComponents(std::stringstream &oss) {
    auto archive = cereal::BinaryOutputArchive{oss};
    LOG_L(L_DEBUG, "%s: Entities before save is %d (%d)", __func__, (int)EcsMain::registry.alive(), (int)oss.tellp());
    {ProcessComponents<entt::snapshot>(archive, entt::snapshot{EcsMain::registry});}
    LOG_L(L_DEBUG, "%s: Save bytes writen %d", __func__, (int)oss.tellp());

    {
        using namespace SystemGlobals;
        archive(systemGlobals);
    }
}
