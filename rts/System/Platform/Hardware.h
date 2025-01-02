/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLATFORM_HARDWARE_H
#define PLATFORM_HARDWARE_H

#include <cstdint>

namespace Platform
{
	uint64_t TotalRAM();
	uint64_t TotalPageFile();
}

#endif // PLATFORM_HARDWARE_H
