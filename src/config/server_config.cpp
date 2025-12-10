#include "config/server_config.hpp"

#include "core/server_defaults.hpp"

#include <netinet/in.h>

#include <cstring>

static sockaddr_in make_ipv4_addr(in_addr_t ip, in_port_t port)
{
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip);
    return addr;
}

ServerConfig make_site1_config()
{
    SharedConfig shared_cfg;
    shared_cfg.document_root = "www/site1";
    shared_cfg.allowed_methods.push_back("GET");
    shared_cfg.allowed_methods.push_back("POST");
    shared_cfg.allowed_methods.push_back("DELETE");
    shared_cfg.index_files.push_back("index.html");
    shared_cfg.max_body_size = WEBSERV_DEFAULT_MAX_BODY_SIZE;
    shared_cfg.uploads_allowed = false;
    shared_cfg.autoindex_enabled = false;

    RouteConfig default_loc;
    default_loc.config = shared_cfg;
    default_loc.route_path = "/";

    RouteConfig pages_loc;
    pages_loc.config = shared_cfg;
    pages_loc.route_path = "/pages";

    RouteConfig images_loc;
    images_loc.config = shared_cfg;
    images_loc.route_path = "/images";

    RouteConfig files_loc;
    files_loc.config = shared_cfg;
    files_loc.route_path = "/files";

    ServerConfig server_cfg;
    server_cfg.config = shared_cfg;
    server_cfg.listen_addrs.push_back(make_ipv4_addr(INADDR_ANY, WEBSERV_DEFAULT_PORT));
    server_cfg.backlog = WEBSERV_DEFAULT_MAX_PENDING_CONNECTIONS;
    server_cfg.is_default_server = true;
    server_cfg.locations.push_back(default_loc);
    server_cfg.locations.push_back(pages_loc);
    server_cfg.locations.push_back(images_loc);
    server_cfg.locations.push_back(files_loc);

    return server_cfg;
}

ServerConfig make_example_config()
{
    SharedConfig shared_cfg;
    shared_cfg.document_root = "www/example";
    shared_cfg.allowed_methods.push_back("GET");
    shared_cfg.allowed_methods.push_back("POST");
    shared_cfg.allowed_methods.push_back("DELETE");
    shared_cfg.index_files.push_back("index.html");
    shared_cfg.max_body_size = WEBSERV_DEFAULT_MAX_BODY_SIZE;
    shared_cfg.uploads_allowed = false;
    shared_cfg.autoindex_enabled = false;

    RouteConfig default_loc;
    default_loc.config = shared_cfg;
    default_loc.route_path = "/";

    RouteConfig loc1;
    loc1.config = shared_cfg;
    loc1.route_path = "/example";

    RouteConfig loc2;
    loc2.config = shared_cfg;
    loc2.route_path = "/example/test_subfolder";

    ServerConfig server_cfg;
    server_cfg.config = shared_cfg;
    server_cfg.listen_addrs.push_back(make_ipv4_addr(INADDR_ANY, WEBSERV_DEFAULT_PORT));
    server_cfg.backlog = WEBSERV_DEFAULT_MAX_PENDING_CONNECTIONS;
    server_cfg.is_default_server = true;
    server_cfg.locations.push_back(default_loc);
    server_cfg.locations.push_back(loc1);
    server_cfg.locations.push_back(loc2);

    return server_cfg;
}

ServerConfig make_python_docs_config()
{
    SharedConfig shared_cfg;
    shared_cfg.document_root = "www/python_docs";
    shared_cfg.allowed_methods.push_back("GET");
    shared_cfg.allowed_methods.push_back("POST");
    shared_cfg.allowed_methods.push_back("DELETE");
    shared_cfg.index_files.push_back("index.html");
    shared_cfg.max_body_size = WEBSERV_DEFAULT_MAX_BODY_SIZE;
    shared_cfg.uploads_allowed = false;
    shared_cfg.autoindex_enabled = false;

    RouteConfig default_loc;
    default_loc.config = shared_cfg;
    default_loc.route_path = "/";

    ServerConfig server_cfg;
    server_cfg.config = shared_cfg;
    server_cfg.listen_addrs.push_back(make_ipv4_addr(INADDR_ANY, WEBSERV_DEFAULT_PORT));
    server_cfg.backlog = WEBSERV_DEFAULT_MAX_PENDING_CONNECTIONS;
    server_cfg.is_default_server = true;
    server_cfg.locations.push_back(default_loc);

    return server_cfg;
}
