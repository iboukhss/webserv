#include "config/server_config.hpp"
#include "core/server.hpp"
#include "core/signals.hpp"

#include <exception>
#include <iostream>

int main(void)
{
    setup_signal_handlers();

    try {
        // HttpConfig cfg = load_http_config("config/youpi_banane.conf");
        // HttpConfig cfg = load_http_config("config/vitepress.conf");
        HttpConfig cfg = load_http_config("config/example.conf");

        Server server(cfg.servers[0]);
        server.init();
        server.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
