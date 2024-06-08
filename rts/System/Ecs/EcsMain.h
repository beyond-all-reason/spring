/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ECS_MAIN_H__
#define ECS_MAIN_H__

#define ENTT_USE_ATOMIC

#include "lib/entt/src/entt/entt.hpp"

// class EcsMain {
// public:
//     static entt::registry registry;
// };

// template<typename TF, typename... TR>
// void AddComponentsIfNotExist(entt::entity entity) {
//     if (!EcsMain::registry.all_of<TF>(entity)){
//         EcsMain::registry.emplace<TF>(entity);
//     }
//     if constexpr (sizeof...(TR) > 0) {
//         AddComponentsIfNotExist<TR...>(entity);
//     }
// }

// // (auto defaultValue) requires -fconcepts-ts or c++20
// template<class T, class V>
// auto& GetOptionalComponent(entt::entity entity, V& defaultValue) {
//     auto checkPtr = EcsMain::registry.try_get<T>(entity);
//     return checkPtr != nullptr ? *checkPtr : defaultValue;
// }

// template<typename T>
// class ReleaseComponentOnExit {
// public:
//     ReleaseComponentOnExit(entt::entity scopedEntity): entity(scopedEntity) {}
//     ~ReleaseComponentOnExit() { EcsMain::registry.remove<T>(entity); }
// private:
//     entt::entity entity;
// };

// template<class T, typename V>
// void TryAddToComponent(entt::entity entity, V addition) {
//     auto comp = EcsMain::registry.try_get<T>(entity);
//     if (comp != nullptr)
//         *comp += addition;
// }


#endif