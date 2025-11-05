#ifndef ROUTER_ROUTER_HPP_
#define ROUTER_ROUTER_HPP_

#include "config/server_config.hpp"
#include "core/client.hpp"

#include <string>

class Router {
public:
    explicit Router(ServerConfig& config);
    ~Router();

    Router(const Router& other);
    Router& operator=(const Router& other);

    bool handle_request(Client* conn, const HttpRequest& request, std::string& full_path);

private:
    Router();
    int prefix_length(const std::string& req_location, const std::string& location);
    const Location* routing(const std::string& req_location);
    bool is_valid_method(const std::string& method);
    std::string build_request_path(const HttpRequest& request, const Location* best_match);
    void set_status(Client* conn, int status_code, const std::string& body);
    ServerConfig config_;
};

#endif // ROUTER_ROUTER_INTERNAL_HPP_
