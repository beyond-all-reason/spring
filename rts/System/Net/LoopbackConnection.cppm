module;

#if defined(__cpp_modules) && (__cpp_modules >= 201907L)

import Recoil.System.Net.Connection;
import Recoil.System.Net.LoopbackConnection;

#else

#include "Connection.h"
#include "LoopbackConnection.h"

#endif

export module Recoil.System.Net.LoopbackConnection;

export namespace netcode {
	using ::netcode::CLoopbackConnection;
}