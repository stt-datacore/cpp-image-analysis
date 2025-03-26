#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>


#include "httpserver.h"

namespace beast = boost::beast;	  // from <boost/beast.hpp>
namespace http = beast::http;	  // from <boost/beast/http.hpp>
namespace net = boost::asio;	  // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

namespace DataCore {

const char HEX2DEC[256] = {
	/*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
	/* 0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 1 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 2 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 3 */ 0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	-1, -1, -1, -1, -1, -1,

	/* 4 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 5 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 6 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 7 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

	/* 8 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 9 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* A */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* B */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

	/* C */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* D */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* E */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* F */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

// From here: https://www.codeguru.com/cpp/cpp/algorithms/strings/article.php/c12759/URI-Encoding-and-Decoding.htm
std::string UriDecode(const std::string &sSrc)
{
	// Note from RFC1630: "Sequences which start with a percent
	// sign but are not followed by two hexadecimal characters
	// (0-9, A-F) are reserved for future extension"

	const unsigned char *pSrc = (const unsigned char *)sSrc.c_str();
	const size_t SRC_LEN = sSrc.length();
	const unsigned char *const SRC_END = pSrc + SRC_LEN;
	// last decodable '%'
	const unsigned char *const SRC_LAST_DEC = SRC_END - 2;

	char *const pStart = new char[SRC_LEN];
	char *pEnd = pStart;

	while (pSrc < SRC_LAST_DEC) {
		if (*pSrc == '%') {
			char dec1, dec2;
			if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)]) && -1 != (dec2 = HEX2DEC[*(pSrc + 2)])) {
				*pEnd++ = (dec1 << 4) + dec2;
				pSrc += 3;
				continue;
			}
		}

		*pEnd++ = *pSrc++;
	}

	// the last 2- chars
	while (pSrc < SRC_END)
		*pEnd++ = *pSrc++;

	std::string sResult(pStart, pEnd);
	delete[] pStart;
	return sResult;
}

class http_connection : public std::enable_shared_from_this<http_connection>
{
  public:
	http_connection(tcp::socket socket, std::function<std::string(std::string &&)> lambda) : socket_(std::move(socket)), lambda_(lambda)
	{
	}

	// Initiate the asynchronous operations associated with the connection.
	void start()
	{
		read_request();
		check_deadline();
	}

  private:
	// The socket for the currently connected client.
	tcp::socket socket_;

	// The buffer for performing reads.
	beast::flat_buffer buffer_{1024};

	// The request message.
	http::request<http::dynamic_body> request_;

	// The response message.
	http::response<http::dynamic_body> response_;

	// The timer for putting a deadline on connection processing.
	net::steady_timer deadline_{socket_.get_executor(), std::chrono::seconds(60)};

	std::function<std::string(std::string &&)> lambda_;

	// Asynchronously receive a complete request message.
	void read_request()
	{
		auto self = shared_from_this();

		http::async_read(socket_, buffer_, request_, [self](beast::error_code ec, std::size_t bytes_transferred) {
			boost::ignore_unused(bytes_transferred);
			if (!ec)
				self->process_request();
		});
	}

	// Determine what needs to be done with the request message.
	void process_request()
	{
		response_.version(request_.version());
		response_.keep_alive(false);

		switch (request_.method()) {
		case http::verb::get:
			response_.result(http::status::ok);
			response_.set(http::field::server, "DataCoreCV");
			create_response();
			break;

		default:
			// We return responses indicating an error if
			// we do not recognize the request method.
			response_.result(http::status::bad_request);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "Invalid request-method '" << std::string(request_.method_string()) << "'";
			break;
		}

		write_response();
	}

	// Construct a response message based on the program state.
	void create_response()
	{
		auto target = std::string(request_.target());
		if (target.find("/api/behold?url=") == 0) {
			response_.set(http::field::content_type, "application/json");
			std::string url = "BOTH" + UriDecode(target.substr(16));
			beast::ostream(response_.body()) << lambda_(std::move(url));
		} else if (target.find("/api/reinit") == 0)  {
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "Received Reinitialize Request OK\r\n";
		} else {
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "Invalid request\r\n";
		}
	}

	// Asynchronously transmit the response message.
	void write_response()
	{
		auto self = shared_from_this();

		response_.set(http::field::content_length, response_.body().size());

		http::async_write(socket_, response_, [self](beast::error_code ec, std::size_t) {
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
		});
	}

	// Check whether we have spent enough time on this connection.
	void check_deadline()
	{
		auto self = shared_from_this();

		deadline_.async_wait([self](beast::error_code ec) {
			if (!ec) {
				// Close socket to cancel any outstanding operation.
				self->socket_.close(ec);
			}
		});
	}
};

// "Loop" forever accepting new connections.
void http_server(tcp::acceptor &acceptor, tcp::socket &socket, std::function<std::string(std::string &&)> lambda)
{
	acceptor.async_accept(socket, [&, lambdacopy = lambda](beast::error_code ec) {
		if (!ec)
			std::make_shared<http_connection>(std::move(socket), lambdacopy)->start();
		http_server(acceptor, socket, lambdacopy);
	});
}

bool start_http_server(std::function<std::string(std::string &&)> lambda, const char *addr, unsigned short port) noexcept
{
	try {
		auto const address = net::ip::make_address(addr);

		net::io_context ioc{1};

		tcp::acceptor acceptor{ioc, {address, port}};
		tcp::socket socket{ioc};
		http_server(acceptor, socket, lambda);

		ioc.run();

		return true;
	} catch (std::exception const &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return false;
	}
}

} // namespace DataCore