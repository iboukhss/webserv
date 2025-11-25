#ifndef HTTP_HTTP_REQUEST_HPP_
#define HTTP_HTTP_REQUEST_HPP_

#include <map>
#include <string>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string http_version;

    size_t content_length;
    bool is_chunked;
    bool keep_alive;

    std::map<std::string, std::string> headers;

    std::string body;
};

#endif // HTTP_HTTP_REQUEST_HPP_
