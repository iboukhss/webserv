#include "config/config_builder.hpp"

#include "config/config_parser.hpp"
#include "config/server_config.hpp"
#include "http/http_response.hpp"
#include "util/log_message.hpp"
#include "util/string.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <stdexcept>

static void config_error(const std::string& msg, int line = -1)
{
    if (line != -1) {
        throw std::runtime_error(msg + " (line " + itoa(line) + ")");
    }
    else {
        throw std::runtime_error(msg);
    }
}

static void expect_argc(const AstNode& node, size_t count)
{
    if (node.args.size() != count) {
        config_error("'" + node.name + "' expected " + itoa(count) + " arguments, got " +
                     itoa(node.args.size()) + " (line " + itoa(node.line) + ")");
    }
}

static void expect_min_argc(const AstNode& node, size_t count)
{
    if (node.args.size() < count) {
        config_error("'" + node.name + "' expected at least " + itoa(count) + " arguments, got " +
                     itoa(node.args.size()) + " (line " + itoa(node.line) + ")");
    }
}

static void expect_max_argc(const AstNode& node, size_t count)
{
    if (node.args.size() > count) {
        config_error("'" + node.name + "' expected at most " + itoa(count) + " arguments, got " +
                     itoa(node.args.size()) + " (line " + itoa(node.line) + ")");
    }
}

ConfigBuilder::ConfigBuilder(const std::vector<AstNode>& nodes)
    : nodes_(nodes)
{
}

HttpConfig ConfigBuilder::build_http_config()
{
    HttpConfig cfg;

    for (size_t i = 0; i < nodes_.size(); i++) {
        const AstNode& node = nodes_[i];

        if (node.name != "server")
            config_error("Unexpected directive '" + node.name + "'", node.line);

        cfg.servers.push_back(build_server_config(node));
    }

    if (cfg.servers.empty())
        config_error("No server blocks defined!");

    return cfg;
}

/* clang-format off */
static const char* g_shared_directives[] = {
    // shared directives
    "allow_methods",
    "allow_uploads",
    "autoindex",
    "cgi_allow_methods",
    "cgi_handler",
    "client_max_body_size",
    "error_page",
    "index",
    "return",
    "root",
    "upload_store"
};
/* clang-format on */

static bool is_shared_directive(const std::string& name)
{
    for (size_t i = 0; i < sizeof(g_shared_directives) / sizeof(g_shared_directives[0]); i++) {
        if (name == g_shared_directives[i])
            return true;
    }
    return false;
}

ServerConfig ConfigBuilder::build_server_config(const AstNode& node)
{
    if (!node.args.empty())
        config_error("'server' directive takes no arguments", node.line);

    std::string server_root;
    std::vector<std::string> listen_addrs;
    std::vector<AstNode> shared_nodes;
    std::vector<AstNode> location_nodes;

    for (size_t i = 0; i < node.children.size(); i++) {
        const AstNode& child = node.children[i];

        if (child.name == "root") {
            expect_argc(child, 1);
            server_root = child.args[0];
        }
        else if (child.name == "listen") {
            expect_argc(child, 1);
            listen_addrs.push_back(child.args[0]);
        }
        else if (child.name == "location") {
            location_nodes.push_back(child);
        }
        else if (is_shared_directive(child.name)) {
            shared_nodes.push_back(child);
        }
        else {
            config_error("Unknown server directive '" + child.name + "'", child.line);
        }
    }

    if (server_root.empty())
        config_error("missing 'root' directive in server", node.line);

    if (listen_addrs.empty())
        config_error("missing 'listen' directive in server", node.line);

    ServerConfig srv;

    for (size_t i = 0; i < listen_addrs.size(); i++) {
        srv.add_listen_addr(listen_addrs[i]);
    }

    SharedConfig shared = build_shared_config(shared_nodes, NULL);
    shared.document_root = server_root;
    srv.shared = shared;

    for (size_t i = 0; i < location_nodes.size(); i++) {
        const AstNode& lnode = location_nodes[i];

        RouteConfig route = build_route_config(lnode, shared);
        srv.locations[route.path] = route;
    }

    if (srv.locations.count("/") == 0) {
        AstNode default_route;
        default_route.name = "location";
        default_route.args.push_back("/");

        srv.locations["/"] = build_route_config(default_route, shared);
    }

    return srv;
}

/* clang-format off */
static const char* g_http_methods[] = {
    "CONNECT",
    "DELETE",
    "GET",
    "HEAD",
    "OPTIONS",
    "PATCH",
    "POST",
    "PUT",
    "TRACE"
};
/* clang-format on */

static bool is_valid_http_method(const std::string& str)
{
    for (size_t i = 0; i < sizeof(g_http_methods) / sizeof(g_http_methods[0]); i++) {
        if (str == g_http_methods[i])
            return true;
    }

    return false;
}

static void parse_allowed_methods(const AstNode& node, SharedConfig& config)
{
    expect_min_argc(node, 1);

    for (size_t i = 0; i < node.args.size(); i++) {
        if (!is_valid_http_method(node.args[i]))
            config_error("'allow_methods' invalid argument '" + node.args[i] + "'", node.line);
    }

    config.allowed_methods = node.args;
}

static void parse_allow_uploads(const AstNode& node, SharedConfig& config)
{
    expect_argc(node, 1);

    if (node.args[0] == "on") {
        config.uploads_allowed = true;
    }
    else if (node.args[0] == "off") {
        config.uploads_allowed = false;
    }
    else {
        config_error("'allow_uploads' invalid argument '" + node.args[0] + "'", node.line);
    }
}

static void parse_autoindex(const AstNode& node, SharedConfig& config)
{
    expect_argc(node, 1);

    if (node.args[0] == "on") {
        config.autoindex_enabled = true;
    }
    else if (node.args[0] == "off") {
        config.autoindex_enabled = false;
    }
    else {
        config_error("'autoindex' invalid argument '" + node.args[0] + "'", node.line);
    }
}

static void parse_cgi_allowed_methods(const AstNode& node, SharedConfig& config)
{
    expect_max_argc(node, 2);

    for (size_t i = 0; i < node.args.size(); i++) {
        if (node.args[i] != "GET" && node.args[i] != "POST")
            config_error("'cgi_allow_methods' invalid argument '" + node.args[i] + "'", node.line);
    }

    config.cgi.allowed_methods = node.args;
}

static void parse_cgi_handler(const AstNode& node, SharedConfig& config)
{
    expect_argc(node, 2);
    config.cgi.extension = node.args[0];
    config.cgi.exec_path = node.args[1];
}

static void parse_client_max_body_size(const AstNode& node, SharedConfig& config)
{
    expect_argc(node, 1);

    int size = std::atoi(node.args[0].c_str());
    config.max_body_size = size;
}

static bool file_readable(const std::string& path)
{
    struct stat sb;
    return (stat(path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode));
}

static bool read_error_page(const std::string& path, std::string& error_page_content)
{
    if (!file_readable(path)) {
        LOG(DEBUG) << "error page not readable : " << path;
        return false;
    }

    int fd;
    char buf[4096];
    ssize_t bytes = 1;
    fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        return false;
    }

    while (bytes) {
        bytes = read(fd, buf, sizeof(buf));
        if (bytes == 0) {
            break;
        }
        if (bytes < 0) {
            close(fd);
            return false;
        }
        error_page_content.append(buf, bytes);
    }
    close(fd);
    return true;
}

static void parse_error_pages(const AstNode& node, SharedConfig& config)
{
    expect_min_argc(node, 2);

    const std::string& url = node.args.back();

    for (size_t i = 0; i < node.args.size() - 1; i++) {
        HttpResponse::Status status = HttpResponse::parse_status(node.args[i]);

        if (status == HttpResponse::kStatusNone)
            config_error("'error_page' unsupported status code '" + node.args[i] + "'", node.line);

        std::string error_page;
        std::string path =
            config.document_root +
            url; // potentially unsave ? if file cannot be loaded, default error pages are used
        if (read_error_page(path, error_page) == true) {
            LOG(DEBUG) << "ERROR page loaded : " << status << " " << url;
            config.error_pages[status] = error_page;
        }
    }
}

static void parse_index_files(const AstNode& node, SharedConfig& config)
{
    expect_min_argc(node, 1);
    config.index_files = node.args;
}

static void parse_redirections(const AstNode& node, SharedConfig& config)
{
    expect_min_argc(node, 1);
    expect_max_argc(node, 2);

    HttpResponse::Status status = HttpResponse::kStatusFound;
    std::string url;

    if (node.args.size() == 1) {
        url = node.args[0];
    }
    else {
        status = HttpResponse::parse_status(node.args[0]);
        if (status == HttpResponse::kStatusNone)
            config_error("'return' unsupported status code '" + node.args[0] + "'", node.line);

        url = node.args[1];
    }

    config.redirect.code = status;
    config.redirect.url = url;
}

static void parse_document_root(const AstNode& node, SharedConfig& config)
{
    expect_argc(node, 1);
    config.document_root = node.args[0];
}

static void parse_upload_path(const AstNode& node, SharedConfig& config)
{
    expect_argc(node, 1);
    config.upload_path = node.args[0];
}

static void parse_shared_directive(const AstNode& node, SharedConfig& config)
{
    if (node.name == "allow_methods") {
        parse_allowed_methods(node, config);
    }
    else if (node.name == "allow_uploads") {
        parse_allow_uploads(node, config);
    }
    else if (node.name == "autoindex") {
        parse_autoindex(node, config);
    }
    else if (node.name == "cgi_allow_methods") {
        parse_cgi_allowed_methods(node, config);
    }
    else if (node.name == "cgi_handler") {
        parse_cgi_handler(node, config);
    }
    else if (node.name == "client_max_body_size") {
        parse_client_max_body_size(node, config);
    }
    else if (node.name == "error_page") {
        parse_error_pages(node, config);
    }
    else if (node.name == "index") {
        parse_index_files(node, config);
    }
    else if (node.name == "return") {
        parse_redirections(node, config);
    }
    else if (node.name == "root") {
        parse_document_root(node, config);
    }
    else if (node.name == "upload_store") {
        parse_upload_path(node, config);
    }
    else {
        config_error("Unknown shared directive '" + node.name + "'", node.line);
    }
}

SharedConfig ConfigBuilder::build_shared_config(const std::vector<AstNode>& nodes,
                                                const SharedConfig* parent)
{
    SharedConfig config = parent ? *parent : SharedConfig();

    for (size_t i = 0; i < nodes.size(); i++) {
        parse_shared_directive(nodes[i], config);
    }

    return config;
}

RouteConfig ConfigBuilder::build_route_config(const AstNode& node, const SharedConfig& parent)
{
    expect_argc(node, 1);

    RouteConfig route;
    route.path = node.args[0];

    std::vector<AstNode> shared_nodes;

    for (size_t i = 0; i < node.children.size(); i++) {
        const AstNode& child = node.children[i];

        if (child.name == "alias") {
            expect_argc(child, 1);
            route.alias = child.args[0];
        }
        else if (is_shared_directive(child.name)) {
            shared_nodes.push_back(child);
        }
        else {
            config_error("Unknown location directive '" + child.name + "'", child.line);
        }
    }

    SharedConfig shared = build_shared_config(shared_nodes, &parent);
    route.shared = shared;

    return route;
}
