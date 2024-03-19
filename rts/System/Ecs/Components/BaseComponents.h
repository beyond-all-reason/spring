/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BASE_COMPONENTS_H__
#define BASE_COMPONENTS_H__

#include <utility>

#include "System/Ecs/EcsMain.h"

template<class T>
struct BasicComponentType {
    T value = 0;
};

template<>
struct BasicComponentType<entt::entity> {
    entt::entity value{entt::null};
};

#define ALIAS_COMPONENT_DEF(Component, T, DefaultValue) \
struct Component : public BasicComponentType<T> { \
    Component() { value = DefaultValue; } \
    Component(T val) { value = std::move(val); } \
    ~Component() = default; \
    Component(const Component &) = default; \
    Component& operator=(const Component &) = default; \
    Component(Component &&) = default; \
    Component& operator=(Component &&) = default; \
};
#define ALIAS_COMPONENT(Component, T) \
struct Component : public BasicComponentType<T> { \
    Component() = default; \
    Component(T val) { value = std::move(val); } \
    ~Component() = default; \
    Component(const Component &) = default; \
    Component& operator=(const Component &) = default; \
    Component(Component &&) = default; \
    Component& operator=(Component &&) = default; \
};
#define VOID_COMPONENT(Component) struct Component {};

#endif