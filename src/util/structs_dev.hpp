#include <string>
#include <vector>

#ifndef UTIL_STRUCTS_DEV_HPP_
#define UTIL_STRUCTS_DEV_HPP_
// for dev purpose only
struct Location {
    std::string path;
    std::string index;
};

struct ServerConfig {
    int listen_port;                      // port to listen
    std::vector<std::string> server_name; // host names
    bool default_server; // default server applies to a port and if none is flagged default_server,
                         // then the first one is the default one
    std::vector<std::string> methods; // allowed methods
    std::string root;
    Location default_location;
    std::vector<Location> locations;
    int domain;
    int max_conn;
    std::string protocol;
};

struct HttpRequest {
    std::string method;
    std::string path;
};

#endif // UTIL_STRUCTS_DEV_HPP_
