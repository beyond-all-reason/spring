#pragma once

#include "System/Matrix44f.h"
#include "System/SpringMem.h"

using MatricesMemStorage = spring::StablePosAllocator<CMatrix44f>;
static constexpr int INIT_NUM_ELEMS = 1 << 16u;

static MatricesMemStorage matricesMemStorage = MatricesMemStorage{ INIT_NUM_ELEMS };