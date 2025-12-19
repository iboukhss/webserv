#ifndef CONFIG_SERVER_CONFIG_HPP_
#define CONFIG_SERVER_CONFIG_HPP_

#include "http/http_response.hpp"

#include <netinet/in.h>

#include <map>
#include <string>
#include <vector>

// Refs:
// https://nginx.org/en/docs/http/ngx_http_core_module.html
// https://nginx.org/en/docs/http/ngx_http_index_module.html
// https://nginx.org/en/docs/http/ngx_http_rewrite_module.html

// Block directives:
// http { ... }
// server { ... }
// location location_path { ... }

// Server directives:
// listen ip:port [default_server] [backlog=number];

// Route directives:
// alias /path/to/loc;

// Shared directives:
// allow_methods GET POST ...;
// allow_uploads (on|off);
// upload_store /path/to/store;
// autoindex (on|off);
// cgi_handler .ext /path/to/exec;
// cgi_allow_methods GET POST ...;
// client_max_body_size 1M;
// error_page 404 403 ... /404.html;
// index index.html index.htm ...;
// return 301 /url;
// root /path/to/root;

struct RedirectConfig {
    HttpResponse::Status code;
    std::string url;
};

struct CgiConfig {
    std::string extension;
    std::string exec_path;
    std::vector<std::string> allowed_methods;
};

struct SharedConfig {
    std::string document_root;
    size_t max_body_size;
    bool uploads_allowed;
    std::string upload_path;
    bool autoindex_enabled;

    std::vector<std::string> allowed_methods;
    std::vector<std::string> index_files;
    std::map<HttpResponse::Status, std::string> error_pages;
    RedirectConfig redirect;

    CgiConfig cgi;

    SharedConfig();
};

struct RouteConfig {
    std::string path;
    std::string alias;

    SharedConfig shared;
};

struct ServerConfig {
    std::vector<sockaddr_in> listen_addrs;
    std::map<std::string, RouteConfig> locations;

    SharedConfig shared;

    void add_listen_addr(const std::string& str);
};

struct HttpConfig {
    std::vector<ServerConfig> servers;
};

HttpConfig load_http_config(const std::string& file_path);

ServerConfig make_youpi_banane_test_config();
ServerConfig make_site1_config();
ServerConfig make_example_config();
ServerConfig make_python_docs_config();
ServerConfig make_vitepress_docs_config();

#endif // CONFIG_SERVER_CONFIG_HPP_
