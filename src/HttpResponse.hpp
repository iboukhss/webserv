#ifndef HTTPRESPONSE_HPP_
#define HTTPRESPONSE_HPP_

#include <string>

struct HttpResponse {
    int status;
    std::string content_type;
    std::string body;
    std::string to_string() const;
};

#endif // HTTPRESPONSE_HPP_
