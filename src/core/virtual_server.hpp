#ifndef CORE_VIRTUAL_SERVER_HPP_
#define CORE_VIRTUAL_SERVER_HPP_

#include "config/server_config.hpp"
#include "router/router.hpp"

class VirtualServer {
public:
    explicit VirtualServer(const ServerConfig& config);

    const ServerConfig& config() const { return config_; }
    const Router& router() const { return router_; }

private:
    const ServerConfig& config_;
    Router router_;
};

#endif // CORE_VIRTUAL_SERVER_HPP_
