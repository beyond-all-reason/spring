#pragma once

#include "System/float3.h"
#include "System/float4.h"
#include "System/Color.h"

// needs Update()
struct alignas(16) StarburstTracerData {

};

// needs Update()
struct alignas(16) StarburstFlareData {

};

static_assert(sizeof(StarburstTracerData) % 16 == 0);
static_assert(sizeof(StarburstFlareData) % 16 == 0);