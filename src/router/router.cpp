#include "core/server.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "router/router_internal.hpp"

#include <sys/stat.h>

#include <iostream>
#include <string>

static int prefix_length(const std::string& req_location, const std::string& location)
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
static const Location* routing(const std::string& req_location, const ServerConfig& config)
{
    int longest_match = 0;
    const Location* best_match = NULL;
    // iterrate through locations to find longest match
    for (size_t i = 0; i < config.locations.size(); i++) {
        int len_match = prefix_length(req_location, config.locations[i].path);
        if (len_match > longest_match) {
            best_match = &config.locations[i];
        }
    }
    return (best_match ? best_match : &(config.default_location));
}

// NOTE(IBO): I changed the return type to bool because I got confused during testing.
// Sidenote, I don't think allowed methods should be stored in the ServerConfig.
static bool is_valid_method(const std::string& method, const ServerConfig& config)
{
    for (size_t i = 0; i < config.methods.size(); ++i) {
        if (method == config.methods[i])
            return (true);
    }
    return (false);
}

// Description : build full path by concat 3 elements and consider for leading/trailing backslashes
// TO DO (DHE) : In the config parser -> ensure that root has no trailing /
static std::string build_request_path(const HttpRequest& request, const Location* best_match,
                                      const ServerConfig& config)
{
    std::string full_path = config.root;
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

// NOTE(IBO): HttpResponse already has a member function to create the raw response.
// Feel free to make any changes to that file.
static void set_status(Client* conn, int status_code, const std::string& body)
{
    HttpResponse res = {status_code, "text/plain", body};

    conn->set_response(res);
}

// NOTE(IBO): Maybe we don't even need to pass a const HttpRequest? And just use the
// stored request with conn->req(), very minor detail.
void Server::handle_request(Client* conn, const HttpRequest& request)
{
    const ServerConfig& conf = config_;
    struct stat sb; // needed by stat to check if file exists

    std::cout << request.method << std::endl;

    if (!is_valid_method(request.method, conf)) {
        set_status(conn, kMethodNotAllowed, ""); // in the end should be done
        return;
    }

    if (request.path.empty()) {
        set_status(conn, kBadRequest, "");
        return;
    }

    const Location* longest_match = routing(request.path, conf);
    if (!longest_match) {
        set_status(conn, kNotFound, "");
        return;
    }

    std::string full_path = build_request_path(request, longest_match, conf);
    std::cout << "longest_match = " << longest_match->path << std::endl;
    std::cout << "full_path = " << full_path << std::endl;

    // NOTE(IBO): This if-else tree is very dangerous, very easy to miss one return;
    // Would be a good idea to refactor.
    if (request.method == "GET") {
        if (!stat(full_path.c_str(), &sb)) {
            set_status(conn, kNotFound, "");
            return;
        }
        set_status(conn, kOk, "Hello from router!\n"); // here the file needs to be read
                                                       // and send to the socket
        return;
    }
    else if (request.method == "DELETE") {
    }
    else if (request.method == "POST") {
    }
    else {
        set_status(conn, kMethodNotAllowed, "");
    }
}
