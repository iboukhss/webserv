#include "config/server_config.hpp"
#include "core/server.hpp"
#include "core/signals.hpp"

#include <exception>
#include <iostream>

int main(void)
{
    setup_signal_handlers();

    try {
        ServerConfig config = make_vitepress_docs_config();
        Server server(config);

        server.init();
        server.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
