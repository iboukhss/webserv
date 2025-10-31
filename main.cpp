

#include "Server.hpp"
#include "SyscallError.hpp"

#include <iostream>

int main(void)
{
    // later params retrieved from config file
    Server server(INADDR_ANY, 8080, 1000);

    try {
        server.run();
    }
    catch (const SyscallError& e) {
        std::cerr << e.what() << std::endl;
        return e.code();
    }
    return 0;
}
