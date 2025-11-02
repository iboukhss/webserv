#ifndef HTTP_HTTP_RESPONSE_HPP_
#define HTTP_HTTP_RESPONSE_HPP_

#include <string>

struct HttpResponse {
    int status;
    std::string content_type;
    std::string body;
    std::string to_string() const;
};

#endif // HTTP_HTTP_RESPONSE_HPP_
