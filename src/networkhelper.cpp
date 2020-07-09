#include <regex>
#include <iostream>

#include "networkhelper.h"

#include <boost/algorithm/string.hpp> // for case-insensitive string comparison

namespace http = boost::beast::http;

struct ParsedURI {
    std::string protocol;
    std::string domain;  // only domain must be present
    std::string port;
    std::string resource;
    std::string query;   // everything after '?', possibly nothing
};

ParsedURI parseURI(const std::string& url) {
    ParsedURI result;
    auto value_or = [](const std::string& value, std::string&& deflt) -> std::string {
        return (value.empty() ? deflt : value);
    };
    // Note: only "http", "https", "ws", and "wss" protocols are supported
    static const std::regex PARSE_URL{ R"((([httpsw]{2,5})://)?([^/ :]+)(:(\d+))?(/([^ ?]+)?)?/?\??([^/ ]+\=[^/ ]+)?)",
                                       std::regex_constants::ECMAScript | std::regex_constants::icase };
    std::smatch match;
    if (std::regex_match(url, match, PARSE_URL) && match.size() == 9) {
        result.protocol = value_or(boost::algorithm::to_lower_copy(std::string(match[2])), "http");
        result.domain = match[3];
        const bool is_sequre_protocol = (result.protocol == "https" || result.protocol == "wss");
        result.port = value_or(match[5], (is_sequre_protocol) ? "443" : "80");
        result.resource = value_or(match[6], "/");
        result.query = match[8];
        assert(!result.domain.empty());
    }
    return result;
}

NetworkHelper::NetworkHelper()
{
}

bool NetworkHelper::performRequest(boost::beast::http::request<boost::beast::http::string_body>& req,
                                   boost::beast::http::response<boost::beast::http::vector_body<uint8_t>>& res,
                                   const char* host, const char* port) noexcept
{
    try
    {
        // These objects perform our I/O
        boost::asio::ip::tcp::resolver resolver{ ioc };
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream{ ioc, ctx };

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host))
        {
            boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
            throw boost::system::system_error{ ec };
        }

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(stream.next_layer(), results.begin(), results.end());

        // Perform the SSL handshake
        stream.handshake(boost::asio::ssl::stream_base::client);

        // Set some basic fields in the request
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        boost::beast::http::write(stream, req);

        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer buffer;

        // Receive the HTTP response
        boost::beast::http::read(stream, buffer, res);

        // Gracefully close the stream
        boost::system::error_code ec;
        stream.shutdown(ec);
        if (ec == boost::asio::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec.assign(0, ec.category());
        }

        if (ec)
        {
            if (ec != boost::asio::ssl::error::stream_errors::stream_truncated) // not a real error, just a normal TLS shutdown
            {
                throw boost::system::system_error{ ec };
            }
        }

        // If we get here then the connection is closed gracefully
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error during performRequest: " << e.what();
        return false;
    }

    return true;
}

bool NetworkHelper::downloadUrl(const std::string& url, std::function<bool(std::vector<uint8_t>&&)> lambda) noexcept
{
    auto uri = parseURI(url);

    http::request<http::string_body> req{ http::verb::get, uri.resource, 11 };

    // Declare a container to hold the response
    http::response<http::vector_body<uint8_t>> res;

    if (!performRequest(req, res, uri.domain.c_str()))
        return false;

    if (res.result() != http::status::ok)
        return false;

    return lambda(std::move(res.body()));
}