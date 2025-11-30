#include "config/server_config.hpp"
#include "core/server.hpp"
#include "core/signals.hpp"
#include "util/syscall_error.hpp"

#include <iostream>

int main(void)
{
    setup_signal_handlers();

    try {
        ServerConfig config = make_site1_config();
        Server server(config);

        server.init();
        server.run();
    }
    catch (const UnrecoverableError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
