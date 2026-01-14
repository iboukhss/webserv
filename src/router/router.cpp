#include "router/router.hpp"

#include "config/server_config.hpp"
#include "handler/cgi_handler.hpp"
#include "handler/delete_handler.hpp"
#include "handler/error_handler.hpp"
#include "handler/redirect_handler.hpp"
#include "handler/static_file_handler.hpp"
#include "handler/upload_handler.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "util/log_message.hpp"

#include <sys/stat.h>

#include <iostream>
#include <string>

Router::Router(const std::map<std::string, RouteConfig>& routes)
    : routes_(routes)
{
}

static bool route_matches(const std::string& request_path, const std::string& route)
{
    if (route == "/")
        return true;

    if (request_path.size() < route.size())
        return false;

    // Check if prefix matches
    if (request_path.compare(0, route.size(), route) != 0)
        return false;

    // Check for exact match
    if (request_path.size() == route.size())
        return true;

    assert(route[0] == '/');
    assert(route.size() > 1 && "At this point we must have matched more than just '/'");

    // Directory match
    if (route[route.size() - 1] == '/')
        return true;

    // Endpoint match, ensure boundary
    if (request_path[route.size()] == '/')
        return true;

    return false;
}

// for the moment it takes some static input
const RouteConfig& Router::find_best_route(const std::string& request_path) const
{
    assert(!routes_.empty() && "There must always be at least one default location");
    assert(routes_.count("/") == 1 && "Default route path is not '/'");

    const RouteConfig* best = &routes_.find("/")->second;
    size_t best_len = 1;

    for (std::map<std::string, RouteConfig>::const_iterator it = routes_.begin();
         it != routes_.end();
         ++it) {

        std::string route_path = it->second.path;
        size_t match_len = route_matches(request_path, route_path) ? route_path.size() : 0;
        if (match_len > best_len) {
            best_len = match_len;
            best = &it->second;
        }
    }
    return *best;
}

// NOTE: we might be missing out on proper PATH_INFO semantics here as this
// would not be considered a match: /foo.py/bar -> .py
// This seems to be valid in some cases but we ignore it right now.
static bool is_cgi_request(const HttpRequest& request, const RouteConfig& route)
{
    if (route.shared.cgi.extension.empty())
        return false;

    size_t dot = request.path.find_last_of(".");
    if (dot == std::string::npos)
        return false;
    std::string ext = request.path.substr(dot);
    return request.path.substr(dot) == route.shared.cgi.extension;
}

static bool is_allowed_cgi_method(const std::string& method, const RouteConfig& route)
{
    for (size_t i = 0; i < route.shared.cgi.allowed_methods.size(); i++) {
        if (method == route.shared.cgi.allowed_methods[i])
            return true;
    }
    return false;
}

// 1. Checks for allowed CGI methods
// 2. Checks if uploads allowed -> POST without body is not considered an upload
// 3. If not CGI or upload, simply checks if the method is allowed on that route
static bool is_allowed_method(const HttpRequest& request, const RouteConfig& route)
{
    if (is_cgi_request(request, route) && !is_allowed_cgi_method(request.method, route))
        return false;

    if (request.method == "POST" && request.content_length > 0 && !route.shared.uploads_allowed)
        return false;

    for (size_t i = 0; i < route.shared.allowed_methods.size(); ++i) {
        if (request.method == route.shared.allowed_methods[i])
            return true;
    }
    return false;
}

// TODO: get rid of this? Should not be the router responsability
static std::string build_request_path(const HttpRequest& request, const RouteConfig& best_route)
{
    assert(request.path[0] == '/' && "Request path must always start with a '/'");

    return best_route.shared.document_root + request.path;
}

// Preconditions:
// Parser does not check allowed methods (only the syntax)
// Parser does not allow empty URI (invalid format)
// Parser always checks if URI starts with a / (forward slash)

Handler* Router::handle_request(const HttpRequest& request) const
{
    assert(!request.path.empty() && "Request URI can never be empty in the router");

    const RouteConfig& best_route = find_best_route(request.path);
    LOG(DEBUG) << "best_route.path = " << best_route.path;
    if (!best_route.shared.redirect.url.empty()) {
        return new RedirectHandler(best_route, request);
    }
    if (!is_allowed_method(request, best_route)) {
        return new ErrorHandler(HttpResponse::kStatusMethodNotAllowed, best_route, request);
    }

    std::string full_path = build_request_path(request, best_route);

    if (is_cgi_request(request, best_route)) {
        return new CgiHandler(full_path, best_route, request);
    }
    if (request.method == "GET") {
        return new StaticFileHandler(full_path, best_route, request);
    }
    if (request.method == "POST") {
        return new UploadHandler(full_path, best_route, request);
    }
    if (request.method == "DELETE") {
        return new DeleteHandler(full_path, best_route, request);
    }

    LOG(WARN) << "Router could not find any match, falling back to ErrorHandler";
    return new ErrorHandler(HttpResponse::kStatusInternalServerError, best_route,
                            request); // Maybe 501 better here?
}
