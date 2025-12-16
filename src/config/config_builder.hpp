#ifndef CONFIG_CONFIG_BUILDER_HPP_
#define CONFIG_CONFIG_BUILDER_HPP_

#include "config/config_parser.hpp"
#include "config/server_config.hpp"

class ConfigBuilder {
public:
    explicit ConfigBuilder(const std::vector<AstNode>& nodes);

    HttpConfig build_http_config();

private:
    static ServerConfig build_server_config(const AstNode& node);
    static SharedConfig build_shared_config(const std::vector<AstNode>& nodes,
                                            const SharedConfig* parent);
    static RouteConfig build_route_config(const AstNode& node, const SharedConfig& parent);

    const std::vector<AstNode>& nodes_;
};

#endif // CONFIG_CONFIG_BUILDER_HPP_
