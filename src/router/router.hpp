#ifndef ROUTER_ROUTER_HPP_
#define ROUTER_ROUTER_HPP_

// NOTE(IBO): I was using this file to store router related stuff while debugging.
// Maybe put your internal functions here? All functions in router.cpp are now static.

#include "config/server_config.hpp"
#include "core/client.hpp"

#include <string>

class Router {
public:
    Router(ServerConfig& config);
    ~Router();
    void handle_request(Client* conn, const HttpRequest& request);

private:
    Router();
    int prefix_length(const std::string& req_location, const std::string& location);
    const Location* routing(const std::string& req_location, const ServerConfig& config);
    bool is_valid_method(const std::string& method, const ServerConfig& config);
    std::string build_request_path(const HttpRequest& request, const Location* best_match,
                                   const ServerConfig& config);
    void set_status(Client* conn, int status_code, const std::string& body);
    ServerConfig config_;
};

#endif // ROUTER_ROUTER_INTERNAL_HPP_
