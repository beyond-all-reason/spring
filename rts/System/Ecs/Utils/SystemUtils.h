/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYSTEM_UTILS_H__
#define SYSTEM_UTILS_H__

#include "System/Ecs/EcsMain.h"

namespace SystemUtils {

class SystemUtils {
public:
    void NotifyUpdate() {
        update.publish();
    }

    void NotifyPreLoad() {
        preLoad.publish();
    }

    void NotifyPostLoad() {
        postLoad.publish();
    }

    [[nodiscard]] auto OnUpdate() noexcept {
        return entt::sink{update};
    }

    [[nodiscard]] auto OnPreLoad() noexcept {
        return entt::sink{preLoad};
    }

    [[nodiscard]] auto OnPostLoad() noexcept {
        return entt::sink{postLoad};
    }

protected:
    entt::sigh<void()> update{};
    entt::sigh<void()> preLoad{};
    entt::sigh<void()> postLoad{};
};

}

#endif