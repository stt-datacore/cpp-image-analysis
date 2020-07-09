#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

class NetworkHelper
{
public:
    NetworkHelper();

    bool downloadUrl(const std::string& url, std::function<bool(std::vector<uint8_t>&&)> lambda) noexcept;

private:
    bool performRequest(boost::beast::http::request<boost::beast::http::string_body>& req, boost::beast::http::response<boost::beast::http::vector_body<uint8_t>>& res, const char* host, const char* port = "443") noexcept;

    boost::asio::io_context ioc;
    boost::asio::ssl::context ctx{ boost::asio::ssl::context::sslv23_client };
};
