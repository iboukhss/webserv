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

void print_response_expected(const std::string& response, const std::string& expected)
{
    std::cout << "reponse = " << std::endl << response << std::endl;
    std::cout << "expected = " << std::endl << expected << std::endl;
    std::cout << "response size = " << response.size() << std::endl;
    std::cout << "expected size = " << expected.size() << std::endl;
}

UTEST(CgiHandler, SimpleGet)
{
    TempCgiScript script("echo \"Content-Type: text/plain\"\n"
                         "echo \"Content-Length: 9\"\n"
                         "echo\n"
                         "echo -n \"Hello CGI\"");

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
    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    res.inline_body = "Hello CGI";

    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, PostBody)
{
    const char* body = "hello world"; // update Content-Length in script if body changes
    TempCgiScript script("echo \"Content-Type: text/plain\"\n"
                         "echo \"Content-Length: 11\"\n"
                         "echo\n"
                         "cat -");

    HttpRequest req;
    req.method = "POST";
    req.path = script.path();
    req.query_string = "";
    req.content_length = strlen(body); // length of body

    RouteConfig cfg;

    CgiHandler handler(script.path(), req, cfg);

    while (handler.needs_input()) {
        handler.write_data(body, req.content_length);
    }

    char buf[1024];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }

    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    res.inline_body = body;

    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, MissingContentType)
{
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
    HttpResponse res = res.make_error(HttpResponse::kStatusBadGateway);
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
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
    HttpResponse res = res.make_error(HttpResponse::kStatusForbidden);
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
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
    size_t n = 0;
    std::string response;
    while (!handler.is_done()) {
        n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, PythonScript_GET)
{
    std::string script = "tests/cgi_upper.py";
    std::string query_string_ = "hello=world=returned=UPPER=case";
    HttpRequest req;
    req.method = "GET";
    req.path = script;
    req.query_string = query_string_;
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
    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    std::string expected_body = query_string_;
    for (size_t i = 0; i < expected_body.size(); ++i)
        expected_body[i] = std::toupper(expected_body[i]);
    res.inline_body = expected_body;
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, PythonScript_POST)
{
    std::string script = "tests/cgi_upper.py";
    const char* body = "hello=world=returned=UPPER=case=after=POST";
    size_t body_len = strlen(body);

    HttpRequest req;
    req.method = "POST";
    req.path = script;
    req.query_string = "SOME=QUERY=STRING";
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
    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    std::string expected_body = body;
    for (size_t i = 0; i < expected_body.size(); ++i)
        expected_body[i] = std::toupper(expected_body[i]);
    res.inline_body = expected_body;
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
}

UTEST(CgiHandler, PythonScript_POST_1MB_Payload)
{

    UTEST_SKIP("TODO: output pipe closed to early, fix required");
    std::string script = "tests/cgi_upper.py";
    const size_t body_len = 1024 * 1024; // 1 MB
    std::string body(body_len, 'a');

    HttpRequest req;
    req.method = "POST";
    req.path = script;
    req.query_string = "SOME=QUERY=STRING";
    req.content_length = body_len;

    RouteConfig cfg;

    CgiHandler handler(script, req, cfg);

    // Write POST body (will require multiple writes)
    size_t written = 0;
    while (handler.needs_input()) {
        size_t chunk = body_len - written;
        size_t n = handler.write_data(body.data() + written, chunk);
        written += n;
    }

    ASSERT_EQ(written, body_len);

    char buf[4096];
    std::string response;

    while (!handler.is_done()) {
        size_t n = handler.read_data(buf, sizeof(buf));
        if (n > 0)
            response.append(buf, n);
    }
    HttpResponse res;
    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/plain";
    res.keep_alive = true;
    std::string expected_body = body;
    for (size_t i = 0; i < expected_body.size(); ++i)
        expected_body[i] = std::toupper(expected_body[i]);
    res.inline_body = expected_body;
    std::string expected = res.to_string();
    // print_response_expected(response, expected);
    ASSERT_TRUE(response == expected);
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
