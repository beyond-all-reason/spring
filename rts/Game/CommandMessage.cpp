/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandMessage.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "System/Misc/TracyDefs.h"
#include "System/Net/PackPacket.h"
#include "System/Net/UnpackPacket.h"

#include <cassert>

using namespace netcode;

CommandMessage::CommandMessage(const std::string& cmd, int playerID)
    : action(Action(cmd))
    , playerID(playerID)
{
}

CommandMessage::CommandMessage(const Action& action, int playerID)
    : action(action)
    , playerID(playerID)
{
}

CommandMessage::CommandMessage(std::shared_ptr<const netcode::RawPacket> pckt)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(pckt->data[0] == NETMSG_CCOMMAND);
	UnpackPacket packet(pckt, 3);
	packet >> playerID;
	packet >> action.command;
	packet >> action.extra;
}

const netcode::RawPacket* CommandMessage::Pack() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	unsigned short size = 3 + sizeof(playerID) + action.command.size() + action.extra.size() + 2;
	PackPacket* buffer = new PackPacket(size, NETMSG_CCOMMAND);
	*buffer << size;
	*buffer << playerID;
	*buffer << action.command;
	*buffer << action.extra;
	return buffer;
}
