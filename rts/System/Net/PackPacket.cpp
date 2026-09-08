/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if defined(__cpp_modules) && (__cpp_modules >= 201907L)

import Recoil.System.Net.Exception;
import Recoil.System.Net.RawPacket;
import Recoil.System.Net.PackPacket;

#else

#include "Exception.h"
#include "RawPacket.h"
#include "PackPacket.h"

#endif