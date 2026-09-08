/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if defined(__cpp_modules) && (__cpp_modules >= 201907L)

import Recoil.System.Net.Exception;
import Recoil.System.Net.RawPacket;
import Recoil.System.Net.UnpackPacket;

#else

#include "Exception.h"
#include "RawPacket.h"
#include "UnpackPacket.h"

#endif

#include <memory>

namespace netcode
{

UnpackPacket::UnpackPacket(std::shared_ptr<const RawPacket> packet, size_t skipBytes)
	: pckt(packet)
	, pos(skipBytes)
{
	if (pos > pckt->length) {
		throw UnpackPacketException("Unpack failure (byte skip)");
	}
}

}
