#include "config/server_config.hpp"

#include "config/config_builder.hpp"
#include "config/config_parser.hpp"
#include "config/config_tokenizer.hpp"
#include "core/server_defaults.hpp"
#include "util/string.hpp"

#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>

#include <cstdlib>
#include <cstring>
#include <stdexcept>

/*
static sockaddr_in make_ipv4_addr(in_addr_t ip, in_port_t port)
{
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip);
    return addr;
}
*/

SharedConfig::SharedConfig()
    : max_body_size(WEBSERV_DEFAULT_MAX_BODY_SIZE),
      uploads_allowed(false),
      autoindex_enabled(false)
{
    allowed_methods.push_back("GET");
    allowed_methods.push_back("POST");
    allowed_methods.push_back("DELETE");

    index_files.push_back("index.html");
    index_files.push_back("index.htm");

    redirect.code = HttpResponse::kStatusNone;

    cgi.allowed_methods.push_back("GET");
    cgi.allowed_methods.push_back("POST");
}

static bool is_valid_ipv4_octet(const std::string& str)
{
    const char* s = str.c_str();
    int res = 0;

    if (*s == '\0')
        return false;

    while (*s && std::isdigit(*s)) {
        res = res * 10 + (*s - '0');
        if (res > 255)
            return false;

        s++;
    }
    return *s == '\0';
}

static bool is_valid_ipv4_addr(const std::string& str)
{
    std::vector<std::string> bytes = str_split(str, ".");

    if (bytes.size() != 4)
        return false;

    for (int i = 0; i < 4; i++) {
        if (!is_valid_ipv4_octet(bytes[i]))
            return false;
    }
    return true;
}

static uint32_t parse_ipv4_addr(const std::string& str)
{
    if (!is_valid_ipv4_addr(str))
        throw std::runtime_error("Invalid IPv4 format: " + str);

    std::vector<std::string> bytes = str_split(str, ".");
    uint32_t addr = 0;

    for (int i = 0; i < 4; i++) {
        int octet = std::atoi(bytes[i].c_str());
        addr = (addr << 8) | octet;
    }
    return addr;
}

static bool is_valid_ipv4_port(const std::string& str)
{
    const char* s = str.c_str();
    int res = 0;

    if (*s == '\0')
        return false;

    while (*s && std::isdigit(*s)) {
        res = res * 10 + (*s - '0');
        if (res > UINT16_MAX)
            return false;

        s++;
    }
    return res != 0 && *s == '\0';
}

static uint16_t parse_ipv4_port(const std::string& str)
{
    if (!is_valid_ipv4_port(str))
        throw std::runtime_error("Invalid IPv4 port: " + str);

    const char* s = str.c_str();
    uint16_t res = 0;

    while (*s && std::isdigit(*s)) {
        res = res * 10 + (*s - '0');
        ++s;
    }
    return res;
}

// TODO(isma): Bad code, should be unit tested.
void ServerConfig::add_listen_addr(const std::string& str)
{
    std::vector<std::string> expr = str_split(str, ":");

    if (expr.empty() || expr.size() > 2)
        throw std::runtime_error("Invalid listen address: " + str);

    in_addr_t ip = INADDR_ANY;
    in_port_t port = 8080;

    if (expr.size() == 1) {
        if (is_valid_ipv4_addr(expr[0])) {
            ip = parse_ipv4_addr(expr[0]);
        }
        else if (is_valid_ipv4_port(expr[0])) {
            port = parse_ipv4_port(expr[0]);
        }
        else {
            throw std::runtime_error("Invalid listen address: " + str);
        }
    }
    else {
        if (!expr[0].empty()) {
            if (!is_valid_ipv4_addr(expr[0])) {
                throw std::runtime_error("Invalid listen address: " + str);
            }
            ip = parse_ipv4_addr(expr[0]);
        }
        if (!expr[1].empty()) {
            if (!is_valid_ipv4_port(expr[1])) {
                throw std::runtime_error("Invalid listen address: " + str);
            }
            port = parse_ipv4_port(expr[1]);
        }
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip);

    listen_addrs.push_back(addr);
}

HttpConfig load_http_config(const std::string& file_path)
{
    ConfigTokenizer tokenizer(file_path);
    std::vector<ConfigToken> tokens = tokenizer.tokenize_file();

    ConfigParser parser(tokens);
    std::vector<AstNode> ast = parser.parse_tokens();

    ConfigBuilder builder(ast);
    return builder.build_http_config();
}

/*
ServerConfig make_site1_config()
{
    ServerConfig srv("www/site1");

    RouteConfig default_loc("/", srv.shared());
    RouteConfig pages_loc("/pages", srv.shared());
    RouteConfig images_loc("/images", srv.shared());
    RouteConfig files_loc("/files", srv.shared());

    srv.add_location(default_loc);
    srv.add_location(pages_loc);
    srv.add_location(images_loc);
    srv.add_location(files_loc);

    return srv;
}

ServerConfig make_example_config()
{
    ServerConfig srv("www/example");

    RouteConfig default_loc("/", srv.shared());
    RouteConfig loc1("/example", srv.shared());
    RouteConfig loc2("/example/test_subfolder", srv.shared());

    srv.add_location(default_loc);
    srv.add_location(loc1);
    srv.add_location(loc2);

    return srv;
}

ServerConfig make_python_docs_config()
{
    ServerConfig srv("www/python_docs");

    RouteConfig default_loc("/", srv.shared());

    srv.add_location(default_loc);

    return srv;
}

// Ref:
// https://vitepress.dev/guide/deploy#nginx

ServerConfig make_vitepress_docs_config()
{
    ServerConfig srv("www/vitepress_docs");

    srv.add_listen_addr("127.0.0.1:8080");

    srv.shared().set_error_page(HttpResponse::kStatusNotFound, "/404.html");
    srv.shared().set_error_page(HttpResponse::kStatusForbidden, "/403.html");

    RouteConfig default_loc("/", srv.shared());

    srv.add_location(default_loc);

    return srv;
}
*/
