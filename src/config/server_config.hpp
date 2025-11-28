#ifndef CONFIG_SERVER_CONFIG_HPP_
#define CONFIG_SERVER_CONFIG_HPP_

#include <netinet/in.h>

#include <string>
#include <vector>

// for dev purpose only
struct Location {
    std::string path;
    std::string index;
};

struct ServerConfig {
    in_addr_t server_ip;   // server ip
    in_port_t listen_port; // server listen port
    int backlog;           // max pending connections in kernel queue
    int max_body_size;     // max client body in bytes

    std::vector<std::string> server_name; // host names
    std::string root;
    bool default_server;

    std::vector<std::string> methods; // allowed methods

    Location default_location;
    std::vector<Location> locations;
};

ServerConfig make_site1_config();
ServerConfig make_example_config();

#endif // CONFIG_SERVER_CONFIG_HPP_
