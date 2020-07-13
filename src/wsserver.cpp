#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "wsserver.h"

namespace beast = boost::beast;			// from <boost/beast.hpp>
namespace http = beast::http;			// from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;			// from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

namespace DataCore {

void do_session(std::function<std::string(std::string &&)> lambda, tcp::socket &socket)
{
	try {
		// Construct the stream by moving in the socket
		websocket::stream<tcp::socket> ws{std::move(socket)};

		// Set a decorator to change the Server of the handshake
		ws.set_option(
			websocket::stream_base::decorator([](websocket::response_type &res) { res.set(http::field::server, "DataCore-CV"); }));

		// Accept the websocket handshake
		ws.accept();

		for (;;) {
			// This buffer will hold the incoming message
			beast::flat_buffer buffer;

			// Read a message
			ws.read(buffer);

			ws.text(ws.got_text());

			std::string message = beast::buffers_to_string(buffer.data());
			std::string reply = lambda(std::move(message));

			ws.write(net::buffer(reply));
		}
	} catch (beast::system_error const &se) {
		// This indicates that the session was closed
		if (se.code() != websocket::error::closed)
			std::cerr << "Error: " << se.code().message() << std::endl;
	} catch (std::exception const &e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

bool start_server(std::function<std::string(std::string &&)> lambda, const char *addr, unsigned short port) noexcept
{
	try {
		auto const address = net::ip::make_address(addr);

		// The io_context is required for all I/O
		net::io_context ioc{1};

		// The acceptor receives incoming connections
		tcp::acceptor acceptor{ioc, {address, port}};
		for (;;) {
			// This will receive the new connection
			tcp::socket socket{ioc};

			// Block until we get a connection
			acceptor.accept(socket);

			// Launch the session, transferring ownership of the socket
			std::thread{std::bind(&do_session, lambda, std::move(socket))}.detach();
		}

		return true;
	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return false;
	}
}

} // namespace DataCore