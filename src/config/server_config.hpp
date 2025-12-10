#ifndef CONFIG_SERVER_CONFIG_HPP_
#define CONFIG_SERVER_CONFIG_HPP_

#include "core/server_defaults.hpp"
#include "http/http_response.hpp"

#include <netinet/in.h>

#include <map>
#include <string>
#include <vector>

// Refs:
// https://nginx.org/en/docs/http/ngx_http_core_module.html
// https://nginx.org/en/docs/http/ngx_http_index_module.html
// https://nginx.org/en/docs/http/ngx_http_rewrite_module.html

// Blocks:
// http_block { ... }
// server_block { ... }
// location_block location_path { ... }

// Shared directives:
// root /path/to/root;
// allow_methods GET POST ...;
// error_page code /404.html;
// index index.html index.htm ...;
// return code URL;

// cgi_handler .ext /path/to/exec;
// cgi_allow_methods GET POST ...;

// client_max_body_size 1M;
// allow_uploads on/off;
// upload_store /path/to/store;
// autoindex on/off;

// Server-specific directives:
// listen ip:port [default_server] [backlog=number];

// Route-specific directives:
// alias /path/to/loc;

struct SharedConfig {
    struct RedirectConfig {
        HttpResponse::Status code;
        std::string url;
    };

    struct CgiConfig {
        std::string extension;
        std::string exec_path;
        std::vector<std::string> allowed_methods;
    };

    std::string document_root;
    std::vector<std::string> allowed_methods;
    std::map<HttpResponse::Status, std::string> error_pages;
    std::vector<std::string> index_files;

    RedirectConfig redirect;
    CgiConfig cgi;

    size_t max_body_size;
    bool uploads_allowed;
    std::string upload_path;

    bool autoindex_enabled;
};

struct RouteConfig {
    SharedConfig config;

    std::string route_path;
    std::string route_alias;
};

struct ServerConfig {
    SharedConfig config;

    std::vector<sockaddr_in> listen_addrs;
    int backlog;

    bool is_default_server;

    std::vector<RouteConfig> locations;
};

struct HttpConfig {
    std::vector<ServerConfig> servers;
};

ServerConfig make_site1_config();
ServerConfig make_example_config();
ServerConfig make_python_docs_config();
ServerConfig make_vitepress_docs_config();

#endif // CONFIG_SERVER_CONFIG_HPP_
