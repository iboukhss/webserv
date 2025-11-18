#include "router/router.hpp"

#include "config/server_config.hpp"
#include "handler/static_file_handler.hpp"
#include "http/http_request.hpp"

#include <sys/stat.h>

#include <iostream>
#include <string>

Router::Router(ServerConfig& config)
    : config_(config)
{
}

int Router::prefix_length(const std::string& req_location, const std::string& location)
{
    int len_min = std::min(req_location.size(), location.size());
    int len_match = 0;
    for (int i = 0; i < len_min; ++i) {
        if (req_location[i] != location[i])
            break;
        ++len_match;
    }
    return (len_match);
}

// for the moment it takes some static input
const Location* Router::routing(const std::string& req_location)
{
    int longest_match = 0;
    const Location* best_match = NULL;
    // iterrate through locations to find longest match
    for (size_t i = 0; i < config_.locations.size(); i++) {
        int len_match = prefix_length(req_location, config_.locations[i].path);
        if (len_match > longest_match) {
            best_match = &config_.locations[i];
        }
    }
    return (best_match ? best_match : &(config_.default_location));
}

// NOTE(IBO): I changed the return type to bool because I got confused during testing.
// Sidenote, I don't think allowed methods should be stored in the ServerConfig.
bool Router::is_valid_method(const std::string& method)
{
    for (size_t i = 0; i < config_.methods.size(); ++i) {
        if (method == config_.methods[i])
            return (true);
    }
    return (false);
}

// Description : build full path by concat 3 elements and consider for leading/trailing backslashes
// TO DO (DHE) : In the config parser -> ensure that root has no trailing /
std::string Router::build_request_path(const HttpRequest& request, const Location* best_match)
{
    std::string full_path = config_.root;
    std::string folder = static_cast<std::string>(best_match->path);
    // std::cout << "root = " << full_path << std::endl;
    std::string file =
        request.path.substr(best_match->path.size(), request.path.size() - best_match->path.size());
    if (full_path[full_path.size() - 1] != '/')
        full_path = full_path + "/";
    if (folder[0] == '/')
        folder.erase(0, 1);
    if (folder[folder.size() - 1] != '/')
        folder = folder + "/";
    if (file[0] == '/')
        file.erase(0, 1);
    full_path = full_path + folder + file;

    if (full_path[full_path.size() - 1] == '/')
        full_path = full_path + "index.html";
    return (full_path);
}

Handler* Router::handle_request(const HttpRequest& request)
{
    if (!is_valid_method(request.method)) {
        // kNotAllowed? create a new ErrorHandler maybe?
        std::cerr << "Invalid method not implemented" << std::endl;
        return NULL;
    }
    if (request.path.empty()) {
        // kBadRequest? same thing
        std::cerr << "Bad request not implemented" << std::endl;
        return NULL;
    }

    const Location* longest_match = routing(request.path);
    if (!longest_match) {
        // kNotFound? no match?
        std::cerr << "No match found not implemented" << std::endl;
        return NULL;
    }
    std::string full_path = build_request_path(request, longest_match);
    if (request.method == "GET") {
        return new StaticFileHandler(full_path);
    }

    // Add other handlers here

    // Fallback, should never happen in theory
    std::cerr << "Something terrible happened in the router" << std::endl;
    return NULL;
}
