#ifndef ROUTER_ROUTER_HPP_
#define ROUTER_ROUTER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_request.hpp"

#include <string>

class Router {
public:
    explicit Router(ServerConfig& config);
    ~Router() {}

    Handler* handle_request(const HttpRequest& request);

private:
    Router();
    Router(const Router& other);
    Router& operator=(const Router& other);

    int prefix_length(const std::string& req_location, const std::string& location);
    const Location* routing(const std::string& req_location);
    bool is_valid_method(const std::string& method);
    std::string build_request_path(const HttpRequest& request, const Location* best_match);

    ServerConfig config_;
};

#endif // ROUTER_ROUTER_INTERNAL_HPP_
