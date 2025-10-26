

#include "Server.hpp"

#include <iostream>

int main(void)
{
    // later params retrieved from config file
    Server server(INADDR_ANY, 8080, 1000);

    try {
        server.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
