#include "core/server.hpp"
#include "core/signals.hpp"
#include "util/syscall_error.hpp"

#include <iostream>

void test_config1(ServerConfig& config)
{
    // ------------------------
    // Primitive fields
    // ------------------------
    config.listen_port = 8080;
    config.default_server = true;
    config.root = "/home/dennis/Documents/42Luxembourg/Core/webserv";
    config.domain = AF_INET;
    config.max_conn = 1000;
    config.protocol = "http";

    // ------------------------
    // Server names
    // ------------------------
    config.server_name.push_back("test_server");

    // ------------------------
    // Allowed methods
    // ------------------------
    config.methods.push_back("GET");
    config.methods.push_back("POST");
    config.methods.push_back("DELETE");

    // ------------------------
    // Default location
    // ------------------------
    config.default_location.path = "/home/dennis/Documents/42Luxembourg/Core/webserv";
    config.default_location.index = "index.html";

    // ------------------------
    // Other locations
    // ------------------------
    Location loc1;
    loc1.path = "/pages";
    loc1.index = "index.html";
    config.locations.push_back(loc1);

    Location loc2;
    loc2.path = "/images";
    loc2.index = "index.html";
    config.locations.push_back(loc2);

    Location loc3;
    loc3.path = "/files";
    loc3.index = "index.html";
    config.locations.push_back(loc3);
}

int main(void)
{
    setup_signal_handlers();

    try {
        ServerConfig config;
        test_config1(config);

        Server server(INADDR_ANY, 8080, 1000, config);

        server.run();
    }
    catch (const UnrecoverableError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
