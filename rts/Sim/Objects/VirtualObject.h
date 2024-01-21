/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VIRTUAL_OBJECT_H__
#define VIRTUAL_OBJECT_H__

#include "SolidObject.h"
#include "System/Threading/ThreadPool.h"

// Temporary until a better solution can be put together for tracking collision over pathing for
// units that can trvale above and below the surface of the water.
extern CSolidObject virtualObjects[ThreadPool::MAX_THREADS];

#endif