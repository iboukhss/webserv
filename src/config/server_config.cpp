#include "config/server_config.hpp"

#include "core/server_defaults.hpp"

ServerConfig make_site1_config()
{
    ServerConfig cfg;

    // ------------------------
    // Server settings
    // ------------------------
    cfg.server_ip = INADDR_ANY;
    cfg.listen_port = WEBSERV_DEFAULT_PORT;
    cfg.backlog = WEBSERV_DEFAULT_BACKLOG;
    cfg.max_body_size = WEBSERV_DEFAULT_MAX_BODY_SIZE;

    // ------------------------
    // Router settings
    // ------------------------
    cfg.server_name.push_back("test_server");
    cfg.root = "www/site1";
    cfg.default_server = true;

    // ------------------------
    // Allowed methods
    // ------------------------
    cfg.methods.push_back("GET");
    cfg.methods.push_back("POST");
    cfg.methods.push_back("DELETE");

    // ------------------------
    // Default location
    // ------------------------
    cfg.default_location.path = "";
    cfg.default_location.index = "index.html";

    // ------------------------
    // Other locations
    // ------------------------
    Location loc1;
    loc1.path = "/pages";
    loc1.index = "index.html";
    cfg.locations.push_back(loc1);

    Location loc2;
    loc2.path = "/images";
    loc2.index = "index.html";
    cfg.locations.push_back(loc2);

    Location loc3;
    loc3.path = "/files";
    loc3.index = "index.html";
    cfg.locations.push_back(loc3);

    return cfg;
}

ServerConfig make_example_config()
{
    ServerConfig cfg;

    // ------------------------
    // Server settings
    // ------------------------
    cfg.server_ip = INADDR_ANY;
    cfg.listen_port = WEBSERV_DEFAULT_PORT;
    cfg.backlog = WEBSERV_DEFAULT_BACKLOG;
    cfg.max_body_size = WEBSERV_DEFAULT_MAX_BODY_SIZE;

    // ------------------------
    // Router settings
    // ------------------------
    cfg.server_name.push_back("test_server");
    cfg.root = "www/example";
    cfg.default_server = true;

    // ------------------------
    // Allowed methods
    // ------------------------
    cfg.methods.push_back("GET");
    cfg.methods.push_back("POST");
    cfg.methods.push_back("DELETE");

    // ------------------------
    // Default location
    // ------------------------
    cfg.default_location.path = "";
    cfg.default_location.index = "index.html";

    // ------------------------
    // Other locations
    // ------------------------
    Location loc1;
    loc1.path = "/example";
    loc1.index = "index.html";
    cfg.locations.push_back(loc1);

    Location loc2;
    loc1.path = "/example/test_subfolder";
    loc1.index = "index.html";
    cfg.locations.push_back(loc2);

    return cfg;
}
