/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Socket.h"

#include "lib/streflop/streflop_cond.h"

#include "System/Log/ILog.h"
#include "System/StringUtil.h"


namespace netcode
{

asio::io_context netcontext;

bool CheckErrorCode(asio::error_code& err)
{
	// connection reset can happen when host did not start up
	// before the client wants to connect
	if (!err || err.value() == asio::error::connection_reset ||
		err.value() == asio::error::try_again) { // this should only ever happen with async sockets, but testing indicates it happens anyway...
		return false;
	} else {
		LOG_L(L_WARNING, "Network error %i: %s", err.value(),
				err.message().c_str());
		return true;
	}
}

asio::ip::udp::endpoint ResolveAddr(const std::string& host, int port, asio::error_code* err)
{
	assert(err);
	using namespace asio;
	ip::address tempAddr = WrapIP(host, err);
	if (!*err)
		return ip::udp::endpoint(tempAddr, port);

	auto errBuf = *err; // WrapResolve() might clear err
	asio::io_context io_context;
	ip::udp::resolver resolver(io_context);
	auto results = WrapResolve(resolver, host, IntToString(port), err);
	if (!*err && !results.empty()) {
		return *results.begin();
	}

	if (!*err) *err = errBuf;
	return ip::udp::endpoint(tempAddr, 0);
}


asio::ip::address WrapIP(const std::string& ip,
		asio::error_code* err)
{
	asio::ip::address addr;

	if (err == NULL) {
		addr = asio::ip::make_address(ip);
	} else {
		addr = asio::ip::make_address(ip, *err);
	}

	// (date of note: 08/05/10)
	// something in make_address() is invalidating the FPU flags
	// tested on win2k and linux (not happening there)
	streflop::streflop_init<streflop::Simple>();
	return addr;
}

asio::ip::udp::resolver::results_type WrapResolve(
		asio::ip::udp::resolver& resolver,
		std::string_view host,
		std::string_view service,
		asio::error_code* err) 
{
	asio::ip::udp::resolver::results_type resolveIt;

	if (err == nullptr) {
		resolveIt = resolver.resolve(host, service);
	} else {
		resolveIt = resolver.resolve(host, service, *err);
	}

	// (date of note: 08/22/10)
	// something in resolve() is invalidating the FPU flags
	streflop::streflop_init<streflop::Simple>();
	return resolveIt;
}


asio::ip::address GetAnyAddress(const bool IPv6)
{
	if (IPv6) {
		return asio::ip::address_v6::any();
	}
	return asio::ip::address_v4::any();
}


} // namespace netcode

