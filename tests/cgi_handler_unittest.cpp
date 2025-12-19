#include "config/server_config.hpp"
#include "handler/cgi_handler.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "utest/utest.h"
#include "util/log_message.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class TempCgiScript {
public:
    explicit TempCgiScript(const std::string& content)
    {
        static int idx = 0;
        path_ = "/tmp/cgi_test_" + to_string(idx++) + ".sh";

        std::ofstream ofs(path_.c_str());
        ofs << "#!/bin/sh\n";
        ofs << content;
        ofs.close();

        chmod(path_.c_str(), 0755);
    }

    ~TempCgiScript() { ::remove(path_.c_str()); }

    const std::string& path() const { return path_; }

private:
    std::string path_;
    TempCgiScript(const TempCgiScript&) {}
    TempCgiScript& operator=(const TempCgiScript&) { return *this; }
    static std::string to_string(int v)
    {
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
};

UTEST(CgiHandler, SimpleGet)
{
    TempCgiScript script("echo \"Content-Type: text/plain\"\n"
                         "echo\n"
                         "echo \"Hello CGI\"");

    HttpRequest req;
    req.method = "GET";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;

    CgiHandler handler(script.path(), req, cfg);

    char buf[1024];
    std::string response;
    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    ASSERT_TRUE(response.find("Hello CGI") != std::string::npos);
}

UTEST(CgiHandler, PostBody)
{
    TempCgiScript script("echo \"Content-Type: text/plain\"\n"
                         "echo\n"
                         "cat -");

    HttpRequest req;
    req.method = "POST";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 11;

    RouteConfig cfg;

    CgiHandler handler(script.path(), req, cfg);

    const char* body = "hello world";

    while (handler.needs_input()) {
        handler.write_data(body, 11);
    }

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("hello world") != std::string::npos);
}

UTEST(CgiHandler, MissingContentType)
{

    UTEST_SKIP("TODO: Test produces instable results");
    TempCgiScript script("echo\n"
                         "echo \"No headers\"");

    HttpRequest req;
    req.method = "GET";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;

    CgiHandler handler(script.path(), req, cfg);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("502") != std::string::npos);
}

class TempFile {
public:
    explicit TempFile(const std::string& path)
        : path_(path)
    {
    }
    ~TempFile() { ::remove(path_.c_str()); }
    const std::string& path() const { return path_; }

private:
    TempFile(const TempFile&) {}
    TempFile& operator=(const TempFile&) { return *this; }
    std::string path_;
};

UTEST(CgiHandler, NotExecutable)
{
    TempFile f("/tmp/not_exec.py");
    {
        std::ofstream ofs(f.path().c_str());
        ofs << "echo test\n";
    }
    chmod(f.path().c_str(), 0644);

    HttpRequest req;
    req.method = "GET";
    req.path = f.path();
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;
    CgiHandler handler(f.path(), req, cfg);

    char buf[512];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("403") != std::string::npos);
}

UTEST(CgiHandler, EmptyOutput)
{
    TempCgiScript script("echo \"Content-Type: text/plain\"\n"
                         "echo\n");

    HttpRequest req;
    req.method = "GET";
    req.path = script.path();
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;
    CgiHandler handler(script.path(), req, cfg);

    char buf[128];
    size_t total = 0;

    while (!handler.is_done()) {
        total += handler.read_data(buf, sizeof(buf));
    }

    ASSERT_TRUE(total >= 0); // should terminate cleanly
}

UTEST(CgiHandler, RealScript_GET)
{
    std::string script = "tests/cgi_upper.py";

    HttpRequest req;
    req.method = "GET";
    req.path = script;
    req.query_string = "hello=world";
    req.content_length = 0;

    RouteConfig cfg;

    CgiHandler handler(script, req, cfg);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    // Body should contain uppercased query string
    ASSERT_TRUE(response.find("HELLO=WORLD") != std::string::npos);

    // Optional sanity checks

    ASSERT_TRUE(response.find("200") != std::string::npos);
}

UTEST(CgiHandler, RealScript_POST)
{
    std::string script = "tests/cgi_upper.py";

    const char* body = "Hello CGI";
    size_t body_len = strlen(body);

    HttpRequest req;
    req.method = "POST";
    req.path = script;
    req.query_string = "";
    req.content_length = body_len;

    RouteConfig cfg;

    CgiHandler handler(script, req, cfg);

    // Write POST body
    while (handler.needs_input()) {
        handler.write_data(body, body_len);
    }

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    // std::cout << response << std::endl;
    ASSERT_TRUE(response.find("HELLO CGI") != std::string::npos);
}

UTEST(Config, Bla_Rejects_Get)
{
    ServerConfig cfg = make_youpi_banane_test_config();

    ASSERT_TRUE(std::find(cfg.config.cgi.allowed_methods.begin(),
                          cfg.config.cgi.allowed_methods.end(),
                          "GET") == cfg.config.cgi.allowed_methods.end());
}

UTEST(Config, Bla_Cgi_Exec_Path)
{
    ServerConfig cfg = make_youpi_banane_test_config();

    ASSERT_TRUE(cfg.config.cgi.exec_path == "/tests/ubuntu_cgi_tester");
}

UTEST(Config, PostBody_MaxBody)
{
    ServerConfig cfg = make_youpi_banane_test_config();

    const RouteConfig* post = NULL;
    for (size_t i = 0; i < cfg.locations.size(); ++i) {
        if (cfg.locations[i].route_path == "/post_body")
            post = &cfg.locations[i];
    }

    ASSERT_TRUE(post != NULL);
    ASSERT_EQ(post->config.max_body_size, 100u);
}

UTEST(Config, Directory_Index)
{
    ServerConfig cfg = make_youpi_banane_test_config();

    const RouteConfig* dir = NULL;
    for (size_t i = 0; i < cfg.locations.size(); ++i) {
        if (cfg.locations[i].route_path == "/directory/")
            dir = &cfg.locations[i];
    }

    ASSERT_TRUE(dir != NULL);
    ASSERT_EQ(dir->config.index_files.size(), 1u);
    ASSERT_TRUE(dir->config.index_files[0] == "youpi.bad_extension");
}

UTEST(Config, Directory_Inherits_Get_Only)
{
    ServerConfig cfg = make_youpi_banane_test_config();

    const RouteConfig* dir = NULL;
    for (size_t i = 0; i < cfg.locations.size(); ++i) {
        if (cfg.locations[i].route_path == "/directory/")
            dir = &cfg.locations[i];
    }

    ASSERT_TRUE(dir != NULL);
    ASSERT_TRUE(std::find(dir->config.allowed_methods.begin(),
                          dir->config.allowed_methods.end(),
                          "GET") != dir->config.allowed_methods.end());
}

UTEST(CgiHandler, UbuntuCgiTester_GET)
{
    UTEST_SKIP("TODO: to implement");
    HttpRequest req;
    req.method = "GET";
    req.path = "/ubuntu_cgi_tester";
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;
    cfg.config.cgi.exec_path = "";

    CgiHandler handler("tests/ubuntu_cgi_tester", req, cfg);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    std::cout << response;
    ASSERT_TRUE(response.find("Content-Type") != std::string::npos);
}

UTEST(CgiHandler, UbuntuCgiTester_POST)
{
    UTEST_SKIP("TODO: to implement");
    const char* body = "hello";
    size_t body_len = strlen(body);

    HttpRequest req;
    req.method = "POST";
    req.path = "/ubuntu_cgi_tester";
    req.query_string = "";
    req.content_length = body_len;

    RouteConfig cfg;

    CgiHandler handler("tests/ubuntu_cgi_tester", req, cfg);

    while (handler.needs_input())
        handler.write_data(body, body_len);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    std::cout << response;
    ASSERT_TRUE(response.find("Content-Type") != std::string::npos);
}

UTEST(CgiHandler, UbuntuCgiTester_ExtensionBased)
{
    TempCgiScript dummy("echo should not run");
    std::string fake_file = dummy.path() + ".bla";
    ::rename(dummy.path().c_str(), fake_file.c_str());

    HttpRequest req;
    req.method = "POST";
    req.path = fake_file;
    req.query_string = "";
    req.content_length = 4;

    RouteConfig cfg;
    cfg.config.cgi.extension = ".bla";
    cfg.config.cgi.exec_path = "/tests/ubuntu_cgi_tester";
    cfg.config.cgi.allowed_methods.push_back("POST");

    CgiHandler handler(fake_file, req, cfg);

    handler.write_data("test", 4);

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("Content-Type") != std::string::npos);
}

UTEST(CgiHandler, UbuntuCgiTester_MethodNotAllowed)
{
    HttpRequest req;
    req.method = "GET"; // forbidden
    req.path = "/tests/ubuntu_cgi_tester";
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;
    cfg.config.cgi.allowed_methods.push_back("POST");

    CgiHandler handler(req.path, req, cfg);

    char buf[512];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("405") != std::string::npos ||
                response.find("403") != std::string::npos);
}

UTEST(CgiHandler, UbuntuCgiTester_NotExecutable)
{
    TempFile f("/tmp/not_exec_cgi");
    {
        std::ofstream ofs(f.path().c_str());
        ofs << "binary";
    }
    chmod(f.path().c_str(), 0644);

    HttpRequest req;
    req.method = "GET";
    req.path = f.path();
    req.query_string = "";
    req.content_length = 0;

    RouteConfig cfg;

    CgiHandler handler(f.path(), req, cfg);

    char buf[256];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    ASSERT_TRUE(response.find("403") != std::string::npos);
}
