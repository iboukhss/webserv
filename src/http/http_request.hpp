#ifndef HTTP_HTTP_REQUEST_HPP_
#define HTTP_HTTP_REQUEST_HPP_

#include <map>
#include <string>

struct HttpRequest {
    HttpRequest();

    std::string method;
    std::string path;
    std::string http_version;

    // Try to keep feature parity with HttpResponse if applicable here.
    size_t content_length;
    bool is_chunked;
    bool keep_alive;

    // Useless headers we do not support, but might need eventually.
    std::map<std::string, std::string> headers;

    std::string body;
};

#endif // HTTP_HTTP_REQUEST_HPP_
