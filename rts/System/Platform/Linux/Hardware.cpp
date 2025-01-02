/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <unistd.h>

#include "System/Platform/Hardware.h"

namespace Platform
{
	uint64_t TotalRAM() {
		long pages = sysconf(_SC_PHYS_PAGES);
		long page_size = sysconf(_SC_PAGE_SIZE);
		return pages * page_size;
	}
	uint64_t TotalPageFile() {
		// NOT IMPLEMENTED
		return 0;
	}

}
