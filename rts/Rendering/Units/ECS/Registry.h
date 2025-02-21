/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SaveLoadUtils.h"
#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Components/BaseComponents.h"

#include "System/float3.h"
#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/Transform.hpp"
//#include "System/Ecs/Utils/SystemGlobalUtils.h"
//#include "System/Ecs/Utils/SystemUtils.h"

namespace RenderUnit {
    ALIAS_CLASS_COMPONENT(PrevFramePos, float3);
    ALIAS_CLASS_COMPONENT(PrevFrameRotMat, CMatrix44f);
    ALIAS_CLASS_COMPONENT(CurrFrameTransform, Transform);
    ALIAS_COMPONENT(UnitID, int);

    extern entt::registry registry;
}