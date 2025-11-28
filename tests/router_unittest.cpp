
#include "handler/static_file_handler.hpp"
#include "http/http_request.hpp"
#include "router/router.hpp"
#include "utest/utest.h"

#include <sys/socket.h>

#include <iostream>
#include <string>

UTEST(RouterTest, GET)
{
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "/files/42.txt";

    Router router(config);

    Handler* h = router.handle_request(req);
    ASSERT_TRUE(h != NULL);
    delete h;
}

UTEST(RouterTest, POST)
{
    UTEST_SKIP("Feature to be implemented");

    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "POST";
    req.path = "/files/42.txt";

    Router router(config);

    Handler* h = router.handle_request(req);
    ASSERT_TRUE(h != NULL);
    delete h;
}

UTEST(RouterTest, DELETE)
{

    UTEST_SKIP("Feature to be implemented");

    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.path = "/files/42.txt";

    Router router(config);

    Handler* h = router.handle_request(req);
    ASSERT_TRUE(h != NULL);
    delete h;
}

UTEST(RouterTest, IncorrectMethod)
{
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "INCORRECT METHOD";
    req.path = "/files/42.txt";

    Router router(config);

    Handler* h = router.handle_request(req);
    ASSERT_TRUE(h == NULL);
    delete h;
}

UTEST(RouterTest, EmptyPath)
{
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "";

    Router router(config);

    Handler* h = router.handle_request(req);
    ASSERT_TRUE(h == NULL);
    delete h;
}

UTEST(RouterTest, BuildRequestPath)
{
    UTEST_SKIP("Router::build_request_path to be fixed for double slashes");
    ServerConfig config = make_site1_config();

    HttpRequest req;
    req.method = "GET";
    req.path = "/index.html";

    Router router(config);

    Handler* handler_ = router.handle_request(req);

    StaticFileHandler* sfh = dynamic_cast<StaticFileHandler*>(handler_);
    ASSERT_TRUE(sfh != NULL);
    ASSERT_STREQ(sfh->path().c_str(), "www/site1/index.html");
    delete handler_;
}
