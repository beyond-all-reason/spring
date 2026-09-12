/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOCKET_H
#define SOCKET_H

#include <asio/io_context.hpp>
#include <asio/ip/udp.hpp>
#include <asio/ip/tcp.hpp>


namespace netcode
{

extern asio::io_context netcontext;

/**
 * Check if a network error occurred and eventually log it.
 * @returns true if a network error occurred, false otherwise
 */
bool CheckErrorCode(asio::error_code& err);

/**
 * Resolves a host name or IP plus port number into a boost UDP end-point,
 * eventually resolving the host, if it is specified as name.
 * @param host name or IP
 */
asio::ip::udp::endpoint ResolveAddr(const std::string& host, int port, asio::error_code* error);

/**
 * Encapsulates the ip::address::from_string(str) function,
 * for sync relevant reasons.
 */
asio::ip::address WrapIP(const std::string& ip,
		asio::error_code* err = NULL);

/**
 * Encapsulates the ip::udp::resolver::resolve(query) function,
 * for sync relevant reasons.
 */

asio::ip::udp::resolver::results_type WrapResolve(
        asio::ip::udp::resolver& resolver,
        std::string_view host,
        std::string_view service,
        asio::error_code* err = nullptr);


asio::ip::address GetAnyAddress(const bool IPv6);

} // namespace netcode

#endif // SOCKET_H
