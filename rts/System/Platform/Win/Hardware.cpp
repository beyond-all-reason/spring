/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/Hardware.h"

#include <windows.h>

namespace Platform {
MEMORYSTATUSEX GetMemoryInfo()
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status;
}

uint64_t TotalRAM()
{
	MEMORYSTATUSEX status = GetMemoryInfo();
	return status.ullTotalPhys;
}

uint64_t TotalPageFile()
{
	MEMORYSTATUSEX status = GetMemoryInfo();
	return status.ullTotalPageFile;
}
} // namespace Platform
