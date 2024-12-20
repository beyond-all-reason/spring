/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AutohostInterface.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "System/Log/ILog.h"
#include "System/Net/Socket.h"

#include <cstring>
#include <vector>
#include <cinttypes>


#define LOG_SECTION_AUTOHOST_INTERFACE "AutohostInterface"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_AUTOHOST_INTERFACE)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_AUTOHOST_INTERFACE


namespace {

/**
 * @enum EVENT Which events can be sent to the autohost.
 *
 * Each packet from engine to autohost interface starts with a byte that
 * indicated the type of the message. Below in brackets we describe the data
 * associated with each message.
 *
 * char[] are *not* delimited in any way e.g. by '\0', so they can be used only
 * as a last element of the UDP packet.
 */
enum EVENT
{
	/**
	 * Server has started
	 *
	 *   ()
	 */
	SERVER_STARTED = 0,

	/**
	 * Server is about to exit
	 *
	 *   ()
	 */
	SERVER_QUIT = 1,

	/**
	 * Game starts
	 *
	 *   (uint32 msgsize, uint8[16] gameId, char[] demoName)
	 */
	SERVER_STARTPLAYING = 2,

	/**
	 * Game has ended
	 *
	 *   (uint8 player, uint8 msgsize, uint8[msgsize - 3] winningAllyTeamss)
	 */
	SERVER_GAMEOVER = 3,

	/**
	 * An information message from server
	 *
	 *   (char[] message)
	 */
	SERVER_MESSAGE = 4,

	/**
	 * A warning message from server
	 *
	 *   (char[] warningmessage)
	 */
	SERVER_WARNING = 5,

	/**
	 * Player has joined the game
	 *
	 *   (uint8 playernumber, char[] name)
	 */
	PLAYER_JOINED = 10,

	/**
	 * Player has left
	 *
	 *   (uint8 playernumber, uint8 reason)
	 *
	 * Reason:
	 *   - 0: lost connection
	 *   - 1: left
	 *   - 2: kicked
	 */
	PLAYER_LEFT = 11,

	/**
	 * Player has updated its ready-state
	 *
	 *   (uint8 playernumber, uint8 state)
	 *
	 * State:
	 *   - 0: not ready
	 *   - 1: ready
	 *   - 2: forced
	 *   - 3: failed to ready (in engine code it says it's not clear if possible)
	 */
	PLAYER_READY = 12,

	/**
	 * Player has sent a chat message
	 *
	 *   (uint8 playernumber, uint8 destination, char[] text)
	 *
	 * Destination can be any of:
	 *   - a playernumber
	 *   - TO_ALLIES = 252
	 *   - TO_SPECTATORS = 253
	 *   - TO_EVERYONE = 254
	 *   - TO_SERVER = 255
	 *
	 * Enum values from rts/Game/ChatMessage.h, value of 255 is a SERVER_PLAYER
	 * through out the engine. Engine doesn't do anything special with this
	 * message, it's just forwarded, which allows AutoHost implementations to
	 * use them for some purposes, e.g. ZK:
	 * https://github.com/ZeroK-RTS/Zero-K/blob/8bd789594153d8a37ef7c6fa0e2827d85ae82a64/LuaRules/Gadgets/awards.lua#L556
	 * https://github.com/ZeroK-RTS/Zero-K-Infrastructure/blob/c1b0aa21aeedb9f0b7d3fc51837135691e50afe8/Shared/LobbyClient/DedicatedServer.cs#L433
	 */
	PLAYER_CHAT = 13,

	/**
	 * Player has been defeated
	 *
	 *   (uint8 playernumber)
	 */
	PLAYER_DEFEATED = 14,

	/**
	 * Message sent by Lua script
	 *
	 *   (uint8 magic = 50, uint16 msgsize, uint8 playernumber, uint16 script, uint8 uiMode, uint8[msgsize - 8] data)
	 *
	 * The message data is a straight copy of the whole NETMSG_LUAMSG packet
	 * including the magic 50 byte. Take a look at CBaseNetProtocol::SendLuaMsg
	 * function and all of it's callers to see how it's constructed.
	 */
	GAME_LUAMSG = 20,

	/**
	 * Team statistics
	 *
	 *   (uint8 teamnumber, TeamStatistics stats)
	 *
	 * TeamStatistics is object as defined in rts/Sim/Misc/TeamStatistics.h
	 */
	GAME_TEAMSTAT = NETMSG_TEAMSTAT, // should be 60
};
}

using namespace asio;

AutohostInterface::AutohostInterface(const std::string& remoteIP, int remotePort, const std::string& localIP, int localPort)
		: autohost(netcode::netservice)
		, initialized(false)
{
	std::string errorMsg = AutohostInterface::TryBindSocket(autohost, remoteIP, remotePort, localIP, localPort);

	if (errorMsg.empty()) {
		initialized = true;
	} else {
		LOG_L(L_ERROR, "Failed to open socket: %s", errorMsg.c_str());
	}
}

std::string AutohostInterface::TryBindSocket(
			asio::ip::udp::socket& socket,
			const std::string& remoteIP, int remotePort,
			const std::string& localIP, int localPort)
{
	std::string errorMsg;

	ip::address localAddr;
	ip::address remoteAddr;
	asio::error_code err;

	try {
		socket.open(ip::udp::v6(), err); // test IP v6 support

		const bool supportsIPv6 = !err;

		remoteAddr = netcode::WrapIP(remoteIP, &err);

		if (err)
			throw std::runtime_error("Failed to parse address " + remoteIP + ": " + err.message());

		if (!supportsIPv6 && remoteAddr.is_v6())
			throw std::runtime_error("IP v6 not supported, can not use address " + remoteAddr.to_string());

		if (localIP.empty()) {
			// use the "any" address as local "from"
			if (remoteAddr.is_v6()) {
				localAddr = ip::address_v6::any();
			} else {
				socket.close();
				socket.open(ip::udp::v4());

				localAddr = ip::address_v4::any();
			}
		} else {
			localAddr = netcode::WrapIP(localIP, &err);

			if (err)
				throw std::runtime_error("Failed to parse local IP " + localIP + ": " + err.message());

			if (localAddr.is_v6() != remoteAddr.is_v6())
				throw std::runtime_error("Local IP " + localAddr.to_string() + " and remote IP " + remoteAddr.to_string() + " are IP v4/v6 mixed");
		}

		socket.bind(ip::udp::endpoint(localAddr, localPort));
		socket.non_blocking(true);
		socket.connect(ip::udp::endpoint(remoteAddr, remotePort));
	} catch (const std::runtime_error& ex) {
		// also includes asio::system_error, inherits from runtime_error
		socket.close();
		errorMsg = ex.what();

		if (errorMsg.empty())
			errorMsg = "Unknown problem";
	}

	return errorMsg;
}


void AutohostInterface::SendStart()
{
	uchar msg = SERVER_STARTED;

	Send(asio::buffer(&msg, sizeof(uchar)));
}

void AutohostInterface::SendQuit()
{
	uchar msg = SERVER_QUIT;

	Send(asio::buffer(&msg, sizeof(uchar)));
}

void AutohostInterface::SendStartPlaying(const unsigned char* gameID, const std::string& demoName)
{
	if (demoName.size() > std::numeric_limits<std::uint32_t>::max() - 30)
		throw std::runtime_error("Path to demofile too long.");

	const std::uint32_t msgsize =
			1                                          // SERVER_STARTPLAYING
			+ sizeof(std::uint32_t)                    // msgsize
			+ 16 * sizeof(std::uint8_t)                // gameID
			+ demoName.size();                         // is 0, if demo recording is off!

	std::vector<std::uint8_t> buffer(msgsize);
	unsigned int pos = 0;

	buffer[pos++] = SERVER_STARTPLAYING;

	memcpy(&buffer[pos], &msgsize, sizeof(msgsize));
	pos += sizeof(msgsize);

	for (unsigned int i = 0; i < 16; i++) {
		buffer[pos++] = gameID[i];
	}

	std::copy(demoName.begin(), demoName.end(), &buffer[pos]);
	assert(int(pos + demoName.size()) == int(msgsize));

	Send(asio::buffer(buffer));
}

void AutohostInterface::SendGameOver(uchar playerNum, const std::vector<uchar>& winningAllyTeams)
{
	const unsigned char msgsize = 1 + 1 + 1 + (winningAllyTeams.size() * sizeof(uchar));
	std::vector<std::uint8_t> buffer(msgsize);
	buffer[0] = SERVER_GAMEOVER;
	buffer[1] = msgsize;
	buffer[2] = playerNum;

	for (unsigned int i = 0; i < winningAllyTeams.size(); i++) {
		buffer[3 + i] = winningAllyTeams[i];
	}
	Send(asio::buffer(buffer));
}

void AutohostInterface::SendPlayerJoined(uchar playerNum, const std::string& name)
{
	if (autohost.is_open()) {
		const auto msgsize = 2 * sizeof(uchar) + name.size();
		std::vector<std::uint8_t> buffer(msgsize);
		buffer[0] = PLAYER_JOINED;
		buffer[1] = playerNum;
		std::copy(name.begin(), name.end(), &buffer[2]);

		Send(asio::buffer(buffer));
	}
}

void AutohostInterface::SendPlayerLeft(uchar playerNum, uchar reason)
{
	uchar msg[3] = {PLAYER_LEFT, playerNum, reason};

	Send(asio::buffer(&msg, 3 * sizeof(uchar)));
}

void AutohostInterface::SendPlayerReady(uchar playerNum, uchar readyState)
{
	uchar msg[3] = {PLAYER_READY, playerNum, readyState};

	Send(asio::buffer(&msg, 3 * sizeof(uchar)));
}

void AutohostInterface::SendPlayerChat(uchar playerNum, uchar destination, const std::string& chatmsg)
{
	if (autohost.is_open()) {
		const auto msgsize = 3 * sizeof(uchar) + chatmsg.size();
		std::vector<std::uint8_t> buffer(msgsize);
		buffer[0] = PLAYER_CHAT;
		buffer[1] = playerNum;
		buffer[2] = destination;
		std::copy(chatmsg.begin(), chatmsg.end(), &buffer[3]);

		Send(asio::buffer(buffer));
	}
}

void AutohostInterface::SendPlayerDefeated(uchar playerNum)
{
	uchar msg[2] = {PLAYER_DEFEATED, playerNum};

	Send(asio::buffer(&msg, 2 * sizeof(uchar)));
}

void AutohostInterface::Message(const std::string& message)
{
	if (autohost.is_open()) {
		const auto msgsize = sizeof(uchar) + message.size();
		std::vector<std::uint8_t> buffer(msgsize);
		buffer[0] = SERVER_MESSAGE;
		std::copy(message.begin(), message.end(), &buffer[1]);

		Send(asio::buffer(buffer));
	}
}

void AutohostInterface::Warning(const std::string& message)
{
	if (autohost.is_open()) {
		const auto msgsize = sizeof(uchar) + message.size();
		std::vector<std::uint8_t> buffer(msgsize);
		buffer[0] = SERVER_WARNING;
		std::copy(message.begin(), message.end(), &buffer[1]);

		Send(asio::buffer(buffer));
	}
}

void AutohostInterface::SendLuaMsg(const std::uint8_t* msg, size_t msgSize)
{
	if (autohost.is_open()) {
		std::vector<std::uint8_t> buffer(msgSize+1);
		buffer[0] = GAME_LUAMSG;
		std::copy(msg, msg + msgSize, buffer.begin() + 1);

		Send(asio::buffer(buffer));
	}
}

void AutohostInterface::Send(const std::uint8_t* msg, size_t msgSize)
{
	if (autohost.is_open()) {
		std::vector<std::uint8_t> buffer(msgSize);
		std::copy(msg, msg + msgSize, buffer.begin());

		Send(asio::buffer(buffer));
	}
}

std::string AutohostInterface::GetChatMessage()
{
	if (autohost.is_open()) {
		size_t bytes_avail = 0;

		if ((bytes_avail = autohost.available()) > 0) {
			std::vector<std::uint8_t> buffer(bytes_avail+1, 0);
			/*const size_t bytesReceived = */autohost.receive(asio::buffer(buffer));
			return std::string((char*)(&buffer[0]));
		}
	}

	return "";
}

void AutohostInterface::Send(asio::mutable_buffers_1 buffer)
{
	if (autohost.is_open()) {
		try {
			autohost.send(buffer);
		} catch (asio::system_error& e) {
			autohost.close();
			LOG_L(L_ERROR,
					"Failed to send buffer; the autohost may not be reachable: %s",
					e.what());
		}
	}
}
