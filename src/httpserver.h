#include <string>

namespace DataCore {

bool start_http_server(std::function<std::string(std::string &&)> lambda, const char *addr = "0.0.0.0",
					   unsigned short port = 5000) noexcept;

}
