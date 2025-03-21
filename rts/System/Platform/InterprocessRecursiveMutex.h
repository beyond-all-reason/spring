#pragma once

#ifndef _WIN32
#include "System/Platform/Linux/InterprocessRecursiveMutex.h"
#else
#include "System/Platform/Win/InterprocessRecursiveMutex.h"
#endif