#define BOOST_ASIO_NO_DEPRECATED

#include "http.h"

#include "../logger.hpp"
#include "listener.h"
#include <thread>

namespace asio = boost::asio;

namespace {

asio::io_context ioc;

std::vector<std::thread> workers = {};

} // namespace

void tfs::http::start(bool bindOnlyOtsIP, std::string_view otsIP, unsigned short port /*= 8080*/, int threads /*= 1*/)
{
	if (port == 0 || threads < 1) {
		return;
	}

	asio::ip::address address = asio::ip::address_v6::any();
	if (bindOnlyOtsIP) {
		address = asio::ip::make_address(otsIP);
	}
	LOG_INFO("HTTP", "Starting HTTP server on " + address.to_string() + ":" + std::to_string(port) +
	                     " with " + std::to_string(threads) + " threads.");

	auto listener = make_listener(ioc, {address, port});
	listener->run();

	workers.reserve(threads);
	for (auto i = 0; i < threads; ++i) {
		workers.emplace_back([] { ioc.run(); });
	}
}

void tfs::http::stop()
{
	if (workers.empty()) {
		return;
	}

	LOG_INFO("HTTP", "Stopping HTTP server...");

	ioc.stop();
	for (auto& worker : workers) {
		worker.join();
	}

	LOG_INFO("HTTP", "Stopped HTTP server.");
}
