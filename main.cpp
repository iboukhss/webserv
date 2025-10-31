#include "Server.hpp"
#include "SyscallError.hpp"

#include <iostream>

volatile sig_atomic_t g_sigint_received = 0;

extern "C" void handle_sigint(int sig)
{
    (void) sig;
    g_sigint_received = 1;
}

int main(void)
{
    // later params retrieved from config file
    Server server(INADDR_ANY, 8080, 1000);

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    try {
        server.run();
    }
    catch (const SyscallError& e) {
        std::cerr << e.what() << std::endl;
        return e.code();
    }
    return 0;
}
