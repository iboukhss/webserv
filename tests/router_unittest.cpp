
#include "http/http_request.hpp"
#include "router/router.hpp"
#include "utest/utest.h"
#include <sys/socket.h>

#include <string>

void test_config1(ServerConfig& config)
{
    // ------------------------
    // Primitive fields
    // ------------------------
    config.listen_port = 8080;
    config.default_server = true;
    config.root = "www/site1";
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
    config.default_location.path = "";
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

UTEST(RouterTest, GET)
{
    ServerConfig config;
    test_config1(config);

    HttpRequest req;
    req.method = "GET";
    req.path = "/files/42.txt";

    Router router(config);

    ASSERT_TRUE(router.handle_request(req));
}

UTEST(RouterTest, POST)
{
    UTEST_SKIP("Feature to be implemented");

    ServerConfig config;
    test_config1(config);

    HttpRequest req;
    req.method = "POST";
    req.path = "/files/42.txt";

    Router router(config);

    ASSERT_TRUE(router.handle_request(req));
}

UTEST(RouterTest, DELETE)
{
    
    UTEST_SKIP("Feature to be implemented");

    ServerConfig config;
    test_config1(config);

    HttpRequest req;
    req.path = "/files/42.txt";

    Router router(config);

    ASSERT_TRUE(router.handle_request(req));
}


UTEST(RouterTest, IncorrectMethod)
{
    ServerConfig config;
    test_config1(config);

    HttpRequest req;
    req.method = "INCORRECT METHOD";
    req.path = "/files/42.txt";

    Router router(config);

    ASSERT_FALSE(router.handle_request(req));
}

UTEST(RouterTest, EmptyPath)
{
    ServerConfig config;
    test_config1(config);

    HttpRequest req;
    req.method = "GET";
    req.path = "";

    Router router(config);

    ASSERT_FALSE(router.handle_request(req));
}

