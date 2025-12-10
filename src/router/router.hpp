#ifndef ROUTER_ROUTER_HPP_
#define ROUTER_ROUTER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_request.hpp"

#include <string>

class Router {
public:
    explicit Router(const ServerConfig& config);

    Handler* handle_request(const HttpRequest& request);
    const RouteConfig& find_best_route(const std::string& request_path);

    const ServerConfig rc;
};

#endif // ROUTER_ROUTER_HPP_
