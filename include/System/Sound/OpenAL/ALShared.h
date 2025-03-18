/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AL_SHARED_H_
#define _AL_SHARED_H_

#include <al.h>

bool CheckError(const char* msg);

static constexpr unsigned int DECODE_BUFFER_SIZE = 512 * 1024; // 512KB

#endif // _AL_SHARED_H_
